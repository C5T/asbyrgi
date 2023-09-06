// This "C" preprocessor file contains the definitions that convert the DSL of an OPA IR policy into valid Kotlin code.

// TODO(dkorolev): Complete the transformation; this only passes the `tests/smoke/sum/policy.rego` test.

#define BeginOPADSL() \
class __KOTLIN_CLASS_NAME__Statics { __INSERT_NEWLINE__ \
    data class FunctionSignature(val name: String, val argIndex: Array<Int>, val retvalIndex: Int) __INSERT_NEWLINE__ \
    companion object {
#define EndOPADSL() \
   __INSERT_NEWLINE__ \
    fun __KOTLIN_CLASS_NAME__(opaInput: AuthzValue, opaData: AuthzValue = AuthzValue.UNDEFINED): AuthzValue { __INSERT_NEWLINE__ \
    return __KOTLIN_CLASS_NAME__Plan0(opaInput, opaData) __INSERT_NEWLINE__ \
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
fun __KOTLIN_CLASS_NAME__Function##function_index(args: MutableMap<Int, AuthzValue>): AuthzValue { __INSERT_NEWLINE__ \
    val functionArgumentIndexes = __KOTLIN_CLASS_NAME__Statics.FUNCTION_SIGNATURES[function_index].argIndex __INSERT_NEWLINE__ \
    val functionReturnValueIndex = __KOTLIN_CLASS_NAME__Statics.FUNCTION_SIGNATURES[function_index].retvalIndex __INSERT_NEWLINE__ \
    val locals: MutableMap<Int, AuthzValue> = mutableMapOf() __INSERT_NEWLINE__ \
    for (i in functionArgumentIndexes.indices) { __INSERT_NEWLINE__ \
        locals[functionArgumentIndexes[i]] = localOrUndefined(args, i) __INSERT_NEWLINE__ \
    }   


#define EndFunction(function_index, function_name) return localOrUndefined(locals, functionReturnValueIndex) __INSERT_NEWLINE__ } __INSERT_NEWLINE__


#define BeginBlock() run {
#define EndBlock() RegoBlock.COMPLETED __INSERT_NEWLINE__ }

#define ArrayAppendStmt(array, value, rowcol) opaAppendToArray(localOrUndefined(locals, array), localOrUndefined(locals, value))

#define AssignIntStmt(value, target, rowcol) locals[target] = AuthzValue.INT(value)
#define AssignVarOnceStmt(source, target, rowcol) if (!(localOrUndefined(locals, target) is AuthzValue.UNDEFINED)) return@run RegoBlock.INTERRUPTED __INSERT_NEWLINE__ locals[target] = localOrUndefined(locals, source)
#define AssignVarStmt(source, target, rowcol) locals[target] = localOrUndefined(locals, source)

// TODO(dkorolev): `BreakStmt`.
// TODO(dkorolev): `CallDynamicStmt`.

#define CallStmtPassArg(arg_index, arg_value) callArgs[arg_index] = localOrUndefined(locals, arg_value)

#define CallBuiltinStmtBegin(func, target, rowcol) locals[target] = run { __INSERT_NEWLINE__ val callArgs: MutableMap<Int, AuthzValue> = mutableMapOf()
#define CallBuiltinStmtEnd(func, target) OpaBuiltins.func(callArgs) __INSERT_NEWLINE__ }

#define CallUserStmtBegin(func, target, rowcol) locals[target] = run { __INSERT_NEWLINE__ val callArgs: MutableMap<Int, AuthzValue> = mutableMapOf()
#define CallUserStmtEnd(func, target) __KOTLIN_CLASS_NAME__Function##func(callArgs) __INSERT_NEWLINE__ }

#define NotStmtBegin(rowcol) if (run {
#define NotStmtEnd() RegoBlock.COMPLETED } == RegoBlock.COMPLETED) return@run RegoBlock.INTERRUPTED

#define DotStmt(source, key, target, rowcol) locals[target] = irGetByKey(localOrUndefined(locals, source), irStringPossiblyFromLocal(locals, key))
#define EqualStmt(a, b, rowcol) if (localOrUndefined(locals, a) != localOrUndefined(locals, b)) return@run RegoBlock.INTERRUPTED
#define IsArrayStmt(array, rowcol) if (!(localOrUndefined(locals, array) is AuthzValue.ARRAY)) return@run RegoBlock.INTERRUPTED
#define IsDefinedStmt(source, rowcol) if (localOrUndefined(locals, source) is AuthzValue.UNDEFINED) return@run RegoBlock.INTERRUPTED
#define IsObjectStmt(object, rowcol) if (!(localOrUndefined(locals, object) is AuthzValue.OBJECT)) return@run RegoBlock.INTERRUPTED
#define IsUndefinedStmt(source, rowcol) if (!(localOrUndefined(locals, source) is AuthzValue.UNDEFINED)) return@run RegoBlock.INTERRUPTED

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
#define MakeNumberRefStmt(index, target, rowcol) locals[target] = AuthzValue.INT(__KOTLIN_CLASS_NAME__Statics.STATIC_STRINGS[index].toInt())
#define MakeObjectStmt(target, rowcol) locals[target] = AuthzValue.OBJECT(mutableMapOf())

#define MakeSetStmt(target, rowcol) locals[target] = AuthzValue.SET(mutableSetOf())

// NOTE(dkorolev): Skipping `NopStmt`.

#define NotEqualStmt(a, b, rowcol) if (localOrUndefined(locals, a) == localOrUndefined(locals, b)) return@run RegoBlock.INTERRUPTED

#if 0
#define ObjectInsertOnceStmt(key, value, object, rowcol) object.v[key] = wrap_for_assignment(value);  // TODO(dkorolev): Checks!
#endif
#define ObjectInsertStmt(key, value, object, rowcol) \
    run { __INSERT_NEWLINE__ \
        val o = locals[object] __INSERT_NEWLINE__ \
        if (o is AuthzValue.OBJECT) { __INSERT_NEWLINE__ \
            o.fields.put(key, localOrUndefined(locals, value)) __INSERT_NEWLINE__ \
        } __INSERT_NEWLINE__ \
    }
#if 0
#define ObjectMergeStmt(a, b, target, rowcol) target = { FIXME_MERGED: [a, b] };  // TODO(dkorolev): Implement this.
#endif

#define ResetLocalStmt(target, rowcol) locals[target] = AuthzValue.UNDEFINED

#define ResultSetAddStmt(value, rowcol) result.add(localOrUndefined(locals, value))
#define ReturnLocalStmt(source, rowcol) // NOTE(dkorolev) Unneeded, as we have the index to return.

#define ScanStmtBegin(source, key, value, rowcol) opaScan(locals, localOrUndefined(locals, source), key, value, inner = { __INSERT_NEWLINE__ run {
#define ScanStmtEnd() RegoBlock.COMPLETED __INSERT_NEWLINE__ } __INSERT_NEWLINE__ })

#define SetAddStmt(value, set, rowcol) opaAddToSet(localOrUndefined(locals, set), localOrUndefined(locals, value))

// TODO(dkorolev): `WithStmt`.

#define Local(a) a
#define OperandLocal(a) a
#define OperandBool(a) AuthzValue.BOOLEAN(a)
#define OperandStringIndex(a, string) string
#define BareNumber(a) a

#define StringConstantIndex(a) a

#define BeginPlan(plan_index, plan_name) \
fun __KOTLIN_CLASS_NAME__Plan##plan_index(opaInput: AuthzValue, opaData: AuthzValue): AuthzValue { __INSERT_NEWLINE__ \
    val locals: MutableMap<Int, AuthzValue> = mutableMapOf() __INSERT_NEWLINE__ \
    val result: ArrayList<AuthzValue> = arrayListOf() __INSERT_NEWLINE__ \
    locals[0] = opaInput __INSERT_NEWLINE__ \
    locals[1] = opaData


#define EndPlan(plan_index, plan_name)  \
    if (result.size == 0) { __INSERT_NEWLINE__ \
        return AuthzValue.UNDEFINED __INSERT_NEWLINE__ \
    } else if (result.size == 1) { __INSERT_NEWLINE__ \
        return result[0] __INSERT_NEWLINE__ \
    } else { __INSERT_NEWLINE__ \
        return AuthzValue.ARRAY(result) __INSERT_NEWLINE__ \
    } __INSERT_NEWLINE__ \
}

