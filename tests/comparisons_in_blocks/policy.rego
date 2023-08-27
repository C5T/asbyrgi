#!TEST smoke comparisons_in_blocks

package smoke

default comparisons_in_blocks := false

# NOTE(dkorolev): These `default`-s are evidently essential!
default cond1 := false
default cond2 := false

comparisons_in_blocks {
  cond1
  cond2
}

cond1 {
  input.n >= 10
}

cond2 {
  input.n <= 10
}
