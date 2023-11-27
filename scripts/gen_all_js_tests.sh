#!/bin/bash

rm -rf js_test
mkdir js_test

(cd js_test; (cd ../tests; find . -iname '*.rego' | sort) | xargs node ../src/generate_all_js_tests.js >all_tests.js)

cat >js_test/mocha.html <<EOF
<!doctype html>
<head>
  <meta charset='utf-8'>
  <title>Ásbyrgi Mocha Tests</title>
  <link href='https://cdn.rawgit.com/mochajs/mocha/2.2.5/mocha.css' rel='stylesheet' />
</head>
<body>
  <div id='mocha'></div>

  <script src='https://cdn.rawgit.com/jquery/jquery/2.1.4/dist/jquery.min.js'></script>
  <script src='https://cdn.rawgit.com/Automattic/expect.js/0.3.1/index.js'></script>
  <script src='https://cdn.rawgit.com/mochajs/mocha/2.2.5/mocha.js'></script>
  <script src='https://cdnjs.cloudflare.com/ajax/libs/chai/3.3.0/chai.js'></script>

  <script language='javascript'>mocha.setup('bdd')</script>
  <script language='javascript'>exported_mains = {};</script>

EOF

(cd tests;
 for i in $(find . -iname '*.rego.js') ; do
   echo "  <script language='javascript'>export_main = (main) => { exported_mains['$i'] = main; };</script>" ;
   echo "  <script src='../tests/$i'></script>" ;
   echo ;
 done) >> js_test/mocha.html

cat >>js_test/mocha.html <<EOF
  <script src='./all_tests.js'></script>

  <script language='javascript'>
    mocha.checkLeaks();
    mocha.globals(['jQuery']);
    mocha.run();
  </script>
</body>
EOF
