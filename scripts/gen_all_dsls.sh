#!/bin/bash

if [ "$JSOPA_CONTAINER_ID" == "" ] ; then
  echo 'Building the container, export `$JSOPA_CONTAINER_ID` to skip this phase.'
  JSOPA_CONTAINER_ID=$(cd docker; docker build -q .)
else
  echo 'Using the exported `$JSOPA_CONTAINER_ID`.'
fi

find . -iname '*.rego.dsl' -exec rm "{}" \;
find . -iname '*.rego' -exec ./gen_all_dsls.sh.helper $JSOPA_CONTAINER_ID "{}" \;
