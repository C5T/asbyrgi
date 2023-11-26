#!/bin/bash

set -e

CONTAINER_ID=${ASBYRGI_CONTAINER_ID:-$(docker build -q .)}

rm -rf kt_test

docker run -i $CONTAINER_ID kt_test.tar.gz | tar xz
mkdir -p "kt_test/src/test/kotlin"

for REGO_TEST_CASE in $(find tests/ -iname '*.rego' | sort); do
  export REGO_KT_IMPL_NAME=$(cat $REGO_TEST_CASE | docker run -i $CONTAINER_ID evalterm data.$(head -n 1 $REGO_TEST_CASE | cut -f2 -d' ').kotlin_export)
  if [ "$REGO_KT_IMPL_NAME" != "null" ] ; then
    ./scripts/gen_all_kt.sh.helper $CONTAINER_ID $REGO_TEST_CASE
    cp $REGO_TEST_CASE.kt kt_test/src/main/kotlin/$REGO_KT_IMPL_NAME.kt
    cat "$(dirname $REGO_TEST_CASE)/tests.json" "$REGO_TEST_CASE.goldens.json" \
    | docker run -i $CONTAINER_ID compose_kt_test $REGO_KT_IMPL_NAME \
    > kt_test/src/test/kotlin/${REGO_KT_IMPL_NAME}Test.kt
  else
    echo "Skipping '$REGO_TEST_CASE' because it has no 'kotlin_export'."
  fi
done
