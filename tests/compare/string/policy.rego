#!TEST comparison equals

package comparison

kotlin_class_name := "CompareStringPolicy"

default equals = false
equals {
  input.x == "42"
}
