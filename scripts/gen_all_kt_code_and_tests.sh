#!/bin/bash

set -e

rm -rf kt_test

docker run -i $(docker build -q .) kt_test.tar.gz | tar xz

echo CHECKPOINT 1
ls -lasR kt_test
echo CHECKPOINT 2
ls -lasR tests

for REGO_TEST_CASE in $(find tests/ -iname '*.rego' | sort); do
  export REGO_KT_IMPL_NAME=$(cat $REGO_TEST_CASE | docker run -i $(docker build -q .) evalterm data.$(head -n 1 $REGO_TEST_CASE | cut -f2 -d' ').kotlin_class_name)
  if [ "$REGO_KT_IMPL_NAME" != "null" ] ; then
    ./scripts/gen_all_kt.sh.helper $(docker build -q .) $REGO_TEST_CASE
    echo CHECKPOINT 3
    ls -lasR tests
    cp $REGO_TEST_CASE.kt kt_test/src/main/kotlin/$REGO_KT_IMPL_NAME.kt
    node tests/generate_kt_test.js "$REGO_TEST_CASE" "$(dirname "$REGO_TEST_CASE")" $REGO_KT_IMPL_NAME >kt_test/src/test/kotlin/${REGO_KT_IMPL_NAME}Test.kt
  else
    echo "Skipping '$REGO_TEST_CASE' because it has no 'kotlin_class_name'."
  fi
done
