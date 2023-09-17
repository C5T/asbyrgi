// This "C" preprocessor file contains the definitions that convert the DSL of an OPA IR policy into valid Kotlin code.

#define BeginOPADSL() \
class __KOTLIN_EXPORT_NAME__Statics { __INSERT_NEWLINE__ \
    data class FunctionSignature(val name: String, val argIndex: Array<Int>, val retvalIndex: Int) __INSERT_NEWLINE__ \
    companion object {

#define EndOPADSL() \
   __INSERT_NEWLINE__ \
    fun __KOTLIN_EXPORT_NAME__(authzInput: AuthzValue, authzData: AuthzValue = AuthzValue.UNDEFINED): AuthzResult { __INSERT_NEWLINE__ \
    return __KOTLIN_EXPORT_NAME__Plan0(authzInput, authzData) __INSERT_NEWLINE__ \
  }

#define BeginStaticStrings() val STATIC_STRINGS: Array<String> = arrayOf(
#define EndStaticStrings() )
#define DeclareStaticString(string_index, string) string,

#define BeginDeclareFunctions() val FUNCTION_SIGNATURES: Array<FunctionSignature> = arrayOf(
#define EndDeclareFunctions(total_functions) ) __INSERT_NEWLINE__ } __INSERT_NEWLINE__ } __INSERT_NEWLINE__

#define BeginDeclareFunction(function_index, function_name) FunctionSignature(function_name
#define EndDeclareFunction(function_index, function_name) ),
#define BeginFunctionArguments(function_index, args_count) ,arrayOf(
#define EndFunctionArguments(function_index) )
#define FunctionArgument(arg_index, local_index) local_index,
#define FunctionReturnValue(function_index, local_return_index) ,local_return_index

#define BeginFunction(function_index, function_name) \
fun __KOTLIN_EXPORT_NAME__Function##function_index(args: MutableMap<Int, AuthzValue>): AuthzValue { __INSERT_NEWLINE__ \
    val functionArgumentIndexes = __KOTLIN_EXPORT_NAME__Statics.FUNCTION_SIGNATURES[function_index].argIndex __INSERT_NEWLINE__ \
    val functionReturnValueIndex = __KOTLIN_EXPORT_NAME__Statics.FUNCTION_SIGNATURES[function_index].retvalIndex __INSERT_NEWLINE__ \
    val locals: MutableMap<Int, AuthzValue> = mutableMapOf() __INSERT_NEWLINE__ \
    for (i in functionArgumentIndexes.indices) { __INSERT_NEWLINE__ \
        locals[functionArgumentIndexes[i]] = regoVal(args, i) __INSERT_NEWLINE__ \
    }   


#define EndFunction(function_index, function_name) return regoVal(locals, functionReturnValueIndex) __INSERT_NEWLINE__ } __INSERT_NEWLINE__


#define BeginBlock() run {
#define EndBlock() RegoBlockResult.COMPLETED __INSERT_NEWLINE__ }

#define ArrayAppendStmt(array, value, rowcol) regoArrayAppendStmt(regoVal(locals, array), regoVal(locals, value))

#define AssignIntStmt(value, target, rowcol) locals[target] = AuthzValue.INT(value)
#define AssignVarOnceStmt(source, target, rowcol) if (!(regoVal(locals, target) is AuthzValue.UNDEFINED)) return@run RegoBlockResult.INTERRUPTED __INSERT_NEWLINE__ locals[target] = regoVal(locals, source)
#define AssignVarStmt(source, target, rowcol) locals[target] = regoVal(locals, source)

// TODO(dkorolev): `BreakStmt`.
// TODO(dkorolev): `CallDynamicStmt`.

#define CallStmtPassArg(arg_index, arg_value) callArgs[arg_index] = regoVal(locals, arg_value)

#define CallBuiltinStmtBegin(func, target, rowcol) locals[target] = run { __INSERT_NEWLINE__ val callArgs: MutableMap<Int, AuthzValue> = mutableMapOf()
#define CallBuiltinStmtEnd(func, target) RegoBuiltins.func(callArgs) __INSERT_NEWLINE__ }

#define CallUserStmtBegin(func, target, rowcol) locals[target] = run { __INSERT_NEWLINE__ val callArgs: MutableMap<Int, AuthzValue> = mutableMapOf()
#define CallUserStmtEnd(func, target) __KOTLIN_EXPORT_NAME__Function##func(callArgs) __INSERT_NEWLINE__ }

#define NotStmtBegin(rowcol) if (run {
#define NotStmtEnd() RegoBlockResult.COMPLETED } == RegoBlockResult.COMPLETED) return@run RegoBlockResult.INTERRUPTED

#define DotStmt(source, key, target, rowcol) locals[target] = regoDotStmt(regoVal(locals, source), regoStringWrapper(locals, key))
#define EqualStmt(a, b, rowcol) if (regoVal(locals, a) != regoVal(locals, b)) return@run RegoBlockResult.INTERRUPTED
#define IsArrayStmt(array, rowcol) if (!(regoVal(locals, array) is AuthzValue.ARRAY)) return@run RegoBlockResult.INTERRUPTED
#define IsDefinedStmt(source, rowcol) if (regoVal(locals, source) is AuthzValue.UNDEFINED) return@run RegoBlockResult.INTERRUPTED
#define IsObjectStmt(object, rowcol) if (!(regoVal(locals, object) is AuthzValue.OBJECT)) return@run RegoBlockResult.INTERRUPTED
#define IsUndefinedStmt(source, rowcol) if (!(regoVal(locals, source) is AuthzValue.UNDEFINED)) return@run RegoBlockResult.INTERRUPTED

// TODO(dkorolev): Double-check that `LenStmt` is for both arrays and object. Also sets, right?
#define LenStmt(source, target, rowcol) \
  locals[target] = run { __INSERT_NEWLINE__ \
    val o = locals[source] __INSERT_NEWLINE__\
    if (o is AuthzValue.ARRAY) { __INSERT_NEWLINE__ \
      return@run AuthzValue.INT(o.elements.size) __INSERT_NEWLINE__ \
    } else if (o is AuthzValue.OBJECT) { __INSERT_NEWLINE__ \
      return@run AuthzValue.INT(o.fields.size) __INSERT_NEWLINE__ \
    } else { __INSERT_NEWLINE__ \
      return@run AuthzValue.UNDEFINED __INSERT_NEWLINE__ \
    } __INSERT_NEWLINE__ \
  }

// TODO(dkorolev): Initialize with capacity?
#define MakeArrayStmt(capacity, target, rowcol) locals[target] = AuthzValue.ARRAY(arrayListOf())
#define MakeNullStmt(target, rowcol) locals[target] = AuthzValue.NULL

#define MakeNumberIntStmt(number_value, target, rowcol) locals[target] = AuthzValue.INT(number_value)

// TODO(dkorolev): Int? Maybe `Double`?
#define MakeNumberRefStmt(index, target, rowcol) locals[target] = AuthzValue.INT(__KOTLIN_EXPORT_NAME__Statics.STATIC_STRINGS[index].toInt())
#define MakeObjectStmt(target, rowcol) locals[target] = AuthzValue.OBJECT(mutableMapOf())

#define MakeSetStmt(target, rowcol) locals[target] = AuthzValue.SET(mutableSetOf())

// NOTE(dkorolev): Skipping `NopStmt`.

#define NotEqualStmt(a, b, rowcol) if (regoVal(locals, a) == regoVal(locals, b)) return@run RegoBlockResult.INTERRUPTED

// TODO(dkorolev): Implement & test this for Kotlin.
// #define ObjectInsertOnceStmt(key, value, object, rowcol) THE_OLD_JS_CODE_IS object.v[key] = wrap_for_assignment(value);

#define ObjectInsertStmt(key, value, object, rowcol) \
    run { __INSERT_NEWLINE__ \
        val o = locals[object] __INSERT_NEWLINE__ \
        if (o is AuthzValue.OBJECT) { __INSERT_NEWLINE__ \
            o.fields.put(key, regoVal(locals, value)) __INSERT_NEWLINE__ \
        } __INSERT_NEWLINE__ \
    }

// TODO(dkorolev): Implement & test this for Kotlin.
// #define ObjectMergeStmt(a, b, target, rowcol) THE_OLD_JS_CODE_IS target = { FIXME_MERGED: [a, b] };

#define ResetLocalStmt(target, rowcol) locals[target] = AuthzValue.UNDEFINED

#define ResultSetAddStmt(value, rowcol) result.addToResultSet(regoVal(locals, value))
#define ReturnLocalStmt(source, rowcol) // NOTE(dkorolev) Unneeded, as we have the index to return.

#define ScanStmtBegin(source, key, value, rowcol) regoScanStmt(locals, regoVal(locals, source), key, value, inner = { __INSERT_NEWLINE__ run {
#define ScanStmtEnd() RegoBlockResult.COMPLETED __INSERT_NEWLINE__ } __INSERT_NEWLINE__ })

#define SetAddStmt(value, set, rowcol) regoSetAddStmt(regoVal(locals, set), regoVal(locals, value))

// TODO(dkorolev): `WithStmt`.

#define Local(a) a
#define OperandLocal(a) a
#define OperandBool(a) AuthzValue.BOOLEAN(a)
#define OperandStringIndex(a, string) string
#define BareNumber(a) a

#define StringConstantIndex(a) a

#define BeginPlan(plan_index, plan_name) \
fun __KOTLIN_EXPORT_NAME__Plan##plan_index(authzInput: AuthzValue, authzData: AuthzValue): AuthzResult { __INSERT_NEWLINE__ \
    val locals: MutableMap<Int, AuthzValue> = mutableMapOf() __INSERT_NEWLINE__ \
    val result = AuthzResult() __INSERT_NEWLINE__ \
    locals[0] = authzInput __INSERT_NEWLINE__ \
    locals[1] = authzData

#define EndPlan(plan_index, plan_name)  \
    return result __INSERT_NEWLINE__ \
}
