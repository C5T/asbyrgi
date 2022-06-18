// TODO: Escape all the strings, of course. Once Current/JSON is in the picture.

#include <cstdarg>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
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

struct IRStatement final {
  std::function<void(Locals&, callback_t next, callback_t done)> analyze;
  std::function<void(callback_t)> dump;
  IRStatement(std::function<void(Locals&, callback_t, callback_t)> analyze, std::function<void(callback_t)> dump)
      : analyze(analyze), dump(dump) {}
  static IRStatement Dummy() {
    return IRStatement([](Locals&, callback_t, callback_t) {}, [](callback_t next) { next(); });
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
      [=](callback_t next) {
        std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "].SetBoolean("
                  << std::boolalpha << source.boolean << ");\n";
        next();
        std::cout << "}\n";
      });
}

IRStatement stmtAssignVar(OPALocalWrapper source, OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].Assign(locals[source], next); },
                     [=](callback_t next) {
                       std::cout << "locals[" << target.local << "] = locals[" << source.local << "];\n";
                       next();
                     });
}

IRStatement stmtDot(OPALocalWrapper source, char const* key, OPALocalWrapper target) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) { locals[target].Assign(locals[source].GetByKey(key), next); },
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
      [=](callback_t next) {
        std::cout << "if (locals[" << source.local << "].IsUndefined()) {\n";
        next();
        std::cout << "}\n";
      });
}

IRStatement stmtMakeNull(OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].MakeNull(next); },
                     [=](callback_t next) {
                       std::cout << "locals[" << target.local << "].MakeNull();\n";
                       next();
                     });
}

IRStatement stmtMakeNumberRef(OPAStringConstant value, OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].SetNumber(next); },
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
                     [=](callback_t next) {
                       std::cout << "locals[" << target.local << "].MakeObject();\n";
                       next();
                     });
}

// NOTE(dkorolev): This is a static, `constexpr`, "compile-time" check. It should not generate any output code.
IRStatement stmtNotEqual(OPABoolean a, OPABoolean b) {
  if (a.boolean != b.boolean) {
    return IRStatement([=](Locals&, callback_t next, callback_t) { next(); }, [=](callback_t next) { next(); });
  } else {
    return IRStatement([=](Locals&, callback_t, callback_t done) { done(); }, [=](callback_t) {});
  }
}

IRStatement stmtObjectInsert(char const* key, OPALocalWrapper value, OPALocalWrapper object) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) { locals[object].SetValueForKey(key, locals[value], next); },
      [=](callback_t next) {
        std::cout << "locals[" << object.local << "].SetValueForKey(\"" << key << "\", locals[" << value.local
                  << "]);\n";
        next();
      });
}

IRStatement stmtResetLocal(OPALocalWrapper target) {
  return IRStatement([=](Locals& locals, callback_t next, callback_t) { locals[target].ResetToUndefined(next); },
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
