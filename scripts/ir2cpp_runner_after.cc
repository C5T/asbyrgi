int main() {
  std::string test_input;
  JSONValue test_data_that_is_empty = JSONObject();
  while (std::getline(std::cin, test_input)) {
    std::cout << AsJSON(policy(ParseJSONUniversally(test_input), test_data_that_is_empty).pack()) << std::endl;
  }
}
