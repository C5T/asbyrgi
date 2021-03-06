#!/bin/bash
#
# NOTE(dkorolev): This is an internal script, designed to be run from within a Docker container.
#                 It does work outside that container too if you have OPA installed :-) but that's not the intended usage.

if [ "$1" == "rego2ir" ] ; then
  if [ $# == 3 ] ; then
    if ! opa build /dev/stdin -e "$2"/"$3" -t plan -o /dev/stdout | gunzip | tar x -O plan.json 2>/dev/null ; then
      echo 'OPA run failed.' >/dev/stderr
      exit 1
    fi
    exit 0
  else
    echo 'Usage: cat policy.rego | docker run -i $ASBYRGI_CONTAINER_ID rego2ir myapi result | jq .'
    echo 'The above command would generate the IR for the rule `result` from the package `myapi` of `policy.rego`.'
    echo 'Easiest way to obtain `ASBYRGI_CONTAINER_ID`: run `export ASBYRGI_CONTAINER_ID=$(docker build -q .)` from this repo.'
    exit 1
  fi
elif [ "$1" == "rego2dsl" ] ; then
  if [ $# == 3 ] ; then
    if ! opa build /dev/stdin -e "$2"/"$3" -t plan -o /dev/stdout | gunzip | tar x -O plan.json >/tmp/ir.json 2>/dev/null ; then
      echo 'OPA run failed.' >/dev/stderr
      exit 1
    fi
    if ! [ -s /tmp/ir.json ] ; then
      echo 'No IR generated.' >/dev/stderr
      exit 1
    fi
    # TODO(dkorolev): Pass "$2" and "$3" to the script.
    /src/ir2dsl.js /tmp/ir.json
    rm -f /tmp/ir.json
    exit 0
  else
    echo 'Usage: cat policy.rego | docker run -i $ASBYRGI_CONTAINER_ID rego2dsl myapi result'
    echo 'The above command would generate the DSL for the rule `result` from the package `myapi` of `policy.rego`.'
    echo 'Easiest way to obtain `ASBYRGI_CONTAINER_ID`: run `export ASBYRGI_CONTAINER_ID=$(docker build -q .)` from this repo.'
    exit 1
  fi
elif [ "$1" == "rego2js" ] ; then
  if [ $# == 3 ] ; then
    if ! opa build /dev/stdin -e "$2"/"$3" -t plan -o /dev/stdout | gunzip | tar x -O plan.json >/tmp/ir.json 2>/dev/null ; then
      echo 'OPA run failed.' >/dev/stderr
      exit 1
    fi
    if ! [ -s /tmp/ir.json ] ; then
      echo 'No IR generated.' >/dev/stderr
      exit 1
    fi
    # TODO(dkorolev): Pass "$2" and "$3" to the script.
    (cat /src/preprocess.inl.js; /src/ir2dsl.js /tmp/ir.json) | ucpp | grep -v '^#' | grep -v '^$'
    rm -f /tmp/ir.json
    exit 0
  else
    echo 'Usage: cat policy.rego | docker run -i $ASBYRGI_CONTAINER_ID rego2js myapi result'
    echo 'The above command would generate the JavaScript for the rule `result` from the package `myapi` of `policy.rego`.'
    echo 'Easiest way to obtain `ASBYRGI_CONTAINER_ID`: run `export ASBYRGI_CONTAINER_ID=$(docker build -q .)` from this repo.'
    exit 1
  fi
elif [ "$1" == "rego2cpp" ] ; then
  if [ $# == 3 ] ; then
    if ! opa build /dev/stdin -e "$2"/"$3" -t plan -o /dev/stdout | gunzip | tar x -O plan.json >/tmp/ir.json 2>/dev/null ; then
      echo 'OPA run failed.' >/dev/stderr
      exit 1
    fi
    if ! [ -s /tmp/ir.json ] ; then
      echo 'No IR generated.' >/dev/stderr
      exit 1
    fi
    # TODO(dkorolev): Pass "$2" and "$3" to the script.
    (cat /src/preprocess.inl.h; /src/ir2dsl.js /tmp/ir.json) | ucpp | grep -v '^#' | grep -v '^$'
    rm -f /tmp/ir.json
    exit 0
  else
    echo 'Usage: cat policy.rego | docker run -i $ASBYRGI_CONTAINER_ID rego2cpp myapi result'
    echo 'The above command would generate the C++ source for the rule `result` from the package `myapi` of `policy.rego`.'
    echo 'Easiest way to obtain `ASBYRGI_CONTAINER_ID`: run `export ASBYRGI_CONTAINER_ID=$(docker build -q .)` from this repo.'
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
    echo 'Recommended synopsis: `docker run -v "$PWD"/input $ASBYRGI_CONTAINER_ID gengolden policy.rego myapi result tests.json`.'
    echo 'This requires `policy.rego` and `tests.json` in the current directory. The tests should be one JSON per line.'
    echo 'Easiest way to obtain `ASBYRGI_CONTAINER_ID`: run `export ASBYRGI_CONTAINER_ID=$(docker build -q .)` from this repo.'
    exit 1
  fi
else
  opa $*
fi
