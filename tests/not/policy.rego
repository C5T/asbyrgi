#!TEST smoke odd_divisible_by_three

package smoke

default odd_divisible_by_three := false

odd_divisible_by_three {
  input.n % 3 == 0
  not even
}

# NOTE(dkorolev): This `default` is evidently important!
default even := false
even {
  input.n % 2 == 0
}
