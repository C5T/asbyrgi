#!TEST comparison equals

package comparison

kotlin_class_name := "CompareArraysPolicy"

default equals = false
equals {
  input.x == [1,2]
}
