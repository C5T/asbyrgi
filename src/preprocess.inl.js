// This "C" preprocessor file contains the definitions that convert the DSL of an OPA IR policy into valid JavaScript code.

(() => {

let function_bodies = {};
let plans = {};

const opa_builtins = {
  plus: (args) => { return { t: 'number', v: args[0].v + args[1].v }; },
  minus: (args) => { return { t: 'number', v: args[0].v - args[1].v }; },
  mul: (args) => { return { t: 'number', v: args[0].v * args[1].v }; },
  numbers: {
    range: (args) => {
      let v = [];
      // TODO(dkorolev): This is a dirty hack, of course, fix it.
      for (let i = args[0].v; i <= args[1].v; ++i) {
        v.push({ t: 'number', v: i });
      }
      return { t: 'array', v };
    }
  }
};

const opa_object_get_by_key = (x, key) => {
  if (x === undefined || !('t' in x) || x.t === undefined) {
    return undefined
  }
  const k = (() => {
    if (typeof key === 'object') {
      if ('t' in key) {
        return key.v;
      } else {
        return undefined;
      }
    } else {
      return key;
    }
  })();
  if (k === undefined) {
    return undefined;
  } else {
    // NOTE(dkorolev): Access string characters by index?
    return x.v[k];
  }
};

const opa_scan = (source, locals, key_index, value_index, block) => {
  if (source.t === 'array') {
    source.v.forEach((e, i) => {
      locals[key_index] = {t:'number', v:i};
      locals[value_index] = JSON.parse(JSON.stringify(e));
      block();
    });
  } else if (source.t === 'object') {
    for (let k in source.v) {
      locals[key_index] = {t:'string', v:k};
      locals[value_index] = JSON.parse(JSON.stringify(source.v[k]));
      block();
    }
  }
};

const internal_to_external_impl = {
  number: (x) => { return x; },
  string: (x) => { return x; },
  boolean: (x) => { return x; },
  array: (x) => {
    let result = [];
    x.forEach(e => result.push(internal_to_external(e)));
    return result;
  },
  object: (x) => {
    let result = {};
    for (let k in x) {
      result[k] = internal_to_external(x[k]);
    } 
    return result;
  },
};

const internal_to_external = (x) => {
  if (Array.isArray(x)) {
    // NOTE(dkorolev): Special case for the `result`, which is of "type" "results set", so, an array.
    if (x.length === 1) {
      return internal_to_external(x[0]);
    } else {
      let result = [];
      x.forEach(e => result.push(internal_to_external(e)));
      return result;
    }
  } else if (x === undefined || x.t === undefined) {
    return undefined;
  } else {
    return internal_to_external_impl[x.t](x.v);
  }
};

// TODO(dkorolev): Write this.
const external_to_internal = (v) => {
  if (Array.isArray(v)) {
    let result = [];
    v.forEach(e => result.push(external_to_internal(e)));
    return { t: 'array', v: result };
  }
  const t = typeof v;
  if (t === 'string' || t === 'number' || t === 'boolean') {
    return { t, v };
  } else if (t === 'object') {
    let result = { t: 'object', v: {}};
    for (let k in v) {
      result.v[k] = external_to_internal(v[k]);
    }
    return result;
  } else {
    // TODO(dkorolev): This function is incomplete. Accept arrays, at least, to begin with.
    return undefined;
  }
};

const main = (input, data) => {
  return internal_to_external(plans.main(external_to_internal(input), external_to_internal(data)));
};

if (typeof window === 'undefined') {
  module.exports.main = main;
} else {
  export_main(main);
}

const opa_get_function_impl = (f) => {
  if (typeof f === 'number') {
    return function_bodies[f];
  } else {
    return f.builtin_func;
  }
};

// NOTE(dkorolev): This is a hack for now, to make the JS tests pass.
const wrap_for_assignment = (x) => {
  if (typeof x === 'string') {
    return {t: 'string', v: x};
  } else if (typeof x === 'boolean') {
    return {t: 'boolean', v: x};
  } else {
    return x;
  }
};

// TODO(dkorolev): Expose the number of functions to this macro?
#define BeginOPADSL()
#define EndOPADSL() if (typeof window === 'undefined') { module.exports.plans = plans; } })();

#define BeginStaticStrings() const static_strings = [
#define EndStaticStrings() ];
#define DeclareStaticString(string_index, string)   string,

#define BeginDeclareFunctions() const function_signatures = [
#define EndDeclareFunctions(total_functions) ];
#define BeginDeclareFunction(function_index, function_name) { name: function_name,
#define EndDeclareFunction(function_index, function_name) },
#define BeginFunctionArguments(function_index, args_count) arg_index: [
#define EndFunctionArguments(function_index) ],
#define FunctionArgument(arg_index, local_index) local_index,
#define FunctionReturnValue(function_index, local_return_index) retval_index: local_return_index

// TODO(dkorolev): Expose the number of elements in "locals" to `BeginFunction`?
#define BeginFunction(function_index, function_name) function_bodies[function_index] = (args) => { let retval = null; let locals = []; function_signatures[function_index].arg_index.forEach((a, i) => { locals[a] = args[i]; });
#define EndFunction(function_index, function_name) return retval; };

#define BeginBlock() (() => {
#define EndBlock() })();

// TODO(dkorolev): Checks and early returns everywhere.

#define ArrayAppendStmt(array, value, rowcol) if (array === undefined || array.t !== 'array') return; array.v.push(wrap_for_assignment(value));
#define AssignIntStmt(value, target, rowcol) target = { t: 'number', v: value };  // TODO(dkorolev): Check the type, fail if wrong, I assume?
#define AssignVarOnceStmt(source, target, rowcol) if (target !== undefined) return; target = wrap_for_assignment(source);
#define AssignVarStmt(source, target, rowcol) target = wrap_for_assignment(source);
// TODO(dkorolev): `BreakStmt`.
// TODO(dkorolev): `CallDynamicStmt`.
#define CallStmtBegin(func, target, rowcol) target = (() => { let args = [];
#define CallStmtPassArg(arg_index, arg_value) args[arg_index] = arg_value;
#define CallStmtEnd(func, target) return opa_get_function_impl(func)(args)})();
#define DotStmt(source, key, target, rowcol) target = opa_object_get_by_key(source, key);
#define EqualStmt(a, b, rowcol) if (JSON.stringify(a) !== JSON.stringify(wrap_for_assignment(b))) return;
#define IsArrayStmt(array, rowcol) if (array === undefined || array.t !== 'array') return;
#define IsDefinedStmt(source, rowcol) if (source === undefined) return;
#define IsObjectStmt(source, rowcol) if (source === undefined || source.t !== 'object') return;
#define IsUndefinedStmt(source, rowcol) if (source !== undefined) return;
#define LenStmt(source, target, rowcol) target = {t: 'number', v: Object.keys(source.v).length};  // TODO(dkorolev): Type checks!
#define MakeArrayStmt(capacity, target, rowcol) target = {t:'array', v: []};
#define MakeNullStmt(target, rowcol) target = { t: 'null', v: null };
#define MakeNumberIntStmt(number_value, target, rowcol) target = { t: 'number', v: number_value };
#define MakeNumberRefStmt(index, target, rowcol) target = { t: 'number', v: Number(static_strings[index]) };  // TODO(dkorolev): This is `Ref`!
#define MakeObjectStmt(target, rowcol) target = { t: 'object', v: {} };
#define MakeSetStmt(target, rowcol) target = { t: 'set', v: {} };
// NOTE(dkorolev): Skipping `NopStmt`.
#define NotEqualStmt(a, b, rowcol) if (JSON.stringify(a) === JSON.stringify(b)) return;
// TODO(dkorolev): `NotStmt`.
#define ObjectInsertOnceStmt(key, value, object, rowcol) object.v[key] = wrap_for_assignment(value);  // TODO(dkorolev): Checks!
#define ObjectInsertStmt(key, value, object, rowcol) object.v[key] = wrap_for_assignment(value);  // TODO(dkorolev): Checks!
#define ObjectMergeStmt(a, b, target, rowcol) target = { FIXME_MERGED: [a, b] };  // TODO(dkorolev): Implement this.
#define ResetLocalStmt(target, rowcol) target = undefined;
#define ResultSetAddStmt(value, rowcol) result.push(value);  // TODO(dkorolev): Checks?
#define ReturnLocalStmt(source, rowcol) retval = source; // TODO(dkorolev): Is this even important given we know the return "local" index?
#define ScanStmtBegin(source, key, value, rowcol) opa_scan(source, locals, key, value, () => {
#define ScanStmtEnd() });
#define SetAddStmt(value, set, rowcol) set.v[value] = true;
// TODO(dkorolev): `WithStmt`.

#define Local(a) locals[a]

#define OperandLocal(a) locals[a]
#define OperandBool(a) Boolean(a)
#define OperandStringIndex(a, string) string

#define BareNumber(a) a
#define StringConstantIndex(a) a

#define Func(x) x
#define BuiltinFunc(x) {builtin_func: opa_builtins.x}

// TODO(dkorolev): The `result` should not be global.
#define BeginPlan(plan_index, plan_name) plans[plan_name] = (input, data) => { let locals = [input, data]; let result = [];
#define EndPlan(plan_index, plan_name) return result; };
