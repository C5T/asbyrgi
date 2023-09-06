#!TEST comparison equals

package comparison

kotlin_export := "CompareArraysPolicy"

default equals = false
equals {
  input.x == [1,2]
}
