#!/bin/bash
#
# NOTE(dkorolev): This is an internal script, designed to be run from within a Docker container.
#                 It does work outside that container too if you have OPA installed :-) but that's not the intended usage.

if [ "$1" == "rego2ir" ] ; then
  if [ $# == 4 ] ; then
    if ! [ -e /input/"$2" ] ; then
      echo "The input '$2' file is not found."
      exit 1
    fi
    if ! opa build /input/"$2" -e "$3"/"$4" -t plan -o /dev/stdout | tar xz -O /plan.json 2>/dev/null ; then
      echo 'OPA run failed.' >/dev/stderr
      exit 1
    fi
    exit 0
  else
    echo 'Recommended synopsis: `docker run -v "$PWD"/input $JSOPA_CONTAINER_ID rego2ir policy.rego myapi result | jq .`.'
    echo 'This requires `policy.rego` in the current directory, and outputs the IR for package `myapi`, rule `result`, as a JSON.'
    echo 'Easiest way to obtain `JSOPA_CONTAINER_ID`: `export JSOPA_CONTAINER_ID=$(docker build -q .)` from the `jsopa/docker` dir.'
    exit 1
  fi
elif [ "$1" == "rego2dsl" ] ; then
  if [ $# == 4 ] ; then
    if ! [ -e /input/"$2" ] ; then
      echo "The input '$2' file is not found."
      exit 1
    fi
    if ! opa build /input/"$2" -e "$3"/"$4" -t plan -o /dev/stdout | tar xz -O /plan.json >/tmp/ir.json 2>/dev/null ; then
      echo 'OPA run failed.' >/dev/stderr
      exit 1
    fi
    if ! [ -s /tmp/ir.json ] ; then
      echo 'No IR generated.' >/dev/stderr
      exit 1
    fi
    # TODO(dkorolev): Pass "$3" and "$4" to the script.
    /src/ir2dsl.js /tmp/ir.json
    rm -f /tmp/ir.json
    exit 0
  else
    echo 'Recommended synopsis: `docker run -v "$PWD"/input $JSOPA_CONTAINER_ID rego2dsl policy.rego myapi result`.'
    echo 'This requires `policy.rego` in the current directory, and outputs the IR in the DSL format for it.'
    echo 'Easiest way to obtain `JSOPA_CONTAINER_ID`: `export JSOPA_CONTAINER_ID=$(docker build -q .)` from the `jsopa/docker` dir.'
    exit 1
  fi
elif [ "$1" == "rego2js" ] ; then
  if [ $# == 4 ] ; then
    if ! [ -e /input/"$2" ] ; then
      echo "The input '$2' file is not found."
      exit 1
    fi
    if ! opa build /input/"$2" -e "$3"/"$4" -t plan -o /dev/stdout | tar xz -O /plan.json >/tmp/ir.json 2>/dev/null ; then
      echo 'OPA run failed.' >/dev/stderr
      exit 1
    fi
    if ! [ -s /tmp/ir.json ] ; then
      echo 'No IR generated.' >/dev/stderr
      exit 1
    fi
    # TODO(dkorolev): Pass "$3" and "$4" to the script.
    (cat /src/preprocess.inl.js; /src/ir2dsl.js /tmp/ir.json) | cpp | grep -v '^#' | grep -v '^$'
    rm -f /tmp/ir.json
    exit 0
  else
    echo 'Recommended synopsis: `docker run -v "$PWD"/input $JSOPA_CONTAINER_ID rego2js policy.rego myapi result`.'
    echo 'This requires `policy.rego` in the current directory, and outputs a standalone, runnable JavaScript source file for it.'
    echo 'Easiest way to obtain `JSOPA_CONTAINER_ID`: `export JSOPA_CONTAINER_ID=$(docker build -q .)` from the `jsopa/docker` dir.'
    exit 1
  fi
elif [ "$1" == "gengolden" ] ; then
  if [ $# == 5 ] ; then
    opa build /input/"$2"
    opa run --server bundle.tar.gz -l error >/dev/null 2>dev/null &
    OPA_PID=$!
    sleep 0.5  # TODO(dkorolev): I would love to check `localhost:8181/health`, but it just returns `{}`, w/o HTTP code or body.
    while read -r QUERY ; do
      curl -s -d "{\"input\":$QUERY}" localhost:8181/v1/data | jq -c .result.$3.$4
    done < /input/"$5"
    kill $OPA_PID
    wait
    exit 0
  else
    echo 'Recommended synopsis: `docker run -v "$PWD"/input $JSOPA_CONTAINER_ID gengolden policy.rego myapi result tests.json`.'
    echo 'This requires `policy.rego` and `tests.json` in the current directory. The tests should be one JSON per line.'
    echo 'Easiest way to obtain `JSOPA_CONTAINER_ID`: `export JSOPA_CONTAINER_ID=$(docker build -q .)` from the `jsopa/docker` dir.'
    exit 1
  fi
else
  opa $*
fi
