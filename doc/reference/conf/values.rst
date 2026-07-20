.. index::
    single: Configuration Values

********************
Configuration Values
********************

Text values are exposed as :cpp:type:`erbsland::text::String <erbsland::text::String>`, and lists of text use
:cpp:type:`erbsland::text::StringList <erbsland::text::StringList>`. Read-only strings are returned by value so
callers can keep inexpensive, independent references to their content.

A :cpp:class:`erbsland::conf::Value <erbsland::conf::Value>` is one node in a configuration tree.
Its value type distinguishes scalar values, value lists and matrices, sections, section lists, and sections with text
indexes.
Typed accessors verify this type and return the corresponding scalar or child structure.
Navigation accepts names and name paths, while iteration exposes children without flattening the tree.

The configuration scalar mapping is deliberately shared with the other Core domains: bytes use ``mem::ByteBlock``;
dates, times, date-times, and deltas use the matching ``time`` types; and regular expressions use ``re::RegExPtr``.
Regular-expression literals are compiled lazily, and copies share the same immutable compilation state.
The non-throwing accessors return an empty byte block, invalid date or date-time, midnight, a zero delta, or ``nullptr``
when the requested value is missing or has another type.
The ``...OrThrow()`` variants report a type mismatch instead.

Values also retain their name, absolute name path, location, and validation metadata.
Shared pointers express the ownership contract: callers may keep a value after discarding the parser or document handle,
and const pointers provide read-only traversal.
Core strings use copy-on-write ownership rather than borrowed views.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    const auto server = document->value("server"_el);
    const auto port = server->value("port"_el)->asInteger();
    const erbsland::text::String name = server->value("name"_el)->asText();

Use :cpp:func:`erbsland::conf::Document::toFlatValueMap <erbsland::conf::Document::toFlatValueMap>` when a complete
absolute-path index is more convenient than tree traversal.

Interface
=========

.. doxygenclass:: erbsland::conf::Value
    :members:
.. doxygenclass:: erbsland::conf::ValueIterator
    :members:
.. doxygenclass:: erbsland::conf::ValueType
    :members:
