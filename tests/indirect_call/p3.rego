#!TEST the answer

package the

kotlin_class_name := "IndirectCallP3Policy"

f("add") = input.a + input.b
f("mul") = input.a * input.b
f("range") = numbers.range(input.a, input.b)
answer := f(input.f)
