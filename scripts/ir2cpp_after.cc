
int main() {
  Policy policy;
  PopulatePolicy(policy);
  policy_singleton = &policy;

  // TODO(dkorolev): This would only work for strings that are valid C++ identifiers, of course.
  for (std::string const& s : policy.strings) {
    std::cout << "struct rego_string_" << s << " final{constexpr static char const* s = \"" << s
              << "\"; template <class T> static decltype(std::declval<T>()." << s
              << ") GetValueByKeyFrom(T&& x) { return std::forward<T>(x)." << s
              << "; } static OPAValue GetValueByKeyFrom(OPAValue const& object) { return object.DoGetValueByKey(\"" << s
              << "\");}};";
  }

  for (size_t i = 0; i < policy.function_bodies.size(); ++i) {
    Output output(std::cout, Output::ForFunction(), i, policy.functions[i]);
    policy.function_bodies[i](policy).output(output);
  }

  {
    Output output(std::cout, Output::ForPlan());
    policy.plans["main"](policy).output(output);
  }
}
