#!TEST comparison equals

package comparison

kotlin_export := "CompareNumberPolicy"

default equals = false
equals {
  input.x == 42
}
