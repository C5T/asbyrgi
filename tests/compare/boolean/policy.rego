#!TEST comparison equals

package comparison

kotlin_export := "CompareBooleanPolicy"

default equals = false
equals {
  input.x == true
}
