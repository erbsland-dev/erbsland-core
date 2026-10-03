..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: JSON; Parsing and Rendering
    single: JsonValue
    single: JsonFormatOptions
    single: JsonParseOptions

*******************************
Parsing and Rendering JSON Data
*******************************

JSON appears wherever one program hands structured data to another: in files, API responses, and messages.
Its values are familiar ones: null, booleans, numbers, strings, arrays, and objects.

:cpp:class:`JsonValue <erbsland::data::json::JsonValue>` represents any of those values in one tree. Here you will read
a document, find values within it, build a new one, and write it back as JSON.
The final sections cover output choices and limits for incoming data.

Reading One Complete Value
==========================

Start with ``JsonValue::fromString()`` when invalid input is an ordinary outcome.
It parses one complete JSON value and returns ``std::nullopt`` if parsing fails.

When a failure needs a diagnostic, use ``fromStringOrThrow()``.
It returns the value or raises
:cpp:class:`ParseError <erbsland::err::ParseError>`. Both forms reject a second value after the first, duplicate object
keys, malformed numbers, and other invalid JSON syntax.

The parser does not report malformed UTF-8. If that matters at your input boundary, validate the string before parsing
it.

The example reads a small synthesizer-patch description.
It also shows the throwing form handling a malformed document.

.. erbsland-demo::
    :source: data/JsonValues/ParseDocument.cpp
    :show: f:parseDocument
    :exec: data/json_values --demo ParseDocument
    :sha256: 5a528c593e1deac84f7a911382ee728bc46c9201ee6b8ba658fc0bb29fb028b6

.. code-block:: cpp

    void parseDocument() {
        const auto source = u8R"({"patch":"Yankı","voices":2,"enabled":true})"_el;
        if (const auto value = JsonValue::fromString(source)) {
            el::io::printLine("Patch: "_el, value->getOrThrow("patch"_el).getTextOrThrow());
        }

        // A throwing parse is useful at a boundary where invalid input needs a diagnostic.
        try {
            const auto value = JsonValue::fromStringOrThrow(R"({"voices":2,})"_el);
            el::io::printLine(value.toString());
        } catch (const el::err::ParseError &) {
            el::io::printLine("Invalid JSON document"_el);
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mPatch: Yankı
        Invalid JSON document␛[0m

.. erbsland-demo-end::

JSON strings become Unicode text.
A JSON number may become a signed integer or a finite floating-point value.
When your schema requires a particular numeric type, ask for that type and handle a failed conversion.

Before accepting data from outside your program, you can also limit the size and complexity of the document.
Those controls appear under :ref:`bounding-json-input`.

Following Values Through the Tree
=================================

An object member has a string key.
An array element has an :cpp:type:`ItemIndex <erbsland::unit::ItemIndex>`.
You use either one with ``get()`` or ``getOrThrow()`` to move down the tree.

For a field your schema requires, ``getOrThrow()`` gives you a clear failure when the key or index is missing, or when
the parent has the wrong type.
Plain ``get()`` returns a JSON null value in those cases.

That null result is convenient for optional fields, but it hides one distinction: a missing member looks the same as a
member explicitly set to JSON ``null``.
Use the throwing lookup when that difference matters.

Once you have selected a child, choose a typed accessor.
``getTextOrThrow()``, ``getBoolOrThrow()``, and ``getOrThrow<int64_t>()`` serve required values.
For a field that may have another type, ``get<T>()`` returns an optional result; ``get<T>(fallback)`` supplies a
default.

The generic typed accessors also support ``double``, ``JsonArray``, and ``JsonObject``.

.. erbsland-demo::
    :source: data/JsonValues/ParseDocument.cpp
    :show: f:inspectDocument
    :exec: data/json_values --demo InspectDocument
    :sha256: 40c5ef866b562b8f139ec1744936e1f19eb2f7fbf16f3bdf278bd3d72b3cbd6f

.. code-block:: cpp

    void inspectDocument() {
        const auto document = JsonValue::fromStringOrThrow(
            u8R"({"patch":"Yankı","oscillators":[{"wave":"sine"},{"wave":"triangle"}],"enabled":true})"_el);
        const auto oscillators = document.getOrThrow("oscillators"_el);
        const auto second = oscillators.getOrThrow(el::ItemIndex{1U});
        el::io::printLine("Wave: "_el, second.getOrThrow("wave"_el).getTextOrThrow());
        el::io::printLine("Enabled: "_el, document.getOrThrow("enabled"_el).getBoolOrThrow());
        el::io::printLine("Missing is null: "_el, document.get("gain"_el).is(el::json::JsonType::Null));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mWave: triangle
        Enabled: true
        Missing is null: true␛[0m

.. erbsland-demo-end::

``type()`` reports a value's JSON kind, while ``is()`` tests for one kind.
If the value is an array or object, ``itemCount()`` tells you how many children it contains.
For a primitive value, the count is zero.

Building and Changing a Document
================================

You can construct a ``JsonValue`` from a Boolean, number, ``String``, ``JsonArray``, or ``JsonObject``.
A default-constructed value represents JSON null.

For a document assembled piece by piece, begin with an empty object or array.
Add object members with ``set()`` and array elements with ``append()``.
On an array, ``set()`` replaces an existing element, or appends when its index equals the current item count.

.. erbsland-demo::
    :source: data/JsonValues/ParseDocument.cpp
    :show: f:buildDocument
    :exec: data/json_values --demo BuildDocument
    :sha256: d2d4273585d832c7d105a9e77bd37f9c3d56eb535ccef71b07f3a1d2bb1dd1f8

.. code-block:: cpp

    void buildDocument() {
        auto patch = JsonValue{JsonObject{}};
        patch.set("name"_el, el::String{u8"Yankı"_el});
        patch.set("active"_el, true);
        auto voices = JsonValue{JsonArray{}};
        voices.append("sine"_el).append("triangle"_el);
        patch.set("voices"_el, voices);
        el::io::printLine(patch.toString());

        // Copies share values until changed; the original document still has two voices.
        auto revised = patch;
        auto moreVoices = revised.getOrThrow("voices"_el);
        moreVoices.append("square"_el);
        revised.set("voices"_el, moreVoices);
        el::io::printLine("Original voices: "_el, patch.getOrThrow("voices"_el).itemCount().toSizeT());
        el::io::printLine("Revised voices: "_el, revised.getOrThrow("voices"_el).itemCount().toSizeT());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m{"active":true,"name":"Yankı","voices":["sine","triangle"]}
        Original voices: 2
        Revised voices: 3␛[0m

.. erbsland-demo-end::

A copied ``JsonValue`` shares data with its source until one of them changes.
This lets you keep the original document while preparing a revised version.

Notice how the example updates the nested array.
It retrieves the array as a value, appends an element, then puts the array back into the copied object.
Without that final ``set()``, the parent would still hold the old array.

Writing JSON
============

Call ``toString()`` to turn a value tree into JSON text.
Its default output is compact, which works well for messages and stored fields.

Object keys follow the order of the ``JsonObject`` map, giving the same tree deterministic output.
When the text is meant for a person to read, pass
:cpp:class:`JsonFormatOptions <erbsland::data::json::JsonFormatOptions>` to choose indentation and escaping.

.. erbsland-demo::
    :source: data/JsonValues/ParseDocument.cpp
    :show: f:formatDocument
    :exec: data/json_values --demo FormatDocument
    :sha256: cc7644b49e0e3a7b57f57a2c1c74e5b05a1ee84c86da03704bc540a644c36189

.. code-block:: cpp

    void formatDocument() {
        const auto patch = JsonValue::fromStringOrThrow(u8R"({"patch":"Yankı","voices":["sine","triangle"]})"_el);
        el::io::printLine("Compact: "_el, patch.toString());
        el::io::printLine("Pretty:"_el);
        el::io::printLine(patch.toString(JsonFormatOptions::pretty()));
        el::io::printLine("Four-space array:"_el);
        el::io::printLine(JsonValue{JsonArray{JsonValue{2}}}.toString(JsonFormatOptions{}.setIndentation(el::CpCount{4U})));
        const auto ascii = JsonFormatOptions::compact().setEscapeAmount(el::EscapeAmount::NonAscii);
        el::io::printLine("ASCII: "_el, patch.toString(ascii));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mCompact: {"patch":"Yankı","voices":["sine","triangle"]}
        Pretty:
        {
          "patch": "Yankı",
          "voices": [
            "sine",
            "triangle"
          ]
        }
        Four-space array:
        [
            2
        ]
        ASCII: {"patch":"Yank\u0131","voices":["sine","triangle"]}␛[0m

.. erbsland-demo-end::

Indentation
-----------

``setIndentation()`` sets the number of spaces at each container level.
The value is a
:cpp:type:`CpCount <erbsland::unit::CpCount>`. Zero means compact output, without layout line breaks.

``JsonFormatOptions::compact()`` selects that default.
``pretty()`` uses two spaces instead.
The demo shows both forms of the same document, then sets a four-space indentation explicitly for a small array.

Indentation changes presentation only.
Choose the width that makes a file comfortable to review; use compact output when space matters more than layout.

String Escaping
---------------

``setEscapeAmount()`` controls the spelling of characters inside JSON strings.
By default, ``EscapeAmount::Required`` escapes what JSON syntax requires and leaves ordinary Unicode text readable.

The demo selects ``NonAscii`` for an ASCII-only destination.
The Turkish dotless ``ı`` then appears as ``\u0131`` in the output, while a JSON reader still recovers the same text.

Other :cpp:class:`EscapeAmount <erbsland::text::EscapeAmount>` choices give you more control.
``Balanced`` also escapes invisible control and format characters; ``Everything`` escapes every character.
``Nothing`` skips even required escapes, so a quote or control character in a string can make the result invalid JSON.

Escaping is an output choice.
It does not validate the values in the tree.

.. _bounding-json-input:

Bounding Input
==============

An external document may be much larger or more deeply nested than your application expects.
:cpp:class:`JsonParseOptions <erbsland::data::json::JsonParseOptions>` gives the parser limits for both size and
structure.

The defaults allow 16 MiB of input, 64 open container levels, one million values, and eight million decoded code points
for any one key or string.
The example lowers each limit so you can see what it rejects.

.. erbsland-demo::
    :source: data/JsonValues/ParseDocument.cpp
    :show: f:limitDocument
    :exec: data/json_values --demo LimitDocument
    :sha256: 683b70d971c57c96aeac3eff5c492bd0b63714caf8ff3170883bcbf1e4e7c097

.. code-block:: cpp

    void limitDocument() {
        const auto input = JsonParseOptions{}.setMaximumInputLength(el::ByteCount{3U});
        const auto nesting = JsonParseOptions{}.setMaximumNesting(el::ItemCount{1U});
        const auto values = JsonParseOptions{}.setMaximumValueCount(el::ItemCount{2U});
        const auto strings = JsonParseOptions{}.setMaximumStringLength(el::CpCount{2U});
        el::io::printLine("Input limit accepts null: "_el, JsonValue::fromString("null"_el, input).has_value());
        el::io::printLine("Nesting limit accepts [[1]]: "_el, JsonValue::fromString("[[1]]"_el, nesting).has_value());
        el::io::printLine("Value limit accepts [1,2]: "_el, JsonValue::fromString("[1,2]"_el, values).has_value());
        el::io::printLine("String limit accepts abc: "_el, JsonValue::fromString("\"abc\""_el, strings).has_value());
        el::io::printLine("String limit accepts é: "_el, JsonValue::fromString(u8"\"é\""_el, strings).has_value());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mInput limit accepts null: false
        Nesting limit accepts [[1]]: false
        Value limit accepts [1,2]: false
        String limit accepts abc: false
        String limit accepts é: true␛[0m

.. erbsland-demo-end::

Input Length
------------

``setMaximumInputLength()`` measures the source in bytes.
Punctuation and whitespace count along with the values themselves.

The example allows three bytes.
Even the four-byte literal ``null`` is too long, so parsing stops before that document can be accepted.
This is the broadest bound on the size of incoming text.

Container Nesting
-----------------

``setMaximumNesting()`` counts how many arrays and objects can be open at once.
A primitive value does not add a level.

For example, ``[1]`` needs one level, while ``[[1]]`` needs two.
The demo allows only one, so the second document is rejected.

Value Count
-----------

``setMaximumValueCount()`` counts every value in the tree, including its root.
This is different from the number of items in one array or object.

Consider ``[1,2]``.
It has two array elements, but three JSON values: the array itself and the two numbers.
The demo's limit of two therefore rejects it.

Decoded String Length
---------------------

``setMaximumStringLength()`` applies to each object key and string value after JSON escapes have been decoded.
The limit counts Unicode code points, not the bytes occupied by their UTF-8 encoding.

The demo sets a limit of two.
It rejects ``abc``, which has three code points, but accepts ``é`` even though that character occupies more than one
UTF-8 byte.

A particular field may need a shorter limit than the parser's general one.
You can check that field after parsing its document.
