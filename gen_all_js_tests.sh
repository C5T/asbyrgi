#!/bin/bash

(cd tests; find . -iname '*.rego' | xargs node all_tests.js.regenerate.js >all_tests.js)
