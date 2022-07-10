#!TEST rbac allow

# An example case from https://www.styra.com/blog/4-best-practices-for-microservices-authorization/

package rbac

user_roles := {
  "alice": ["eng", "web"],
  "bob": ["hr"]  # NOTE: "hr"` without `[]` would be a good example of strong typing.
}

role_permissions := {
 "eng": [{"action": "read", "object": "server123"}],
 "web": [{"action": "read", "object": "server123"},
         {"action": "write", "object": "server123"}],
 "hr": [{"action": "read", "object": "database456"}],
}

default allow = false
allow {
  roles := user_roles[input.user]
  r := roles[_]
  permissions := role_permissions[r]
  p := permissions[_]
  p == {"action": input.action, "object": input.object}
}
