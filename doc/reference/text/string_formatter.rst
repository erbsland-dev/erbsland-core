.. index::
    single: String Formatter and Related Types

**********************************
String Formatter and Related Types
**********************************

Introduction
============

Format Argument
---------------

:cpp:class:`FormatArgument <erbsland::text::FormatArgument>` is the internal runtime payload used by the formatter
engine after public :cpp:struct:`FormatAs <erbsland::text::FormatAs>` adapters have converted user values to supported
argument types.
User code should extend formatting with
:cpp:struct:`FormatAs <erbsland::text::FormatAs>` instead of constructing this type directly.

Format As
---------

The :cpp:struct:`FormatAs <erbsland::text::FormatAs>` templates are the public extension point for adapting user values
to the formatter system.
Specialize exactly one ``FormatAs*`` template for a custom type and return one of the supported argument payloads.

Integer Example
~~~~~~~~~~~~~~~

.. code-block:: cpp

    template <>
    struct erbsland::text::FormatAsInt64<MyIndex> : erbsland::text::FormatAs<MyIndex, int64_t> {
        [[nodiscard]] auto format(const MyIndex &value) const -> int64_t {
            return value.toRawValue();
        }
    };

Interface
=========

.. doxygenclass:: erbsland::text::FormatArgument
    :members:
.. doxygenenum:: erbsland::text::FormatArgumentKind
.. doxygenstruct:: erbsland::text::FormatAs
    :members:

.. doxygenstruct:: erbsland::text::FormatAsInt64
    :members:

.. doxygenstruct:: erbsland::text::FormatAsUInt64
    :members:

.. doxygenstruct:: erbsland::text::FormatAsDouble
    :members:

.. doxygenstruct:: erbsland::text::FormatAsBool
    :members:

.. doxygenstruct:: erbsland::text::FormatAsChar
    :members:

.. doxygenstruct:: erbsland::text::FormatAsText
    :members:

.. doxygenstruct:: erbsland::text::FormatAsU8Text
    :members:

.. doxygenstruct:: erbsland::text::FormatAsU16Text
    :members:

.. doxygenstruct:: erbsland::text::FormatAsU32Text
    :members:
.. doxygenclass:: erbsland::text::FormatError
    :members:
.. doxygentypedef:: erbsland::text::StringFormat
.. doxygenclass:: erbsland::text::U16Format
    :members:
.. doxygenclass:: erbsland::text::U32Format
    :members:
.. doxygenclass:: erbsland::text::U8Format
    :members:
