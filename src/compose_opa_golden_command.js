const fs = require('fs');
const readline = require('readline');
const rl = readline.createInterface({ input: process.stdin });
rl.on('line', (line) => {
  const fields = line.split('\t');
  let field_index = 0;
  try {
    if (fields.length === 0) {
      console.error('Need a single JSON, or multiple tab-separated JSONs, as each line of `tests.txt`.');
      process.exit(1);
    }
    let command = [];
    command.push('-i /tmp/opa_input.callopa.tmp');
    fs.writeFileSync('/tmp/opa_input.callopa.tmp', JSON.stringify(JSON.parse(fields[0])) + '\n');
    if (fields.length > 1) {
      for (field_index = 1; field_index < fields.length; ++field_index) {
        const elem = JSON.parse(fields[field_index]);
        const keys = Object.keys(elem);
        if (keys.length !== 1) {
          console.error('Each `data` JSON, after the tab, should only have one key.');
          process.exit(1);
        }
        const fn = `/tmp/opa_data_${field_index}.callopa.tmp`;
        command.push(`-d ${fn}`);

        let body = {
          data: {}
        };
        let placeholder = body;
        let placeholder_key = 'data';
        let step_made = false;
        keys[0].split('.').forEach(k => {
          step_made = true;
          if (typeof placeholder[placeholder_key] != 'object') {
            placeholder[placeholder_key] = {};
          }
          placeholder = placeholder[placeholder_key];
          placeholder_key = k;
        });
        if (!step_made) {
          console.error('Each `data` JSON, after the tab, should only have one key, and this key should not be empty.');
          process.exit(1);
        }
        placeholder[placeholder_key] = elem[keys[0]];
        fs.writeFileSync(fn, JSON.stringify(body.data) + '\n');
      }
    }
    console.log(command.join(' '));
  } catch (e) {
    console.error(`Malformed JSON: ${fields[field_index]}`);
    process.exit(1);
  }
});
