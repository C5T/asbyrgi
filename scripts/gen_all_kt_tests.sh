#!/bin/bash

set -e

# NOTE(dkorolev): This is, of course, temporary.
OUTPUT_DIR=../simple-kotlin

for REGO_TEST_CASE in $(find tests/ -iname 'policy.rego'); do
  export REGO_KT_IMPL_NAME=$(cat $REGO_TEST_CASE | docker run -i $(docker build -q .) evalterm data.$(head -n 1 $REGO_TEST_CASE | cut -f2 -d' ').kotlin_class_name)
  if [ "$REGO_KT_IMPL_NAME" != "null" ] ; then
    ./scripts/gen_all_kt.sh.helper $(docker build -q .) $REGO_TEST_CASE
    cp $REGO_TEST_CASE.kt $OUTPUT_DIR/src/main/kotlin/$REGO_KT_IMPL_NAME.kt
    (cd $OUTPUT_DIR; ./ktlint --format src/main/kotlin/$REGO_KT_IMPL_NAME.kt)
    node tests/generate_kt_test.js "$(dirname $REGO_TEST_CASE)" $REGO_KT_IMPL_NAME >$OUTPUT_DIR/src/test/kotlin/${REGO_KT_IMPL_NAME}Test.kt
  else
    echo "Skipping '$REGO_TEST_CASE' because it has no 'kotlin_class_name'."
  fi
done

# Then, within the `{gradle-devenv}` docker container in `../simple-kotlin`, run: `gradle test`
