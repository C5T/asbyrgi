#!TEST smoke test_split

package smoke

kotlin_class_name := "SplitStringPolicy"

test_split := split(input.s, ",")[input.i]
