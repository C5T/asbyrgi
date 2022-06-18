// TODO: Escape all the strings, of course. Once Current/JSON is in the picture.

#include <cstdarg>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

struct OPALocalWrapper final {
  size_t const local;
  OPALocalWrapper() = delete;
  explicit OPALocalWrapper(size_t local) : local(local) {}
};

struct OPAUserDefinedFunction final {
  size_t const function;
  explicit OPAUserDefinedFunction(size_t function) : function(function) {}
};

struct OPABuiltin final {
  char const* builtin_name;
  size_t const args_count;
  OPABuiltin(char const* builtin_name, size_t args_count) : builtin_name(builtin_name), args_count(args_count) {}
};

struct OPABuiltins final {
  OPABuiltin plus = OPABuiltin("opa_plus", 2);
  OPABuiltin minus = OPABuiltin("opa_minus", 2);
  OPABuiltin mul = OPABuiltin("opa_mul", 2);
  struct {
    OPABuiltin range = OPABuiltin("opa_range", 2);
  } numbers;
};

inline OPABuiltins opa_builtin;
inline OPAUserDefinedFunction OPAFunctionWrapper(size_t function) { return OPAUserDefinedFunction(function); }
inline OPABuiltin OPAFunctionWrapper(OPABuiltin function) { return function; }

struct OPAStringConstant final {
  size_t const string_index;
  explicit OPAStringConstant(size_t string_index) : string_index(string_index) {}
};

struct OPABoolean final {
  bool const boolean;
  OPABoolean() = delete;
  explicit OPABoolean(bool boolean) : boolean(boolean) {}
};

using callback_t = std::function<void()>;

enum class LocalType : uint8_t {
  Undefined = 0,
  Null = 1,
  String = 2,
  Number = 3,
  Boolean = 4,
  Array = 5,
  Object = 6,
  FunctionArgument = 7,  // NOTE(dkorolev): Effectively, `Any`.
  Unspecified = 255
};

struct LocalValue final {
  LocalType type = LocalType::Undefined;
  std::map<std::string, LocalValue> object_keys;

  static LocalValue Unspecified() {
    LocalValue x;
    x.type = LocalType::Unspecified;
    return x;
  }

  static LocalValue const& StaticUndefined() {
    static LocalValue undefined;
    return undefined;
  }

  static LocalValue const& StaticFunctionArgument() {
    static LocalValue any;
    any.type = LocalType::FunctionArgument;
    return any;
  }

  void ResetToUndefined(callback_t next) {
    LocalType const save_type = type;
    type = LocalType::Undefined;
    next();
    type = save_type;
  }

  void MakeNull(callback_t next) {
    LocalType const save_type = type;
    type = LocalType::Null;
    next();
    type = save_type;
  }

  void SetNumber(callback_t next) {
    LocalType const save_type = type;
    type = LocalType::Number;
    next();
    type = save_type;
  }

  void MakeObject(callback_t next) {
    LocalType const save_type = type;
    type = LocalType::Object;
    next();
    type = save_type;
  }

  void MarkAsFunctionArgument() { type = LocalType::FunctionArgument; }

  bool CouldBeUndefined() const { return type == LocalType::Undefined || type == LocalType::FunctionArgument; }
  bool CouldBeDefined() const { return type != LocalType::Undefined; };
  bool CouldBeString() const { return type == LocalType::String || type == LocalType::String; }

  void Assign(LocalValue const& rhs, callback_t next) {
    LocalType const save_type = type;
    auto const save_keys = object_keys;
    type = rhs.type;
    object_keys = rhs.object_keys;
    next();
    type = save_type;
    object_keys = save_keys;
  }

  void Assign(char const*, callback_t next) {
    LocalType const save_type = type;
    type = LocalType::String;
    next();
    type = save_type;
  }

  void Assign(OPABoolean, callback_t next) {
    LocalType const save_type = type;
    type = LocalType::String;
    next();
    type = save_type;
  }

  LocalValue const& GetByKey(char const* key) {
    if (type == LocalType::FunctionArgument) {
      return StaticFunctionArgument();
    }
    if (type != LocalType::Object) {
      // TODO(dkorolev): Message type, `file:row:col`.
      return StaticUndefined();
    }
    auto const cit = object_keys.find(key);
    if (cit == object_keys.end()) {
      // TODO(dkorolev): Message type, `file:row:col`.
      return StaticUndefined();
    }
    return cit->second;
  }

  void SetValueForKey(char const* key, LocalValue const& value, callback_t next) {
    if (type != LocalType::Object && type != LocalType::FunctionArgument) {
      // TODO(dkorolev): Message type, `file:row:col`.
      throw std::logic_error("Internal invariant failed.");
    }
    auto const save_type = type;
    type = LocalType::Object;
    if (object_keys.count(key)) {
      auto& placeholder = object_keys[key];
      auto const save = placeholder;
      placeholder = value;
      next();
      placeholder = save;
    } else {
      object_keys[key] = value;
      next();
      object_keys.erase(key);
    }
    type = save_type;
  }
};

inline std::ostream& operator<<(std::ostream& os, LocalValue const& value) {
  if (value.type == LocalType::Undefined) {
    os << "undefined";
  } else if (value.type == LocalType::FunctionArgument) {
    os << "function_argument";
  } else if (value.type == LocalType::Null) {
    os << "null";
  } else if (value.type == LocalType::String) {
    os << "string";
  } else if (value.type == LocalType::Number) {
    os << "number";
  } else if (value.type == LocalType::Boolean) {
    os << "boolean";
  } else if (value.type == LocalType::Array) {
    os << "array[TODO(dkorolev)]";
  } else if (value.type == LocalType::Object) {
    os << '{';
    bool first = true;
    for (auto const& e : value.object_keys) {
      if (first) {
        first = false;
      } else {
        os << ',';
      }
      os << e.first << ':' << e.second;
    }
    os << '}';
  } else {
    os << "unknown";
    throw std::logic_error("Internal invariant failed.");
  }
  return os;
}

struct Locals final {
  std::map<size_t, LocalValue> values;
  LocalValue return_value = LocalValue::Unspecified();
  std::vector<LocalValue> result_sets;  // TODO(dkorolev): Use a `set`? Optimize?

  LocalValue& operator[](OPALocalWrapper local) { return values[local.local]; }

  void AddToResultsSet(LocalValue const& value) {
    result_sets.push_back(value);
    // TODO(dkorolev): Can only use `AddToResultsSet` from plans, not from functions, right?
  }

  void SetReturnValue(LocalValue const& value, callback_t next) {
    if (return_value.type == LocalType::Undefined) {
      // TODO(dkorolev): Implement this.
      return;
    }
    return_value = value;
    next();
    return_value.type = LocalType::Undefined;
  }
};

inline std::ostream& operator<<(std::ostream& os, Locals const& locals) {
  for (auto const& e : locals.values) {
    os << "// locals[" << e.first << "]: " << e.second << std::endl;
  }
  for (auto const& e : locals.result_sets) {
    os << "// result: " << e << std::endl;
  }
  if (locals.return_value.type != LocalType::Unspecified) {
    os << "// return: " << locals.return_value << std::endl;
  }
  return os;
}

struct DeferClosingBrace final {};

enum class OutputHierarchyType : int { Break = 0, Statement = 1, Block = 2 };

struct OutputHierarchy final {
  OutputHierarchyType type;
  std::string stmt;
  std::vector<std::unique_ptr<OutputHierarchy>> children;
  bool defer_closing_brace = false;

  OutputHierarchy() = delete;

  struct OutputHierarchyBreak final {};
  struct OutputHierarchyStatement final {};
  struct OutputHierarchyBlock final {};

  explicit OutputHierarchy(OutputHierarchyBreak) : type(OutputHierarchyType::Break) {}
  explicit OutputHierarchy(OutputHierarchyStatement) : type(OutputHierarchyType::Statement) {}
  explicit OutputHierarchy(OutputHierarchyBlock) : type(OutputHierarchyType::Block) {}

  struct StatementInProgress final {
    std::string& destination;
    std::ostringstream os;
    bool& defer_closing_brace;
    StatementInProgress(std::string& destination, bool& defer_closing_brace)
        : destination(destination), defer_closing_brace(defer_closing_brace) {}
    ~StatementInProgress() { destination = os.str(); }
    template <typename T>
    StatementInProgress& operator<<(T&& x) {
      os << std::forward<T>(x);
      return *this;
    }
    StatementInProgress& operator<<(DeferClosingBrace) {
      defer_closing_brace = true;
      return *this;
    }
  };

  StatementInProgress AppendStatement() {
    children.emplace_back(std::make_unique<OutputHierarchy>(OutputHierarchyStatement()));
    return StatementInProgress(children.back()->stmt, children.back()->defer_closing_brace);
  }

  OutputHierarchy* AppendBlock() {
    children.emplace_back(std::make_unique<OutputHierarchy>(OutputHierarchyBlock()));
    return children.back().get();
  }

  void AppendBreak() { children.emplace_back(std::make_unique<OutputHierarchy>(OutputHierarchyBreak())); }

  void TopDownPrint(std::ostream& os) const {
    if (type == OutputHierarchyType::Block) {
      size_t end = 0u;
      while (end < children.size() && children[end]->type != OutputHierarchyType::Break) {
        ++end;
      }
      size_t braces_to_close = 0u;
      for (size_t i = 0u; i < end; ++i) {
        children[i]->TopDownPrint(os);
        if (children[i]->defer_closing_brace) {
          ++braces_to_close;
        }
      }
      while (braces_to_close) {
        os << '}';
        --braces_to_close;
      }
    } else {
      os << stmt;
    }
  }
};

struct Output final {
  Output() = delete;

  std::ostream& os;
  std::string at_end;

  OutputHierarchy root;
  OutputHierarchy* current;

  struct ForFunction final {};
  Output(std::ostream& os, ForFunction, size_t index, std::vector<size_t> const& args)
      : os(os), root(OutputHierarchy::OutputHierarchyBlock()), current(&root) {
    os << "value_t function_" << index << "(";
    for (size_t j = 0u; j < args.size(); ++j) {
      if (j) {
        os << ", ";
      }
      os << "value_t p" << j + 1u;
    }
    os << "){locals_t locals;";
    for (size_t j = 0u; j < args.size(); ++j) {
      os << "locals[" << args[j] << "]=p" << j + 1u << ";";
    }
    os << "value_t retval;";
    at_end = "return retval;}";
  }

  struct ForPlan final {};
  Output(std::ostream& os, ForPlan) : os(os), root(OutputHierarchy::OutputHierarchyBlock()), current(&root) {
    os << "result_set_t policy(value_t input,value_t data){";
    os << "locals_t locals;locals[0]=input;locals[1]=data;result_set_t result;";
    at_end = "return result;}";
  }

  ~Output() {
    root.TopDownPrint(os);
    os << at_end;
  }

  void ResetToUndefined(OPALocalWrapper source) {
    current->AppendStatement() << "locals[" << source.local << "].ResetToUndefined();";
  }

  void IsDefined(OPALocalWrapper source) {
    current->AppendStatement() << "if(!locals[" << source.local << "].IsUndefined()){" << DeferClosingBrace();
  }

  void IsUndefined(OPALocalWrapper source) {
    current->AppendStatement() << "if(locals[" << source.local << "].IsUndefined()){" << DeferClosingBrace();
  }

  void MakeNull(OPALocalWrapper target) { current->AppendStatement() << "locals[" << target.local << "].MakeNull();"; }

  void MakeObject(OPALocalWrapper target) {
    current->AppendStatement() << "locals[" << target.local << "].MakeObject();";
  }
  void MakeNumberRef(OPALocalWrapper target, std::string const& s) {
    current->AppendStatement() << "locals[" << target.local << "].SetNumberFromString(\"" << s << "\");";
  }

  void AssignVar(OPALocalWrapper source, OPALocalWrapper target) {
    current->AppendStatement() << "locals[" << target.local << "] = locals[" << source.local << "];";
  }
  void AssignVar(char const* s, OPALocalWrapper target) {
    current->AppendStatement() << "locals[" << target.local << "] = \"" << s << "\"";
  }
  void AssignVar(OPABoolean source, OPALocalWrapper target) {
    current->AppendStatement() << "locals[" << target.local << "].SetBoolean(" << std::boolalpha << source.boolean
                               << ");\n";
  }

  void AssignVarOnce(OPALocalWrapper source, OPALocalWrapper target) {
    current->AppendStatement() << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local
                               << "] = locals[" << source.local << "];}";
  }
  void AssignVarOnce(char const* s, OPALocalWrapper target) {
    current->AppendStatement() << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local
                               << "] = \"" << s << "\";}";
  }
  void AssignVarOnce(OPABoolean source, OPALocalWrapper target) {
    current->AppendStatement() << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local
                               << "].SetBoolean(" << std::boolalpha << source.boolean << ");}";
  }

  void Equal(OPALocalWrapper local, char const* s) {
    current->AppendStatement() << "if (locals[" << local.local << "].IsString(\"" << s << "\")){"
                               << DeferClosingBrace();
  }

  void Dot(OPALocalWrapper target, OPALocalWrapper source, char const* key) {
    current->AppendStatement() << "locals[" << target.local << "] = locals[" << source.local << "].GetByKey(\"" << key
                               << "\");";
  }

  void ObjectInsert(char const* key, OPALocalWrapper value, OPALocalWrapper object) {
    current->AppendStatement() << "locals[" << object.local << "].SetValueForKey(\"" << key << "\", locals["
                               << value.local << "]);";
  }

  void Call(OPALocalWrapper target, OPAUserDefinedFunction f, std::vector<OPALocalWrapper> const& args) {
    auto stmt = current->AppendStatement();
    stmt << "locals[" << target.local << "] = function_" << f.function << "(";
    for (size_t i = 0u; i < args.size(); ++i) {
      if (i) {
        stmt << ", ";
      }
      stmt << "locals[" << args[i].local << "]";
    }
    stmt << ");";
  }

  void Call(OPALocalWrapper target, OPABuiltin f, std::vector<OPALocalWrapper> const& args) {
    auto stmt = current->AppendStatement();
    stmt << "locals[" << target.local << "] = " << f.builtin_name << "(";
    for (size_t i = 0u; i < args.size(); ++i) {
      if (i) {
        stmt << ", ";
      }
      stmt << "locals[" << args[i].local << "]";
    }
    stmt << ");";
  }

  void AddToResultsSet(OPALocalWrapper value) {
    current->AppendStatement() << "result.AddToResultSet(locals[" << value.local << "]);\n";
  }

  void SetReturnValue(OPALocalWrapper source) {
    current->AppendStatement() << "retval = locals[" << source.local << "];\n";
  }

  void Break() { current->AppendBreak(); }

  struct InnerScope final {
    Output& self;
    OutputHierarchy* save;
    explicit InnerScope(Output& self) : self(self), save(self.current) { self.current = self.current->AppendBlock(); }
    ~InnerScope() { self.current = save; }
  };
};

struct IRStatement final {
  using analyze_t = std::function<void(Locals&, callback_t next, callback_t done)>;
  using output_t = std::function<void(Output&)>;
  using dump_t = std::function<void(callback_t)>;
  analyze_t analyze;
  output_t output;
  dump_t dump;
  IRStatement(analyze_t analyze, output_t output, dump_t dump) : analyze(analyze), output(output), dump(dump) {}
  static IRStatement Dummy() {
    return IRStatement(
        [](Locals&, callback_t, callback_t) {}, [](Output& callback_t) {}, [](callback_t next) { next(); });
  }
};

struct Policy final {
  using code_t = IRStatement;

  std::vector<std::string> strings;
  std::vector<std::vector<size_t>> functions;
  std::vector<std::function<IRStatement(Policy const& policy)>> function_bodies;
  std::map<std::string, std::function<IRStatement(Policy const& policy)>> plans;
};

inline static Policy* policy_singleton;

IRStatement stmtAssignVarOnce(OPALocalWrapper source, OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].Assign(locals[source], next); },
                     [=](Output& output) { output.AssignVarOnce(source, target); },
                     [=](callback_t next) {
                       std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local
                                 << "] = locals[" << source.local << "];\n";
                       next();
                       std::cout << "}\n";
                     });
}

IRStatement stmtAssignVarOnce(const char* source, OPALocalWrapper target) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        if (locals[target].CouldBeUndefined()) {
          // TODO(dkorolev): Or is such a failure "breaking" the block?
          locals[target].Assign(source, next);
        } else {
          next();
        }
      },
      [=](Output& output) { output.AssignVarOnce(source, target); },
      [=](callback_t next) {
        std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "].SetString(\""
                  << source << "\");\n";
        next();
        std::cout << "}\n";
      });
}

IRStatement stmtAssignVarOnce(OPABoolean source, OPALocalWrapper target) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        if (locals[target].CouldBeUndefined()) {
          locals[target].Assign(source, next);
        } else {
          next();
        }
      },
      [=](Output& output) { output.AssignVarOnce(source, target); },
      [=](callback_t next) {
        // TODO(dkorolev): When to break?
        std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "].SetBoolean("
                  << std::boolalpha << source.boolean << ");\n";
        next();
        std::cout << "}\n";
      });
}

IRStatement stmtAssignVar(OPALocalWrapper source, OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].Assign(locals[source], next); },
                     [=](Output& output) { output.AssignVar(source, target); },
                     [=](callback_t next) {
                       std::cout << "locals[" << target.local << "] = locals[" << source.local << "];\n";
                       next();
                     });
}

IRStatement stmtDot(OPALocalWrapper source, char const* key, OPALocalWrapper target) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) { locals[target].Assign(locals[source].GetByKey(key), next); },
      [=](Output& output) { output.Dot(target, source, key); },
      [=](callback_t next) {
        std::cout << "locals[" << target.local << "] = locals[" << source.local << "].GetByKey(\"" << key << "\");";
        next();
      });
}

IRStatement stmtEqual(OPALocalWrapper a, const char* b) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t done) {
        done();
        if (locals[a].CouldBeString()) {
          // NOTE(dkorolev): No reason to move forward if the comparison is obviously false.
          next();
        }
      },
      [=](Output& output) { output.Equal(a, b); },
      [=](callback_t next) {
        std::cout << "if (locals[" << a.local << "].IsString(\"" << b << "\")) {\n";
        next();
        std::cout << "}\n";
      });
}

IRStatement stmtIsDefined(OPALocalWrapper source) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t done) {
        if (locals[source].CouldBeDefined()) {
          next();
        } else {
          done();
        }
      },
      [=](Output& output) { output.IsDefined(source); },
      [=](callback_t next) {
        std::cout << "if (!locals[" << source.local << "].IsUndefined()) {\n";
        next();
        std::cout << "}\n";
      });
}

IRStatement stmtIsUndefined(OPALocalWrapper source) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t done) {
        if (locals[source].CouldBeUndefined()) {
          next();
        } else {
          done();
        }
      },
      [=](Output& output) { output.IsUndefined(source); },
      [=](callback_t next) {
        std::cout << "if (locals[" << source.local << "].IsUndefined()) {\n";
        next();
        std::cout << "}\n";
      });
}

IRStatement stmtMakeNull(OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].MakeNull(next); },
                     [=](Output& output) { output.MakeNull(target); },
                     [=](callback_t next) {
                       std::cout << "locals[" << target.local << "].MakeNull();\n";
                       next();
                     });
}

IRStatement stmtMakeNumberRef(OPAStringConstant value, OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].SetNumber(next); },
                     [=](Output& output) {
                       Policy const& policy = *policy_singleton;
                       output.MakeNumberRef(target, policy.strings[value.string_index]);
                     },
                     [=](callback_t next) {
                       Policy const& policy = *policy_singleton;
                       // TODO: Not `FromString`, of course.
                       std::cout << "locals[" << target.local << "].SetNumberFromString(\""
                                 << policy.strings[value.string_index] << "\");\n";
                       next();
                     });
}

IRStatement stmtMakeObject(OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].MakeObject(next); },
                     [=](Output& output) { output.MakeObject(target); },
                     [=](callback_t next) {
                       std::cout << "locals[" << target.local << "].MakeObject();\n";
                       next();
                     });
}

// NOTE(dkorolev): This is a static, `constexpr`, "compile-time" check. It should not generate any output code.
IRStatement stmtNotEqual(OPABoolean a, OPABoolean b) {
  if (a.boolean != b.boolean) {
    return IRStatement([=](Locals&, callback_t next, callback_t) { next(); },
                       [=](Output&) {},  // NOTE(dkorolev: A no-op.
                       [=](callback_t next) { next(); });
  } else {
    return IRStatement([=](Locals&, callback_t, callback_t done) { done(); },
                       [=](Output& output) { output.Break(); },
                       [=](callback_t) {});
  }
}

IRStatement stmtObjectInsert(char const* key, OPALocalWrapper value, OPALocalWrapper object) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) { locals[object].SetValueForKey(key, locals[value], next); },
      [=](Output& output) { output.ObjectInsert(key, value, object); },
      [=](callback_t next) {
        std::cout << "locals[" << object.local << "].SetValueForKey(\"" << key << "\", locals[" << value.local
                  << "]);\n";
        next();
      });
}

IRStatement stmtResetLocal(OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].ResetToUndefined(next); },
                     [=](Output& output) { output.ResetToUndefined(target); },
                     [=](callback_t next) {
                       std::cout << "locals[" << target.local << "].ResetToUndefined();\n";
                       next();
                     });
}

IRStatement stmtResultSetAdd(OPALocalWrapper value) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        locals.AddToResultsSet(locals[value]);
        next();
      },
      [=](Output& output) { output.AddToResultsSet(value); },
      [=](callback_t next) {
        std::cout << "result.AddToResultSet(locals[" << value.local << "]);\n";
        next();
      });
}

// TODO(dkorolev): Think deeper about "recursion" / `walk` in this aspect.
IRStatement stmtReturnLocalStmt(OPALocalWrapper source) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        // TODO(dkorolev): Ask the OPA folks whether this should "return" or just save the return value. And is there
        // redundancy.
        locals.SetReturnValue(locals[source], next);
      },
      [=](Output& output) {
        // TODO(dkorolev): Ask the OPA folks whether this should "return" or just save the return value. And is there
        // redundancy.
        output.SetReturnValue(source);
      },
      [=](callback_t next) {
        std::cout << "retval = locals[" << source.local << "];\n";
        next();
      });
}

IRStatement stmtCall(OPAUserDefinedFunction f, std::vector<OPALocalWrapper> const& args, OPALocalWrapper target) {
  Policy const& policy = *policy_singleton;
  if (args.size() != policy.functions[f.function].size()) {
    throw std::logic_error("Internal invariant failed.");
  }

  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        next();
        // TODO(dkorolev): Implement this.
      },
      [=](Output& output) { output.Call(target, f, args); },
      [=](callback_t next) {
        std::cout << "locals[" << target.local << "] = function_" << f.function << "(";
        for (size_t i = 0u; i < args.size(); ++i) {
          if (i) {
            std::cout << ", ";
          }
          std::cout << "locals[" << args[i].local << "]";
        }
        std::cout << ");\n";

        next();
      });
}

IRStatement stmtCall(OPABuiltin f, std::vector<OPALocalWrapper> const& args, OPALocalWrapper target) {
  if (args.size() != f.args_count) {
    throw std::logic_error("Internal invariant failed.");
  }

  // TODO: Fix this copy-paste in `stmtCall`.
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        next();
        // TODO(dkorolev): Implement this.
      },
      [=](Output& output) { output.Call(target, f, args); },
      [=](callback_t next) {
        std::cout << "locals[" << target.local << "] = " << f.builtin_name << "(";
        for (size_t i = 0u; i < f.args_count; ++i) {
          if (i) {
            std::cout << ", ";
          }
          std::cout << "locals[" << args[i].local << "]";
        }
        std::cout << ");\n";

        next();
      });
}

IRStatement stmtBlock(std::vector<IRStatement> const& code) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t unused_done) {
        if (!code.empty()) {
          std::function<void(size_t)> doit = [&](size_t i) {
            if (i < code.size()) {
              code[i].analyze(
                  locals, [i, &doit, &next]() { doit(i + 1u); }, next);
            }
          };
          doit(0u);
        }
        next();
      },
      [=](Output& output) {
        typename Output::InnerScope const inner(output);
        for (IRStatement const& stmt : code) {
          stmt.output(output);
        }
      },
      [=](callback_t next) {
        if (!code.empty()) {
          std::function<void(size_t)> doit = [&](size_t i) {
            if (i < code.size()) {
              code[i].dump([i, &doit]() { doit(i + 1u); });
            }
          };
          doit(0u);
        }
        next();
      });
}
