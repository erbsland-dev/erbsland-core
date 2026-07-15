.. index::
    single: String Converter

****************
String Converter
****************

Introduction
============

``StringConverter`` is the explicit conversion entry point for Erbsland strings, Erbsland string views and literals, and
the supported standard-library string types.
It keeps conversion helpers out of the core string classes while still allowing concise call sites.

.. code-block:: cpp

    auto text = el::StringConverter{std::string_view{"Hello"}}.toString();
    auto utf16 = el::StringConverter{text}.toStdU16String();
    auto view = el::StringConverter{utf16}.toStringView();

``toStringView()`` returns an aliasing UTF-8 view when the source is already compatible, and an owning UTF-8 view when
conversion needs temporary storage.
All conversion methods accept :cpp:enum:`EncodingErrorMode <erbsland::text::EncodingErrorMode>`.

Extension libraries can support additional source types by specializing
:cpp:struct:`StringConverterTraits <erbsland::text::StringConverterTraits>`.

Encoding Error Mode
-------------------

:cpp:enum:`EncodingErrorMode <erbsland::text::EncodingErrorMode>` controls how encoding errors are handled during
string conversion.

String Decoder
--------------

:cpp:class:`StringDecoder <erbsland::text::StringDecoder>` decodes :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` or
:cpp:class:`ByteBlockView <erbsland::mem::ByteBlockView>` input into Erbsland strings.

.. code-block:: cpp

    auto text = el::StringDecoder{bytes}.decode(el::StringEncoding::Utf8);
    auto utf16 = el::StringDecoder{bytes}.toU16String(el::StringEncoding::Utf16);

The decoder accepts :cpp:enum:`StringEncoding <erbsland::text::StringEncoding>`,
:cpp:enum:`StringBomMode <erbsland::text::StringBomMode>`, and
:cpp:enum:`EncodingErrorMode <erbsland::text::EncodingErrorMode>` for all target string widths.

String Encoder
--------------

``StringEncoder`` encodes Erbsland strings, views, and UTF-8/UTF-16 character views into
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` data.
It is intentionally limited to Erbsland text sources.

.. code-block:: cpp

    auto bytes = el::StringEncoder{text}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject);

Extension libraries can support additional Erbsland-compatible source types by specializing
:cpp:struct:`StringEncoderTraits <erbsland::text::StringEncoderTraits>`.

String Kind
-----------

:cpp:enum:`StringKind <erbsland::text::StringKind>` selects the concrete string encoding used by generic text
construction APIs.
Use it when user code should decide whether a result is built as :cpp:class:`U8String <erbsland::text::U8String>`,
:cpp:class:`U16String <erbsland::text::U16String>`, or :cpp:class:`U32String <erbsland::text::U32String>`.

Usage
~~~~~

Pass :cpp:enum:`StringKind <erbsland::text::StringKind>` to :cpp:class:`StringBuilder <erbsland::text::StringBuilder>`
when you need an empty builder for a specific output encoding.

.. code-block:: cpp

    auto utf8 = el::StringBuilder{el::StringKind::U8};
    auto utf16 = el::StringBuilder{el::StringKind::U16};
    auto utf32 = el::StringBuilder{el::StringKind::U32};

Standalone Conversion Helpers
-----------------------------

The ``toString()`` overloads convert common text, numeric, boolean, and ordering values into the common
:cpp:type:`String <erbsland::text::String>` type.
Boolean conversion accepts :cpp:class:`BooleanFormat <erbsland::text::BooleanFormat>`.
The ``std::strong_ordering`` overload returns ``less``, ``equal``, or ``greater``.

Interface
=========

.. doxygenclass:: erbsland::text::AnyString
    :members:
.. doxygenclass:: erbsland::text::AnyStringView
    :members:
.. doxygenclass:: erbsland::text::EncodingError
    :members:
.. doxygenenum:: erbsland::text::EncodingErrorMode
.. doxygenclass:: erbsland::text::StringConverter
    :members:
.. doxygenclass:: erbsland::text::StringDecoder
    :members:
.. doxygenclass:: erbsland::text::StringEncoder
    :members:
.. doxygenenum:: erbsland::text::StringEncoding
.. doxygenenum:: erbsland::text::StringKind
.. doxygenfunction:: erbsland::text::toString(const String &value) -> String

.. doxygenfunction:: erbsland::text::toString(const StringView &value) -> String

.. doxygenfunction:: erbsland::text::toString(bool value, BooleanFormat format = BooleanFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(std::strong_ordering value) -> String

.. doxygenfunction:: erbsland::text::toString(int8_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(int16_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(int32_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(int64_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(uint8_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(uint16_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(uint32_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(uint64_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(float value, FloatFormat format = FloatFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(double value, FloatFormat format = FloatFormat::defaultFormat()) -> String
.. doxygenclass:: erbsland::text::U16EncodingError
    :members:
.. doxygenclass:: erbsland::text::U32EncodingError
    :members:
.. doxygenclass:: erbsland::text::U8EncodingError
    :members:
