#!TEST use not_yet_dp

package use

kotlin_export := "DataProviderSmokeTestPolicyV0"

default input_x := "unset"
input_x = input.x

default input_y := "unset"
input_y = input.y

default input_z := "unset"
input_z = input.z

not_yet_dp := concat("", [
    "input:{x:\"",
    input_x,
    "\",y:\"",
    input_y,
    "\",z:\"",
    input_z,
    "\"}"
  ])
