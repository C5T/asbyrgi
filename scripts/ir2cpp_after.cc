
int main() {
  Policy policy;
  PopulatePolicy(policy);
  policy_singleton = &policy;

  for (size_t i = 0; i < policy.function_bodies.size(); ++i) {
    Output output(std::cout, Output::ForFunction(), i, policy.functions[i]);
    policy.function_bodies[i](policy).output(output);
  }

  {
    Output output(std::cout, Output::ForPlan());
    policy.plans["main"](policy).output(output);
  }
}
