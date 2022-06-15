int main() {
  OPAValue input;
  OPAValue data;
  OPAResultSet result;

  TranspilationContext ctx(TranspilationContext::CreateNew(), input, data, result);
  rego_policy(ctx);

  for (size_t i = 0; i < ctx.opa_function_bodies.size(); ++i) {
    std::cout << "value_t function_" << i << "(";
    for (size_t j = 0u; j < ctx.opa_functions[i].size(); ++j) {
      if (j) {
        std::cout << ", ";
      }
      std::cout << "value_t p" << j + 1u;
    }
    std::cout << ") { locals_t locals; ";
    for (size_t j = 0u; j < ctx.opa_functions[i].size(); ++j) {
      std::cout << "locals[" << ctx.opa_functions[i][j] << "] = p" << j << ";\n";
    }
    std::cout << "value_t retval; \n";
    ctx.opa_function_bodies[i](ctx)([]() {});
    std::cout << "return retval; }\n";
  }

  std::cout << "result_set_t policy(value_t input, value_t data) {\n";
  std::cout << "locals_t locals; locals[0] = input; locals[1] = data; result_set_t result;\n";
  ctx.plans["main"](ctx)([]() {});
  std::cout << "return result; }\n";
}

