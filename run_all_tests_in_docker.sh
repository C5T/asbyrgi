#!/bin/bash

if [ "$JSOPA_CONTAINER_ID" == "" ] ; then
  echo 'Building the container, export `$JSOPA_CONTAINER_ID` to skip this phase.'
  JSOPA_CONTAINER_ID=$(cd docker; docker build -q .)
else
  echo 'Using the exported `$JSOPA_CONTAINER_ID`.'
fi

echo 'Running the tests.'

if docker run -v "$PWD/tests":/tests $JSOPA_CONTAINER_ID run_all_tests ; then
  echo -e "\n\033[32m\033[1mALL TESTS PASSED.\033[0m"
else
  echo -e "\n\033[31m\033[1mSOME TESTS FAILED.\033[0m"
  exit 1
fi
