#!/usr/bin/env node

// TODO(dkorolev): Accept package name and default policy name as inputs.
// TODO(dkorolev): Expose the number of arguments per function.
// TODO(dkorolev): Expose the number of blocks per "block group".
// TODO(dkorolev): Expose the number of functions.
// TODO(dkorolev): Save the maximum `Local` index per block.
// TODO(dkorolev): List function argument indexes natively.

const fs = require('fs');

const ir = JSON.parse(fs.readFileSync(process.argv[2] || 'example_policy.ir.json'));

const fileName = (i) => { return JSON.stringify(ir.static.files[i].value); };

console.log('BeginOPADSL()');
console.log('');

let static_strings = {};
console.log('BeginStaticStrings()');
ir.static.strings.forEach((e, i) => {
  console.log(`  DeclareStaticString(${i}, ${JSON.stringify(e.value)})`);
  static_strings[i] = e.value;
});
console.log('EndStaticStrings()');
console.log('');

let funcs = {};

if ('builtin_funcs' in ir.static && ir.static.builtin_funcs !== null) {
  ir.static.builtin_funcs.forEach(e => { funcs[e.name] = {builtin: true, name: e.name} });
}

let has_funcs = false;
let num_funcs = 0;
console.log(`BeginDeclareFunctions()`);
if ('funcs' in ir && 'funcs' in ir.funcs && ir.funcs.funcs !== null) {
  has_funcs = true;
  ir.funcs.funcs.forEach((e, i) => {
    ++num_funcs;
    let args = [];
    if ('params' in e) {
      e.params.forEach(p => args.push(p));
    }
    console.log(`  BeginDeclareFunction(${i}, ${JSON.stringify(e.name)})`);
    console.log(`    BeginFunctionArguments(${i}, ${args.length})`);
    args.forEach((e, i) => console.log(`      FunctionArgument(${i}, ${e})`));
    console.log(`    EndFunctionArguments(${i})`);
    console.log(`    FunctionReturnValue(${i}, ${e.return})`);
    console.log(`  EndDeclareFunction(${i}, ${JSON.stringify(e.name)})`);
    funcs[e.name] = {builtin: false, index: i};
  });
}
console.log(`EndDeclareFunctions(${num_funcs})`);

const wrappers = {
  local: e => `OperandLocal(${e.value})`,
  bool: e => `OperandBool(${e.value})`,
  // NOTE(dkorolev): This `static_strings[e.value]` is unnecessary, it's for my eyes in manual deep dives.
  string_index: e => `OperandStringIndex(${e.value}, ${JSON.stringify(static_strings[e.value])})`
};
const wrap = (a) => {
  const t = typeof a;
  if (t === 'number') {
    return `Local(${a})`;
  } else if (t === 'object') {
    if (a.type in wrappers) {
      return wrappers[a.type](a);
    } else {
      return `UnknownValue(${JSON.stringify(a.type)})`;
    }
  } else {
    return `UnknownValue(UnknownValueType(${t}))`;
  }
};

let global_indent = '    ';
const emit = (s) => console.log(global_indent + s);

const statement_processors = {
  ArrayAppendStmt: ['array', 'value'],
  AssignVarOnceStmt: ['source', 'target'],
  AssignVarStmt: ['source', 'target'],
  BreakStmt: [],
  // TODO(dkorolev): `CallDynamicStmt`.
  DotStmt: ['source', 'key', 'target'],
  EqualStmt: ['a', 'b'],
  IsArrayStmt: ['source'],
  IsDefinedStmt: ['source'],
  IsObjectStmt: ['source'],
  IsUndefinedStmt: ['source'],
  LenStmt: ['source', 'target'],
  MakeNullStmt: ['target'],
  MakeObjectStmt: ['target'],
  MakeSetStmt: ['target'],
  // NOTE(dkorolev): Skipping `NopStmt`.
  NotEqualStmt: ['a', 'b'],
  ObjectInsertOnceStmt: ['key', 'value', 'object'],
  ObjectInsertStmt: ['key', 'value', 'object'],
  ObjectMergeStmt: ['a', 'b', 'target'],
  ResetLocalStmt: ['target'],
  ResultSetAddStmt: ['value'],
  ReturnLocalStmt: ['source'],
  SetAddStmt: ['value', 'set'],

  // `MakeArrayStmt` is a special case, as its first argument, `capacity`, is a `BareNumber`, not a `local` to `wrap`.
  MakeArrayStmt: e => {
    if (e.stmt.row && e.stmt.col) {
      rc = `RowCol(${fileName(e.stmt.file)},${e.stmt.row}, ${e.stmt.col})`;
    } else {
      rc = 'RowColNotProvided()';
    }
    emit(`MakeArrayStmt(BareNumber(${e.stmt.capacity}), ${wrap(e.stmt.target)}, ${rc})`);
  },

  // TODO(dkorolev): This is a hack for v0.41.
  MakeNumberRefStmt: e => {
    let rc = '';
    if (e.stmt.row && e.stmt.col) {
      rc = `RowCol(${fileName(e.stmt.file)},${e.stmt.row}, ${e.stmt.col})`;
    } else {
      rc = 'RowColNotProvided()';
    }
	  emit(`MakeNumberRefStmt(StringConstantIndex(${e.stmt.Index}), ${wrap(e.stmt.target)}, ${rc})`);
  },

  AssignIntStmt: e => {
    let rc = '';
    if (e.stmt.row && e.stmt.col) {
      rc = `RowCol(${fileName(e.stmt.file)},${e.stmt.row}, ${e.stmt.col})`;
    } else {
      rc = 'RowColNotProvided()';
    }
    emit(`AssignIntStmt(BareNumber(${e.stmt.value}), ${wrap(e.stmt.target)}, ${rc})`);
  },
  MakeNumberIntStmt: e => {
    let rc = '';
    if (e.stmt.row && e.stmt.col) {
      rc = `RowCol(${fileName(e.stmt.file)},${e.stmt.row}, ${e.stmt.col})`;
    } else {
      rc = 'RowColNotProvided()';
    }
    emit(`MakeNumberIntStmt(BareNumber(${e.stmt.value}), ${wrap(e.stmt.target)}, ${rc})`);
  },

  BlockStmt: e => {
    emit(`BeginBlockStmt(RowCol(${fileName(e.stmt.file)}, ${e.stmt.row}, ${e.stmt.col}))`);
    const save_indent = global_indent;
    global_indent = global_indent + '  ';
    processBlocks(e.stmt.blocks);
    global_indent = save_indent;
    emit('EndBlockStmt()');
  },

  NotStmt: e => {
    emit(`NotStmtBegin(RowCol(${fileName(e.stmt.file)}, ${e.stmt.row}, ${e.stmt.col}))`);
    const save_indent = global_indent;
    global_indent = global_indent + '  ';
    processStatements(e.stmt.block.stmts);
    global_indent = save_indent;
    emit('NotStmtEnd()');
  },

  CallStmt: e => {
    let indexes = [`Local(${e.stmt.result})`];
    e.stmt.args.forEach(arg => indexes.push(wrap(arg)));
    const f = funcs[e.stmt.func];
    if (f.builtin) {
      emit(`CallBuiltinStmtBegin(${f.name}, Local(${e.stmt.result}), RowCol(${fileName(e.stmt.file)}, ${e.stmt.row}, ${e.stmt.col}))`);
    } else {
      emit(`CallUserStmtBegin(${f.index}, Local(${e.stmt.result}), RowCol(${fileName(e.stmt.file)}, ${e.stmt.row}, ${e.stmt.col}))`);
    }
    e.stmt.args.forEach((arg, arg_index) => {
      emit(`  CallStmtPassArg(${arg_index}, ${wrap(arg)})`);
    });
    if (f.builtin) {
      emit(`CallBuiltinStmtEnd(${f.name}, Local(${e.stmt.result}))`);
    } else {
      emit(`CallUserStmtEnd(${f.index}, Local(${e.stmt.result}))`);
    }
  },

  WithStmt: e => {
    let wrappedPath;
    if (e.stmt.path) {
      wrappedPath = `WrappedPath((${e.stmt.path.join(', ')}))`;
    } else {
      wrappedPath = 'WrappedPathEmpty()';
    }
    emit(`WithStmtBegin(Local(${e.stmt.local}), ${wrap(e.stmt.value)}, ${wrappedPath}, RowCol(${fileName(e.stmt.file)}, ${e.stmt.row}, ${e.stmt.col}))`);
    const save_indent = global_indent;
    global_indent = global_indent + '  ';
    processStatements(e.stmt.block.stmts);
    global_indent = save_indent;
    emit('WithStmtEnd()');
  },

  ScanStmt: e => {
    emit(`ScanStmtBegin(Local(${e.stmt.source}), ${e.stmt.key}, ${e.stmt.value}, RowCol(${fileName(e.stmt.file)}, ${e.stmt.row}, ${e.stmt.col}))`);
    const save_indent = global_indent;
    global_indent = global_indent + '  ';
    processStatements(e.stmt.block.stmts);
    global_indent = save_indent;
    emit('ScanStmtEnd()');
  },
};

processStatements = statements => {
  statements.forEach(e => {
    if (e.type in statement_processors) {
      const p = statement_processors[e.type];
      if (typeof p === 'object') {
        let args = [];
        p.forEach(k => args.push(wrap(e.stmt[k])));
        if (e.stmt && e.stmt.row && e.stmt.col) {
          args.push(`RowCol(${fileName(e.stmt.file)}, ${e.stmt.row}, ${e.stmt.col})`);
        } else {
          args.push('RowColNotProvided()');
        }
        emit(`${e.type}(${args.join(', ')})`);
      } else {
        statement_processors[e.type](e);
      }
    } else {
      console.log(`${global_indent}UnknownStatement(${JSON.stringify(e.type)})`);
    }
  });
};

processBlocks = blocks => {
  blocks.forEach(block => {
    console.log(global_indent + 'BeginBlock()');
    const save_indent = global_indent;
    global_indent = global_indent + '  ';
    processStatements(block.stmts);
    global_indent = save_indent;
    console.log(global_indent + 'EndBlock()');
  });
};

if (has_funcs) {
  console.log('');
  ir.funcs.funcs.forEach((e, i) => {
    console.log(`BeginFunction(${i}, ${JSON.stringify(e.name)})`);
    global_indent = '  ';
    processBlocks(e.blocks, '  ');
    console.log(`EndFunction(${i}, ${JSON.stringify(e.name)})`);
  });
}

global_indent = '  ';
ir.plans.plans.forEach((e, i) => {
  console.log('');
  const plan_name = i ? `plan_${i + 1}` : 'main';
  console.log(`BeginPlan(${i}, ${JSON.stringify(plan_name)})`);
  global_indent = '  ';
  processBlocks(e.blocks);
  console.log(`EndPlan(${i}, ${JSON.stringify(plan_name)})`);
});

console.log('');
console.log('EndOPADSL()');
