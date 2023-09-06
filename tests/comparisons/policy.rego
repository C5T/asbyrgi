#!TEST smoke comparisons

package smoke

kotlin_export := "ComparisonsPolicy"

default comparisons = false

comparisons {
  input.n > 3
  input.n < 7
}
