#!TEST comparison equals

package comparison

kotlin_export := "CompareObjectPolicy"

default equals = false
equals {
  input.x == {"y": 42}
}
