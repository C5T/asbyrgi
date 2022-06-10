#!/bin/bash

if [ "$1" == "ir" ] ; then
  echo "Here be dragons."
else
  opa $*
fi
