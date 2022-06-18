int main() {
  Policy policy;
  PopulatePolicy(policy);
  policy_singleton = &policy;

#if 0
  for (size_t i = 0; i < policy.function_bodies.size(); ++i) {
    std::cout << "// function " << i << ":\n";
    Locals locals;
    for (size_t j = 0u; j < policy.functions[i].size(); ++j) {
      locals[OPALocalWrapper(policy.functions[i][j])].MarkAsFunctionArgument();
    }
    policy.function_bodies[i](policy).analyze(
        locals,
        [&locals]() { std::cout << locals << locals << std::endl; },
        [&locals]() {
          std::cout << "// DONE?\n" << locals << std::endl;
          throw std::logic_error("Internal invariant failed.");
        });
  }

  {
    Locals locals;
    locals[OPALocalWrapper(0)].MarkAsFunctionArgument();
    locals[OPALocalWrapper(1)].MarkAsFunctionArgument();
    policy.plans["main"](policy).analyze(
        locals,
        [&locals]() { std::cout << locals << locals << std::endl; },
        [&locals]() {
          std::cout << "// DONE?\n" << locals << std::endl;
          throw std::logic_error("Internal invariant failed.");
        });
  }
#endif

#if 0
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
#endif

  for (size_t i = 0; i < policy.function_bodies.size(); ++i) {
    Output output(std::cout, Output::ForFunction(), i, policy.functions[i]);
    policy.function_bodies[i](policy).output(output);
  }

  {
    Output output(std::cout, Output::ForPlan());
    policy.plans["main"](policy).output(output);
  }

  /*
  std::cout << "result_set_t policy(value_t input, value_t data) {\n";
  std::cout << "locals_t locals; locals[0] = input; locals[1] = data; result_set_t result;\n";
  policy.plans["main"](policy).dump([]() {});
  std::cout << "return result; }\n";
  */
}
