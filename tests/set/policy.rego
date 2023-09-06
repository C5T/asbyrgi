#!TEST smoke set

package smoke

import future.keywords.in

kotlin_export := "SmokeSetPolicy"

default set := false
set {
  input.n in { 2,3,5 }
}
