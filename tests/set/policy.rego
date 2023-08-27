#!TEST smoke set

package smoke

import future.keywords.in

default set := false
set {
  input.n in { 2,3,5 }
}
