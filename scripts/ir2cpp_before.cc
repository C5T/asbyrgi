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

struct IRStatement final {
  std::function<void(callback_t)> dump;
  IRStatement(std::function<void(callback_t)> dump) : dump(dump) {}
  static IRStatement Dummy() {
    return IRStatement([](callback_t next) { next(); });
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
  return IRStatement([=](callback_t next) {
    std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "] = locals["
              << source.local << "];\n";
    next();
    std::cout << "}\n";
  });
}

IRStatement stmtAssignVarOnce(const char* source, OPALocalWrapper target) {
  return IRStatement([=](callback_t next) {
    std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "].SetString(\""
              << source << "\");\n";
    next();
    std::cout << "}\n";
  });
}

IRStatement stmtAssignVarOnce(OPABoolean source, OPALocalWrapper target) {
  return IRStatement([=](callback_t next) {
    std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "].SetBoolean("
              << std::boolalpha << source.boolean << ");\n";
    next();
    std::cout << "}\n";
  });
}

IRStatement stmtAssignVar(OPALocalWrapper source, OPALocalWrapper target) {
  return IRStatement([=](callback_t next) {
    std::cout << "locals[" << target.local << "] = locals[" << source.local << "];\n";
    next();
  });
}

IRStatement stmtDot(OPALocalWrapper source, char const* key, OPALocalWrapper target) {
  return IRStatement([=](callback_t next) {
    std::cout << "locals[" << target.local << "] = locals[" << source.local << "].GetByKey(\"" << key << "\");";
    next();
  });
}

IRStatement stmtEqual(OPALocalWrapper a, const char* b) {
  return IRStatement([=](callback_t next) {
    std::cout << "if (locals[" << a.local << "].IsString(\"" << b << "\")) {\n";
    next();
    std::cout << "}\n";
  });
}

IRStatement stmtIsDefined(OPALocalWrapper source) {
  return IRStatement([=](callback_t next) {
    std::cout << "if (!locals[" << source.local << "].IsUndefined()) {\n";
    next();
    std::cout << "}\n";
  });
}

IRStatement stmtIsUndefined(OPALocalWrapper source) {
  return IRStatement([=](callback_t next) {
    std::cout << "if (locals[" << source.local << "].IsUndefined()) {\n";
    next();
    std::cout << "}\n";
  });
}

IRStatement stmtMakeNull(OPALocalWrapper target) {
  return IRStatement([=](callback_t next) {
    std::cout << "locals[" << target.local << "].MakeNull();\n";
    next();
  });
}

IRStatement stmtMakeNumberRef(OPAStringConstant value, OPALocalWrapper target) {
  return IRStatement([=](callback_t next) {
    Policy const& policy = *policy_singleton;
    // TODO: Not `FromString`, of course.
    std::cout << "locals[" << target.local << "].SetNumberFromString(\"" << policy.strings[value.string_index]
              << "\");\n";
    next();
  });
}

IRStatement stmtMakeObject(OPALocalWrapper target) {
  return IRStatement([=](callback_t next) {
    std::cout << "locals[" << target.local << "].MakeObject();\n";
    next();
  });
}

IRStatement stmtNotEqual(OPABoolean a, OPABoolean b) {
  if (a.boolean != b.boolean) {
    return IRStatement([=](callback_t next) { next(); });
  } else {
    return IRStatement::Dummy();
  }
}

IRStatement stmtObjectInsert(char const* key, OPALocalWrapper value, OPALocalWrapper object) {
  return IRStatement([=](callback_t next) {
    std::cout << "locals[" << object.local << "].SetValueForKey(\"" << key << "\", locals[" << value.local << "]);\n";
    next();
  });
}

IRStatement stmtResetLocal(OPALocalWrapper target) {
  return IRStatement([=](callback_t next) {
    std::cout << "locals[" << target.local << "].ResetToUndefined();\n";
    next();
  });
}

IRStatement stmtResultSetAdd(OPALocalWrapper value) {
  return IRStatement([=](callback_t next) {
    std::cout << "result.AddToResultSet(locals[" << value.local << "]);\n";
    next();
  });
}

IRStatement stmtReturnLocalStmt(OPALocalWrapper source) {
  return IRStatement([=](callback_t next) {
    std::cout << "retval = locals[" << source.local << "];\n";
    next();
  });
}

IRStatement stmtCall(OPAUserDefinedFunction f, std::vector<OPALocalWrapper> const& args, OPALocalWrapper target) {
  Policy const& policy = *policy_singleton;
  if (args.size() != policy.functions[f.function].size()) {
    throw std::logic_error("Internal invariant failed.");
  }

  return IRStatement([=](callback_t next) {
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
  return IRStatement([=](callback_t next) {
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
  return IRStatement([code](callback_t done) {
    if (!code.empty()) {
      size_t i = 0u;

      callback_t next = [&]() {
        if (i < code.size()) {
          size_t const save = i;
          ++i;
          code[save].dump(next);
        }
      };

      next();
    }
    done();
  });
}
