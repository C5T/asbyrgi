#!/bin/bash

# TODO(dkorolev): Remove this comment! =)
COMMENT="""
# NOTE(dkorolev): Here is how I'm testing this code.

export REGO_TEST_CASE=tests/compare/array/policy.rego

./scripts/gen_all_kt.sh.helper $(docker build -q .) $REGO_TEST_CASE && \
cp $REGO_TEST_CASE.kt ../simple-kotlin/src/main/kotlin/SmokeSumPolicy.kt && \
(cd /home/ubuntu/github/dkorolev/simple-kotlin; ./ktlint --format src/main/kotlin/SmokeSumPolicy.kt)

# Don't forget the test:
node tests/all_tests.kt.regenerate.js $REGO_TEST_CASE >../simple-kotlin/src/test/kotlin/SmokeSumPolicyTest.kt

# Then, within the `{gradle-devenv}` docker container in `../simple-kotlin`:
gradle test
"""

(cd tests; find . -iname '*.rego' | sort | xargs node all_tests.kt.regenerate.js)
