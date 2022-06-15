#define BeginOPADSL() template <class CTX> void rego_policy(CTX& ctx) {
#define EndOPADSL() }

#define BeginStaticStrings() char const* inl_rego_strings[] = {
#define EndStaticStrings() \
  }; \
  for (size_t i = 0u; i < sizeof(inl_rego_strings) / sizeof(inl_rego_strings[0]); ++i) { \
    ctx.rego_strings.push_back(inl_rego_strings[i]); \
  }
#define DeclareStaticString(string_index, string)   string,

#define BeginDeclareFunctions() std::vector<size_t> const inl_opa_functions[] = {
#define EndDeclareFunctions(total_functions) \
  }; \
  for (size_t i = 0u; i < sizeof(inl_opa_functions) / sizeof(inl_opa_functions[0]); ++i) { \
    ctx.opa_functions.push_back(inl_opa_functions[i]); \
  }
#define BeginDeclareFunction(function_index, function_name) {
#define EndDeclareFunction(function_index, function_name) },
#define BeginFunctionArguments(function_index, args_count)
#define EndFunctionArguments(function_index)
#define FunctionArgument(arg_index, local_index) local_index,
#define FunctionReturnValue(function_index, local_return_index)  // Unnecessary, because `ReturnLocalStmt`.

#define BeginFunction(function_index, function_name) \
  if (!(function_index == ctx.opa_function_bodies.size())) { \
    throw std::logic_error("Internal invariant failed."); \
  } \
  ctx.opa_function_bodies.push_back( \
    [&inl_opa_functions](CTX const& outer_ctx) -> typename CTX::ir_t { \
    CTX ctx(typename CTX::ScopeDown(), outer_ctx); \
    return stmtBlock(ctx, {
#define EndFunction(function_index, function_name) }); });

#define BeginBlock() stmtBlock(ctx, {
#define EndBlock() }),

#define ArrayAppendStmt(array, value) stmtArrayAppend(ctx, array, value),
#define AssignIntStmt(value, target) stmtAssignInt(ctx, value, target),
#define AssignVarOnceStmt(source, target) stmtAssignVarOnce(ctx, source, target),
#define AssignVarStmt(source, target) stmtAssignVar(ctx, source, target),
#define CallStmtBegin(func, target) stmtCall(ctx, func, {
#define CallStmtPassArg(arg_index, arg_value) arg_value,
#define CallStmtEnd(func, target) }, target),
#define DotStmt(source, key, target) stmtDot(ctx, source, key, target),
#define EqualStmt(a, b) stmtEqual(ctx, a, b),
#define IsArrayStmt(array) stmtIsArray(ctx, array),
#define IsDefinedStmt(source) stmtIsDefined(ctx, source),
#define IsObjectStmt(source) stmtIsObject(ctx, source),
#define IsUndefinedStmt(source) stmtIsUndefined(ctx, source),
#define LenStmt(source, target) stmtLen(ctx, source, target),
#define MakeArrayStmt(capacity, target) stmtMakeArray(ctx, capacity, target),
#define MakeNullStmt(target) stmtMakeNull(ctx, target),
#define MakeNumberIntStmt(value, target) stmtMakeNumberInt(ctx, value, target),
#define MakeNumberRefStmt(index, target) stmtMakeNumberRef(ctx, index, target),
#define MakeObjectStmt(target) stmtMakeObject(ctx, target),
#define MakeSetStmt(target) stmtMakeSet(ctx),
#define NotEqualStmt(a, b) stmtNotEqual(ctx, a, b),
#define ObjectInsertOnceStmt(key, value, object) stmtObjectInsertOnce(ctx, key, value, object),
#define ObjectInsertStmt(key, value, object) stmtObjectInsert(ctx, key, value, object),
#define ObjectMergeStmt(a, b, target) stmtObjectMerge(ctx, a, b, target),
#define ResetLocalStmt(target) stmtResetLocal(ctx, target),
#define ResultSetAddStmt(value) stmtResultSetAdd(ctx, value),
#define ReturnLocalStmt(source) stmtReturnLocalStmt(ctx, source),
#define SetAddStmt(value, set) stmtSetAdd(ctx, value, set),

#define Local(a) OPALocalWrapper(a)

#define OperandLocal(a) OPALocalWrapper(a)
#define OperandBool(a) OPABoolean(a)
#define OperandStringIndex(a, string) string  // TODO: Compile-time strings for strong typing.

#define StringConstantIndex(a) StringConstant(a)

#define Func(x) OPAFunctionWrapper(x)
#define BuiltinFunc(x) opa_builtin.x

#define BeginPlan(plan_index, plan_name) \
  ctx.plans[plan_name] = [](CTX const& outer_ctx) -> typename CTX::ir_t { \
    CTX ctx(typename CTX::ScopeDown(), outer_ctx); \
    ctx.locals[0] = ctx.input; \
    ctx.locals[1] = ctx.data; \
    return stmtBlock(ctx, {
#define EndPlan(plan_index, plan_name) }); };
