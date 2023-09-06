#!TEST smoke set

package smoke

import future.keywords.in

kotlin_class_name := "SmokeSetPolicy"

default set := false
set {
  input.n in { 2,3,5 }
}
