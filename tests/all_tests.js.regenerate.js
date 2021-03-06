const fs = require('fs');

let rego = [];
process.argv.forEach(fn => { if (fn.match(/.*\.rego$/)) rego.push(fn); });

if (!rego.length) {
  console.error('Provide one or more `.rego` files as the input, or use the `gen_all_js_tests.sh` script.');
  process.exit(1);
}

console.log('// AUTOGENERATED FILE, do not edit.');
console.log('// To run the tests: node_modules/mocha/bin/mocha.js tests/all_tests.js');
console.log('//               Or: open `tests/mocha.html` in the browser.');
console.log('');

console.log('const expect = (typeof window === "undefined") ? require("chai").expect : chai.expect;');
console.log('');

console.log('const get_policy_main = (fn) => {');
console.log('  try {');
console.log('    if (typeof window === "undefined") {');
console.log('      return require(`./${fn}.js`).main;');
console.log('    } else {');
console.log('      return exported_mains[`${fn}.js`];');
console.log('    }');
console.log('  } catch(e) {');
console.log('    console.error("Error loading `./${fn}.js`, did you run `./scripts/gen_all_js.sh`?");');
console.log('    process.exit(1);');
console.log('  }');
console.log('};');
console.log('');

let cases = {};

rego.forEach(fn => {
  let path = fn.split('/');
  path[path.length - 1] = 'tests.json';
  const tests_path = path.join('/');
  const tests = (() => {
    try {
      return fs.readFileSync(tests_path, {encoding:'utf8'}).split('\n').filter(x => x !== '').map(JSON.parse);
    } catch(e) {
      console.error(`Error reading '${tests_path}', something's wrong with the '.rego' files in the repo.`);
      process.exit(1);
    }
  })();
  const goldens_path = fn + '.goldens.json';
  const goldens = (() => {
    try {
      return fs.readFileSync(goldens_path, {encoding:'utf8'}).split('\n').filter(x => x !== '').map(JSON.parse);
    } catch(e) {
      console.error(`Error reading '${goldens_path}', did you run './scripts/gen_all_goldens.sh'?`);
      process.exit(1);
    }
  })();
  if (tests.length !== goldens.length) {
    console.error(`The number of tests and goldens don't match for '${fn}'.`);
    process.exit(1);
  }
  cases[fn] = { tests, goldens };
});

Object.keys(cases).sort().forEach(fn => {
  const t = cases[fn];
  console.log(``);
  console.log(`describe('${fn}', () => {`);
  console.log(`  const policy = get_policy_main('${fn}');`);
  for (let i = 0; i < t.tests.length; ++i) {
    console.log(`  it('${JSON.stringify(t.tests[i])}', () => {`)
    console.log(`    expect(policy(${JSON.stringify(t.tests[i])})).to.deep.equal(${JSON.stringify({result: t.goldens[i]})});`);
    console.log(`  });`);
  }
  console.log(`});`);
});
