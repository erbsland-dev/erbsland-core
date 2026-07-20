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
``ByteBlock`` uses ``FormatAsBytes`` and is stored as an owning runtime argument.

Integer Example
~~~~~~~~~~~~~~~

.. code-block:: cpp

    template <>
    struct erbsland::text::FormatAsInt64<MyIndex> : erbsland::text::FormatAs<MyIndex, int64_t> {
        [[nodiscard]] auto format(const MyIndex &value) const -> int64_t {
            return value.toRawValue();
        }
    };

Format Strings
--------------

Fields use ``{}`` or ``{index}`` for automatic or explicit argument selection.
The existing compact ``std::format`` -style specification remains available and unchanged.
An untyped ``{}`` formats a ``ByteBlock`` as compact lowercase hexadecimal text.
Legacy non-empty specifications do not select byte-formatting options.

Typed fields have this strict form:

.. code-block:: text

    {[index]:selector:option,option,...}

The selector and its trailing colon are mandatory.
For example, ``{:bytes:}`` selects default byte formatting, and ``{2:number:base=hexadecimal,alternate}`` selects the
third argument as a hexadecimal number.
Typed fields accept only the named options described below; compact legacy modifiers cannot follow a selector.

Selectors lock the accepted argument type:

*   ``text`` accepts UTF-8, UTF-16, and UTF-32 text plus ``Char``.
*   ``number`` accepts signed integers, unsigned integers, and floating-point values.
*   ``bool`` accepts only boolean values.
*   ``bytes`` accepts only ``ByteBlock``.

A selector/type mismatch raises ``FormatError``.

Named Option Grammar
~~~~~~~~~~~~~~~~~~~~

Named options are comma-separated and order-independent.
They contain no insignificant whitespace.
Option names and enum values are ASCII case-insensitive.
An option may appear only once, including through a mixture of its full name and alias.

Unsigned decimal values may omit ``=``, so ``width=32``, ``width32``, and ``w32`` are equivalent.
Enum values require ``=``.
Flags take no value.
``fill`` consumes exactly one direct safe Unicode code point and preserves its spelling.
All other characters in a named specification must also be safe Unicode, while identifiers themselves are ASCII.

Unknown options or values, duplicate options, missing values, incompatible combinations, leading or trailing empty
options, empty options between commas, and unsafe characters raise ``FormatError``.

Options by Domain
~~~~~~~~~~~~~~~~~

.. list-table::
    :header-rows: 1
    :widths: 16 50 34

    *   - Domain
        - Options
        - Notes
    *   - Shared layout
        - ``width`` / ``w``, ``alignment`` / ``al``, ``fill`` / ``fl``
        - Available for ``text``, ``number``, and ``bool``.
    *   - Text
        - ``maximum`` / ``max``, ``escape`` / ``esc``, ``escape-amount`` / ``ea``
        - Maximum truncates source code points before escaping; layout is applied afterward.
    *   - Number
        - ``base`` / ``bs``, ``notation`` / ``nt``, ``letter-case`` / ``lc``, ``sign`` / ``sg``,
          ``precision`` / ``pr``, ``alternate`` / ``alt``, ``zero-fill`` / ``zf``
        - ``alternate`` and ``zero-fill`` are flags. Integer-only and floating-point-only options reject the wrong
          numeric subtype.
    *   - Boolean
        - ``style`` / ``sty``, ``capitalization`` / ``cap``
        - Rendering uses ``BooleanFormat`` before applying the shared layout.
    *   - Bytes
        - ``separator`` / ``sep``, ``maximum`` / ``max``, ``truncate`` / ``tr``
        - ``separator`` is a flag. Truncated output uses the fixed Unicode ellipsis ``…``.

The short option aliases are globally stable and unique.
Value aliases are local to their option:

.. list-table::
    :header-rows: 1
    :widths: 25 75

    *   - Option
        - Values
    *   - ``alignment``
        - ``left`` / ``l``, ``right`` / ``r``, ``center`` / ``c``
    *   - ``escape``
        - ``none`` / ``n``, ``html`` / ``h``, ``json`` / ``j``, ``cpp`` / ``cp``, ``xml`` / ``x``,
          ``regex`` / ``rx``, ``display`` / ``d``, ``config`` / ``cf``, ``config_test`` / ``ct``
    *   - ``escape-amount``
        - ``nothing`` / ``n``, ``required`` / ``r``, ``balanced`` / ``b``, ``non-ascii`` / ``na``, ``all`` / ``a``
    *   - ``base``
        - ``decimal`` / ``d``, ``hexadecimal`` / ``x``, ``binary`` / ``b``, ``octal`` / ``o``
    *   - ``notation``
        - ``default`` / ``d``, ``fixed`` / ``f``, ``scientific`` / ``s``, ``general`` / ``g``,
          ``hexadecimal`` / ``x``
    *   - ``letter-case``
        - ``lowercase`` / ``l``, ``uppercase`` / ``u``
    *   - ``sign``
        - ``negative-only`` / ``n``, ``always`` / ``a``, ``space`` / ``s``
    *   - ``style``
        - ``true`` / ``t``, ``yes`` / ``y``, ``on`` / ``o``, ``enabled`` / ``e``
    *   - ``capitalization``
        - ``lowercase`` / ``l``, ``uppercase`` / ``u``, ``titlecase`` / ``t``
    *   - ``truncate``
        - ``end`` / ``e``, ``middle`` / ``m``, ``begin`` / ``b``

The ``bytes`` maximum has the same output-item semantics as ``ByteFormat``.
Its non-empty Unicode ellipsis occupies one item, so a maximum of 16 retains 15 bytes when truncation is necessary.
Middle truncation assigns an odd extra retained byte to the prefix.

Examples
~~~~~~~~

.. code-block:: cpp

    auto title = "{:text:maximum=20,width=24,alignment=center}"_ef.format(name);
    auto mask = "{:number:bs=x,lc=u,alternate,zf,w=10}"_ef.format(value);
    auto state = "{:bool:sty=yes,cap=titlecase}"_ef.format(enabled);
    auto diagnostic = "{:bytes:maximum=16,truncate=middle}"_ef.format(data);

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

.. doxygenstruct:: erbsland::text::FormatAsBytes
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
