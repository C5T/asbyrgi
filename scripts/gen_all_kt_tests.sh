#!/bin/bash

# TODO(dkorolev): Remove this comment! =)
COMMENT="""
# NOTE(dkorolev): Here is how I'm testing this code.

./scripts/gen_all_kt.sh.helper $(docker build -q .) tests/smoke/sum/policy.rego && \
cp tests/smoke/sum/policy.rego.kt ../simple-kotlin/src/main/kotlin/SmokeSumPolicy.kt && \
(cd /home/ubuntu/github/dkorolev/simple-kotlin; ./ktlint --format src/main/kotlin/SmokeSumPolicy.kt)

# Then, within the `{gradle-devenv}` docker container in `../simple-kotlin`:
gradle test
"""

(cd tests; find . -iname '*.rego' | sort | xargs node all_tests.kt.regenerate.js)
