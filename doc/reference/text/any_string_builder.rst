.. index::
    single: Any String Builder

******************
Any String Builder
******************

Introduction
============

:cpp:class:`AnyStringBuilder <erbsland::text::AnyStringBuilder>` builds UTF-8, UTF-16, or UTF-32 strings through one
decoded-character API.
It is useful for low-level algorithms that should not care which final string encoding is requested.

The builder stores one owning string internally.
The target encoding is selected with :cpp:enum:`StringKind <erbsland::text::StringKind>`.
A default builder uses ``StringKind::U8``, matching the library's default :cpp:type:`String <erbsland::text::String>`
encoding.

Basic Usage
===========

Choose the target kind when you create the builder, append decoded characters or Erbsland Core strings, and convert the final
result to the string type you need.
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
==========================

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
==============

:cpp:func:`append() <erbsland::text::AnyStringBuilder::append>` accepts :cpp:class:`Char <erbsland::text::Char>`, repeated :cpp:class:`Char <erbsland::text::Char>` values, Erbsland Core string
views, and Erbsland Core string literals.

When the source encoding matches the builder kind, the builder uses the existing owning string append operation.
This preserves the same behavior as ``U8StringEditor::append()``, ``U16StringEditor::append()``, and
``U32StringEditor::append()``.

When the source encoding differs from the builder kind, the source is decoded tolerantly and appended character by
character.
Malformed encoded data becomes ``Char::replacement()`` during this cross-encoding append.

Appending Integers
==================

:cpp:func:`appendInteger() <erbsland::text::AnyStringBuilder::appendInteger>` appends an integer using
:cpp:class:`IntegerFormat <erbsland::text::IntegerFormat>`.
It uses the same formatting implementation as ``U8String::fromInteger()``, ``U16String::fromInteger()``, and
``U32String::fromInteger()``.

.. code-block:: cpp

    auto builder = el::AnyStringBuilder{};
    builder.appendInteger(255, el::IntegerFormat::hexadecimal().setFlags(el::IntegerFormatFlag::BasePrefix));

Appending Byte Blocks
=====================

``appendByteBlock()`` appends a :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` as hexadecimal text.
It uses ``ByteFormat`` for compact hash strings, separated byte lists, or multi-line memory dumps.

.. code-block:: cpp

    auto builder = el::AnyStringBuilder{};
    builder.appendByteBlock(hashBytes);
    builder.append(U'\n');
    builder.appendByteBlock(memory, el::ByteFormat::memoryDump());

Taking or Copying the Result
============================

The unsuffixed ``to*String()`` methods return a completed read-only value and keep the builder usable.
For a matching target kind this is a cheap copy-on-write string value.

The unsuffixed ``take*String()`` methods return a completed read-only value and reset the builder to an empty string of
its original kind.
When the requested result matches the builder kind, the internal storage is moved out.

Use an explicit ``to*StringEditor()`` or ``take*StringEditor()`` method only when the caller intentionally continues
with in-place editing or local mutable construction.

Copying Builders
================

Copying a builder copies the builder state.
The copied builders may initially share copy-on-write string storage, but later append operations are independent.

.. code-block:: cpp

    auto first = el::AnyStringBuilder{};
    first.append(u8"Hei"_el);

    auto second = first;
    second.append(U'!');

    auto a = first.toU8String();  // "Hei"
    auto b = second.toU8String(); // "Hei!"

Interface
=========

.. doxygenclass:: erbsland::text::AnyStringBuilder
    :members:
