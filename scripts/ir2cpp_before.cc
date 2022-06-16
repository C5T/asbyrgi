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
// Dirty hack: This `code_t` just dumps the code to standard output for now.
using code_t = std::function<void(callback_t)>;

struct Policy final {
  using code_t = ::code_t;

  std::vector<std::string> strings;
  std::vector<std::vector<size_t>> functions;
  std::vector<std::function<code_t(Policy const& policy)>> function_bodies;
  std::map<std::string, std::function<code_t(Policy const& policy)>> plans;
};

inline static Policy* policy_singleton;

code_t stmtAssignVarOnce(OPALocalWrapper source, OPALocalWrapper target) {
  return [=](callback_t next) {
    std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "] = locals["
              << source.local << "];\n";
    next();
    std::cout << "}\n";
  };
}

code_t stmtAssignVarOnce(const char* source, OPALocalWrapper target) {
  return [=](callback_t next) {
    std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "].SetString(\""
              << source << "\");\n";
    next();
    std::cout << "}\n";
  };
}

code_t stmtAssignVarOnce(OPABoolean source, OPALocalWrapper target) {
  return [=](callback_t next) {
    std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "].SetBoolean("
              << std::boolalpha << source.boolean << ");\n";
    next();
    std::cout << "}\n";
  };
}

code_t stmtAssignVar(OPALocalWrapper source, OPALocalWrapper target) {
  return [=](callback_t next) {
    std::cout << "locals[" << target.local << "] = locals[" << source.local << "];\n";
    next();
  };
}

code_t stmtDot(OPALocalWrapper source, char const* key, OPALocalWrapper target) {
  return [=](callback_t next) {
    std::cout << "locals[" << target.local << "] = locals[" << source.local << "].GetByKey(\"" << key << "\");";
    next();
  };
}

code_t stmtEqual(OPALocalWrapper a, const char* b) {
  return [=](callback_t next) {
    std::cout << "if (locals[" << a.local << "].IsString(\"" << b << "\")) {\n";
    next();
    std::cout << "}\n";
  };
}

code_t stmtIsDefined(OPALocalWrapper source) {
  return [=](callback_t next) {
    std::cout << "if (!locals[" << source.local << "].IsUndefined()) {\n";
    next();
    std::cout << "}\n";
  };
}

code_t stmtIsUndefined(OPALocalWrapper source) {
  return [=](callback_t next) {
    std::cout << "if (locals[" << source.local << "].IsUndefined()) {\n";
    next();
    std::cout << "}\n";
  };
}

code_t stmtMakeNull(OPALocalWrapper target) {
  return [=](callback_t next) {
    std::cout << "locals[" << target.local << "].MakeNull();\n";
    next();
  };
}

code_t stmtMakeNumberRef(OPAStringConstant value, OPALocalWrapper target) {
  return [=](callback_t next) {
    Policy const& policy = *policy_singleton;
    // TODO: Not `FromString`, of course.
    std::cout << "locals[" << target.local << "].SetNumberFromString(\"" << policy.strings[value.string_index]
              << "\");\n";
    next();
  };
}

code_t stmtMakeObject(OPALocalWrapper target) {
  return [=](callback_t next) {
    std::cout << "locals[" << target.local << "].MakeObject();\n";
    next();
  };
}

code_t stmtNotEqual(OPABoolean a, OPABoolean b) {
  if (a.boolean != b.boolean) {
    return [=](callback_t next) { next(); };
  } else {
    return [](callback_t) {};
  }
}

code_t stmtObjectInsert(char const* key, OPALocalWrapper value, OPALocalWrapper object) {
  return [=](callback_t next) {
    std::cout << "locals[" << object.local << "].SetValueForKey(\"" << key << "\", locals[" << value.local << "]);\n";
    next();
  };
}

code_t stmtResetLocal(OPALocalWrapper target) {
  return [=](callback_t next) {
    std::cout << "locals[" << target.local << "].ResetToUndefined();\n";
    next();
  };
}

code_t stmtResultSetAdd(OPALocalWrapper value) {
  return [=](callback_t next) {
    std::cout << "result.AddToResultSet(locals[" << value.local << "]);\n";
    next();
  };
}

code_t stmtReturnLocalStmt(OPALocalWrapper source) {
  return [=](callback_t next) {
    std::cout << "retval = locals[" << source.local << "];\n";
    next();
  };
}

code_t stmtCall(OPAUserDefinedFunction f, std::vector<OPALocalWrapper> const& args, OPALocalWrapper target) {
  Policy const& policy = *policy_singleton;
  if (args.size() != policy.functions[f.function].size()) {
    throw std::logic_error("Internal invariant failed.");
  }

  return [=](callback_t next) {
    std::cout << "locals[" << target.local << "] = function_" << f.function << "(";
    for (size_t i = 0u; i < args.size(); ++i) {
      if (i) {
        std::cout << ", ";
      }
      std::cout << "locals[" << args[i].local << "]";
    }
    std::cout << ");\n";

    next();
  };
}

code_t stmtCall(OPABuiltin f, std::vector<OPALocalWrapper> const& args, OPALocalWrapper target) {
  if (args.size() != f.args_count) {
    throw std::logic_error("Internal invariant failed.");
  }

  // TODO: Fix this copy-paste in `stmtCall`.
  return [=](callback_t next) {
    std::cout << "locals[" << target.local << "] = " << f.builtin_name << "(";
    for (size_t i = 0u; i < f.args_count; ++i) {
      if (i) {
        std::cout << ", ";
      }
      std::cout << "locals[" << args[i].local << "]";
    }
    std::cout << ");\n";

    next();
  };
}

code_t stmtBlock(std::vector<code_t> const& code) {
  return [code](callback_t done) {
    if (!code.empty()) {
      size_t i = 0u;

      callback_t next = [&]() {
        if (i < code.size()) {
          size_t const save = i;
          ++i;
          code[save](next);
        }
      };

      next();
    }
    done();
  };
}
