#!/bin/bash

set -e

CONTAINER_ID=${ASBYRGI_CONTAINER_ID:-$(docker build -q .)}

rm -rf kt_test

docker run -i $CONTAINER_ID kt_test.tar.gz | tar xz
mkdir -p "kt_test/src/test/kotlin"

for REGO_TEST_CASE in $(find tests/ -iname '*.rego' | sort); do
  export REGO_KT_IMPL_NAME=$(cat $REGO_TEST_CASE | docker run -i $CONTAINER_ID evalterm data.$(head -n 1 $REGO_TEST_CASE | cut -f2 -d' ').kotlin_class_name)
  if [ "$REGO_KT_IMPL_NAME" != "null" ] ; then
    ./scripts/gen_all_kt.sh.helper $CONTAINER_ID $REGO_TEST_CASE
    cp $REGO_TEST_CASE.kt kt_test/src/main/kotlin/$REGO_KT_IMPL_NAME.kt

    HEADER=$(head -n 1 "$REGO_TEST_CASE")
    if [[ "$HEADER" =~ \#!TEST ]] ; then
      read -r UNUNSED_TEST PACKAGE RULE <<< "$HEADER"
      while IFS= read -r line; do
        echo -en "$line\t"
        echo "$line" \
        | docker run -v "$PWD/$(dirname "$REGO_TEST_CASE")":/$(dirname "$REGO_TEST_CASE") -i $CONTAINER_ID eval --data "$REGO_TEST_CASE" --input /dev/stdin data.$PACKAGE.$RULE \
        | jq .result[0].expressions[0].value
      done < "$(dirname "$REGO_TEST_CASE")/tests.json" | tr - - \
      | node tests/compose_kt_test.js $REGO_KT_IMPL_NAME \
      > kt_test/src/test/kotlin/${REGO_KT_IMPL_NAME}Test.kt
    fi
  else
    echo "Skipping '$REGO_TEST_CASE' because it has no 'kotlin_class_name'."
  fi
done
