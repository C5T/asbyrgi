#!TEST use data_provider_output

package use

kotlin_export := "DataProviderSmokeTestPolicyV0"

default input_x := "unset"
input_x = input.x

default input_y := "unset"
input_y = input.y

default input_z := "unset"
input_z = input.z

default data_a := "unset"
data_a = data.a

default data_p_q := "unset"
data_p_q = data.p.q

data_provider_output := concat("", [
    "input:{x:\"",
    input_x,
    "\",y:\"",
    input_y,
    "\",z:\"",
    input_z,
    "\"},data:{a:\"",
    data_a,
    "\",p.q:\"",
    data_p_q,
    "\"}"
  ])
