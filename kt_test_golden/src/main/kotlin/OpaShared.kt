import kotlinx.serialization.json.JsonArray
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonNull
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive

sealed class OpaValue {
    object ValueUndefined : OpaValue()
    object ValueNull : OpaValue()
    data class ValueBoolean(val boolean: Boolean) : OpaValue()
    data class ValueInt(val number: Int) : OpaValue()
    data class ValueDouble(val number: Double) : OpaValue()
    data class ValueString(val string: String) : OpaValue()
    data class ValueArray(val elements: ArrayList<OpaValue>) : OpaValue()
    data class ValueObject(val fields: MutableMap<String, OpaValue>) : OpaValue()
    data class ValueSet(val elems: MutableSet<OpaValue>) : OpaValue()
}

fun jsonToOpaValue(element: JsonElement): OpaValue = when (element) {
    is JsonNull -> {
        OpaValue.ValueNull
    }
    is JsonArray -> {
        var elements = ArrayList<OpaValue>()
        element.forEach { elements.add(jsonToOpaValue(it)) }
        OpaValue.ValueArray(elements)
    }
    is JsonObject -> {
        var fields: MutableMap<String, OpaValue> = mutableMapOf()
        element.entries.forEach { (key, value) -> fields[key] = jsonToOpaValue(value) }
        OpaValue.ValueObject(fields)
    }
    is JsonPrimitive -> {
        val tmp: JsonPrimitive = element
        if (tmp.isString) {
            OpaValue.ValueString(tmp.content)
        } else {
            // NOTE(dkorolev): Hack but works. I couldn't find a better way with `kotlinx.serialization`.
            if (tmp.content == "true") {
                OpaValue.ValueBoolean(true)
            } else if (tmp.content == "false") {
                OpaValue.ValueBoolean(false)
            } else {
                val v: Int? = tmp.content.toIntOrNull()
                if (v != null) {
                    OpaValue.ValueInt(v)
                } else {
                    val v2: Double? = tmp.content.toDoubleOrNull()
                    if (v2 != null) {
                        OpaValue.ValueDouble(v2)
                    } else {
                        // TODO(dkorolev): Proper error handling.
                        println("FAIL 1")
                        OpaValue.ValueUndefined
                    }
                }
            }
        }
    }
}

fun opaValueToJson(node: OpaValue): JsonElement = when (node) {
    is OpaValue.ValueUndefined -> JsonPrimitive("***UNDEFINED***")
    is OpaValue.ValueNull -> JsonNull
    is OpaValue.ValueBoolean -> JsonPrimitive(node.boolean)
    is OpaValue.ValueString -> JsonPrimitive(node.string)
    is OpaValue.ValueInt -> JsonPrimitive(node.number)
    is OpaValue.ValueDouble -> JsonPrimitive(node.number)
    is OpaValue.ValueArray -> {
        val elements: ArrayList<JsonElement> = arrayListOf()
        node.elements.forEach { elements.add(opaValueToJson(it)) }
        JsonArray(elements)
    }
    is OpaValue.ValueObject -> {
        val fields: MutableMap<String, JsonElement> = mutableMapOf()
        node.fields.forEach { (key, value) -> fields[key] = opaValueToJson(value) }
        JsonObject(fields)
    }
    is OpaValue.ValueSet -> {
        val elements: ArrayList<JsonElement> = arrayListOf()
        node.elems.forEach { elements.add(opaValueToJson(it)) }
        JsonArray(elements)
    }
}

fun irGetByKey(input: OpaValue, key: String): OpaValue = when (input) {
    is OpaValue.ValueObject -> input.fields.getOrElse(key, { OpaValue.ValueUndefined })
    is OpaValue.ValueArray -> {
        val i = key.toInt()
        println("INDIRECT GET: $i")
        if (i >= 0 && i < input.elements.size) {
            input.elements[i]
        } else {
            OpaValue.ValueUndefined
        }
    }
    else -> OpaValue.ValueUndefined
}

fun opaAppendToArray(a: OpaValue, v: OpaValue) = when (a) {
    is OpaValue.ValueArray -> a.elements.add(v)
    else -> Unit // TODO(dkorolev): Error!
}

fun opaAddToSet(a: OpaValue, v: OpaValue) = when (a) {
    is OpaValue.ValueSet -> a.elems.add(v)
    else -> Unit // TODO(dkorolev): Error!
}

fun opaScan(locals: MutableMap<Int, OpaValue>, x: OpaValue, k: Int, v: Int, inner: () -> RegoBlock) {
    locals[k] = OpaValue.ValueUndefined
    locals[v] = OpaValue.ValueUndefined
    when (x) {
        is OpaValue.ValueArray -> {
            // TODO(dkorolev): Use `x.elements.forEach {}`.
            for (i in 0..(x.elements.size - 1)) {
                locals[k] = OpaValue.ValueInt(i)
                locals[v] = x.elements[i]
                inner()
            }
            locals[k] = OpaValue.ValueUndefined
            locals[v] = OpaValue.ValueUndefined
        }
        is OpaValue.ValueObject -> {
            for (e in x.fields.iterator()) {
                locals[k] = OpaValue.ValueString(e.key)
                locals[v] = e.value
                inner()
            }
            locals[k] = OpaValue.ValueUndefined
            locals[v] = OpaValue.ValueUndefined
        }
        is OpaValue.ValueSet -> {
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

fun localOrUndefined(locals: MutableMap<Int, OpaValue>, index: Int): OpaValue = locals.getOrElse(index, { OpaValue.ValueUndefined })

// TODO(dkorolev): Rename `localOrUndefined` into some `getValue`.
fun localOrUndefined(unused: MutableMap<Int, OpaValue>, value: OpaValue.ValueBoolean): OpaValue = value
fun localOrUndefined(unused: MutableMap<Int, OpaValue>, value: String): OpaValue = OpaValue.ValueString(value)

// TODO(dkorolev): Rename this, and the value is not a `String`, but it can be an index too!
fun irStringPossiblyFromLocal(unused: MutableMap<Int, OpaValue>, s: String): String = s
fun irStringPossiblyFromLocal(locals: MutableMap<Int, OpaValue>, i: Int): String {
    val maybeStringValue = locals[i]
    if (maybeStringValue is OpaValue.ValueString) {
        return maybeStringValue.string
    } else if (maybeStringValue is OpaValue.ValueInt) {
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
        fun plus(args: MutableMap<Int, OpaValue>): OpaValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: OpaValue = localOrUndefined(args, 0)
            val b: OpaValue = localOrUndefined(args, 1)
            if (a is OpaValue.ValueInt && b is OpaValue.ValueInt) {
                return OpaValue.ValueInt(a.number + b.number)
            } else {
                return OpaValue.ValueUndefined
            }
        }

        fun lt(args: MutableMap<Int, OpaValue>): OpaValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: OpaValue = localOrUndefined(args, 0)
            val b: OpaValue = localOrUndefined(args, 1)
            if (a is OpaValue.ValueInt && b is OpaValue.ValueInt) {
                return OpaValue.ValueBoolean(a.number < b.number)
            } else {
                return OpaValue.ValueUndefined
            }
        }

        fun gt(args: MutableMap<Int, OpaValue>): OpaValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: OpaValue = localOrUndefined(args, 0)
            val b: OpaValue = localOrUndefined(args, 1)
            if (a is OpaValue.ValueInt && b is OpaValue.ValueInt) {
                return OpaValue.ValueBoolean(a.number > b.number)
            } else {
                return OpaValue.ValueUndefined
            }
        }

        fun lte(args: MutableMap<Int, OpaValue>): OpaValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: OpaValue = localOrUndefined(args, 0)
            val b: OpaValue = localOrUndefined(args, 1)
            if (a is OpaValue.ValueInt && b is OpaValue.ValueInt) {
                return OpaValue.ValueBoolean(a.number <= b.number)
            } else {
                return OpaValue.ValueUndefined
            }
        }

        fun gte(args: MutableMap<Int, OpaValue>): OpaValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: OpaValue = localOrUndefined(args, 0)
            val b: OpaValue = localOrUndefined(args, 1)
            if (a is OpaValue.ValueInt && b is OpaValue.ValueInt) {
                return OpaValue.ValueBoolean(a.number >= b.number)
            } else {
                return OpaValue.ValueUndefined
            }
        }

        fun minus(args: MutableMap<Int, OpaValue>): OpaValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: OpaValue = localOrUndefined(args, 0)
            val b: OpaValue = localOrUndefined(args, 1)
            if (a is OpaValue.ValueInt && b is OpaValue.ValueInt) {
                return OpaValue.ValueInt(a.number - b.number)
            } else {
                return OpaValue.ValueUndefined
            }
        }

        fun mul(args: MutableMap<Int, OpaValue>): OpaValue {
            // TODO(dkorolev): We're certainly not only working with `Int`-s here!
            val a: OpaValue = localOrUndefined(args, 0)
            val b: OpaValue = localOrUndefined(args, 1)
            if (a is OpaValue.ValueInt && b is OpaValue.ValueInt) {
                return OpaValue.ValueInt(a.number * b.number)
            } else {
                return OpaValue.ValueUndefined
            }
        }

        fun rem(args: MutableMap<Int, OpaValue>): OpaValue {
            val a: OpaValue = localOrUndefined(args, 0)
            val b: OpaValue = localOrUndefined(args, 1)
            if (a is OpaValue.ValueInt && b is OpaValue.ValueInt) {
                if (b.number > 0) {
                    return OpaValue.ValueInt(a.number % b.number)
                } else {
                    return OpaValue.ValueUndefined
                }
            } else {
                return OpaValue.ValueUndefined
            }
        }
        fun split(args: MutableMap<Int, OpaValue>): OpaValue {
            val a: OpaValue = localOrUndefined(args, 0)
            val b: OpaValue = localOrUndefined(args, 1)
            if (a is OpaValue.ValueString && b is OpaValue.ValueString) {
                val r = ArrayList<OpaValue>()
                // TODO(dkorolev): I have no idea whether this is correct for multi-char `delim` string.
                a.string.split(b.string).forEach { r.add(OpaValue.ValueString(it)) }
                return OpaValue.ValueArray(r)
            } else {
                return OpaValue.ValueUndefined
            }
        }
    }
    class numbers {
        companion object {
            fun range(args: MutableMap<Int, OpaValue>): OpaValue {
                val a: OpaValue = localOrUndefined(args, 0)
                val b: OpaValue = localOrUndefined(args, 1)
                if (a is OpaValue.ValueInt && b is OpaValue.ValueInt) {
                    // TODO(dkorolev): This constraint is obviously artificial.
                    if (b.number >= a.number) {
                        val r = ArrayList<OpaValue>()
                        for (i in a.number..b.number) {
                            r.add(OpaValue.ValueInt(i))
                        }
                        return OpaValue.ValueArray(r)
                    } else {
                        return OpaValue.ValueUndefined
                    }
                } else {
                    return OpaValue.ValueUndefined
                }
            }
        }
    }
    class internal {
        companion object {
            fun member_2(args: MutableMap<Int, OpaValue>): OpaValue {
                val a: OpaValue = localOrUndefined(args, 0)
                val b: OpaValue = localOrUndefined(args, 1)
                if (b is OpaValue.ValueSet) {
                    return OpaValue.ValueBoolean(b.elems.contains(a))
                } else {
                    // TODO(dkorolev): This should be a proper error!
                    return OpaValue.ValueUndefined
                }
            }
        }
    }
}
