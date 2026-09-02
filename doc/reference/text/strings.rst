..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Strings; Reference
    single: String Collections; Reference
    single: String Types
    single: String Width Variants
    single: Unicode Normalization
    single: NormalizationForm
    single: String Literals
    single: Any String Builder
    single: String Collections
    single: String Width Collections
    single: String Iterators
    single: String Tree
    single: Standard Library Compatibility

***********************
Strings and Collections
***********************

String Types
============

Introduction
------------

:cpp:type:`String <erbsland::text::String>` is the primary string type used throughout the library, defined as
:cpp:class:`U8String <erbsland::text::U8String>`.
It stores an owning, read-only UTF-8 value with copy-on-write storage.

Use :cpp:type:`StringEditor <erbsland::text::StringEditor>` as a local mutable working value for explicit in-place
editing or small construction tasks.
For parameters, read-only storage, and ordinary transformations, prefer ``String`` and its copy-returning operations.

The practical workflow and allocation trade-offs are described in :doc:`/topics/text/overview`,
:doc:`/topics/text/transforming_strings`, and :doc:`/topics/text/editing_strings_in_place`.

For a full description of the underlying type, see the string-width variants below.

String Width Variants
=====================

.. _u8-string-storage-management:
.. _u8-string-byte-based-reading:
.. _u8-string-advance-retreat:
.. _u8-string-character-indexed-reading:
.. _u8-string-indexed-sequential-read:
.. _u16-string-storage-management:
.. _u16-string-code-unit-based-reading:
.. _u16-string-advance-retreat:
.. _u16-string-character-indexed-reading:
.. _u16-string-indexed-sequential-read:
.. _u32-string-storage-management:
.. _u32-string-code-unit-based-reading:
.. _u32-string-advance-retreat:
.. _u32-string-character-indexed-reading:
.. _u32-string-indexed-sequential-read:

Indexed Character Access
------------------------

The UTF-8 and UTF-16 read-only and editor types support direct character access by native data index and by
``unit::CpIndex``.
Native data-index access reads from the given byte or UTF-16 data position.
``unit::CpIndex`` access is a convenience for small offsets and may be slow for large strings, because the
implementation must iterate from the start to find the requested character position.

All indexed character access follows the ``charAt`` signal behavior: the exact end position returns
``Char::endOfData()``, invalid or past-end positions return ``Char::noCodePoint()``, and malformed encoded data is
decoded as ``Char::replacement()``.

Indexed Sequential Reads
------------------------

``readCharAndAdvance(index)`` reads the character at a native data index and advances the index to the position after
the decoded character.
At the exact end position, it returns ``Char::endOfData()`` and leaves the index unchanged.
For ``noIndex`` or a past-end index, it returns ``Char::noCodePoint()`` and leaves the index unchanged.

Malformed encoding is returned as ``Char::replacement()`` and advances according to the tolerant decoding rules.
Call ``isValidUtf8()`` before the read loop when a UTF-8-only parser must reject malformed internal text.

``readCharAndRetreat(index)`` treats the index as the position after the character to read.
It reads the previous character and retreats the index to that character's start.
This works with an index initialized from ``indexAt(StringSide::Back)`` to read backwards from the end of a string.
At zero, it returns ``Char::endOfData()`` and leaves the index unchanged.
For ``noIndex`` or a past-end index, it returns ``Char::noCodePoint()`` and leaves the index unchanged.
Unlike ``retreat(index)``, this method does not clamp a past-end index to the end before reading.

Character-Indexed Slices
------------------------

The UTF-8, UTF-16 and UTF-32 read-only and editor types support direct slicing by ``unit::CpRange`` and by
``StringSide`` with ``unit::CpLength``.
For UTF-8 and UTF-16, character-indexed slices return ranges aligned to decoded code-point boundaries, while the native
``ByteRange`` and ``U16DataRange`` overloads remain available for raw data-unit slices.
Trailing character slices are found from the back of the native data, so requesting the last few code points does not
require counting the entire string first.
Zero-length, invalid, or out-of-bounds ranges return empty strings.
Side-based slices with zero length return an empty string, while infinite length returns the entire string.

Display Width
-------------

``displayWidth()`` returns the approximate display width of a string by summing the decoded
:cpp:class:`Char <erbsland::text::Char>` display widths.
Unicode control characters, including line breaks, contribute ``0``.

This is intentionally a simple per-code-point measurement.
It does not perform line layout, grapheme-cluster shaping, bidirectional reordering, emoji ZWJ sequence handling, or
terminal/font-specific corrections.
For text containing line breaks, the result is usually not the width of any rendered line.

Unicode Normalization
---------------------

The read-only and editor types for all three widths support NFC, NFD, NFKC, and NFKD normalization through an explicit
:cpp:enum:`NormalizationForm <erbsland::text::NormalizationForm>` argument.
Read :doc:`/topics/text/normalizing_strings` for guidance about choosing a form, compatibility-changing behavior,
malformed input, storage reuse, and concatenation.

``normalized(form)`` returns a read-only value of the same width and is the preferred operation in application code.
Editors also expose ``normalize(form)`` for an explicit in-place editing workflow.
They should remain local mutable working values rather than default parameter or read-only storage types.
When valid text is already in the requested form, these methods preserve the original shared allocation.
Normalization uses constant bounded working memory and creates replacement storage only after the first changed
sequence.
After decomposition, a canonical sequence with more than 30 consecutive non-starters is replaced completely with one
U+FFFD as a defensive input limit.

Sensitive UTF-8 Storage
-----------------------

``U8String`` and ``U8StringEditor`` can mark their shared allocation with ``markAsSensitive()``.
The mark is one-way and is visible to every alias of the same allocation.
Copies, slices, trims, and same-string modified results preserve it, while inserting marked text into an ordinary
destination does not change that destination.
Conversions to another string width, encoded data, standard-library strings, escaped text, formatted text, and
diagnostics produce ordinary unmarked results.

Marking a non-empty literal first materializes shared storage.
Storage-less empty strings remain unmarked.
Marked allocations are securely erased when replaced or finally released.
This facility is best-effort storage hygiene rather than a high-security container or information-flow policy.

Searching
---------

All string ``find...`` overloads that accept a start or end position treat a no-index position as invalid input and
return the matching ``noIndex()`` value immediately.

Boolean Conversion
------------------

Every read-only and editor string width provides ``toBoolean(defaultValue)`` and ``toBooleanOrThrow()``.
Both recognize the complete ASCII-case-insensitive ELCL literals ``true``, ``on``, ``yes``, ``enabled``, ``false``,
``off``, ``no``, and ``disabled``.
Empty input, surrounding whitespace, partial matches, and all other text are invalid.
``toBoolean()`` returns its supplied default for invalid text, while ``toBooleanOrThrow()`` raises
:cpp:class:`ParseError <erbsland::err::ParseError>`.

String Literals
===============

Introduction
------------

The literal helpers in ``erbsland::text::literals`` are the preferred way to write static UTF-8 text in Erbsland Core
code.
They keep the type of the literal visible, avoid unsafe pointer-and-size pairs in user code, and let read-only APIs
refer directly to the original literal storage.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    constexpr auto label = u8"Status"_el;        // U8StringLiteral<char8_t>
    auto labelText = el::U8String{label};         // owning read-only value
    auto labelEditor = el::U8StringEditor{label}; // begin an explicit mutable workflow
    labelEditor.append(u8": ready"_el);

Use ``"_el"`` when you want a constexpr-capable :cpp:class:`U8StringLiteral <erbsland::text::U8StringLiteral>`.
Pass the literal directly when an API accepts it.
Otherwise construct :cpp:class:`U8String <erbsland::text::U8String>` explicitly.
Construct :cpp:class:`U8StringEditor <erbsland::text::U8StringEditor>` when the literal begins an explicit in-place edit
or local construction workflow.

Any String Builder
==================

Introduction
------------

:cpp:class:`AnyStringBuilder <erbsland::text::AnyStringBuilder>` builds UTF-8, UTF-16, or UTF-32 strings through one
decoded-character API.
It is useful for low-level algorithms that should not care which final string encoding is requested.

The builder stores one owning string internally.
The target encoding is selected with :cpp:enum:`StringKind <erbsland::text::StringKind>`.
A default builder uses ``StringKind::U8``, matching the library's default :cpp:type:`String <erbsland::text::String>`
encoding.

Basic Usage
-----------

Choose the target kind when you create the builder, append decoded characters or Erbsland Core strings, and convert the
final result to the string type you need.
Single decoded code points can be appended as either :cpp:class:`Char <erbsland::text::Char>` or ``char32_t``.

.. code-block:: cpp

    auto builder = el::AnyStringBuilder{el::StringKind::U16};

    builder.append(U'4');
    builder.append(U'2');
    builder.append(u8" cafés"_el);

    auto result = builder.toU16String();

:cpp:func:`length() <erbsland::text::AnyStringBuilder::length>` returns the cached decoded code-point length.
It is therefore independent of the target encoding's byte or code-unit size.

Choosing Kind and Capacity
--------------------------

Use the static factories when the target kind should be visible at the creation site.
The factories without a capacity are equivalent to the matching :cpp:enum:`StringKind <erbsland::text::StringKind>`
constructor.

.. code-block:: cpp

    auto u8 = el::AnyStringBuilder::u8();
    auto u16 = el::AnyStringBuilder::u16();
    auto u32 = el::AnyStringBuilder::u32();

When the final size is known approximately, choose the capacity factory once before appending.
The encoding-specific factories use the native storage unit of the target string.

.. code-block:: cpp

    auto bytes = el::AnyStringBuilder::u8(el::ByteLength{1024});
    auto words = el::AnyStringBuilder::u16(el::U16DataLength{512});
    auto codePoints = el::AnyStringBuilder::u32(el::CpLength{256});

:cpp:func:`withCapacity() <erbsland::text::AnyStringBuilder::withCapacity>` is the generic form for code that only has
a :cpp:enum:`StringKind <erbsland::text::StringKind>`.
Its capacity is a decoded code-point count.
For UTF-8 it reserves up to four bytes per code point, and for UTF-16 it reserves up to two code units per code point.
This can reserve more storage than eventually needed, but it keeps the kind-erased API safe and unambiguous.

Appending Text
--------------

:cpp:func:`append() <erbsland::text::AnyStringBuilder::append>` accepts :cpp:class:`Char <erbsland::text::Char>`, repeated :cpp:class:`Char <erbsland::text::Char>` values, Erbsland Core string
views, and Erbsland Core string literals.

When the source encoding matches the builder kind, the builder uses the existing owning string append operation.
This preserves the same behavior as ``U8StringEditor::append()``, ``U16StringEditor::append()``, and
``U32StringEditor::append()``.

When the source encoding differs from the builder kind, the source is decoded tolerantly and appended character by
character.
Malformed encoded data becomes ``Char::replacement()`` during this cross-encoding append.

Appending Integers
------------------

:cpp:func:`appendInteger() <erbsland::text::AnyStringBuilder::appendInteger>` appends an integer using
:cpp:class:`IntegerFormat <erbsland::text::IntegerFormat>`.
It uses the same formatting implementation as ``U8String::fromInteger()``, ``U16String::fromInteger()``, and
``U32String::fromInteger()``.

.. code-block:: cpp

    auto builder = el::AnyStringBuilder{};
    builder.appendInteger(255, el::IntegerFormat::hexadecimal().setFlags(el::IntegerFormatFlag::BasePrefix));

Appending Byte Blocks
---------------------

``appendByteBlock()`` appends a :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` as hexadecimal text.
It uses ``ByteFormat`` for compact hash strings, separated byte lists, or multi-line memory dumps.

.. code-block:: cpp

    auto builder = el::AnyStringBuilder{};
    builder.appendByteBlock(hashBytes);
    builder.append(U'\n');
    builder.appendByteBlock(memory, el::ByteFormat::memoryDump());

Taking or Copying the Result
----------------------------

The unsuffixed ``to*String()`` methods return a completed read-only value and keep the builder usable.
For a matching target kind this is a cheap copy-on-write string value.

The unsuffixed ``take*String()`` methods return a completed read-only value and reset the builder to an empty string of
its original kind.
When the requested result matches the builder kind, the internal storage is moved out.

Use an explicit ``to*StringEditor()`` or ``take*StringEditor()`` method only when the caller intentionally continues
with in-place editing or local mutable construction.

Copying Builders
----------------

Copying a builder copies the builder state.
The copied builders may initially share copy-on-write string storage, but later append operations are independent.

.. code-block:: cpp

    auto first = el::AnyStringBuilder{};
    first.append(u8"Hei"_el);

    auto second = first;
    second.append(U'!');

    auto a = first.toU8String();  // "Hei"
    auto b = second.toU8String(); // "Hei!"

String Collections
==================


String Width Collections
========================


String Iterators
================


String Tree
===========


Standard Library Compatibility
==============================

Interface
=========

.. doxygenclass:: erbsland::text::AnyString
    :members:
.. doxygenclass:: erbsland::text::AnyStringBuilder
    :members:
.. doxygenclass:: erbsland::text::AnyStringEditor
    :members:
.. doxygenfunction:: erbsland::text::literals::operator""_el(const char *data, const std::size_t size) noexcept -> U8StringLiteral<char>

.. doxygenfunction:: erbsland::text::literals::operator""_el(const char8_t *data, const std::size_t size) noexcept -> U8StringLiteral<char8_t>

.. doxygenfunction:: erbsland::text::literals::operator""_el(const char16_t *data, const std::size_t size) noexcept -> U16StringLiteral

.. doxygenfunction:: erbsland::text::literals::operator""_el(const char32_t *data, const std::size_t size) noexcept -> U32StringLiteral
.. doxygenenum:: erbsland::text::NormalizationForm
.. doxygentypedef:: erbsland::text::String
.. doxygenenum:: erbsland::text::StringBomMode
.. doxygentypedef:: erbsland::text::StringCIHashMap
.. doxygentypedef:: erbsland::text::StringCIHashSet
.. doxygentypedef:: erbsland::text::StringCIMap
.. doxygentypedef:: erbsland::text::StringCISet
.. doxygentypedef:: erbsland::text::StringEditor
.. doxygentypedef:: erbsland::text::StringEditorList
.. doxygentypedef:: erbsland::text::StringHashMap
.. doxygentypedef:: erbsland::text::StringHashSet
.. doxygentypedef:: erbsland::text::StringList
.. doxygentypedef:: erbsland::text::StringLiteral
.. doxygentypedef:: erbsland::text::StringMap
.. doxygentypedef:: erbsland::text::StringSet
.. doxygenclass:: erbsland::text::StringTree
    :members:
.. doxygenclass:: erbsland::text::U16String
    :members:
.. doxygentypedef:: erbsland::text::U16StringCIHashMap
.. doxygentypedef:: erbsland::text::U16StringCIHashSet
.. doxygentypedef:: erbsland::text::U16StringCIMap
.. doxygentypedef:: erbsland::text::U16StringCISet
.. doxygenclass:: erbsland::text::U16StringConstIterator
    :members:
.. doxygenclass:: erbsland::text::U16StringEditor
    :members:
.. doxygentypedef:: erbsland::text::U16StringEditorList
.. doxygentypedef:: erbsland::text::U16StringHashMap
.. doxygentypedef:: erbsland::text::U16StringHashSet
.. doxygentypedef:: erbsland::text::U16StringList
.. doxygenclass:: erbsland::text::U16StringLiteral
    :members:
.. doxygentypedef:: erbsland::text::U16StringMap
.. doxygentypedef:: erbsland::text::U16StringSet
.. doxygenclass:: erbsland::text::U32String
    :members:
.. doxygentypedef:: erbsland::text::U32StringCIHashMap
.. doxygentypedef:: erbsland::text::U32StringCIHashSet
.. doxygentypedef:: erbsland::text::U32StringCIMap
.. doxygentypedef:: erbsland::text::U32StringCISet
.. doxygenclass:: erbsland::text::U32StringConstIterator
    :members:
.. doxygenclass:: erbsland::text::U32StringEditor
    :members:
.. doxygentypedef:: erbsland::text::U32StringEditorList
.. doxygentypedef:: erbsland::text::U32StringHashMap
.. doxygentypedef:: erbsland::text::U32StringHashSet
.. doxygentypedef:: erbsland::text::U32StringList
.. doxygenclass:: erbsland::text::U32StringLiteral
    :members:
.. doxygentypedef:: erbsland::text::U32StringMap
.. doxygentypedef:: erbsland::text::U32StringSet
.. doxygenclass:: erbsland::text::U8String
    :members:
.. doxygentypedef:: erbsland::text::U8StringCIHashMap
.. doxygentypedef:: erbsland::text::U8StringCIHashSet
.. doxygentypedef:: erbsland::text::U8StringCIMap
.. doxygentypedef:: erbsland::text::U8StringCISet
.. doxygenclass:: erbsland::text::U8StringConstIterator
    :members:
.. doxygenclass:: erbsland::text::U8StringEditor
    :members:
.. doxygentypedef:: erbsland::text::U8StringEditorList
.. doxygentypedef:: erbsland::text::U8StringHashMap
.. doxygentypedef:: erbsland::text::U8StringHashSet
.. doxygentypedef:: erbsland::text::U8StringList
.. doxygenclass:: erbsland::text::U8StringLiteral
    :members:
.. doxygentypedef:: erbsland::text::U8StringMap
.. doxygentypedef:: erbsland::text::U8StringSet
