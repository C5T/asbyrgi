// This "C" preprocessor file contains the definitions that convert the DSL of an OPA IR policy into valid Kotlin code.

// TODO(dkorolev): Complete the transformation; this only passes the `tests/smoke/sum/policy.rego` test.

#define BeginOPADSL() \
class SmokeSumPolicyImplStatics { __INSERT_NEWLINE__ \
    data class FunctionSignature(val name: String, val argIndex: Array<Int>, val retvalIndex: Int) __INSERT_NEWLINE__ \
    companion object {
#define EndOPADSL() \
   __INSERT_NEWLINE__ \
    fun SmokeSumPolicyImpl(opaInput: OpaValue, opaData: OpaValue = OpaValue.ValueUndefined): OpaValue { __INSERT_NEWLINE__ \
    return SmokeSumPolicyImplPlan0(opaInput, opaData) __INSERT_NEWLINE__ \
  }

#define BeginStaticStrings() val STATIC_STRINGS: Array<String> = arrayOf(
#define EndStaticStrings() )
#define DeclareStaticString(string_index, string) string,

#define BeginDeclareFunctions() val FUNCTION_SIGNATURES: Array<FunctionSignature> = arrayOf(
#define EndDeclareFunctions(total_functions) ) __INSERT_NEWLINE__ } __INSERT_NEWLINE__ } __INSERT_NEWLINE__

#define BeginDeclareFunction(function_index, function_name) FunctionSignature(function_name
#define EndDeclareFunction(function_index, function_name) )
#define BeginFunctionArguments(function_index, args_count) ,arrayOf(
#define EndFunctionArguments(function_index) )
#define FunctionArgument(arg_index, local_index) local_index,
#define FunctionReturnValue(function_index, local_return_index) ,local_return_index


#define BeginFunction(function_index, function_name) \
fun SmokeSumPolicyImplFunction##function_index(args: MutableMap<Int, OpaValue>): OpaValue { __INSERT_NEWLINE__ \
    val functionArgumentIndexes = SmokeSumPolicyImplStatics.FUNCTION_SIGNATURES[function_index].argIndex __INSERT_NEWLINE__ \
    val functionReturnValueIndex = SmokeSumPolicyImplStatics.FUNCTION_SIGNATURES[function_index].retvalIndex __INSERT_NEWLINE__ \
    val locals: MutableMap<Int, OpaValue> = mutableMapOf() __INSERT_NEWLINE__ \
    for (i in functionArgumentIndexes.indices) { __INSERT_NEWLINE__ \
        locals[functionArgumentIndexes[i]] = localOrUndefined(args, i) __INSERT_NEWLINE__ \
    }   


#define EndFunction(function_index, function_name) return localOrUndefined(locals, functionReturnValueIndex) __INSERT_NEWLINE__ } __INSERT_NEWLINE__


#define BeginBlock() run {
#define EndBlock() }

#if 0

// TODO(dkorolev): Checks and early returns everywhere.

#define ArrayAppendStmt(array, value, rowcol) if (array === undefined || array.t !== 'array') return; array.v.push(wrap_for_assignment(value));
#define AssignIntStmt(value, target, rowcol) target = { t: 'number', v: value };  // TODO(dkorolev): Check the type, fail if wrong, I assume?
#endif
#define AssignVarOnceStmt(source, target, rowcol) if (!(localOrUndefined(locals, target) is OpaValue.ValueUndefined)) return@run __INSERT_NEWLINE__ locals[target] = localOrUndefined(locals, source)
#define AssignVarStmt(source, target, rowcol) locals[target] = localOrUndefined(locals, source)
// TODO(dkorolev): `BreakStmt`.
// TODO(dkorolev): `CallDynamicStmt`.

#define CallStmtPassArg(arg_index, arg_value) callArgs[arg_index] = localOrUndefined(locals, arg_value)

#define CallBuiltinStmtBegin(func, target, rowcol) locals[target] = run { __INSERT_NEWLINE__ val callArgs: MutableMap<Int, OpaValue> = mutableMapOf()
#define CallBuiltinStmtEnd(func, target) opaBuiltinFunction_##func(callArgs) __INSERT_NEWLINE__ }

#define CallUserStmtBegin(func, target, rowcol) locals[target] = run { __INSERT_NEWLINE__ val callArgs: MutableMap<Int, OpaValue> = mutableMapOf()
#define CallUserStmtEnd(func, target) SmokeSumPolicyImplFunction##func(callArgs) __INSERT_NEWLINE__ }

#if 0
#define NotStmtBegin(rowcol) if ((() => {
#define NotStmtEnd() ; return true; })() === true) return;
#endif
#define DotStmt(source, key, target, rowcol) locals[target] = irGetByKey(localOrUndefined(locals, source), key)
#if 0
#define EqualStmt(a, b, rowcol) if (JSON.stringify(a) !== JSON.stringify(wrap_for_assignment(b))) return;
#define IsArrayStmt(array, rowcol) if (array === undefined || array.t !== 'array') return;
#endif
#define IsDefinedStmt(source, rowcol) if (localOrUndefined(locals, source) is OpaValue.ValueUndefined) return@run
#if 0
#define IsObjectStmt(source, rowcol) if (source === undefined || source.t !== 'object') return;
#define IsUndefinedStmt(source, rowcol) if (source !== undefined) return;
#define LenStmt(source, target, rowcol) target = {t: 'number', v: Object.keys(source.v).length};  // TODO(dkorolev): Type checks!
#define MakeArrayStmt(capacity, target, rowcol) target = {t:'array', v: []};
#define MakeNullStmt(target, rowcol) target = { t: 'null', v: null };
#define MakeNumberIntStmt(number_value, target, rowcol) target = { t: 'number', v: number_value };
#define MakeNumberRefStmt(index, target, rowcol) target = { t: 'number', v: Number(static_strings[index]) };  // TODO(dkorolev): This is `Ref`!
#endif
#define MakeObjectStmt(target, rowcol) locals[target] = OpaValue.ValueObject(mutableMapOf())
#if 0
#define MakeSetStmt(target, rowcol) target = { t: 'set', v: {} };
// NOTE(dkorolev): Skipping `NopStmt`.
#endif
#define NotEqualStmt(a, b, rowcol) if (a == b) return@run
#if 0
#define ObjectInsertOnceStmt(key, value, object, rowcol) object.v[key] = wrap_for_assignment(value);  // TODO(dkorolev): Checks!
#endif
#define ObjectInsertStmt(key, value, object, rowcol) \
    run { __INSERT_NEWLINE__ \
        val o = locals[object] __INSERT_NEWLINE__ \
        if (o is OpaValue.ValueObject) { __INSERT_NEWLINE__ \
            o.fields.put(key, localOrUndefined(locals, value)) __INSERT_NEWLINE__ \
        } __INSERT_NEWLINE__ \
    }
#if 0
#define ObjectMergeStmt(a, b, target, rowcol) target = { FIXME_MERGED: [a, b] };  // TODO(dkorolev): Implement this.
#endif

#define ResetLocalStmt(target, rowcol) locals[target] = OpaValue.ValueUndefined

#define ResultSetAddStmt(value, rowcol) result.add(localOrUndefined(locals, value))
#define ReturnLocalStmt(source, rowcol) // retval = source <-- NOTE(dkorolev) Unneeded, as we have the index to return.
#if 0
#define ScanStmtBegin(source, key, value, rowcol) opa_scan(source, locals, key, value, () => {
#define ScanStmtEnd() });
#define SetAddStmt(value, set, rowcol) if (!(value.t in set.v)) { set.v[value.t] = new Set(); } set.v[value.t].add(value.v);
// TODO(dkorolev): `WithStmt`.

#endif
#define Local(a) a
#define OperandLocal(a) a
#define OperandBool(a) OpaValue.ValueBoolean(a)
#define OperandStringIndex(a, string) string
#if 0

#define BareNumber(a) a
#define StringConstantIndex(a) a

#endif
#define Func(x) OpaProvidedFunction(x)
#define BuiltinFunc(x) #x
#if 0

// TODO(dkorolev): The `result` should not be global.
#endif
#define BeginPlan(plan_index, plan_name) \
fun SmokeSumPolicyImplPlan##plan_index(opaInput: OpaValue, opaData: OpaValue): OpaValue { __INSERT_NEWLINE__ \
    val locals: MutableMap<Int, OpaValue> = mutableMapOf() __INSERT_NEWLINE__ \
    val result: ArrayList<OpaValue> = arrayListOf() __INSERT_NEWLINE__ \
    locals[0] = opaInput __INSERT_NEWLINE__ \
    locals[1] = opaData


#define EndPlan(plan_index, plan_name)  \
    if (result.size == 0) { __INSERT_NEWLINE__ \
        return OpaValue.ValueUndefined __INSERT_NEWLINE__ \
    } else if (result.size == 1) { __INSERT_NEWLINE__ \
        return result[0] __INSERT_NEWLINE__ \
    } else { __INSERT_NEWLINE__ \
        return OpaValue.ValueArray(result.toTypedArray()) __INSERT_NEWLINE__ \
    } __INSERT_NEWLINE__ \
}

