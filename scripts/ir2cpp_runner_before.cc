#include <cstdarg>
#include <iostream>
#include <map>

// TODO: Not a relative path, of course.
#include "../current/blocks/json/json.h"

using namespace current::json;

using OPAString = Optional<std::string>;
using OPANumber = Optional<double>;
using OPABoolean = Optional<bool>;

class OPAValue final {
  // TODO(dkorolev): Make it `private`, once the builtins are cleaned up.
 public:
  JSONValue opa_value;  // TODO: OPA's `undefined` is not the same as `null`, of course.

 public:
  OPAValue() : opa_value(JSONNull()) {}
  OPAValue(JSONValue opa_value) : opa_value(std::move(opa_value)) {}
  OPAValue(OPAString const& s) {
    if (Exists(s)) {
      opa_value = JSONString(Value(s));
    }
  }
  OPAValue(OPANumber const& v) {
    if (Exists(v)) {
      opa_value = JSONNumber(Value(v));
    }
  }
  OPAValue(OPABoolean const& b) {
    if (Exists(b)) {
      opa_value = JSONBoolean(Value(b));
    }
  }

  void DoResetToUndefined() { opa_value = JSONNull(); }
  bool DoIsUndefined() const { return Exists<JSONNull>(opa_value); }

  bool DoIsStringEqualTo(char const* desired) const {
    return Exists<JSONString>(opa_value) && Value<JSONString>(opa_value).string == desired;
  }

  void DoMakeObject() { opa_value = JSONObject(); }

  void DoMakeNull() {
    opa_value = JSONNull();  // TODO: See above re. `null` vs. `undefined`.
  }

  OPAValue DoGetValueByKey(char const* key) const {
    return Exists<JSONObject>(opa_value) ? Value<JSONObject>(opa_value)[key] : OPAValue();
  }

  void DoSetValueForKey(char const* key, OPAValue const& value) {
    if (Exists<JSONObject>(opa_value)) {
      Value<JSONObject>(opa_value).push_back(key, value.opa_value);
    }
  }
};

inline void ResetToUndefined(OPAValue& value) { value.DoResetToUndefined(); }
inline void ResetToUndefined(Optional<std::string>& value) { value = nullptr; }

inline void MakeNull(OPAValue& value) { value.DoMakeNull(); }
inline void MakeObject(OPAValue& value) { value.DoMakeObject(); }

inline bool IsUndefined(OPAValue const& value) { return value.DoIsUndefined(); }
inline bool IsUndefined(Optional<std::string> const& value) { return Exists(value); }

inline bool IsStringEqualTo(OPAValue const& value, char const* s) { return value.DoIsStringEqualTo(s); }

inline bool IsStringEqualTo(OPAString const& value, char const* s) { return Exists(value) && Value(value) == s; }

// TODO(dkorolev): Should return a custom type that can be assigned to a string "variable" too!
inline OPAValue Undefined() {
  OPAValue undefined;
  return undefined;
}
inline OPAValue Object() {
  OPAValue object;
  object.DoMakeObject();
  return object;
}
inline JSONValue Null() {
  static JSONValue null = JSONNull();  // TODO(dkorolev): This `null` is the same as `undefined`.
  return null;
}

// TODO(dkorolev): Use a proper `template` for user-defined types here.
inline OPAValue GetValueByKey(OPAValue const& object, char const* key) { return object.DoGetValueByKey(key); }

inline void SetValueForKey(OPAValue& target, char const* key, OPAValue const& value) {
  target.DoSetValueForKey(key, value);
}

inline OPANumber NumberFromString(char const* s) { return OPANumber(current::FromString<double>(s)); }

inline OPAValue opa_plus(OPAValue const& a, OPAValue const& b) {
  if (Exists<JSONNumber>(a.opa_value) && Exists<JSONNumber>(b.opa_value)) {
    return OPAValue(JSONNumber(Value<JSONNumber>(a.opa_value).number + Value<JSONNumber>(b.opa_value).number));
  } else {
    return OPAValue();
  }
}

inline OPAValue opa_minus(OPAValue const& a, OPAValue const& b) {
  // TODO(dkorolev): Only work on numbers!
  if (Exists<JSONNumber>(a.opa_value) && Exists<JSONNumber>(b.opa_value)) {
    return OPAValue(JSONNumber(Value<JSONNumber>(a.opa_value).number - Value<JSONNumber>(b.opa_value).number));
  } else {
    return OPAValue();
  }
}

inline OPAValue opa_mul(OPAValue const& a, OPAValue const& b) {
  if (Exists<JSONNumber>(a.opa_value) && Exists<JSONNumber>(b.opa_value)) {
    return OPAValue(JSONNumber(Value<JSONNumber>(a.opa_value).number * Value<JSONNumber>(b.opa_value).number));
  } else {
    return OPAValue();
  }
}

inline OPAValue opa_range(OPAValue const& a, OPAValue const& b) {
  if (Exists<JSONNumber>(a.opa_value) && Exists<JSONNumber>(b.opa_value)) {
    JSONArray array;
    int const va = static_cast<int>(Value<JSONNumber>(a.opa_value).number);
    int const vb = static_cast<int>(Value<JSONNumber>(b.opa_value).number);
    for (int i = va; i <= vb; ++i) {
      array.push_back(JSONNumber(i));
    }
    return OPAValue(array);
  } else {
    return OPAValue();
  }
}

struct OPAResult final {
  std::vector<OPAValue> result_set;
  void AddToResultSet(OPAValue value) { result_set.push_back(std::move(value)); }
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
      for (OPAValue const& value : result_set) {
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
