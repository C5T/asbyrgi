#!TEST comparison equals

package comparison

kotlin_export := "CompareStringPolicy"

default equals = false
equals {
  input.x == "42"
}
