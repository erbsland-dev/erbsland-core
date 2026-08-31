.. index::
    single: JSON
    single: JSON Value

***********
JSON Values
***********

Value Tree
==========

``JsonValue`` is a copy-on-write value tree for JSON nulls, booleans, signed 64-bit integers, finite floating-point
numbers, Unicode strings, arrays, and objects.
``JsonArray`` and ``JsonObject`` use the regular Erbsland Core list and ordered string-map containers.
Copying a tree is inexpensive; the first mutation detaches the changed value.

Parsing and Formatting
======================

``fromStringOrThrow()`` strictly parses one complete RFC 8259 value and reports syntax, duplicate-key, encoding, and
configured-limit failures as :cpp:class:`ParseError <erbsland::err::ParseError>`.
``fromString()`` provides the optional-returning form.
Default limits accept 16 MiB documents, 64 container levels, one million values, and 8 MiB decoded strings.

``toString()`` produces deterministic compact JSON with object keys in ``StringMap`` order.
``JsonFormatOptions`` can enable space-indented output and non-ASCII escaping.

Interface
=========

.. doxygentypedef:: erbsland::text::json::JsonArray

.. doxygentypedef:: erbsland::text::json::JsonObject
.. doxygenclass:: erbsland::text::json::JsonFormatOptions
    :members:
.. doxygenclass:: erbsland::text::json::JsonParseOptions
    :members:
.. doxygenenum:: erbsland::text::json::JsonType
.. doxygenclass:: erbsland::text::json::JsonValue
    :members:
