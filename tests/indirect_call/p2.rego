#!TEST the answer

package the
default answer = null
answer = input.a + input.b { input.f == "add" }
answer = input.a * input.b { input.f == "mul" }
answer = numbers.range(input.a, input.b) { input.f == "range" }
