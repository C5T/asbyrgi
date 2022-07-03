
inline bool IsValidIdentifier(std::string const& s) {
  if (s.empty()) {
    return false;
  }
  if (!(s[0] == '_' || std::isalpha(s[0]))) {
    return false;
  }
  for (char c : s) {
    if (!(c == '_' || std::isalnum(c))) {
      return false;
    }
  }
  return true;
}

int main() {
  Policy policy;
  PopulatePolicy(policy);
  policy_singleton = &policy;

  // TODO(dkorolev): Escape the string once `current` is in the picture.
  for (size_t i = 0u; i < policy.strings.size(); ++i) {
    std::string const& s = policy.strings[i];
    std::cout << "struct s" << i << " final{constexpr static char const* s = \"" << s << "\";";
    if (IsValidIdentifier(s)) {
      std::cout << "template <class T> static decltype(std::declval<T>()." << s << ") GetValueByKeyFrom(T&& x)"
                << "{ return std::forward<T>(x)." << s << "; }";
    }
    std::cout << "static OPAValue GetValueByKeyFrom(OPAValue const& object)"
              << "{ return object.DoGetValueByKey(\"" << s << "\");}";
    std::cout << "};";
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
