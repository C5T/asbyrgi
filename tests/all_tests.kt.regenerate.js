// NOTE(dkorolev): Adapted from `tests/all_tests.js.regenerate.js`.

const fs = require('fs');

let rego = [];
process.argv.forEach(fn => { if (fn.match(/.*\.rego$/)) rego.push(fn); });

if (!rego.length) {
  console.error('Provide one or more `.rego` files as the input, or use the `gen_all_js_tests.sh` script.');
  process.exit(1);
}

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

console.log('import kotlinx.serialization.json.Json');
console.log('import org.junit.jupiter.api.Assertions.assertEquals');
console.log('import org.junit.jupiter.api.Test');
console.log('');
console.log('fun runOpaTestCase(goldenJson: String, policy: (OpaValue, OpaValue) -> OpaValue, inputJson: String) {');
    console.log('assertEquals(goldenJson, opaValueToJson(policy(jsonToOpaValue(Json.parseToJsonElement(inputJson)), OpaValue.ValueUndefined)).toString())');

Object.keys(cases).sort().forEach(fn => {
  const t = cases[fn];
  const name = fn.replace(/^\.\//, '').replace(/^tests/, '').replace(/_/g, '/').replace(/\.rego$/, '').split('/').map(s => s.charAt(0).toUpperCase() + s.substr(1)).join('');
  console.log('');
  console.log(`class ${name}Test {`);
  for (let i = 0; i < t.tests.length; ++i) {
    if (i > 0 ) {
      console.log('');
    }
    console.log('    @Test');
    console.log(`    fun test${i+1}() {`);
    console.log(`        runOpaTestCase("""{"result":${JSON.stringify(t.goldens[i])}}""", ` +
                `::${name}Impl, """${JSON.stringify(t.tests[i])}""")`);
    console.log(`    }`);
  }
  console.log(`}`);
});
