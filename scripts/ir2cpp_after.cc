int main() {
  Policy policy;
  PopulatePolicy(policy);
  policy_singleton = &policy;

#if 0
  {
    Locals locals;
    policy.plans["main"](policy).analyze(
        locals,
        [&locals]() { std::cout << locals << locals << std::endl; },
        [&locals]() {
          std::cout << "// DONE?\n" << locals << std::endl;
          throw std::logic_error("Internal invariant failed.");
        });
  }
#endif

  for (size_t i = 0; i < policy.function_bodies.size(); ++i) {
    std::cout << "value_t function_" << i << "(";
    for (size_t j = 0u; j < policy.functions[i].size(); ++j) {
      if (j) {
        std::cout << ", ";
      }
      std::cout << "value_t p" << j + 1u;
    }
    std::cout << ") { locals_t locals; ";
    for (size_t j = 0u; j < policy.functions[i].size(); ++j) {
      std::cout << "locals[" << policy.functions[i][j] << "] = p" << j + 1u << ";\n";
    }
    std::cout << "value_t retval; \n";
    policy.function_bodies[i](policy).dump([]() {});
    std::cout << "return retval; }\n";
  }

  std::cout << "result_set_t policy(value_t input, value_t data) {\n";
  std::cout << "locals_t locals; locals[0] = input; locals[1] = data; result_set_t result;\n";
  policy.plans["main"](policy).dump([]() {});
  std::cout << "return result; }\n";
}
