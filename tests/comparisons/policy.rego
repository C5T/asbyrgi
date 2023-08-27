#!TEST smoke comparisons

package smoke

default comparisons = false

comparisons {
  input.n > 3
  input.n < 7
}
