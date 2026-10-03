..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Strings; Reference
    single: String Collections; Reference
    single: String Types
    single: String Transformations
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

The practical workflow and allocation trade-offs are described in :doc:`/topics/text_strings/overview`,
:doc:`/topics/text_strings/transforming_strings`, and :doc:`/topics/text_strings/editing_strings_in_place`.

For a full description of the underlying type, see the string-width variants below.

String Joins
============

Include ``<erbsland/StringJoins.hpp>`` for ``el::join(a, b, ...)`` or ``el::join({a, b, ...})``.
These functions concatenate a fixed number of Core strings, editors, and literals without separators.
All inputs must have the same encoding; UTF-8 ``char`` and ``char8_t`` literals can be combined.
The result is an owning read-only ``String``, ``U16String``, or ``U32String``.
A single input shares its storage with the result, including slices.
Multiple inputs use the existing checked ``fromJoined`` implementation, which plans the final size before copying.
UTF-8 results retain sensitivity from every input, including empty sensitive fragments.

Empty calls, mixed encodings, standard strings, raw character pointers, and runtime-width strings are not supported.
The braced form uses a fixed array extent; runtime collections continue to use their existing ``join`` methods.
Inputs are not modified, and results remain valid after temporary strings or editors are destroyed.
``fromJoined`` remains available for explicitly selected string types.

.. _string-transformations:

String Transformations
======================

Include ``<erbsland/StringTransformations.hpp>`` for free functions such as ``el::toAsciiLowercase(text)``.
They accept concrete UTF-8, UTF-16, and UTF-32 strings and editors and always return a read-only string of the same
encoding.
Convert the result explicitly to an editor when further editing is required.
The templates do not accept literals, standard strings, or type-erased strings directly.
Unchanged results share source storage, including slices; changed UTF-8 results retain the source sensitivity mark.

Case Conversion
---------------

``toLowercase()``, ``toUppercase()``, and ``toCaseFolded()`` use the existing simple one-code-point Unicode mappings.
They do not perform locale-dependent casing, multi-character expansions, or Unicode normalization.
``toAsciiLowercase()`` and ``toAsciiUppercase()`` affect only ASCII letters and do not require Unicode tables.
``toIdentifierNormalized()`` lowercases ASCII letters and replaces U+0020 spaces with underscores.

``toCapitalized()`` uppercases the first code point and lowercases every remaining code point.
It treats the complete input as one word: ``hELLO wORLD`` becomes ``Hello world``.
Punctuation at the beginning still occupies the first position: ``!HELLO`` becomes ``!hello``.
``toAsciiCapitalized()`` applies the same rule using ASCII mappings.
``toCapitalization(text, capitalization)`` and ``toAsciiCapitalization()`` select these operations through
``Capitalization::Lowercase``, ``Uppercase``, or ``Capitalized``.

Word Conversion
---------------

``toCamelCaseFromWords(text, wordSeparators, capitalization)`` removes characters in the supplied ``CharSet``.
Without capitalization, it preserves the first nonempty word and uppercases the first code point of each later word.
Repeated and edge separators produce no empty words.
Other characters and remaining letter casing are preserved.
For underscore separators, ``foo_HTTP_bar`` becomes ``fooHTTPBar``.
With explicit capitalization, every word follows the selected rule, including the first: the results are ``FooHttpBar``,
``foohttpbar``, and ``FOOHTTPBAR`` for capitalized, lowercase, and uppercase respectively.

``toWordsFromCamelCase(text, wordSeparator, capitalization)`` inserts the given character at camel-case boundaries.
A boundary precedes an uppercase letter after a lowercase letter or digit, or the final uppercase letter of an acronym
followed by a lowercase letter.
Examples are ``HTTPServer`` → ``HTTP Server``, ``URL2Value`` → ``URL2 Value``, and ``version2`` → ``version2``.
Optional capitalization applies independently to each resulting word.
Boundaries use adjacent decoded code points, including Unicode uppercase/titlecase letters, lowercase letters, and
decimal digits.
Combining marks and other characters are preserved and do not themselves create boundaries.

The corresponding ``toAsciiCamelCaseFromWords()`` and ``toAsciiWordsFromCamelCase()`` functions use ASCII classification
and case mapping throughout.
For example, lowercase underscore conversion of ``éValue`` produces ``évalue`` with ASCII rules and ``é_value`` with
Unicode rules.
These functions assume validated, prepared text; they do not sanitize identifiers or normalize their encoding.
An explicit output separator must be a valid Unicode scalar value.

Both directions offer overloads omitting the separator argument while retaining optional capitalization.
Word joining defaults to ASCII whitespace for the ASCII variant and Unicode whitespace for the Unicode variant.
Underscores and hyphens require explicit separators.
Camel-case splitting defaults to U+0020 space.

.. code-block:: cpp

    const auto words = el::String{"HTTP server name"_el};
    const auto typeName = el::toCamelCaseFromWords(words, el::Capitalization::Capitalized); // HttpServerName
    const auto heading = el::toWordsFromCamelCase(typeName); // Http Server Name
    const auto key = el::toAsciiWordsFromCamelCase(typeName, U'_', el::Capitalization::Lowercase); // http_server_name

Whitespace Simplification
-------------------------

``toWhitespaceSimplified(text, wordSeparator)`` trims surrounding whitespace and replaces each internal run with one
separator, defaulting to U+0020 space.
Empty and whitespace-only inputs produce an empty string.
``toAsciiWhitespaceSimplified()`` recognizes U+0020 and U+0009 through U+000D.
The Unicode variant additionally recognizes U+0085 and the Unicode space, line, and paragraph separator categories.
Zero-width space and byte-order marks are not whitespace under these rules.
These are also the default separator sets for joining words into camel case.

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

Storage Bounds
--------------

Violating storage capacity, either by supplying unrealistic parameters or by exceeding ressource limits is a fatal non
recoverable failure.
It results either in a program termination, a ``ParameterFailure`` or ``OutOfRangeFailure`` exception.

Encoding Validation
-------------------

All UTF-8, UTF-16, and UTF-32 strings, editors, and literals expose ``isEncodingValid()`` and
``encodingValidOrThrow()``.
``AnyString`` and ``AnyStringEditor`` provide the same methods for their current encoding.
Both inspect only the visible storage range without changing it; empty strings are valid.

Use ``isEncodingValid()`` for a non-throwing Boolean result.
Use ``encodingValidOrThrow()`` at boundaries where invalid text should interrupt processing.
It returns ``void`` on success and throws ``U8EncodingError``, ``U16EncodingError``, or ``U32EncodingError`` on failure.
The exception provides the reason and the native index where the malformed data was detected: a byte index for UTF-8, a
code-unit index for UTF-16, or a code-point index for UTF-32. Indexes are relative to the visible string, including when
validating a slice.
A truncated UTF-8 sequence can report the end position where another byte was required.

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
Call ``encodingValidOrThrow()`` before the read loop when a parser must reject malformed internal text.

``readCharAndRetreat(index)`` treats the index as the position after the character to read.
It reads the previous character and retreats the index to that character's start.
This works with an index initialized from ``indexAt(StringSide::Back)`` to read backwards from the end of a string.
At zero, it returns ``Char::endOfData()`` and leaves the index unchanged.
For ``noIndex`` or a past-end index, it returns ``Char::noCodePoint()`` and leaves the index unchanged.
Unlike ``retreat(index)``, this method does not clamp a past-end index to the end before reading.

Character-Indexed Slices
------------------------

The UTF-8, UTF-16 and UTF-32 read-only and editor types support direct slicing by ``unit::CpRange`` and by
``StringSide`` with ``unit::CpCount``.
For UTF-8 and UTF-16, character-indexed slices return ranges aligned to decoded code-point boundaries, while the native
``ByteRange`` and ``U16Range`` overloads remain available for raw data-unit slices.
Trailing character slices are found from the back of the native data, so requesting the last few code points does not
require counting the entire string first.
Zero-length, invalid, or out-of-bounds ranges return empty strings.
Side-based slices with zero length return an empty string, while infinite length returns the entire string.

.. _string-separator-splitting:

Splitting at a Separator
------------------------

``splitAtFirst(separator)`` and ``splitAtLast(separator)`` split at the first or rightmost matching separator.
They return ``std::optional<std::pair<T, T>>``, where ``T`` is the receiver's string or editor type.
All six UTF-8, UTF-16, and UTF-32 types provide these operations as ``const noexcept`` methods.
A successful result contains the text before and after the match, excluding the complete separator.

.. code-block:: cpp

    const auto path = String{"archive/report.txt"_el};
    if (const auto parts = path.splitAtLast("/"_el)) {
        const auto &[directory, filename] = *parts;
        // directory == "archive", filename == "report.txt"
    }

    const auto fields = String{"name###value###tail"_el}.splitAtFirst("###"_el);
    // fields->first == "name", fields->second == "value###tail"

    const auto overlap = String{"aaa"_el}.splitAtLast("aa"_el);
    // overlap->first == "a", overlap->second is empty

Missing separators, empty string separators, and empty character sets return ``std::nullopt``.
Leading, trailing, and whole-input matches succeed with one or both halves empty.
The optional result therefore distinguishes an absent delimiter from a present delimiter with an empty field.
Rightmost matches include overlapping occurrences.

The string overload accepts the corresponding read-only string type and an optional ``CharCompareFn``.
It compares decoded code points using the same tolerant decoding as ordinary string searches.
The callback receives the source character first and the separator character second.
Matching retains the actual source boundaries, even when equivalent source and separator characters occupy different
numbers of native data units.
For example, case-folded matching can split a UTF-8 source containing ``K`` using the separator ``k``.
No normalization or locale collation is implicit.

The ``CharSet`` overload matches exactly one decoded character belonging to the set, using direct membership.
It does not consume a run of separators or accept a comparison callback.
For example, splitting ``"a//b"`` at the first slash returns ``"a"`` and ``"/b"``.

Nonempty halves share the original storage without copying text, including when the receiver is already a slice.
Editor results detach independently on mutation.
Malformed native units outside the matched separator are preserved unchanged.
UTF-8 results preserve the source's sensitivity mark, including empty halves.
Long-separator preprocessing and the existing handling of empty results may allocate.

Use :doc:`matching_and_splitting` for sequential ``StringSplitter`` traversal when every part is needed.

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
Read :doc:`/topics/text_strings/normalizing_strings` for guidance about choosing a form, compatibility-changing
behavior, malformed input, storage reuse, and concatenation.

``normalized(form)`` returns a read-only value of the same width and is the preferred operation in application code.
Editors also expose ``normalize(form)`` for an explicit in-place editing workflow.
They should remain local mutable working values rather than default parameter or read-only storage types.
When valid text is already in the requested form, these methods preserve the original shared allocation.
Normalization uses constant bounded working memory and creates replacement storage only after the first changed
sequence.
After decomposition, a canonical sequence with more than 30 consecutive non-starters is replaced completely with one
U+FFFD as a defensive input limit.

Unchanged Results and Pruned Copies
-----------------------------------

``removed()``, ``removedFirst()``, ``removedAll()``, ``inserted()``, ``replaced()``, ``replacedFirst()``,
``replacedAll()``, ``truncated()``, ``aligned()``, and ``toEscaped()`` retain the original storage when the resulting
native data units are unchanged.
The behavior applies to all three string widths and their editor types, including literal and sliced storage.
Empty searches or character sets, missing matches, empty insertions, and identical replacements preserve storage.
Invalid ranges and no-index insertion positions also preserve storage when they leave the text unchanged.
Truncation preserves storage when the text already fits, and alignment does so when no padding is required or the fill
character is invalid.
Replacement output is created only at the first changed match, and matching continues from the end of that match without
repeating the search.

Escaping preserves storage when disabled or when no character needs escaping.
With escaping enabled, malformed encoding is still replaced by U+FFFD, so repairing malformed data creates a changed
result even when no escape sequence is needed.
UTF-8 escape results preserve the source's sensitivity mark, including changed results.

``kept()`` deliberately returns a pruned copy of the selected range, releasing the result from the original backing
allocation.
Every non-empty result has separate storage, even when the range covers the whole string.
Use ``slice()`` when the result should share that allocation.
See :doc:`/topics/text_strings/transforming_strings` for replacement and selection examples.

Sensitive UTF-8 Storage
-----------------------

``U8String`` and ``U8StringEditor`` can mark their shared allocation with ``markAsSensitive()``.
The mark is one-way and is visible to every alias of the same allocation.
Copies, slices, trims, escaped text, and modified results preserve it.
Appending, inserting, replacing, and joining marked text mark the UTF-8 destination before copying the secret bytes.
UTF-8 builders and formatters preserve marked string arguments.
Conversions to another string width, encoded data, and standard-library strings produce ordinary unmarked results.
Custom formatting and diagnostic code remains responsible for any plaintext copies it creates.

Marking a literal or storage-less empty string materializes shared storage.
An explicitly marked empty result retains its mark, including through subsequent placeholder filters.
Marked allocations are securely erased when replaced or finally released.
This facility is best-effort storage hygiene rather than a high-security container or information-flow policy.

Comparison and Ordering
-----------------------

``compare()`` and the ordinary comparison operators order decoded code points, replacing malformed encoding with
``Char::replacement()``.
They perform no normalization or locale collation.
``compare()`` accepts an optional character comparator for explicit folding rules.
``AnyString::compare()`` compares different encodings without conversion.

``compareBinary()`` provides allocation-free ordering of visible unsigned storage bytes on every concrete string and
editor type, plus ``AnyString`` and ``AnyStringEditor``.
It preserves malformed bytes and uses native byte order for UTF-16/32. The wrappers order empty contents first, then
encoding (U8, U16, U32), then bytes.
This representation ordering can differ from decoded equality and is an explicit choice; existing operators and
containers retain their decoded comparison rules.
See :doc:`/topics/text_strings/comparing_strings` for examples, prefix rules, and portability considerations.

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

    auto bytes = el::AnyStringBuilder::u8(el::ByteCount{1024});
    auto words = el::AnyStringBuilder::u16(el::U16Count{512});
    auto codePoints = el::AnyStringBuilder::u32(el::CpCount{256});

Pass ``true`` as the second argument to ``u8()`` to allocate sensitive UTF-8 storage before appending text.
The builder keeps sensitive mode when its contents are taken and it is reused.
The :cpp:class:`AnyString <erbsland::text::AnyString>` wrapper exposes ``isSensitive()`` to inspect UTF-8 storage
without converting UTF-16 or UTF-32 text.

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

``StringList`` stores an ordered sequence of UTF-8 strings.
``StringMap`` and ``StringSet`` use decoded string ordering; ``StringHashMap`` and ``StringHashSet`` provide hashed
lookup with matching decoded equality and hashing.
Their ``CI`` variants use Unicode simple case folding.
Binary string comparison does not change these container contracts.

The U8, U16, and U32 collection families provide corresponding containers for each storage width.
The unsuffixed aliases select UTF-8, matching ``String``.
Use read-only strings as keys and stored values; editor lists are available for explicitly mutable working data.

String Iterators
================

Read-only strings and editors expose const iterators over decoded characters, including tolerant replacement of
malformed encoded data.
Use range-based iteration for character inspection and native data indexes when an algorithm needs encoding-specific
positions.
See :doc:`/topics/text_strings/using_strings` for the string access model.

String Tree
===========

``StringTree`` collects titled text, labeled values, and nested subtrees for diagnostics and debug views.
Append entries in their display order, then call ``toString()`` to render indented text.
It is a display structure rather than a lookup container.

Standard Library Compatibility
==============================

String values provide explicit constructors for the corresponding standard string and string-view types.
Use ``StringConverter`` at boundaries that require standard-library strings or a different encoding.
Keep Erbsland string values within library code so storage sharing, tolerant decoding, and typed indexes remain
consistent.

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
.. doxygenfunction:: erbsland::text::join(const tFirst &first, const tRest &...rest) -> impl::JoinedStringFor<tFirst>

.. doxygenfunction:: erbsland::text::join(const U8String (&parts)[tSize]) -> U8String

.. doxygenfunction:: erbsland::text::join(const U16String (&parts)[tSize]) -> U16String

.. doxygenfunction:: erbsland::text::join(const U32String (&parts)[tSize]) -> U32String
.. doxygentypedef:: erbsland::text::StringList
.. doxygentypedef:: erbsland::text::StringLiteral
.. doxygentypedef:: erbsland::text::StringMap
.. doxygentypedef:: erbsland::text::StringSet
.. doxygenfunction:: erbsland::text::toAsciiLowercase(const T &text) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toAsciiUppercase(const T &text) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toAsciiCapitalized(const T &text) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toAsciiCapitalization(const T &text, Capitalization capitalization) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toLowercase(const T &text) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toUppercase(const T &text) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toCapitalized(const T &text) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toCapitalization(const T &text, Capitalization capitalization) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toCaseFolded(const T &text) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toIdentifierNormalized(const T &text) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toAsciiCamelCaseFromWords(const T &text, const CharSet &wordSeparators, std::optional<Capitalization> capitalization = std::nullopt) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toAsciiCamelCaseFromWords(const T &text, std::optional<Capitalization> capitalization = std::nullopt) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toAsciiWordsFromCamelCase(const T &text, Char wordSeparator, std::optional<Capitalization> capitalization = std::nullopt) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toAsciiWordsFromCamelCase(const T &text, std::optional<Capitalization> capitalization = std::nullopt) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toAsciiWhitespaceSimplified(const T &text, Char wordSeparator = U' ') -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toCamelCaseFromWords(const T &text, const CharSet &wordSeparators, std::optional<Capitalization> capitalization = std::nullopt) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toCamelCaseFromWords(const T &text, std::optional<Capitalization> capitalization = std::nullopt) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toWordsFromCamelCase(const T &text, Char wordSeparator, std::optional<Capitalization> capitalization = std::nullopt) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toWordsFromCamelCase(const T &text, std::optional<Capitalization> capitalization = std::nullopt) -> impl::ReadOnlyStringFor<T>

.. doxygenfunction:: erbsland::text::toWhitespaceSimplified(const T &text, Char wordSeparator = U' ') -> impl::ReadOnlyStringFor<T>
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
