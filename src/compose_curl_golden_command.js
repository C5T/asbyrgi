const readline = require('readline');
const rl = readline.createInterface({ input: process.stdin });
rl.on('line', (line) => {
  const fields = line.split('\t');
  let body = { input: JSON.parse(fields[0]) };
  console.log(JSON.stringify(body));
});
