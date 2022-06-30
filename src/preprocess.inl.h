#define BeginOPADSL() template <class POLICY> void PopulatePolicy(POLICY& policy) {
#define EndOPADSL() }

#define BeginStaticStrings() char const* inl_strings[] = {
#define EndStaticStrings() \
  }; \
  for (size_t i = 0u; i < sizeof(inl_strings) / sizeof(inl_strings[0]); ++i) { \
    policy.strings.push_back(inl_strings[i]); \
  }
#define DeclareStaticString(string_index, string)   string,

#define BeginDeclareFunctions() std::vector<size_t> const inl_functions[] = {
#define EndDeclareFunctions(total_functions) \
  }; \
  for (size_t i = 0u; i < sizeof(inl_functions) / sizeof(inl_functions[0]); ++i) { \
    policy.functions.push_back(inl_functions[i]); \
  }
#define BeginDeclareFunction(function_index, function_name) {
#define EndDeclareFunction(function_index, function_name) },
#define BeginFunctionArguments(function_index, args_count)
#define EndFunctionArguments(function_index)
#define FunctionArgument(arg_index, local_index) local_index,
#define FunctionReturnValue(function_index, local_return_index)  // Unnecessary, because `ReturnLocalStmt`.

#define BeginFunction(function_index, function_name) \
  if (!(function_index == policy.function_bodies.size())) { \
    throw std::logic_error("Internal invariant failed."); \
  } \
  policy.function_bodies.push_back( \
    [&inl_functions](POLICY const& policy) -> typename POLICY::code_t { \
    return stmtBlock({
#define EndFunction(function_index, function_name) }); });

#define BeginBlock() stmtBlock({
#define EndBlock() }),

#define CallStmtBegin(func, target, rowcol) stmtCall(func, rowcol, {
#define CallStmtPassArg(arg_index, arg_value) arg_value,
#define CallStmtEnd(func, target) }, target),

#define ArrayAppendStmt(...) stmtArrayAppend(__VA_ARGS__),
#define AssignIntStmt(...) stmtAssignInt(__VA_ARGS__),
#define AssignVarOnceStmt(...) stmtAssignVarOnce(__VA_ARGS__),
#define AssignVarStmt(...) stmtAssignVar(__VA_ARGS__),
#define DotStmt(...) stmtDot(__VA_ARGS__),
#define EqualStmt(...) stmtEqual(__VA_ARGS__),
#define IsArrayStmt(...) stmtIsArray(__VA_ARGS__),
#define IsDefinedStmt(...) stmtIsDefined(__VA_ARGS__),
#define IsObjectStmt(...) stmtIsObject(__VA_ARGS__),
#define IsUndefinedStmt(...) stmtIsUndefined(__VA_ARGS__),
#define LenStmt(...) stmtLen(__VA_ARGS__),
#define MakeArrayStmt(...) stmtMakeArray(__VA_ARGS__),
#define MakeNullStmt(...) stmtMakeNull(__VA_ARGS__),
#define MakeNumberIntStmt(...) stmtMakeNumberInt(__VA_ARGS__),
#define MakeNumberRefStmt(...) stmtMakeNumberRef(__VA_ARGS__),
#define MakeObjectStmt(...) stmtMakeObject(__VA_ARGS__),
#define MakeSetStmt(...) stmtMakeSet(__VA_ARGS__),
#define NotEqualStmt(...) stmtNotEqual(__VA_ARGS__),
#define ObjectInsertOnceStmt(...) stmtObjectInsertOnce(__VA_ARGS__),
#define ObjectInsertStmt(...) stmtObjectInsert(__VA_ARGS__),
#define ObjectMergeStmt(...) stmtObjectMerge(__VA_ARGS__),
#define ResetLocalStmt(...) stmtResetLocal(__VA_ARGS__),
#define ResultSetAddStmt(...) stmtResultSetAdd(__VA_ARGS__),
#define ReturnLocalStmt(...) stmtReturnLocalStmt(__VA_ARGS__),
#define SetAddStmt(...) stmtSetAdd(__VA_ARGS__),

#define Local(a) OPALocalWrapper(a)

#define OperandLocal(a) OPALocalWrapper(a)
#define OperandBool(a) OPABoolean(a)
#define OperandStringIndex(a, string) string  // TODO: Compile-time strings for strong typing.

#define StringConstantIndex(a) OPAStringConstant(a)

#define RowCol(row,col) OPARowCol(row, col)

#define Func(x) OPAFunctionWrapper(x)
#define BuiltinFunc(x) opa_builtin.x

#define BeginPlan(plan_index, plan_name) \
  policy.plans[plan_name] = [](POLICY const& policy) -> typename POLICY::code_t { \
    return stmtBlock({
#define EndPlan(plan_index, plan_name) }); };
