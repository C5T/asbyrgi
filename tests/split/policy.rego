#!TEST smoke test_split

package smoke

kotlin_export := "SplitStringPolicy"

test_split := split(input.s, ",")[input.i]
