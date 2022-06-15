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

struct OPAFunctionWrapper final {
  size_t const function;
  explicit OPAFunctionWrapper(size_t function) : function(function) {}
};

struct OPAStringConstant final {
  size_t const string_index;
  explicit OPAStringConstant(size_t string_index) : string_index(string_index) {}
};

struct OPABoolean final {
  bool const boolean;
  OPABoolean() = delete;
  explicit OPABoolean(bool boolean) : boolean(boolean) {}
};

struct OPAValue final {
  OPAValue() {}
  OPAValue(OPAValue const&) {}
  // TODO: This will be needed soon. Or will it, hmm?
};

inline std::ostream& operator<<(std::ostream& os, OPAValue const& value) {
  os << "{{TODO:OPAValue}}";
  return os;
}

struct OPAResultSet final {};

using f_t = std::function<void()>;
using OPAStmt = std::function<void(f_t)>;

struct TranspilationContext final {
  using input_t = OPAValue;
  using data_t = OPAValue;
  using value_t = OPAValue;
  using result_set_t = OPAResultSet;

  using ir_t = OPAStmt;

  input_t const& input;
  data_t const& data;
  result_set_t& result;
  std::map<size_t, value_t> locals;

  mutable std::vector<std::string> rego_strings;
  mutable std::vector<std::vector<size_t>> opa_functions;
  mutable std::vector<std::function<ir_t(TranspilationContext const& ctx)>> opa_function_bodies;
  mutable std::map<std::string, std::function<ir_t(TranspilationContext const& ctx)>> plans;

  TranspilationContext() = delete;

  struct CreateNew final {};
  TranspilationContext(CreateNew, input_t const& input, data_t const& data, result_set_t& result)
      : input(input), data(data), result(result) {}

  // TODO: This extra copy is a dirty hack. Need to redesign `ctx` vs. `vm` or `runtime`.
  struct ScopeDown final {};
  TranspilationContext(ScopeDown, TranspilationContext const& outer)
      : input(outer.input), data(outer.data), result(outer.result) {
    rego_strings = outer.rego_strings;
    opa_functions = outer.opa_functions;
  }
};

OPAStmt stmtArrayAppend(TranspilationContext const& ctx, OPAValue array, OPAValue value) {
  return [=](f_t next) {
    std::cout << array << ".push(" << value << ");\n";
    next();
  };
}
OPAStmt stmtAssignInt(TranspilationContext const& ctx, OPAValue value, OPALocalWrapper target) {
  return [=](f_t next) {
    std::cout << "locals[" << target.local << " = {{TODO:INT}};";
    next();
  };
}

OPAStmt stmtAssignVarOnce(TranspilationContext const& ctx, OPALocalWrapper source, OPALocalWrapper target) {
  return [=](f_t next) {
    std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "] = locals["
              << source.local << "];\n";
    next();
    std::cout << "}\n";
  };
}

OPAStmt stmtAssignVarOnce(TranspilationContext const& ctx, const char* source, OPALocalWrapper target) {
  return [=](f_t next) {
    std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "].SetString(\"" << source << "\");\n";
    next();
    std::cout << "}\n";
  };
}

OPAStmt stmtAssignVarOnce(TranspilationContext const& ctx, OPABoolean source, OPALocalWrapper target) {
  return [=](f_t next) {
    std::cout << "if (locals[" << target.local << "].IsUndefined()) { locals[" << target.local << "].SetBoolean(" << std::boolalpha << source.boolean << ");\n";
    next();
    std::cout << "}\n";
  };
}

OPAStmt stmtAssignVar(TranspilationContext const& ctx, OPALocalWrapper source, OPALocalWrapper target) {
  return [=](f_t next) {
    std::cout << "locals[" << target.local << "] = locals[" << source.local << "];\n";
    next();
  };
}

OPAStmt stmtDot(TranspilationContext const& ctx, OPALocalWrapper source, char const* key, OPALocalWrapper target) {
  return [=](f_t next) {
    std::cout << "locals[" << target.local << "] = locals[" << source.local << "].GetByKey(\""
              << key
              << "\");";
    next();
  };
}

OPAStmt stmtEqual(TranspilationContext const& ctx, OPALocalWrapper a, const char* b) {
  return [=](f_t next) {
    std::cout << "if (locals[" << a.local << "].IsString(\"" << b << "\")) {\n";
    next();
    std::cout << "}\n";
  };
}

OPAStmt stmtIsDefined(TranspilationContext const& ctx, OPALocalWrapper source) {
  return [=](f_t next) {
    std::cout << "if (!locals[" << source.local << "].IsUndefined()) {\n";
    next();
    std::cout << "}\n";
  };
}

OPAStmt stmtIsUndefined(TranspilationContext const& ctx, OPALocalWrapper source) {
  return [=](f_t next) {
    std::cout << "if (locals[" << source.local << "].IsUndefined()) {\n";
    next();
    std::cout << "}\n";
  };
}

OPAStmt stmtMakeNull(TranspilationContext const& ctx, OPALocalWrapper target) {
  return [=](f_t next) {
    std::cout << "locals[" << target.local << "].MakeNull();\n";
    next();
  };
}

/*
OPAStmt stmtMakeNumberInt(TranspilationContext const& ctx, OPAStringConstant value, OPALocalWrapper target) {
  return [=](f_t next) {
    // TODO: Not `FromString`, of course.
    std::cout << "locals[" << target.local << "].SetNumberFromString(\"" << ctx.rego_strings[value.string_index] << "\");\n";
    next();
  };
}
*/

OPAStmt stmtMakeNumberRef(TranspilationContext const& ctx, OPAStringConstant value, OPALocalWrapper target) {
  return [=](f_t next) {
    // TODO: Not `FromString`, of course.
    std::cout << "locals[" << target.local << "].SetNumberFromString(\"" << ctx.rego_strings[value.string_index] << "\");\n";
    next();
  };
}

OPAStmt stmtMakeObject(TranspilationContext const& ctx, OPALocalWrapper target) {
  return [=](f_t next) {
    std::cout << "locals[" << target.local << "].MakeObject();\n";
    next();
  };
}

OPAStmt stmtNotEqual(TranspilationContext const& ctx, OPABoolean a, OPABoolean b) {
  if (a.boolean != b.boolean) {
    return [=](f_t next) { next(); };
  } else {
    return [](f_t) {};
  }
}

OPAStmt stmtObjectInsert(TranspilationContext const& ctx,
                         char const* key,
                         OPALocalWrapper value,
                         OPALocalWrapper object) {
  return [=](f_t next) {
    std::cout << "locals[" << object.local << "].SetValueForKey(\"" << key << "\", locals[" << value.local << "]);\n";
    next();
  };
}

OPAStmt stmtObjectMerge(TranspilationContext const& ctx, OPAValue a, OPAValue b, OPAValue taret) {
  return [=](f_t next) {
    std::cout << "stmtObjectMerge();\n";
    next();
  };
}

OPAStmt stmtResetLocal(TranspilationContext const& ctx, OPALocalWrapper target) {
  return [=](f_t next) {
    std::cout << "locals[" << target.local << "].ResetToUndefined();\n";
    next();
  };
}

OPAStmt stmtResultSetAdd(TranspilationContext const& ctx, OPALocalWrapper value) {
  return [=](f_t next) {
    std::cout << "result.AddToResultSet(locals[" << value.local << "]);\n";
    next();
  };
}

OPAStmt stmtReturnLocalStmt(TranspilationContext const& ctx, OPALocalWrapper source) {
  return [=](f_t next) {
    std::cout << "retval = locals[" << source.local << "];\n";
    next();
  };
}

OPAStmt stmtSetAdd(TranspilationContext const& ctx, OPAValue value, OPAValue set) {
  return [=](f_t next) {
    std::cout << "stmtSetAdd();\n";
    next();
  };
}

OPAStmt stmtCall(TranspilationContext const& ctx, OPAFunctionWrapper f, std::vector<OPALocalWrapper> const&, OPALocalWrapper target) {
  return [=](f_t next) {
    std::cout << "locals[" << target.local << "] = function_" << f.function << "(";
    for (size_t i = 0u; i < ctx.opa_functions[f.function].size(); ++i) {
      if (i) {
        std::cout << ", ";
      }
      std::cout << "locals[" << ctx.opa_functions[f.function][i] << "]";
    }
    std::cout << ");\n";

    next();
  };
}

OPAStmt stmtBlock(TranspilationContext const&, std::vector<OPAStmt> const& stmts) {
  return [stmts](f_t done) {
    if (!stmts.empty()) {
      size_t i = 0u;

      f_t next = [&]() {
        if (i < stmts.size()) {
          size_t const save = i;
          ++i;
          stmts[save](next);
        }
      };

      next();
    }
    done();
  };
}

struct opa_builtin_t final {
  bool plus;
  bool minus;
  bool mul;
  struct numbers_t final {
    bool range;
    numbers_t() {}
  };
  opa_builtin_t() {}
  numbers_t const numbers;
};

opa_builtin_t opa_builtin;
