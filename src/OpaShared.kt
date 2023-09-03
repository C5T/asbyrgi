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
    data class ValueArray(val elements: Array<OpaValue>) : OpaValue()
    data class ValueObject(val fields: MutableMap<String, OpaValue>) : OpaValue()
}

fun jsonToOpaValue(element: JsonElement): OpaValue = when (element) {
    is JsonNull -> {
        OpaValue.ValueNull
    }
    is JsonArray -> {
        var elements = ArrayList<OpaValue>()
        element.forEach { elements.add(jsonToOpaValue(it)) }
        OpaValue.ValueArray(elements.toTypedArray())
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
                        println("FAIL")
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
}

fun irGetByKey(input: OpaValue, key: String): OpaValue = when (input) {
    is OpaValue.ValueObject -> input.fields.getOrElse(key, { OpaValue.ValueUndefined })
    else -> OpaValue.ValueUndefined
}

fun localOrUndefined(locals: MutableMap<Int, OpaValue>, index: Int): OpaValue = locals.getOrElse(index, { OpaValue.ValueUndefined })

fun opaBuiltinFunction_plus(args: MutableMap<Int, OpaValue>): OpaValue {
    val a: OpaValue = localOrUndefined(args, 0)
    val b: OpaValue = localOrUndefined(args, 1)
    if (a is OpaValue.ValueInt && b is OpaValue.ValueInt) {
        return OpaValue.ValueInt(a.number + b.number)
    } else {
        return OpaValue.ValueUndefined
    }
}
