#!/bin/bash

if [ "$1" == "ir" ] ; then
  if [ $# == 4 ] ; then
    opa build /input/"$2" -e "$3"/"$4" -t plan -o /dev/stdout | tar xz -O /plan.json 2>/dev/null
  else
    echo 'Recommended synopsis: `docker run -v $PWD:/input $CONTAINER_ID ir policy.rego myapi result | jq .`.'
    echo 'This requires `policy.rego` in the current directory, and outputs the IR for package `myapi`, rule `result`, as a JSON.'
    echo 'Easiest way to obtain `CONTAINER_ID`: `export CONTAINER_ID=$(docker build -q .)` from the `jsopa/docker` repo.'
    exit 1
  fi
else
  opa $*
fi
