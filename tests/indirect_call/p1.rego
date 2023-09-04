#!TEST the answer

package the

kotlin_class_name := "IndirectCallP1Policy"

f("add") = x { x := input.a + input.b }
f("mul") = x { x := input.a * input.b }
f("range") = x { x := numbers.range(input.a, input.b) }
answer := f(input.f)
