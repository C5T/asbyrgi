#!/bin/bash
#
# NOTE(dkorolev): This is an internal script, designed to be run from within a Docker container.
#                 It does work outside that container too if you have OPA installed :-) but that's not the intended usage.

if [ "$1" == "ir" ] ; then
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
    sleep 0.5  # TODO(dkorolev): I would love to check `localhost:8181/health`, but it just returns `{}`, w/o HTTP code or body.
    while read -r QUERY ; do
      curl -s -d "{\"input\":$QUERY}" localhost:8181/v1/data | sed 's/^{"result"://' | sed 's/}$//' ; echo
    done < /input/"$5"
    kill $OPA_PID
    wait
    exit 0
  else
    echo 'Recommended synopsis: `docker run -v $PWD:/input $CONTAINER_ID gengolden policy.rego myapi result tests.json`.'
    echo 'This requires `policy.rego` and `tests.json` in the current directory. The tests should be one JSON per line.'
    echo 'Easiest way to obtain `CONTAINER_ID`: `export CONTAINER_ID=$(docker build -q .)` from the `jsopa/docker` repo.'
    exit 1
  fi
else
  opa $*
fi
