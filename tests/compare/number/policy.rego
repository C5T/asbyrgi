#!TEST comparison equals

package comparison

kotlin_class_name := "CompareNumberPolicy"

default equals = false
equals {
  input.x == 42
}
