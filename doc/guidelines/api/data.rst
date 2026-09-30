**************************
Data Domain API Guidelines
**************************

Core Semantics
==============

.. code-block:: text

    data = namespace for serialization formats, with no immediate value types
    format namespace = bson, cbor, json, or xml below data
    parse options = bounded input size, nesting, item count, and string length
    format options = output policy and a stable API extension point
    opaque value = recognized wire payload preserved without interpreting or executing it

Primary Types
=============

.. code-block:: text

    json::JsonValue, json::JsonType // copy-on-write JSON value tree and semantic type
    json::JsonArray, json::JsonObject // ordered JSON containers
    bson::BsonValue, bson::BsonType // copy-on-write BSON document tree and semantic type
    bson::BsonArray, bson::BsonDocument // ordered BSON containers
    bson::BsonBinary, bson::BsonOpaqueValue // subtype-bearing binary and uninterpreted known wire values
    cbor::CborValue, cbor::CborType // copy-on-write CBOR value tree and semantic type
    cbor::CborArray, cbor::CborMap, cbor::CborLink // ordered containers and binary CID link
    xml::XmlDocument, xml::XmlNode, xml::XmlAttribute, xml::XmlNodeType // ordered XML DOM

Secondary Types
===============

.. code-block:: text

    json::JsonParseOptions, json::JsonFormatOptions // JSON input limits and output controls
    bson::BsonParseOptions, bson::BsonFormatOptions // BSON input limits and output extension point
    cbor::CborParseOptions, cbor::CborFormatOptions // CBOR input limits and DAG-CBOR profile
    xml::XmlParseOptions, xml::XmlFormatOptions // XML input limits and output extension point

Value Patterns
==============

.. code-block:: text

    o.get(index-or-key)/getOrThrow(index-or-key) -> Value // access a child value
    o.get❮Type❯() -> optional<T> // typed value access without conversion loss
    o.set(index-or-key, value) -> Value& // replace or add a child
    o.append(value) -> Value& // append an array item
    o.toByteBlock(options) -> ByteBlock // encode BSON or CBOR
    T::fromByteBlock(bytes, options) -> optional<T> // parse binary input, or return no value
    T::fromByteBlockOrThrow(bytes, options) -> T // parse binary input or throw ParseError
    o.toString(options) -> String // encode JSON or XML
    T::fromString(text, options) -> optional<T> // parse text, or return no value
    T::fromStringOrThrow(text, options) -> T // parse text or throw ParseError

XML Patterns
============

.. code-block:: text

    T::createElement(name, text) -> XmlNodePtr // create an element with optional text
    o.addElement(name, text) -> XmlNodePtr // append an element with optional text
    o.textContent() -> optional<String> // no value if an unresolved entity prevents complete text
    o.textContentOrThrow() -> String // complete text or a diagnostic
