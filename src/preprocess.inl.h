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

#define CallStmtBegin(func, target) stmtCall(func, {
#define CallStmtPassArg(arg_index, arg_value) arg_value,
#define CallStmtEnd(func, target) }, target),

#define ArrayAppendStmt(array, value) stmtArrayAppend(array, value),
#define AssignIntStmt(value, target) stmtAssignInt(value, target),
#define AssignVarOnceStmt(source, target) stmtAssignVarOnce(source, target),
#define AssignVarStmt(source, target) stmtAssignVar(source, target),
#define DotStmt(source, key, target) stmtDot(source, key, target),
#define EqualStmt(a, b) stmtEqual(a, b),
#define IsArrayStmt(array) stmtIsArray(array),
#define IsDefinedStmt(source) stmtIsDefined(source),
#define IsObjectStmt(source) stmtIsObject(source),
#define IsUndefinedStmt(source) stmtIsUndefined(source),
#define LenStmt(source, target) stmtLen(source, target),
#define MakeArrayStmt(capacity, target) stmtMakeArray(capacity, target),
#define MakeNullStmt(target) stmtMakeNull(target),
#define MakeNumberIntStmt(value, target) stmtMakeNumberInt(value, target),
#define MakeNumberRefStmt(index, target) stmtMakeNumberRef(index, target),
#define MakeObjectStmt(target) stmtMakeObject(target),
#define MakeSetStmt(target) stmtMakeSet(target),
#define NotEqualStmt(a, b) stmtNotEqual(a, b),
#define ObjectInsertOnceStmt(key, value, object) stmtObjectInsertOnce(key, value, object),
#define ObjectInsertStmt(key, value, object) stmtObjectInsert(key, value, object),
#define ObjectMergeStmt(a, b, target) stmtObjectMerge(a, b, target),
#define ResetLocalStmt(target) stmtResetLocal(target),
#define ResultSetAddStmt(value) stmtResultSetAdd(value),
#define ReturnLocalStmt(source) stmtReturnLocalStmt(source),
#define SetAddStmt(value, set) stmtSetAdd(value, set),

#define Local(a) OPALocalWrapper(a)

#define OperandLocal(a) OPALocalWrapper(a)
#define OperandBool(a) OPABoolean(a)
#define OperandStringIndex(a, string) string  // TODO: Compile-time strings for strong typing.

#define StringConstantIndex(a) OPAStringConstant(a)

#define Func(x) OPAFunctionWrapper(x)
#define BuiltinFunc(x) opa_builtin.x

#define BeginPlan(plan_index, plan_name) \
  policy.plans[plan_name] = [](POLICY const& policy) -> typename POLICY::code_t { \
    return stmtBlock({
#define EndPlan(plan_index, plan_name) }); };
