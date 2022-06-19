#include <cstdarg>
#include <iostream>
#include <map>

// TODO: Not a relative path, of course.
#include "../current/blocks/json/json.h"

using namespace current::json;

struct value_t final {
  JSONValue opa_value;  // TODO: OPA's `undefined` is not the same as `null`, of course.

  value_t() : opa_value(JSONNull()) {}
  value_t(JSONValue opa_value) : opa_value(std::move(opa_value)) {}

  void ResetToUndefined() { opa_value = JSONNull(); }

  bool IsUndefined() const { return Exists<JSONNull>(opa_value); }

  bool IsString(char const* desired) const {
    return Exists<JSONString>(opa_value) && Value<JSONString>(opa_value).string == desired;
  }

  void MakeObject() { opa_value = JSONObject(); }

  void MakeNull() {
    opa_value = JSONNull();  // TODO: See above.
  }

  void SetBoolean(bool value) { opa_value = JSONBoolean(value); }

  void SetString(char const* value) { opa_value = JSONString(value); }

  void SetNumberFromString(char const* value) { opa_value = JSONNumber(current::FromString<double>(value)); }

  value_t GetByKey(char const* key) {
    return Exists<JSONObject>(opa_value) ? Value<JSONObject>(opa_value)[key] : value_t();
  }

  void SetValueForKey(char const* key, value_t const& value) {
    if (Exists<JSONObject>(opa_value)) {
      Value<JSONObject>(opa_value).push_back(key, value.opa_value);
    }
  }
};

inline value_t ResetToUndefined() { return value_t(); }
inline value_t MakeNull() {
  value_t res;
  res.MakeNull();
  return res;
}
inline value_t MakeObject() {
  value_t res;
  res.MakeObject();
  return res;
}
inline value_t SetString(char const* s) {
  value_t res;
  res.SetString(s);
  return res;
}
inline value_t SetBoolean(bool b) {
  value_t res;
  res.SetBoolean(b);
  return res;
}
inline value_t SetNumberFromString(char const* s) {
  value_t res;
  res.SetNumberFromString(s);
  return res;
}

inline value_t opa_plus(value_t const& a, value_t const& b) {
  if (Exists<JSONNumber>(a.opa_value) && Exists<JSONNumber>(b.opa_value)) {
    return value_t(JSONNumber(Value<JSONNumber>(a.opa_value).number + Value<JSONNumber>(b.opa_value).number));
  } else {
    return value_t();
  }
}

inline value_t opa_minus(value_t const& a, value_t const& b) {
  if (Exists<JSONNumber>(a.opa_value) && Exists<JSONNumber>(b.opa_value)) {
    return value_t(JSONNumber(Value<JSONNumber>(a.opa_value).number - Value<JSONNumber>(b.opa_value).number));
  } else {
    return value_t();
  }
}

inline value_t opa_mul(value_t const& a, value_t const& b) {
  if (Exists<JSONNumber>(a.opa_value) && Exists<JSONNumber>(b.opa_value)) {
    return value_t(JSONNumber(Value<JSONNumber>(a.opa_value).number * Value<JSONNumber>(b.opa_value).number));
  } else {
    return value_t();
  }
}

inline value_t opa_range(value_t const& a, value_t const& b) {
  if (Exists<JSONNumber>(a.opa_value) && Exists<JSONNumber>(b.opa_value)) {
    JSONArray array;
    int const va = static_cast<int>(Value<JSONNumber>(a.opa_value).number);
    int const vb = static_cast<int>(Value<JSONNumber>(b.opa_value).number);
    for (int i = va; i <= vb; ++i) {
      array.push_back(JSONNumber(i));
    }
    return value_t(array);
  } else {
    return value_t();
  }
}

struct result_set_t final {
  std::vector<value_t> result_set;
  void AddToResultSet(value_t value) { result_set.push_back(std::move(value)); }
  JSONValue pack() const {
    if (result_set.empty()) {
      return JSONNull();
    } else if (result_set.size() == 1u) {
      JSONValue const& v = result_set.front().opa_value;
      if (Exists<JSONObject>(v)) {
        return Value<JSONObject>(v)["result"];
      } else {
        throw std::logic_error("The response from the policy should be an object for now.");
      }
    } else {
      JSONArray array;
      for (value_t const& value : result_set) {
        JSONValue const& v = value.opa_value;
        if (Exists<JSONObject>(v)) {
          array.push_back(Value<JSONObject>(v)["result"]);
        } else {
          throw std::logic_error("The response from the policy should be an object for now.");
        }
      }
      return array;
    }
  }
};
