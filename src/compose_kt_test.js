if (process.argv.length < 2) {
  console.log('Synopsis: cat tests.json some_goldens.json | node compose_kt_test.js KotlinIdentifiedName')
  console.log(process.argv);
  process.exit(1);
}

const readline = require('readline');

const kotlin_export = process.argv[process.argv.length - 1];

let lines = []
const rl = readline.createInterface({ input: process.stdin });
rl.on('line', (line) => {
  lines.push(line);
});

const prepareKotlinTestCase = (text) => {
  const fields = text.split('\t');
  const input = JSON.parse(fields[0]);
  let desc = JSON.stringify(input);
  if (fields.length === 1) {
    return { desc, input };
  } else {
    let data = [];
    for (let i = 1; i < fields.length; ++i) {
      const elem = JSON.parse(fields[i]);
      const keys = Object.keys(elem);
      if (keys.length !== 1) {
        console.error('Each `data` JSON, after the tab, should only have one key.');
        process.exit(1);
      }
      const key = keys[0];
      const value = elem[keys[0]];
      data.push({key, value});
      desc += ` + data.${key} = ${JSON.stringify(value)}`;
    }
    return { desc, input, data };
  }
};

rl.on('close', () => {
  if (lines.length === 0) {
    console.error('No test data to work with.');
    process.exit(1);
  }

  if ((lines.length % 2) !== 0) {
    console.error('Should be an even number of input lines, first N tests then N goldens.');
    process.exit(1);
  }

  const N = lines.length / 2;

  let tests = [];
  for (let i = 0; i < N; ++i) {
    tests.push({
      test: prepareKotlinTestCase(lines[i]),
      golden: lines[i + N]
    });
  }

    console.log('import kotlinx.serialization.json.Json');
    console.log('import org.junit.jupiter.api.Assertions.assertEquals');
    console.log('import org.junit.jupiter.api.Test');
    console.log('import org.junit.jupiter.api.DisplayName');
    console.log('import org.junit.jupiter.api.MethodOrderer.OrderAnnotation');
    console.log('import org.junit.jupiter.api.Order');
    console.log('import org.junit.jupiter.api.TestMethodOrder');

    console.log('');
    console.log(`fun run${kotlin_export}TestCase(goldenJson: String, inputJson: String, dataProvider: AuthzDataProvider = AuthzDataProvider()) {`);
    console.log('    assertEquals(');
    console.log('        goldenJson,');
    console.log(`        authzResultToJson(${kotlin_export}(jsonToAuthzValue(Json.parseToJsonElement(inputJson)), dataProvider)).toString())`);
    console.log('}');
    console.log('');
    console.log("// NOTE: The tests below, at least originally, were autogenerated from `tests.json` and from OPA's golden outputs.");
    console.log(`@TestMethodOrder(OrderAnnotation::class)`);
    console.log(`class ${kotlin_export}AutogeneratedTestSuite {`);

    let i = 0;
    tests.forEach((t) => {
      if (!i) {
        i = 1;
        first = false;
      } else {
        console.log('');
        ++i;
      }
      console.log('    @Test');
      console.log(`    @Order(${i})`);
      console.log(`    @DisplayName("""${t.test.desc} => ${t.golden}""")`);
      if (!('data' in t.test)) {
        console.log(`    fun test${kotlin_export}${i}() = ` +
                    `run${kotlin_export}TestCase("""${t.golden}""", """${JSON.stringify(t.test.input)}""")`);
      } else {
        console.log(`    fun test${kotlin_export}${i}() {`);
        let init = [];
        let inject = [];
        let q = 0;
        t.test.data.forEach((e) => {
          ++q;
          if (typeof e.value === 'string') {
            init.push(`fun dp${q}(): String = ${JSON.stringify(e.value)}`);
            inject.push(`String("${'data.' + e.key}", ::dp${q})`);
          } else if (typeof e.value === 'number') {
            init.push(`fun dp${q}(): Int = ${JSON.stringify(e.value)}`);
            inject.push(`Int("${'data.' + e.key}", ::dp${q})`);
          } else {
            init.push(`fun dp${q}(): AuthzValue = jsonToAuthzValue(Json.parseToJsonElement("""${JSON.stringify(e.value)}"""))`);
            inject.push(`Value("${'data.' + e.key}", ::dp${q})`);
          }
        });
        init.forEach((e) => {
          console.log(`        ${e}`);
        });
        console.log(`        var dp = AuthzDataProvider()`);
        inject.forEach((e) => {
          console.log(`        dp.inject${e}`);
        });
        console.log(`        run${kotlin_export}TestCase("""${t.golden}""", """${JSON.stringify(t.test.input)}""", dp)`);
        console.log(`    }`);
      }
    });
    console.log(`}`);
});
