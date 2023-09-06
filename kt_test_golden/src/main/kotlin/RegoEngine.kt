import kotlinx.serialization.json.JsonArray
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonNull
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive

sealed class AuthzValue {
    object ValueUndefined : AuthzValue()
    object ValueNull : AuthzValue()
    data class BOOLEAN(val boolean: Boolean) : AuthzValue()
    data class ValueInt(val number: Int) : AuthzValue()
    data class ValueDouble(val number: Double) : AuthzValue()
    data class ValueString(val string: String) : AuthzValue()
    data class ValueArray(val elements: ArrayList<AuthzValue>) : AuthzValue()
    data class ValueObject(val fields: MutableMap<String, AuthzValue>) : AuthzValue()
    data class ValueSet(val elems: MutableSet<AuthzValue>) : AuthzValue()
}

fun jsonToOpaValue(element: JsonElement): AuthzValue = when (element) {
    is JsonNull -> {
        AuthzValue.ValueNull
    }
    is JsonArray -> {
        var elements = ArrayList<AuthzValue>()
        element.forEach { elements.add(jsonToOpaValue(it)) }
        AuthzValue.ValueArray(elements)
    }
    is JsonObject -> {
        var fields: MutableMap<String, AuthzValue> = mutableMapOf()
        element.entries.forEach { (key, value) -> fields[key] = jsonToOpaValue(value) }
        AuthzValue.ValueObject(fields)
    }
    is JsonPrimitive -> {
        val tmp: JsonPrimitive = element
        if (tmp.isString) {
            AuthzValue.ValueString(tmp.content)
        } else {
            // NOTE(dkorolev): Hack but works. I couldn't find a better way with `kotlinx.serialization`.
            if (tmp.content == "true") {
                AuthzValue.BOOLEAN(true)
            } else if (tmp.content == "false") {
                AuthzValue.BOOLEAN(false)
            } else {
                val v: Int? = tmp.content.toIntOrNull()
                if (v != null) {
                    AuthzValue.ValueInt(v)
                } else {
                    val v2: Double? = tmp.content.toDoubleOrNull()
                    if (v2 != null) {
                        AuthzValue.ValueDouble(v2)
                    } else {
                        // TODO(dkorolev): Proper error handling.
                        println("FAIL 1")
                        AuthzValue.ValueUndefined
                    }
                }
            }
        }
    }
}

fun opaValueToJson(node: AuthzValue): JsonElement = when (node) {
    is AuthzValue.ValueUndefined -> JsonPrimitive("***UNDEFINED***")
    is AuthzValue.ValueNull -> JsonNull
    is AuthzValue.BOOLEAN -> JsonPrimitive(node.boolean)
    is AuthzValue.ValueString -> JsonPrimitive(node.string)
    is AuthzValue.ValueInt -> JsonPrimitive(node.number)
    is AuthzValue.ValueDouble -> JsonPrimitive(node.number)
    is AuthzValue.ValueArray -> {
        val elements: ArrayList<JsonElement> = arrayListOf()
        node.elements.forEach { elements.add(opaValueToJson(it)) }
        JsonArray(elements)
    }
    is AuthzValue.ValueObject -> {
        val fields: MutableMap<String, JsonElement> = mutableMapOf()
        node.fields.forEach { (key, value) -> fields[key] = opaValueToJson(value) }
        JsonObject(fields)
    }
    is AuthzValue.ValueSet -> {
        val elements: ArrayList<JsonElement> = arrayListOf()
        node.elems.forEach { elements.add(opaValueToJson(it)) }
        JsonArray(elements)
    }
}

fun irGetByKey(input: AuthzValue, key: String): AuthzValue = when (input) {
    is AuthzValue.ValueObject -> input.fields.getOrElse(key, { AuthzValue.ValueUndefined })
    is AuthzValue.ValueArray -> {
        val i = key.toInt()
        println("INDIRECT GET: $i")
        if (i >= 0 && i < input.elements.size) {
            input.elements[i]
        } else {
            AuthzValue.ValueUndefined
        }
    }
    else -> AuthzValue.ValueUndefined
}

fun opaAppendToArray(a: AuthzValue, v: AuthzValue) = when (a) {
    is AuthzValue.ValueArray -> a.elements.add(v)
    else -> Unit // TODO(dkorolev): Error!
}

fun opaAddToSet(a: AuthzValue, v: AuthzValue) = when (a) {
    is AuthzValue.ValueSet -> a.elems.add(v)
    else -> Unit // TODO(dkorolev): Error!
}

fun opaScan(locals: MutableMap<Int, AuthzValue>, x: AuthzValue, k: Int, v: Int, inner: () -> RegoBlock) {
    locals[k] = AuthzValue.ValueUndefined
    locals[v] = AuthzValue.ValueUndefined
    when (x) {
        is AuthzValue.ValueArray -> {
            // TODO(dkorolev): Use `x.elements.forEach {}`.
            for (i in 0..(x.elements.size - 1)) {
                locals[k] = AuthzValue.ValueInt(i)
                locals[v] = x.elements[i]
                inner()
            }
            locals[k] = AuthzValue.ValueUndefined
            locals[v] = AuthzValue.ValueUndefined
        }
        is AuthzValue.ValueObject -> {
            for (e in x.fields.iterator()) {
                locals[k] = AuthzValue.ValueString(e.key)
                locals[v] = e.value
                inner()
            }
            locals[k] = AuthzValue.ValueUndefined
            locals[v] = AuthzValue.ValueUndefined
        }
        is AuthzValue.ValueSet -> {
            // TODO(dkorolev): Implement this. Although not sure. Just make it an error for now.
            println("OPASCAN SET")
        }
        else -> {
            // TODO(dkorolev): Exception, or something more major here.
            println("MASSIVE FAILURE!")
        }
    }
}

enum class RegoBlock { INTERRUPTED, COMPLETED }

fun localOrUndefined(locals: MutableMap<Int, AuthzValue>, index: Int): AuthzValue = locals.getOrElse(index, { AuthzValue.ValueUndefined })

// TODO(dkorolev): Rename `localOrUndefined` into some `getValue`.
fun localOrUndefined(unused: MutableMap<Int, AuthzValue>, value: AuthzValue.BOOLEAN): AuthzValue = value
fun localOrUndefined(unused: MutableMap<Int, AuthzValue>, value: String): AuthzValue = AuthzValue.ValueString(value)

// TODO(dkorolev): Rename this, and the value is not a `String`, but it can be an index too!
fun irStringPossiblyFromLocal(unused: MutableMap<Int, AuthzValue>, s: String): String = s
fun irStringPossiblyFromLocal(locals: MutableMap<Int, AuthzValue>, i: Int): String {
    val maybeStringValue = locals[i]
    if (maybeStringValue is AuthzValue.ValueString) {
        return maybeStringValue.string
    } else if (maybeStringValue is AuthzValue.ValueInt) {
        println("WARN 1")
        return maybeStringValue.number.toString()
    } else {
        println("FAIL 2")
        return ""
    }
}

// TODO(dkorolev): Not sure if this is the best way to do this, but I'm learning Kotlin, so why not?!
class OpaBuiltins {
    companion object {
        fun plus(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = localOrUndefined(args, 0)
            val b: AuthzValue = localOrUndefined(args, 1)
            if (a is AuthzValue.ValueInt && b is AuthzValue.ValueInt) {
                return AuthzValue.ValueInt(a.number + b.number)
            } else {
                return AuthzValue.ValueUndefined
            }
        }

        fun lt(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = localOrUndefined(args, 0)
            val b: AuthzValue = localOrUndefined(args, 1)
            if (a is AuthzValue.ValueInt && b is AuthzValue.ValueInt) {
                return AuthzValue.BOOLEAN(a.number < b.number)
            } else {
                return AuthzValue.ValueUndefined
            }
        }

        fun gt(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = localOrUndefined(args, 0)
            val b: AuthzValue = localOrUndefined(args, 1)
            if (a is AuthzValue.ValueInt && b is AuthzValue.ValueInt) {
                return AuthzValue.BOOLEAN(a.number > b.number)
            } else {
                return AuthzValue.ValueUndefined
            }
        }

        fun lte(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = localOrUndefined(args, 0)
            val b: AuthzValue = localOrUndefined(args, 1)
            if (a is AuthzValue.ValueInt && b is AuthzValue.ValueInt) {
                return AuthzValue.BOOLEAN(a.number <= b.number)
            } else {
                return AuthzValue.ValueUndefined
            }
        }

        fun gte(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = localOrUndefined(args, 0)
            val b: AuthzValue = localOrUndefined(args, 1)
            if (a is AuthzValue.ValueInt && b is AuthzValue.ValueInt) {
                return AuthzValue.BOOLEAN(a.number >= b.number)
            } else {
                return AuthzValue.ValueUndefined
            }
        }

        fun minus(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = localOrUndefined(args, 0)
            val b: AuthzValue = localOrUndefined(args, 1)
            if (a is AuthzValue.ValueInt && b is AuthzValue.ValueInt) {
                return AuthzValue.ValueInt(a.number - b.number)
            } else {
                return AuthzValue.ValueUndefined
            }
        }

        fun mul(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = localOrUndefined(args, 0)
            val b: AuthzValue = localOrUndefined(args, 1)
            if (a is AuthzValue.ValueInt && b is AuthzValue.ValueInt) {
                return AuthzValue.ValueInt(a.number * b.number)
            } else {
                return AuthzValue.ValueUndefined
            }
        }

        fun rem(args: MutableMap<Int, AuthzValue>): AuthzValue {
            val a: AuthzValue = localOrUndefined(args, 0)
            val b: AuthzValue = localOrUndefined(args, 1)
            if (a is AuthzValue.ValueInt && b is AuthzValue.ValueInt) {
                if (b.number > 0) {
                    return AuthzValue.ValueInt(a.number % b.number)
                } else {
                    return AuthzValue.ValueUndefined
                }
            } else {
                return AuthzValue.ValueUndefined
            }
        }
        fun split(args: MutableMap<Int, AuthzValue>): AuthzValue {
            val a: AuthzValue = localOrUndefined(args, 0)
            val b: AuthzValue = localOrUndefined(args, 1)
            if (a is AuthzValue.ValueString && b is AuthzValue.ValueString) {
                val r = ArrayList<AuthzValue>()
                // TODO(dkorolev): I have no idea whether this is correct for multi-char `delim` string.
                a.string.split(b.string).forEach { r.add(AuthzValue.ValueString(it)) }
                return AuthzValue.ValueArray(r)
            } else {
                return AuthzValue.ValueUndefined
            }
        }
    }
    class numbers {
        companion object {
            fun range(args: MutableMap<Int, AuthzValue>): AuthzValue {
                val a: AuthzValue = localOrUndefined(args, 0)
                val b: AuthzValue = localOrUndefined(args, 1)
                if (a is AuthzValue.ValueInt && b is AuthzValue.ValueInt) {
                    // TODO(dkorolev): This constraint is obviously artificial.
                    if (b.number >= a.number) {
                        val r = ArrayList<AuthzValue>()
                        for (i in a.number..b.number) {
                            r.add(AuthzValue.ValueInt(i))
                        }
                        return AuthzValue.ValueArray(r)
                    } else {
                        return AuthzValue.ValueUndefined
                    }
                } else {
                    return AuthzValue.ValueUndefined
                }
            }
        }
    }
    class internal {
        companion object {
            fun member_2(args: MutableMap<Int, AuthzValue>): AuthzValue {
                val a: AuthzValue = localOrUndefined(args, 0)
                val b: AuthzValue = localOrUndefined(args, 1)
                if (b is AuthzValue.ValueSet) {
                    return AuthzValue.BOOLEAN(b.elems.contains(a))
                } else {
                    // TODO(dkorolev): This should be a proper error!
                    return AuthzValue.ValueUndefined
                }
            }
        }
    }
}
