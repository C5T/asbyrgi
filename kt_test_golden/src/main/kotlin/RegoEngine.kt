// DO NOT EDIT!
//
// This file comes from Asbyrgi. It may and will evolve.
//
// If this file is checked into your repository, it is only there to simplify running & debugging the code.
// It is safe to assume it will eventually be overwritten by a newer verison.
// In fact, it may even be overwritten by a git hook or a Github action, so, please, let it live as is.

import kotlin.system.exitProcess
import kotlinx.serialization.json.JsonArray
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonNull
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive

// TODO(dkorolev): Make the fields internal, expose getters/setters.
sealed class AuthzValue {
    object UNDEFINED : AuthzValue()
    object NULL : AuthzValue()
    data class BOOLEAN(val boolean: Boolean) : AuthzValue()
    data class INT(val number: Int) : AuthzValue()
    data class DOUBLE(val number: Double) : AuthzValue()
    data class STRING(val string: String) : AuthzValue()
    data class ARRAY(val elements: ArrayList<AuthzValue>) : AuthzValue()
    data class OBJECT(val fields: MutableMap<String, AuthzValue>) : AuthzValue()
    data class SET(val elems: MutableSet<AuthzValue>) : AuthzValue()
}

// TODO(dkorolev): Add a custom type for per-plan output, i.e. the `ResultsSet`.

fun jsonToAuthzValue(element: JsonElement): AuthzValue = when (element) {
    is JsonNull -> {
        AuthzValue.NULL
    }
    is JsonArray -> {
        var elements = ArrayList<AuthzValue>()
        element.forEach { elements.add(jsonToAuthzValue(it)) }
        AuthzValue.ARRAY(elements)
    }
    is JsonObject -> {
        var fields: MutableMap<String, AuthzValue> = mutableMapOf()
        element.entries.forEach { (key, value) -> fields[key] = jsonToAuthzValue(value) }
        AuthzValue.OBJECT(fields)
    }
    is JsonPrimitive -> {
        val tmp: JsonPrimitive = element
        if (tmp.isString) {
            AuthzValue.STRING(tmp.content)
        } else {
            // NOTE(dkorolev): Hack but works. I couldn't find a better way with `kotlinx.serialization`.
            if (tmp.content == "true") {
                AuthzValue.BOOLEAN(true)
            } else if (tmp.content == "false") {
                AuthzValue.BOOLEAN(false)
            } else {
                val v: Int? = tmp.content.toIntOrNull()
                if (v != null) {
                    AuthzValue.INT(v)
                } else {
                    val v2: Double? = tmp.content.toDoubleOrNull()
                    if (v2 != null) {
                        AuthzValue.DOUBLE(v2)
                    } else {
                        // TODO(dkorolev): Proper error handling.
                        println("""JSON primitives should be bool-s, string-s, or numbers, seeing ${tmp.content}.""")
                        exitProcess(1)
                        AuthzValue.UNDEFINED
                    }
                }
            }
        }
    }
}

fun authzValueToJson(node: AuthzValue): JsonElement = when (node) {
    is AuthzValue.UNDEFINED -> JsonPrimitive("***UNDEFINED***")
    is AuthzValue.NULL -> JsonNull
    is AuthzValue.BOOLEAN -> JsonPrimitive(node.boolean)
    is AuthzValue.STRING -> JsonPrimitive(node.string)
    is AuthzValue.INT -> JsonPrimitive(node.number)
    is AuthzValue.DOUBLE -> JsonPrimitive(node.number)
    is AuthzValue.ARRAY -> {
        val elements: ArrayList<JsonElement> = arrayListOf()
        node.elements.forEach { elements.add(authzValueToJson(it)) }
        JsonArray(elements)
    }
    is AuthzValue.OBJECT -> {
        val fields: MutableMap<String, JsonElement> = mutableMapOf()
        node.fields.forEach { (key, value) -> fields[key] = authzValueToJson(value) }
        JsonObject(fields)
    }
    is AuthzValue.SET -> {
        val elements: ArrayList<JsonElement> = arrayListOf()
        node.elems.forEach { elements.add(authzValueToJson(it)) }
        JsonArray(elements)
    }
}

fun regoDotStmt(input: AuthzValue, key: String): AuthzValue = when (input) {
    is AuthzValue.OBJECT -> input.fields.getOrElse(key, { AuthzValue.UNDEFINED })
    is AuthzValue.ARRAY -> {
        val i = key.toInt()
        if (i >= 0 && i < input.elements.size) {
            input.elements[i]
        } else {
            // TODO(dkorolev): Is this acceptable?
            AuthzValue.UNDEFINED
        }
    }
    // TODO(dkorolev): Is this acceptable?
    else -> AuthzValue.UNDEFINED
}

fun regoArrayAppendStmt(a: AuthzValue, v: AuthzValue) = when (a) {
    is AuthzValue.ARRAY -> a.elements.add(v)
    else -> Unit // TODO(dkorolev): Error!
}

fun regoSetAddStmt(a: AuthzValue, v: AuthzValue) = when (a) {
    is AuthzValue.SET -> a.elems.add(v)
    else -> Unit // TODO(dkorolev): Error!
}

fun regoScanStmt(locals: MutableMap<Int, AuthzValue>, x: AuthzValue, k: Int, v: Int, inner: () -> RegoBlockResult) {
    locals[k] = AuthzValue.UNDEFINED
    locals[v] = AuthzValue.UNDEFINED
    when (x) {
        is AuthzValue.ARRAY -> {
            // TODO(dkorolev): Use `x.elements.forEach {}`.
            for (i in 0..(x.elements.size - 1)) {
                locals[k] = AuthzValue.INT(i)
                locals[v] = x.elements[i]
                inner()
            }
            locals[k] = AuthzValue.UNDEFINED
            locals[v] = AuthzValue.UNDEFINED
        }
        is AuthzValue.OBJECT -> {
            for (e in x.fields.iterator()) {
                locals[k] = AuthzValue.STRING(e.key)
                locals[v] = e.value
                inner()
            }
            locals[k] = AuthzValue.UNDEFINED
            locals[v] = AuthzValue.UNDEFINED
        }
        is AuthzValue.SET -> {
            // TODO(dkorolev): Implement this. Although not sure. Just make it an error for now.
            println("OPASCAN SET")
        }
        else -> {
            // TODO(dkorolev): Exception, or something more major here.
            println("MASSIVE FAILURE!")
        }
    }
}

enum class RegoBlockResult { INTERRUPTED, COMPLETED }

fun regoVal(locals: MutableMap<Int, AuthzValue>, index: Int): AuthzValue = locals.getOrElse(index, { AuthzValue.UNDEFINED })

fun regoVal(unused: MutableMap<Int, AuthzValue>, value: AuthzValue.BOOLEAN): AuthzValue = value
fun regoVal(unused: MutableMap<Int, AuthzValue>, value: String): AuthzValue = AuthzValue.STRING(value)

// TODO(dkorolev): A special type for indexes, as indexing arrays as string is sort of dated in 2023.
fun regoStringWrapper(unused: MutableMap<Int, AuthzValue>, s: String): String = s
fun regoStringWrapper(locals: MutableMap<Int, AuthzValue>, i: Int): String {
    val maybeStringValue = locals[i]
    if (maybeStringValue is AuthzValue.STRING) {
        return maybeStringValue.string
    } else if (maybeStringValue is AuthzValue.INT) {
        return maybeStringValue.number.toString()
    } else {
        println("We are not really accepting non-string and non-int indexes.")
        exitProcess(1)
        return ""
    }
}

// TODO(dkorolev): Not sure if this is the best way to do this, but I'm learning Kotlin, so why not?!
class RegoBuiltins {
    companion object {
        fun plus(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = regoVal(args, 0)
            val b: AuthzValue = regoVal(args, 1)
            if (a is AuthzValue.INT && b is AuthzValue.INT) {
                return AuthzValue.INT(a.number + b.number)
            } else {
                return AuthzValue.UNDEFINED
            }
        }

        fun lt(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = regoVal(args, 0)
            val b: AuthzValue = regoVal(args, 1)
            if (a is AuthzValue.INT && b is AuthzValue.INT) {
                return AuthzValue.BOOLEAN(a.number < b.number)
            } else {
                return AuthzValue.UNDEFINED
            }
        }

        fun gt(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = regoVal(args, 0)
            val b: AuthzValue = regoVal(args, 1)
            if (a is AuthzValue.INT && b is AuthzValue.INT) {
                return AuthzValue.BOOLEAN(a.number > b.number)
            } else {
                return AuthzValue.UNDEFINED
            }
        }

        fun lte(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = regoVal(args, 0)
            val b: AuthzValue = regoVal(args, 1)
            if (a is AuthzValue.INT && b is AuthzValue.INT) {
                return AuthzValue.BOOLEAN(a.number <= b.number)
            } else {
                return AuthzValue.UNDEFINED
            }
        }

        fun gte(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = regoVal(args, 0)
            val b: AuthzValue = regoVal(args, 1)
            if (a is AuthzValue.INT && b is AuthzValue.INT) {
                return AuthzValue.BOOLEAN(a.number >= b.number)
            } else {
                return AuthzValue.UNDEFINED
            }
        }

        fun minus(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = regoVal(args, 0)
            val b: AuthzValue = regoVal(args, 1)
            if (a is AuthzValue.INT && b is AuthzValue.INT) {
                return AuthzValue.INT(a.number - b.number)
            } else {
                return AuthzValue.UNDEFINED
            }
        }

        fun mul(args: MutableMap<Int, AuthzValue>): AuthzValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: AuthzValue = regoVal(args, 0)
            val b: AuthzValue = regoVal(args, 1)
            if (a is AuthzValue.INT && b is AuthzValue.INT) {
                return AuthzValue.INT(a.number * b.number)
            } else {
                return AuthzValue.UNDEFINED
            }
        }

        fun rem(args: MutableMap<Int, AuthzValue>): AuthzValue {
            val a: AuthzValue = regoVal(args, 0)
            val b: AuthzValue = regoVal(args, 1)
            if (a is AuthzValue.INT && b is AuthzValue.INT) {
                if (b.number > 0) {
                    return AuthzValue.INT(a.number % b.number)
                } else {
                    return AuthzValue.UNDEFINED
                }
            } else {
                return AuthzValue.UNDEFINED
            }
        }
        fun split(args: MutableMap<Int, AuthzValue>): AuthzValue {
            val a: AuthzValue = regoVal(args, 0)
            val b: AuthzValue = regoVal(args, 1)
            if (a is AuthzValue.STRING && b is AuthzValue.STRING) {
                val r = ArrayList<AuthzValue>()
                // TODO(dkorolev): I have no idea whether this is correct for multi-char `delim` string.
                a.string.split(b.string).forEach { r.add(AuthzValue.STRING(it)) }
                return AuthzValue.ARRAY(r)
            } else {
                return AuthzValue.UNDEFINED
            }
        }
    }
    class numbers {
        companion object {
            fun range(args: MutableMap<Int, AuthzValue>): AuthzValue {
                val a: AuthzValue = regoVal(args, 0)
                val b: AuthzValue = regoVal(args, 1)
                if (a is AuthzValue.INT && b is AuthzValue.INT) {
                    // TODO(dkorolev): This constraint is obviously artificial.
                    if (b.number >= a.number) {
                        val r = ArrayList<AuthzValue>()
                        for (i in a.number..b.number) {
                            r.add(AuthzValue.INT(i))
                        }
                        return AuthzValue.ARRAY(r)
                    } else {
                        return AuthzValue.UNDEFINED
                    }
                } else {
                    return AuthzValue.UNDEFINED
                }
            }
        }
    }
    class internal {
        companion object {
            fun member_2(args: MutableMap<Int, AuthzValue>): AuthzValue {
                // NOTE(dkorolev): This `member_2` OPA builtin checks whether a value is present in a set.
                val a: AuthzValue = regoVal(args, 0)
                val b: AuthzValue = regoVal(args, 1)
                if (b is AuthzValue.SET) {
                    return AuthzValue.BOOLEAN(b.elems.contains(a))
                } else {
                    // TODO(dkorolev): This should be a proper error!
                    return AuthzValue.UNDEFINED
                }
            }
        }
    }
}
