#!/bin/bash

SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"

if [ "$ASBYRGI_CONTAINER_ID" == "" ] ; then
  echo 'Building the container, export `$ASBYRGI_CONTAINER_ID` to skip this phase.'
  ASBYRGI_CONTAINER_ID=$(docker build -q .)
else
  echo 'Using the exported `$ASBYRGI_CONTAINER_ID`.'
fi

find . -iname '*.rego.dsl' -exec rm "{}" \;
find . -iname '*.rego' -exec "$SCRIPT_DIR/gen_all_dsls.sh.helper" $ASBYRGI_CONTAINER_ID "{}" \;
