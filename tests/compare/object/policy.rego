#!TEST comparison equals

package comparison

kotlin_class_name := "CompareObjectPolicy"

default equals = false
equals {
  input.x == {"y": 42}
}
