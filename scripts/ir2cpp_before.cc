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

enum class LocalType : uint8_t { Undefined = 0, Null = 1, String = 2, Number = 3, Boolean = 4, Array = 5, Object = 6 };

struct LocalValue final {
  LocalType type = LocalType::Undefined;
  std::map<std::string, LocalValue> object_keys;

  void ResetToUndefined() { type = LocalType::Undefined; }
  void MakeNull() { type = LocalType::Null; }
  void SetNumber() { type = LocalType::Number; }
  void MakeObject() { type = LocalType::Object; }
  bool IsUndefined() const { return type == LocalType::Undefined; }

  LocalValue& operator=(LocalValue const& rhs) {
    type = rhs.type;
    return *this;
  }
  LocalValue& operator=(char const*) {
    type = LocalType::String;
    return *this;
  }
  LocalValue& operator=(OPABoolean) {
    type = LocalType::Boolean;
    return *this;
  }

  LocalValue& GetByKey(char const* key) {
    if (type != LocalType::Object) {
      // TODO(dkorolev): Message type, `file:row:col`.
      throw std::logic_error("Internal invariant failed.");
    }
    auto const cit = object_keys.find(key);
    if (cit == object_keys.end()) {
      // TODO(dkorolev): Message type, `file:row:col`.
      throw std::logic_error("Internal invariant failed.");
    }
    return cit->second;
  }

  void SetValueForKey(char const* key, LocalValue const& value) { object_keys[key] = value; }
};

struct Locals final {
  std::map<size_t, LocalValue> values;
  LocalValue return_value;

  LocalValue& operator[](OPALocalWrapper local) { return values[local.local]; }

  void AddToResultsSet(LocalValue const&) {
    // TODO(dkorolev): Implement this.
    // TODO(dkorolev): Can only use `AddToResultsSet` from plans, not from functions, right?
  }

  void SetReturnValue(LocalValue const& value) { return_value = value; }
};

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
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        locals[target] = locals[source];
        next();
      },
      [=](callback_t next) {
        std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "] = locals["
                  << source.local << "];\n";
        next();
        std::cout << "}\n";
      });
}

IRStatement stmtAssignVarOnce(const char* source, OPALocalWrapper target) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        if (locals[target].IsUndefined()) {
          // TODO(dkorolev): Or is such a failure "breaking" the block?
          locals[target] = source;
        }
        next();
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
        if (locals[target].IsUndefined()) {
          locals[target] = source;
        }
        next();
      },
      [=](callback_t next) {
        std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "].SetBoolean("
                  << std::boolalpha << source.boolean << ");\n";
        next();
        std::cout << "}\n";
      });
}

IRStatement stmtAssignVar(OPALocalWrapper source, OPALocalWrapper target) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        locals[target] = locals[source];
        next();
      },
      [=](callback_t next) {
        std::cout << "locals[" << target.local << "] = locals[" << source.local << "];\n";
        next();
      });
}

IRStatement stmtDot(OPALocalWrapper source, char const* key, OPALocalWrapper target) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        locals[target] = locals[source].GetByKey(key);
        next();
      },
      [=](callback_t next) {
        std::cout << "locals[" << target.local << "] = locals[" << source.local << "].GetByKey(\"" << key << "\");";
        next();
      });
}

IRStatement stmtEqual(OPALocalWrapper a, const char* b) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t done) {
        next();
        done();
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
        if (locals[source].IsUndefined()) {
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
        if (!locals[source].IsUndefined()) {
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
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        locals[target].MakeNull();
        next();
      },
      [=](callback_t next) {
        std::cout << "locals[" << target.local << "].MakeNull();\n";
        next();
      });
}

IRStatement stmtMakeNumberRef(OPAStringConstant value, OPALocalWrapper target) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        locals[target].SetNumber();
        next();
      },
      [=](callback_t next) {
        Policy const& policy = *policy_singleton;
        // TODO: Not `FromString`, of course.
        std::cout << "locals[" << target.local << "].SetNumberFromString(\"" << policy.strings[value.string_index]
                  << "\");\n";
        next();
      });
}

IRStatement stmtMakeObject(OPALocalWrapper target) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        locals[target].MakeObject();
        next();
      },
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
      [=](Locals& locals, callback_t next, callback_t) {
        locals[object].SetValueForKey(key, locals[value]);
        next();
      },
      [=](callback_t next) {
        std::cout << "locals[" << object.local << "].SetValueForKey(\"" << key << "\", locals[" << value.local
                  << "]);\n";
        next();
      });
}

IRStatement stmtResetLocal(OPALocalWrapper target) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        locals[target].ResetToUndefined();
        next();
      },
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

IRStatement stmtReturnLocalStmt(OPALocalWrapper source) {
  return IRStatement(
      [=](Locals& locals, callback_t next, callback_t) {
        // TODO(dkorolev): Ask the OPA folks whether this should "return" or just save the return value. And is there
        // redundancy.
        locals.SetReturnValue(locals[source]);
        next();
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
      [=](Locals& locals, callback_t next, callback_t done) {
        // TODO(dkorolev): Implement this.
      },
      [=](callback_t next) {
        if (!code.empty()) {
          size_t i = 0u;

          callback_t inner_next = [&]() {
            if (i < code.size()) {
              size_t const save = i;
              ++i;
              code[save].dump(inner_next);
            }
          };

          inner_next();
        }
        next();
      });
}
