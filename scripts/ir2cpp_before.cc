// TODO: Escape all the strings, of course. Once Current/JSON is in the picture.

#include <cstdarg>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
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

struct OutputVarsState final {
  std::map<size_t, std::string> var_name;
  size_t vars_count = 0u;
  std::set<std::string> all;
};

struct DeferClosingBrace final {};

enum class OutputHierarchyType : int { Break = 0, Statement = 1, Block = 2 };

struct OutputHierarchy final {
  OutputHierarchyType type;
  OutputVarsState& vars;

  std::string stmt;
  std::vector<std::unique_ptr<OutputHierarchy>> children;
  bool defer_closing_brace = false;

  OutputHierarchy() = delete;

  struct OutputHierarchyBreak final {};
  struct OutputHierarchyStatement final {};
  struct OutputHierarchyBlock final {};

  OutputHierarchy(OutputHierarchyBreak, OutputVarsState& vars) : type(OutputHierarchyType::Break), vars(vars) {}
  OutputHierarchy(OutputHierarchyStatement, OutputVarsState& vars) : type(OutputHierarchyType::Statement), vars(vars) {}
  OutputHierarchy(OutputHierarchyBlock, OutputVarsState& vars) : type(OutputHierarchyType::Block), vars(vars) {}

  struct StatementInProgress final {
    OutputVarsState& vars;
    std::string& destination;
    std::ostringstream os;
    bool& defer_closing_brace;
    StatementInProgress(OutputVarsState& vars, std::string& destination, bool& defer_closing_brace)
        : vars(vars), destination(destination), defer_closing_brace(defer_closing_brace) {}
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
    children.emplace_back(std::make_unique<OutputHierarchy>(OutputHierarchyStatement(), vars));
    return StatementInProgress(vars, children.back()->stmt, children.back()->defer_closing_brace);
  }

  OutputHierarchy* AppendBlock() {
    children.emplace_back(std::make_unique<OutputHierarchy>(OutputHierarchyBlock(), vars));
    return children.back().get();
  }

  void AppendBreak() { children.emplace_back(std::make_unique<OutputHierarchy>(OutputHierarchyBreak(), vars)); }

  void AppendIntroduceOrInitialize(OPALocalWrapper var, std::string const& initializer) {
    if (initializer.empty()) {
      throw std::logic_error("Internal invariant failed.");
    }
    std::string& var_name_placeholder = vars.var_name[var.local];
    if (var_name_placeholder.empty()) {
      std::ostringstream os;
      os << 'x' << ++vars.vars_count;
      var_name_placeholder = os.str();
      vars.all.insert(var_name_placeholder);
    }
    AppendStatement() << var_name_placeholder << '=' << initializer << ';';
  }

  void AppendIntroduceOrInitialize(OPALocalWrapper var, OPALocalWrapper initializer) {
    if (vars.var_name[initializer.local].empty()) {
      throw std::logic_error("Internal invariant failed.");
    }
    std::string& var_name_placeholder = vars.var_name[var.local];
    if (var_name_placeholder.empty()) {
      std::ostringstream os;
      os << 'x' << ++vars.vars_count;
      var_name_placeholder = os.str();
      vars.all.insert(var_name_placeholder);
    }
    AppendStatement() << var_name_placeholder << '=' << vars.var_name[initializer.local] << ';';
  }

  void AppendInitializeIfUndefined(OPALocalWrapper var, std::string const& initializer) {
    std::string& var_name_placeholder = vars.var_name[var.local];
    if (var_name_placeholder.empty()) {
      std::ostringstream os;
      os << 'x' << ++vars.vars_count;
      var_name_placeholder = os.str();
      vars.all.insert(var_name_placeholder);
      AppendStatement() << var_name_placeholder << '=' << initializer << ';';
    } else {
      AppendStatement() << "if(" << var_name_placeholder << ".IsUndefined()){" << var_name_placeholder << '='
                        << initializer << ";}";
    }
  }

  void AppendInitializeIfUndefined(OPALocalWrapper var, OPALocalWrapper initializer) {
    std::string& var_name_placeholder = vars.var_name[var.local];
    if (var_name_placeholder.empty()) {
      std::ostringstream os;
      os << 'x' << ++vars.vars_count;
      var_name_placeholder = os.str();
      vars.all.insert(var_name_placeholder);
      AppendStatement() << var_name_placeholder << '=' << vars.var_name[initializer.local] << ';';
    } else {
      AppendStatement() << "if(" << var_name_placeholder << ".IsUndefined()){" << var_name_placeholder << '='
                        << vars.var_name[initializer.local] << ";}";
    }
  }

  void AppendAssignOrInitialize(OPALocalWrapper var, std::string const& initializer) {
    // TODO(dkorolev): Remove this WIP rudiment.
    AppendIntroduceOrInitialize(var, initializer);
  }

  void AppendAssignOrInitialize(OPALocalWrapper var, OPALocalWrapper initializer) {
    // TODO(dkorolev): Remove this WIP rudiment.
    AppendIntroduceOrInitialize(var, initializer);
  }

  std::string Materialize(OPALocalWrapper var) {
    // TODO(dkorolev): Remove this WIP rudiment.
    if (vars.var_name[var.local].empty()) {
      std::ostringstream os;
      os << "WTF[" << var.local << "]";
      return os.str();
    }
    return vars.var_name[var.local];
  }

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

  OutputVarsState vars;

  OutputHierarchy root;
  OutputHierarchy* current;

  struct ForFunction final {};
  Output(std::ostream& os, ForFunction, size_t index, std::vector<size_t> const& args)
      : os(os), root(OutputHierarchy::OutputHierarchyBlock(), vars), current(&root) {
    os << "value_t function_" << index << "(";
    for (size_t j = 0u; j < args.size(); ++j) {
      if (j) {
        os << ", ";
      }
      os << "value_t p" << j + 1u;
    }
    os << "){";
    for (size_t j = 0u; j < args.size(); ++j) {
      std::ostringstream os;
      os << 'p' << j + 1u;
      vars.var_name[args[j]] = os.str();
    }
    os << "value_t retval;";
    at_end = "return retval;}";
  }

  struct ForPlan final {};
  Output(std::ostream& os, ForPlan) : os(os), root(OutputHierarchy::OutputHierarchyBlock(), vars), current(&root) {
    os << "result_set_t policy(value_t input,value_t data){result_set_t result;";
    vars.var_name[0] = "input";
    vars.var_name[1] = "data";
    at_end = "return result;}";
  }

  ~Output() {
    if (!vars.all.empty()) {
      bool first = true;
      os << "value_t ";
      for (auto const& v : vars.all) {
        if (first) {
          first = false;
        } else {
          os << ',';
        }
        os << v;
      }
      os << ';';
    }
    root.TopDownPrint(os);
    os << at_end;
  }

  void ResetToUndefined(OPALocalWrapper source) { current->AppendIntroduceOrInitialize(source, "ResetToUndefined()"); }

  void IsDefined(OPALocalWrapper value) {
    std::string const materialized_value = current->Materialize(value);
    current->AppendStatement() << "if(!" << materialized_value << ".IsUndefined()){" << DeferClosingBrace();
  }

  void IsUndefined(OPALocalWrapper value) {
    std::string const materialized_value = current->Materialize(value);
    current->AppendStatement() << "if(" << materialized_value << ".IsUndefined()){" << DeferClosingBrace();
  }

  void MakeNull(OPALocalWrapper target) { current->AppendIntroduceOrInitialize(target, "MakeNull()"); }

  void MakeObject(OPALocalWrapper target) { current->AppendIntroduceOrInitialize(target, "MakeObject()"); }
  void MakeNumberRef(OPALocalWrapper target, std::string const& s) {
    std::ostringstream os;
    os << "SetNumberFromString(\"" << s << "\")";
    current->AppendIntroduceOrInitialize(target, os.str());
  }

  void AssignVar(OPALocalWrapper source, OPALocalWrapper target) { current->AppendAssignOrInitialize(target, source); }
  void AssignVar(char const* s, OPALocalWrapper target) {
    std::ostringstream os;
    os << "SetString(\"" << s << "\")";
    current->AppendAssignOrInitialize(target, os.str());
  }
  void AssignVar(OPABoolean source, OPALocalWrapper target) {
    std::ostringstream os;
    os << "SetBoolean(" << std::boolalpha << source.boolean << ')';
    current->AppendAssignOrInitialize(target, os.str());
  }

  void AssignVarOnce(OPALocalWrapper source, OPALocalWrapper target) {
    current->AppendInitializeIfUndefined(target, source);
  }
  void AssignVarOnce(char const* s, OPALocalWrapper target) {
    std::ostringstream os;
    os << "SetString(\"" << s << "\")";
    current->AppendInitializeIfUndefined(target, os.str());
  }
  void AssignVarOnce(OPABoolean source, OPALocalWrapper target) {
    std::ostringstream os;
    os << "SetBoolean(" << std::boolalpha << source.boolean << ')';
    current->AppendInitializeIfUndefined(target, os.str());
  }

  void Equal(OPALocalWrapper value, char const* s) {
    std::string const materialized_value = current->Materialize(value);
    current->AppendStatement() << "if (" << materialized_value << ".IsString(\"" << s << "\")){" << DeferClosingBrace();
  }

  void Dot(OPALocalWrapper target, OPALocalWrapper source, char const* key) {
    std::ostringstream os;
    os << current->Materialize(source) << ".GetByKey(\"" << key << "\")";
    current->AppendAssignOrInitialize(target, os.str());
  }

  void ObjectInsert(char const* key, OPALocalWrapper value, OPALocalWrapper object) {
    std::string const materialized_value = current->Materialize(value);
    current->AppendStatement() << current->Materialize(object) << ".SetValueForKey(\"" << key << "\", "
                               << materialized_value << ");";
  }

  void Call(OPALocalWrapper target, OPAUserDefinedFunction f, std::vector<OPALocalWrapper> const& args) {
    std::vector<std::string> materialized_args(args.size());
    for (size_t i = 0u; i < args.size(); ++i) {
      materialized_args[i] = current->Materialize(args[i]);
    }
    std::ostringstream os;
    os << "function_" << f.function << '(';
    for (size_t i = 0u; i < args.size(); ++i) {
      if (i) {
        os << ',';
      }
      os << materialized_args[i];
    }
    os << ')';
    current->AppendAssignOrInitialize(target, os.str());
  }

  void Call(OPALocalWrapper target, OPABuiltin f, std::vector<OPALocalWrapper> const& args) {
    std::vector<std::string> materialized_args(args.size());
    for (size_t i = 0u; i < args.size(); ++i) {
      materialized_args[i] = current->Materialize(args[i]);
    }
    std::ostringstream os;
    os << f.builtin_name << '(';
    for (size_t i = 0u; i < args.size(); ++i) {
      if (i) {
        os << ',';
      }
      os << materialized_args[i];
    }
    os << ')';
    current->AppendAssignOrInitialize(target, os.str());
  }

  void AddToResultsSet(OPALocalWrapper value) {
    std::string const materialized_value = current->Materialize(value);
    current->AppendStatement() << "result.AddToResultSet(" << materialized_value << ");";
  }

  void SetReturnValue(OPALocalWrapper value) {
    std::string const materialized_value = current->Materialize(value);
    current->AppendStatement() << "retval=" << materialized_value << ';';
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
  analyze_t analyze;
  output_t output;
  IRStatement(analyze_t analyze, output_t output) : analyze(analyze), output(output) {}
  static IRStatement Dummy() {
    return IRStatement([](Locals&, callback_t, callback_t) {}, [](Output& callback_t) {});
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
                     [=](Output& output) { output.AssignVarOnce(source, target); });
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
      [=](Output& output) { output.AssignVarOnce(source, target); });
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
      [=](Output& output) { output.AssignVarOnce(source, target); });
}

IRStatement stmtAssignVar(OPALocalWrapper source, OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].Assign(locals[source], next); },
                     [=](Output& output) { output.AssignVar(source, target); });
}

IRStatement stmtDot(OPALocalWrapper source, char const* key, OPALocalWrapper target) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) { locals[target].Assign(locals[source].GetByKey(key), next); },
      [=](Output& output) { output.Dot(target, source, key); });
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
      [=](Output& output) { output.Equal(a, b); });
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
      [=](Output& output) { output.IsDefined(source); });
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
      [=](Output& output) { output.IsUndefined(source); });
}

IRStatement stmtMakeNull(OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].MakeNull(next); },
                     [=](Output& output) { output.MakeNull(target); });
}

IRStatement stmtMakeNumberRef(OPAStringConstant value, OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].SetNumber(next); },
                     [=](Output& output) {
                       Policy const& policy = *policy_singleton;
                       output.MakeNumberRef(target, policy.strings[value.string_index]);
                     });
}

IRStatement stmtMakeObject(OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].MakeObject(next); },
                     [=](Output& output) { output.MakeObject(target); });
}

// NOTE(dkorolev): This is a static, `constexpr`, "compile-time" check. It should not generate any output code.
IRStatement stmtNotEqual(OPABoolean a, OPABoolean b) {
  if (a.boolean != b.boolean) {
    return IRStatement([=](Locals&, callback_t next, callback_t) { next(); },
                       [=](Output&) {});  // NOTE(dkorolev: A no-op.
  } else {
    return IRStatement([=](Locals&, callback_t, callback_t done) { done(); }, [=](Output& output) { output.Break(); });
  }
}

IRStatement stmtObjectInsert(char const* key, OPALocalWrapper value, OPALocalWrapper object) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) { locals[object].SetValueForKey(key, locals[value], next); },
      [=](Output& output) { output.ObjectInsert(key, value, object); });
}

IRStatement stmtResetLocal(OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].ResetToUndefined(next); },
                     [=](Output& output) { output.ResetToUndefined(target); });
}

IRStatement stmtResultSetAdd(OPALocalWrapper value) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        locals.AddToResultsSet(locals[value]);
        next();
      },
      [=](Output& output) { output.AddToResultsSet(value); });
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
      [=](Output& output) { output.Call(target, f, args); });
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
      [=](Output& output) { output.Call(target, f, args); });
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
      });
}
