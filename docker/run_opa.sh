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
elif [ "$1" == "gengolden" ] ; then
  if [ $# == 5 ] ; then
    opa build /input/"$2"
    opa run --server bundle.tar.gz -l error >/dev/null 2>dev/null &
    OPA_PID=$!
    sleep 0.5  # TODO(dkorolev): I would love to check `localhots:8181/health`, but it just returns `{}`, w/o HTTP code or body.
    while read -r QUERY ; do
      curl -s -d "{\"input\":$QUERY}" localhost:8181/v1/data | sed 's/^{"result"://' | sed 's/}$//' ; echo
    done < /input/"$5"
    kill $OPA_PID
    wait
  else
    echo 'Recommended synopsis: `docker run -v $PWD:/input $CONTAINER_ID gengolden policy.rego myapi result tests.json`.'
    echo 'This requires `policy.rego` and `tests.json` in the current directory. The tests should be one JSON per line.'
    echo 'Easiest way to obtain `CONTAINER_ID`: `export CONTAINER_ID=$(docker build -q .)` from the `jsopa/docker` repo.'
    exit 1
  fi
else
  opa $*
fi
