#!TEST comparison equals

package comparison

kotlin_class_name := "CompareBooleanPolicy"

default equals = false
equals {
  input.x == true
}
