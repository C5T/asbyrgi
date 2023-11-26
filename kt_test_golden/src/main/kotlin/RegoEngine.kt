// DO NOT EDIT!
//
// This file comes from Asbyrgi. It may and will evolve.
//
// If this file is checked into your repository, it is only there to simplify running & debugging the code locally.
// It is safe to assume it will eventually be overwritten by a newer verison.
// In fact, it may even be overwritten by a git hook or a Github action.
// So, please, let it live as is. Much appreciated! =)

import kotlin.system.exitProcess
import kotlinx.serialization.json.JsonArray
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonNull
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive

// TODO(dkorolev): Make the fields internal, expose getters/setters.
sealed class AuthzValue {
    object UNDEFINED : AuthzValue() {
        override fun toString(): String = "AuthzValue.UNDEFINED"
    }
    object NULL : AuthzValue() {
        override fun toString(): String = "AuthzValue.NULL"
    }
    data class BOOLEAN(val boolean: Boolean) : AuthzValue() {
        override fun toString(): String = "AuthzValue.BOOLEAN(${boolean})"
    }
    data class INT(val number: Int) : AuthzValue() {
        override fun toString(): String = "AuthzValue.INT(${number})"
    }
    data class DOUBLE(val number: Double) : AuthzValue() {
        override fun toString(): String = "AuthzValue.DOUBLE(${number})"
    }
    data class STRING(val string: String) : AuthzValue() {
        override fun toString(): String = """AuthzValue.STRING("${string}")"""
    }
    data class ARRAY(val elements: ArrayList<AuthzValue> = arrayListOf()) : AuthzValue() {
        override fun toString(): String = "AuthzValue.ARRAY(${elements})"
        // TODO(dkorolev): This calls for templates!
        fun addBoolean(value: Boolean): ARRAY {
            elements.add(AuthzValue.BOOLEAN(value))
            return this
        }
        fun addInt(value: Int): ARRAY {
            elements.add(AuthzValue.INT(value))
            return this
        }
        fun addString(value: String): ARRAY {
            elements.add(AuthzValue.STRING(value))
            return this
        }
        fun addValue(value: AuthzValue): ARRAY {
            elements.add(value)
            return this
        }
    }
    data class OBJECT(val fields: MutableMap<String, AuthzValue> = mutableMapOf()) : AuthzValue() {
        override fun toString(): String = "AuthzValue.OBJECT(${fields})"
        // TODO(dkorolev): This calls for templates!
        fun setBoolean(key: String, value: Boolean): OBJECT {
            fields[key] = AuthzValue.BOOLEAN(value)
            return this
        }
        fun setInt(key: String, value: Int): OBJECT {
            fields[key] = AuthzValue.INT(value)
            return this
        }
        fun setString(key: String, value: String): OBJECT {
            fields[key] = AuthzValue.STRING(value)
            return this
        }
        fun setValue(key: String, value: AuthzValue): OBJECT {
            fields[key] = value
            return this
        }
    }
    data class SET(val elems: MutableSet<AuthzValue>) : AuthzValue()

    data class DATA(val path: String, val dataProvider: AuthzDataProvider) : AuthzValue() {
        override fun toString(): String = """AuthzValue.DATA("${path}")"""
    }
}

class AuthzResult {
    private var hasSomeResult: Boolean = false
    private var hasUniqueResult: Boolean = false
    private var someResult: AuthzValue = AuthzValue.UNDEFINED
    private var allResultsSet: MutableSet<AuthzValue> = mutableSetOf()
    private var allResultsList: ArrayList<AuthzValue> = arrayListOf()

    override fun equals(rhs: Any?): Boolean {
        if (rhs !is AuthzResult) return false
        return allResultsSet == rhs.allResultsSet
    }

    override fun toString(): String {
        if (!hasSomeResult) {
            return "AuthzResult(EMPTY)"
        } else if (hasUniqueResult) {
            return "AuthzResult(${someResult})"
        } else {
            return "AuthzResult[${allResultsSet}]";
        }
    }

    fun addToResultSet(resultObject: AuthzValue) {
        // NOTE(dkorolev): We know that what's added to the result set is a `{"result":...}` object.
        if (resultObject is AuthzValue.OBJECT) {
            val result = resultObject.fields["result"]
            if (result != null) {
                if (!hasSomeResult) {
                    hasSomeResult = true
                    hasUniqueResult = true
                    someResult = result
                    allResultsSet.add(result)
                    allResultsList.add(result)
                } else if (!hasUniqueResult || result != someResult) {
                    hasUniqueResult = false
                    allResultsSet.add(result)
                    allResultsList.add(result)
                } else if (!allResultsSet.contains(result)) {
                    allResultsSet.add(result)
                    allResultsList.add(result)
                }
            } else {
               println("""The key `result` must be part of the object that is being added into the results set.""")
               exitProcess(1)
            }
        } else {
            println("""Attempted to add a non-object value into the result set. Treating as an error for now.""")
            exitProcess(1)
        }
    }

    fun isUnique() = hasUniqueResult

    fun getUniqueResultOrFail(): AuthzValue {
        if (hasUniqueResult) {
            return someResult
        } else {
            println("""Attempted to add an `undefined` value into the result set. Treating as an error for now.""")
            exitProcess(1)
            return AuthzValue.UNDEFINED
        }
    }

    fun getBooleanResult(): Boolean {
        if (hasUniqueResult) {
            // TODO(dkorolev): `Smart cast to 'AuthzValue.BOOLEAN' is impossible, because ...`.
            val copy = someResult
            if (copy is AuthzValue.BOOLEAN) {
                return copy.boolean
            }
        }
        return false
    }

    fun getBooleanResultOrFail(): Boolean {
        if (hasUniqueResult) {
            // TODO(dkorolev): `Smart cast to 'AuthzValue.BOOLEAN' is impossible, because ...`.
            val copy = someResult
            if (copy is AuthzValue.BOOLEAN) {
                return copy.boolean
            }
        }
        println("""Expected a `boolean` result, seeing something else.""")
        exitProcess(1)
        return false
    }

    fun asJsonElement(): JsonElement {
        if (!hasSomeResult) {
            return JsonNull
        } else if (hasUniqueResult) {
            return authzValueToJson(someResult)
        } else {
            val elements: ArrayList<JsonElement> = arrayListOf()
            allResultsList.forEach { elements.add(authzValueToJson(it)) }
            return JsonArray(elements)
        }
    }

    companion object {
        fun BOOLEAN(b: Boolean): AuthzResult {
            var r = AuthzResult()
            r.hasSomeResult = true
            r.hasUniqueResult = true
            r.someResult = AuthzValue.BOOLEAN(b)
            r.allResultsSet.add(r.someResult)
            r.allResultsList.add(r.someResult)
            return r
        }

        // TODO(dkorolev): More in-place "constructors" for `AuthzResult`.
    }
}

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
    is AuthzValue.DATA -> {
        JsonPrimitive("***DATA[${node.path}]***")
    }
}

fun authzResultToJson(result: AuthzResult): JsonElement = result.asJsonElement()

fun regoDotStmt(input: AuthzValue, key: String, authzDataProvider: AuthzDataProvider): AuthzValue = when (input) {
    is AuthzValue.OBJECT -> input.fields.getOrElse(key, { AuthzValue.UNDEFINED })
    is AuthzValue.ARRAY -> {
        val i = key.toInt()
        if (i >= 0 && i < input.elements.size) {
            input.elements[i]
        } else {
            // TODO(dkorolev): Is this acceptable?
            println("""DotStmt on an array with an out-of-bounds index.""")
            exitProcess(1)
            AuthzValue.UNDEFINED
        }
    }
    is AuthzValue.DATA -> {
        val augmentedPath = input.path + "." + key
        val maybeResult = authzDataProvider.maybeCompute(augmentedPath)
        if (maybeResult != null) {
            maybeResult
        } else {
            AuthzValue.DATA(augmentedPath, authzDataProvider)
        }
    }
    else -> {
        // TODO(dkorolev): Is this acceptable?
        println("""DotStmt on an array with an out-of-bounds index.""")
        exitProcess(1)
        AuthzValue.UNDEFINED
    }
}

fun regoArrayAppendStmt(a: AuthzValue, v: AuthzValue) = when (a) {
    is AuthzValue.ARRAY -> a.elements.add(v)
    else -> {
        println("""ArrayAppendStmt on a non-array.""")
        exitProcess(1)
    }
}

fun regoSetAddStmt(a: AuthzValue, v: AuthzValue) = when (a) {
    is AuthzValue.SET -> a.elems.add(v)
    else -> {
        println("""SetAddStmt on a non-set.""")
        exitProcess(1)
    }
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
            println("""ScanStmt on a Set not implemented (yet; should be straightforward).""")
            exitProcess(1)
        }
        else -> {
            println("""ScanStmt on a something that is not an Array/Object/Set, should not happen.""")
            exitProcess(1)
        }
    }
}

enum class RegoBlockResult { INTERRUPTED, COMPLETED }

fun regoVal(locals: MutableMap<Int, AuthzValue>, index: Int): AuthzValue = locals.getOrElse(index, { AuthzValue.UNDEFINED })

fun regoVal(unused: MutableMap<Int, AuthzValue>, value: AuthzValue.BOOLEAN): AuthzValue = value
fun regoVal(unused: MutableMap<Int, AuthzValue>, value: String): AuthzValue = AuthzValue.STRING(value)

// Treat `DATA` as `UNDEFINED`, because it practically is!
// Any `DATA` field from `.data` that is defined immediately becomes the respective value.
fun isRegoValNotDefined(v: AuthzValue): Boolean = (v is AuthzValue.UNDEFINED) || (v is AuthzValue.DATA)

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

class AuthzDataProvider {
    // TODO(dkorolev): This absolutely should be a trie one day, but this would do for the POC.
    // TODO(dkorolev): Probably support the top-level `data` object too.

    private var paths: MutableMap<String, () -> AuthzValue> = mutableMapOf()

    fun injectBoolean(path: String, cb: () -> Boolean): AuthzDataProvider {
        fun helper(): AuthzValue = AuthzValue.BOOLEAN(cb())
        paths[path] = ::helper
        return this
    }

    fun injectInt(path: String, cb: () -> Int): AuthzDataProvider {
        fun helper(): AuthzValue {
            return AuthzValue.INT(cb())
        }
        paths[path] = ::helper
        return this
    }

    fun injectString(path: String, cb: () -> String): AuthzDataProvider {
        fun helper(): AuthzValue {
            return AuthzValue.STRING(cb())
        }
        paths[path] = ::helper
        return this
    }

    // TODO(dkorolev): More injectors!

    fun injectValue(path: String, cb: () -> AuthzValue): AuthzDataProvider {
        paths[path] = cb
        return this
    }

    fun maybeCompute(path: String): AuthzValue? {
        val handler = paths[path]
        if (handler != null) {
            return handler()
        } else {
            return null
        }
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
                // NOTE(dkorolev): Instrument all other `println`-s next to `exitProcess`-es with operands?
                println("""`plus` on non-Ints, not allowed for now, seeing ${a} + ${b}.""")
                exitProcess(1)
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
                println("""`<` on non-Ints, not allowed for now.""")
                exitProcess(1)
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
                println("""`>` on non-Ints, not allowed for now.""")
                exitProcess(1)
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
                println("""`<=` on non-Ints, not allowed for now.""")
                exitProcess(1)
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
                println("""`>=` on non-Ints, not allowed for now.""")
                exitProcess(1)
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
                println("""`minus` on non-Ints, not allowed for now.""")
                exitProcess(1)
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
                println("""`mul` on non-Ints, not allowed for now.""")
                exitProcess(1)
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
                    println("""`rem` on a non-positive divider, not allowed for now.""")
                    exitProcess(1)
                    return AuthzValue.UNDEFINED
                }
            } else {
                println("""`rem` on non-Ints, not allowed for now.""")
                exitProcess(1)
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
                println("""`split` on non-Strings, not allowed for now.""")
                exitProcess(1)
                return AuthzValue.UNDEFINED
            }
        }
        fun concat(args: MutableMap<Int, AuthzValue>): AuthzValue {
            val a: AuthzValue = regoVal(args, 0)
            val b: AuthzValue = regoVal(args, 1)
            if (a is AuthzValue.STRING && b is AuthzValue.ARRAY) {
                val r = ArrayList<String>()
                b.elements.forEach {
                    if (it is AuthzValue.STRING) {
                        r.add(it.string)
                    } else {
                        // NOTE(dkorolev): I assume this should not be allowed ever, not just "for now".
                        println("""An element in the array passed to `concat` is not a STRING, not allowed.""")
                        exitProcess(1)
                        return AuthzValue.UNDEFINED
                    }
                }
                return AuthzValue.STRING(r.joinToString(separator=a.string))
            } else {
                // NOTE(dkorolev): I assume this should not be allowed ever, not just "for now".
                println("""`concat` not on {STRING,ARRAY}, not allowed.""")
                exitProcess(1)
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
                        println("""`number.range` on a malformed range, not allowed for now.""")
                        exitProcess(1)
                        return AuthzValue.UNDEFINED
                    }
                } else {
                    println("""`number.range` on non-Ints, not allowed for now.""")
                    exitProcess(1)
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
                    println("""`internal.member_2`, which is effectively `setContains()` on a non-Set.""")
                    exitProcess(1)
                    return AuthzValue.UNDEFINED
                }
            }
        }
    }
}
