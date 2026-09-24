.. index::
    single: Memory; Byte blocks
    single: Byte block; Copy-on-write storage
    single: Binary data; Owning blocks

************************
Working with Byte Blocks
************************

Most binary data in an application needs a stable owner, but it does not need to be mutable everywhere it travels.
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` is the general-purpose owning value for that data, while
:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` provides a focused place to assemble or change it.
This page explains how the two types work together, how copy-on-write affects their cost, and how to read, edit, retain,
and compare byte data without giving up clear ownership.

One Storage, Several Values
===========================

Copying a :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` copies a small value that shares the underlying storage.
The bytes themselves are not copied.
This makes blocks inexpensive to pass by value, return from functions, and keep in several objects.

A :cpp:func:`slice() <erbsland::mem::ByteBlock::slice>` is equally inexpensive.
It owns a reference to the same allocation and merely selects a visible range within it.
Because the slice owns that reference, its bytes remain valid even after the block from which it was created has gone
out of scope.
This is an important difference from a borrowed :cpp:type:`ConstByteSpan <erbsland::mem::ConstByteSpan>`.

Sharing has two consequences worth keeping in mind.
It avoids eager copying, especially when a parser retains several fields from one input block, but a tiny slice also
keeps the complete original allocation alive.
Likewise, the first mutation through a shared :cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` must detach
its storage so existing read-only values remain unchanged.
That deferred copy is the "copy on write" in the design.

Use :cpp:func:`copy() <erbsland::mem::ByteBlock::copy>` when you need independent storage for all currently visible
bytes.
Use :cpp:func:`kept() <erbsland::mem::ByteBlock::kept>` when you already know the range to retain: it creates the same
result as ``block.slice(range).copy()``, but copies the selected range directly.
For short-lived parsing, a slice is usually the natural choice.
For a small field that will be cached long after a large packet or file is released, ``kept()`` prevents that large
allocation from remaining alive.

.. erbsland-demo::
    :source: mem/ByteBlock/SlicingBlocks.cpp
    :exec: mem/byte_block --demo SlicingBlocks
    :source-sha256: a20e4b20e86db6182590b37929fb0eedc038f18ff292ad6c70150b6e3c02622c

.. code-block:: cpp

    /// Choose between shared slices and independent byte blocks.
    ///
    /// `slice()` creates a cheap read-only view that keeps the complete shared
    /// allocation alive. `copy()` and `kept()` allocate only the visible bytes, which
    /// is useful when a small result must outlive a much larger source.
    void slicingBlocks() {
        const auto completeTree = el::ByteBlock{
            el::Byte{1U}, el::Byte{2U}, el::Byte{3U}, el::Byte{5U}, el::Byte{8U}, el::Byte{13U}, el::Byte{21U}};
        const auto branchRange = el::ByteRange{el::ByteIndex{2U}, el::ByteLength{3U}};

        // Share the original allocation when the source and branch have similar lifetimes.
        const auto sharedBranch = completeTree.slice(branchRange);

        // Keep only the selected bytes when the branch will be stored independently.
        const auto compactBranch = completeTree.kept(branchRange);
        const auto explicitCopy = sharedBranch.copy();

        el::io::printLine("Skill tree         : Arbre du veilleur"_el);
        el::io::printLine("Complete tree      : "_el, el::ByteFormat::separated(), completeTree);
        el::io::printLine("Shared branch      : "_el, el::ByteFormat::separated(), sharedBranch);
        el::io::printLine("Compact branch     : "_el, el::ByteFormat::separated(), compactBranch);
        el::io::printLine("Explicit copy      : "_el, el::ByteFormat::separated(), explicitCopy);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Skill tree         : Arbre du veilleur
    Complete tree      : 01 02 03 05 08 0d 15
    Shared branch      : 03 05 08
    Compact branch     : 03 05 08
    Explicit copy      : 03 05 08

.. erbsland-demo-end::

Choosing the Right Owner
========================

A :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` is the usual application-level byte value.
It is immutable from the holder's perspective, easy to share, and well suited to parameters, return values, stored
records, and parsing input.
A :cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` is its mutable companion for assembling small records or
making local changes.
Converting an editor to a block is implicit and shares the data; a later editor mutation detaches and cannot change the
block that was already handed out.

Compiled-in tables and resource data can start as a
:cpp:class:`ByteBlockLiteral <erbsland::mem::ByteBlockLiteral>`.
A block made from a literal refers directly to static storage, so copies and slices need neither a heap allocation nor a
byte copy.
Other byte containers solve different problems.
A :cpp:class:`ByteArray <erbsland::mem::ByteArray>` has a compile-time size and works well for fixed algorithm state.
A :cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` owns dynamic storage without copy-on-write, which is useful for a
mutable working buffer whose copies should always be independent.
:cpp:class:`RingBuffer <erbsland::mem::RingBuffer>` and
:cpp:class:`ByteRingBuffer <erbsland::mem::ByteRingBuffer>` model queued stream data with bounded capacity and
consuming reads; they are not general stored values or record builders.

Creating Blocks
===============

Both block types can start empty, contain a repeated byte, copy an initializer list, or copy a fixed
:cpp:class:`ByteArray <erbsland::mem::ByteArray>`.
The :cpp:func:`fromSpan() <erbsland::mem::ByteBlock::fromSpan>` factories make ownership explicit when bytes arrive
through an Erbsland or standard span.
:cpp:func:`fromVector() <erbsland::mem::ByteBlock::fromVector>` is the matching compatibility boundary for raw
``uint8_t`` and ``char`` vectors.

These factories copy their input, so the resulting block no longer depends on the source lifetime.
Passing a :cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` where a block is expected is different: the
implicit conversion shares its allocation and gives the receiver a stable read-only snapshot.

For compiled-in data, construct a :cpp:class:`ByteBlockLiteral <erbsland::mem::ByteBlockLiteral>` from a static
``constexpr`` array or span.
The literal constructors require constant evaluation, which prevents ordinary temporary or local storage from being
retained accidentally.
For a short sequence, ``ByteBlockLiteral::fromValues<...>()`` is the safe initializer-list equivalent because each value
pack owns static backing storage.
Converting either form to ``ByteBlock`` is implicit and does not copy its bytes.

.. erbsland-demo::
    :source: mem/ByteBlock/CreatingBlocks.cpp
    :exec: mem/byte_block --demo CreatingBlocks
    :source-sha256: c230b0f44830c4fdcad6ba5c46e78034cf20ee24524422615cce1fa428ecd853

.. code-block:: cpp

    /// Create owning byte blocks from common sources.
    ///
    /// `ByteBlock` is the read-only owning value for storing and passing binary data.
    /// `ByteBlockEditor` provides the same construction choices when the bytes must
    /// remain mutable, and converts implicitly to a read-only block without copying.
    void creatingBlocks() {
        static constexpr auto compiledSkills = el::ByteBlockLiteral::fromValues<34U, 55U, 89U>();

        // Create blocks with a repeated value and with individual skill bytes.
        const auto emptySlots = el::ByteBlock{el::ByteLength{4U}, el::Byte{0U}};
        const auto learnedSkills = el::ByteBlock{el::Byte{1U}, el::Byte{3U}, el::Byte{8U}};

        // Copy fixed arrays, borrowed spans, and standard vectors into owned blocks.
        const auto fixedSkills = el::ByteArray{el::Byte{2U}, el::Byte{5U}, el::Byte{13U}};
        const auto networkBytes = std::array<std::byte, 3>{std::byte{21U}, std::byte{34U}, std::byte{55U}};
        const auto savedBytes = std::vector<uint8_t>{89U, 144U};
        const auto fromArray = el::ByteBlock{fixedSkills};
        const auto fromSpan = el::ByteBlock::fromSpan(std::span{networkBytes});
        const auto fromVector = el::ByteBlock::fromVector(savedBytes);
        const el::ByteBlock fromLiteral = compiledSkills;

        // Build mutable data and hand it to an API expecting a read-only block.
        auto editor = el::ByteBlockEditor{el::Byte{1U}, el::Byte{2U}};
        editor.append(el::Byte{3U});
        const el::ByteBlock immutableSkills = editor;

        el::io::printLine("Character          : Gardienne des brumes"_el);
        el::io::printLine("Empty slots        : "_el, el::ByteFormat::separated(), emptySlots);
        el::io::printLine("Learned skills     : "_el, el::ByteFormat::separated(), learnedSkills);
        el::io::printLine("From fixed array   : "_el, el::ByteFormat::separated(), fromArray);
        el::io::printLine("From borrowed span : "_el, el::ByteFormat::separated(), fromSpan);
        el::io::printLine("From vector        : "_el, el::ByteFormat::separated(), fromVector);
        el::io::printLine("From static literal: "_el, el::ByteFormat::separated(), fromLiteral);
        el::io::printLine("Editor as block    : "_el, el::ByteFormat::separated(), immutableSkills);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Character          : Gardienne des brumes
    Empty slots        : 00 00 00 00
    Learned skills     : 01 03 08
    From fixed array   : 02 05 0d
    From borrowed span : 15 22 37
    From vector        : 59 90
    From static literal: 22 37 59
    Editor as block    : 01 02 03

.. erbsland-demo-end::

Recognizing Binary Structure
============================

Binary formats often begin with a marker, end with a trailer, or contain a delimiter.
:cpp:func:`startsWith() <erbsland::mem::ByteBlock::startsWith>`,
:cpp:func:`endsWith() <erbsland::mem::ByteBlock::endsWith>`, and
:cpp:func:`contains() <erbsland::mem::ByteBlock::contains>` express those checks directly.
:cpp:func:`find() <erbsland::mem::ByteBlock::find>` returns the first matching byte index, optionally at or after a
chosen start, while :cpp:func:`findLast() <erbsland::mem::ByteBlock::findLast>` searches from the other end.
A missing match is reported as ``ByteIndex::noIndex()``.

:cpp:func:`length() <erbsland::mem::ByteBlock::length>` and
:cpp:func:`isEmpty() <erbsland::mem::ByteBlock::isEmpty>` complete the common structural checks.
All search inputs may be blocks, initializer lists, or borrowed spans, so validation does not require a temporary owning
object.

.. erbsland-demo::
    :source: mem/ByteBlock/InspectingContents.cpp
    :exec: mem/byte_block --demo InspectingContents
    :source-sha256: fde66db0d0c29b6087b1fed9e809797a05f6767fcb7b5e497ff8be4a36324442

.. code-block:: cpp

    /// Inspect and search the contents of a byte block.
    ///
    /// Length and membership tests make common binary validation readable, while
    /// `find()` and `findLast()` return byte-aware indexes for matched sequences.
    void inspectingContents() {
        const auto skillPath = el::ByteBlock{el::Byte{0x10U}, el::Byte{0x21U}, el::Byte{0x34U}, el::Byte{0x21U}};

        // Validate the expected beginning, ending, and an unlock marker in the path.
        const auto hasRoot = skillPath.startsWith({el::Byte{0x10U}, el::Byte{0x21U}});
        const auto hasMastery = skillPath.endsWith({el::Byte{0x34U}, el::Byte{0x21U}});
        const auto hasUnlock = skillPath.contains({el::Byte{0x21U}});

        // Locate the first and final occurrence of the repeated unlock marker.
        const auto firstUnlock = skillPath.find({el::Byte{0x21U}});
        const auto lastUnlock = skillPath.findLast({el::Byte{0x21U}});

        el::io::printLine("Skill path         : Voie des brumes"_el);
        el::io::printLine("Byte count         : "_el, skillPath.length().toSizeT());
        el::io::printLine("Path is empty      : "_el, el::BooleanFormat::yesNo(), skillPath.isEmpty());
        el::io::printLine("Expected root      : "_el, el::BooleanFormat::yesNo(), hasRoot);
        el::io::printLine("Mastery ending     : "_el, el::BooleanFormat::yesNo(), hasMastery);
        el::io::printLine("Contains unlock    : "_el, el::BooleanFormat::yesNo(), hasUnlock);
        el::io::printLine("First unlock index : "_el, firstUnlock.toSizeT());
        el::io::printLine("Last unlock index  : "_el, lastUnlock.toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Skill path         : Voie des brumes
    Byte count         : 4
    Path is empty      : no
    Expected root      : yes
    Mastery ending     : yes
    Contains unlock    : yes
    First unlock index : 1
    Last unlock index  : 3

.. erbsland-demo-end::

Reading Individual Bytes
========================

Choose tolerant or strict access according to the meaning of a missing byte.
:cpp:func:`get() <erbsland::mem::ByteBlock::get>` returns a chosen fallback when the index is invalid or beyond the end.
That is convenient for an optional field.
:cpp:func:`getOrThrow() <erbsland::mem::ByteBlock::getOrThrow>` raises
:cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>` when a required layout invariant is broken.

:cpp:func:`endIndex() <erbsland::mem::ByteBlock::endIndex>` names the position immediately after the final byte.
For complete traversal, :cpp:func:`forEach() <erbsland::mem::ByteBlock::forEach>` invokes a callback with each byte and,
when accepted by the callback, its index.
The returned :cpp:enum:`LoopResult <erbsland::util::LoopResult>` also allows callbacks designed for early exit.

.. erbsland-demo::
    :source: mem/ByteBlock/AccessingBytes.cpp
    :exec: mem/byte_block --demo AccessingBytes
    :source-sha256: 688c2bbdfbca115966acb21ba8b4133d2370e58c78dd4075b1f1404c0cef2868

.. code-block:: cpp

    /// Read individual bytes or visit a complete block.
    ///
    /// Tolerant access returns a chosen fallback for an invalid index. Strict access
    /// throws when a required byte is absent, and `forEach()` visits every visible
    /// byte with an optional byte index.
    void accessingBytes() {
        const auto skillCosts = el::ByteBlock{el::Byte{3U}, el::Byte{5U}, el::Byte{8U}, el::Byte{13U}};

        // Read one optional cost and one position guaranteed by the skill-tree layout.
        const auto missingCost = skillCosts.get(el::ByteIndex{20U}, el::Byte{255U});
        const auto masteryCost = skillCosts.getOrThrow(el::ByteIndex{3U});

        // Visit every cost together with its index to calculate a total.
        auto totalCost = uint32_t{};
        const auto visitResult = skillCosts.forEach(
            [&totalCost](const el::Byte cost, const el::ByteIndex) -> void { totalCost += cost.toUInt32(); });

        el::io::printLine("Skill tree         : Sentier du lynx"_el);
        el::io::printLine("End index          : "_el, skillCosts.endIndex().toSizeT());
        el::io::printLine("Missing fallback   : "_el, missingCost.toUInt32());
        el::io::printLine("Mastery cost       : "_el, masteryCost.toUInt32());
        el::io::printLine("Total cost         : "_el, totalCost);
        el::io::printLine("Visited all bytes  : "_el, el::BooleanFormat::yesNo(), visitResult == el::LoopResult::Success);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Skill tree         : Sentier du lynx
    End index          : 4
    Missing fallback   : 255
    Mastery cost       : 13
    Total cost         : 29
    Visited all bytes  : yes

.. erbsland-demo-end::

Reading Native Integers
=======================

When a binary layout stores an ordinary C++ integer at a known offset,
:cpp:func:`getInteger() <erbsland::mem::ByteBlock::getInteger>` decodes it with a fallback for an incomplete range.
:cpp:func:`getIntegerInto() <erbsland::mem::ByteBlock::getIntegerInto>` instead reports success and leaves an existing
destination unchanged on failure.
:cpp:func:`getIntegerOrThrow() <erbsland::mem::ByteBlock::getIntegerOrThrow>` makes the complete field an invariant.

All three operations accept :cpp:enum:`Endianness <erbsland::mem::Endianness>` and default to little endian.
They decode the native width of their template type.
For wire formats whose signedness or encoded width differs from a native type, including compact integers, see
:doc:`byte_integer_formats` and use a :cpp:class:`ByteReader <erbsland::mem::ByteReader>`.

.. erbsland-demo::
    :source: mem/ByteBlock/ReadingIntegers.cpp
    :exec: mem/byte_block --demo ReadingIntegers
    :source-sha256: 4e2d240eb3e1ed163d149dc43c3566038c6d02e4bccb4f9cffe546d87d43b5d7

.. code-block:: cpp

    /// Decode native integers from known positions in a byte block.
    ///
    /// Integer access can return a fallback, report success while preserving an
    /// existing destination on failure, or throw when the binary layout guarantees
    /// that the complete integer is present.
    void readingIntegers() {
        const auto record = el::ByteBlock{
            el::Byte{0x34U},
            el::Byte{0x12U},
            el::Byte{0x00U},
            el::Byte{0x00U},
            el::Byte{0x00U},
            el::Byte{0x00U},
            el::Byte{0x01U},
            el::Byte{0x2cU}};

        // Read a little-endian rank and a big-endian experience value.
        const auto rank = record.getIntegerOrThrow<uint16_t>(el::ByteIndex{0U}, el::Endianness::Little);
        const auto experience = record.getIntegerOrThrow<uint32_t>(el::ByteIndex{4U}, el::Endianness::Big);

        // Preserve an existing value when an optional field is incomplete.
        auto prestige = uint32_t{7U};
        const auto hasPrestige = record.getIntegerInto(prestige, el::ByteIndex{7U}, el::Endianness::Big);
        const auto fallback = record.getInteger<uint32_t>(el::ByteIndex{7U}, el::Endianness::Big, uint32_t{99U});

        el::io::printLine("Character          : Éclaireuse lunaire"_el);
        el::io::printLine("Rank               : "_el, rank);
        el::io::printLine("Experience         : "_el, experience);
        el::io::printLine("Prestige available : "_el, el::BooleanFormat::yesNo(), hasPrestige);
        el::io::printLine("Preserved prestige : "_el, prestige);
        el::io::printLine("Fallback prestige  : "_el, fallback);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Character          : Éclaireuse lunaire
    Rank               : 4660
    Experience         : 300
    Prestige available : no
    Preserved prestige : 7
    Fallback prestige  : 99

.. erbsland-demo-end::

Crossing Container Boundaries
=============================

Keep a block as a block while data remains inside APIs that understand Erbsland Core byte types.
At a boundary that specifically requires another owner,
:cpp:func:`toByteBuffer() <erbsland::mem::ByteBlock::toByteBuffer>` creates an independent mutable
:cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>`.
:cpp:func:`toUInt8Vector() <erbsland::mem::ByteBlock::toUInt8Vector>` and
:cpp:func:`toCharVector() <erbsland::mem::ByteBlock::toCharVector>` create independent standard vectors.

These are deliberate deep-copying conversions.
If the receiving operation only borrows bytes during the call, :cpp:func:`span() <erbsland::mem::ByteBlock::span>` is
the lower-cost boundary, provided the block outlives the span.

.. erbsland-demo::
    :source: mem/ByteBlock/ConvertingBlocks.cpp
    :exec: mem/byte_block --demo ConvertingBlocks
    :source-sha256: 5afa11cbf46728a3c4b01a42b788f36f83ee8a31b29fa9787a4108fb84f50d5f

.. code-block:: cpp

    /// Convert a byte block at an interoperability boundary.
    ///
    /// `toByteBuffer()` creates an independent mutable Erbsland Core buffer. The
    /// vector conversions copy bytes into standard-library containers for APIs that
    /// specifically require unsigned-byte or character storage.
    void convertingBlocks() {
        const auto encodedBranch = el::ByteBlock{el::Byte{0x41U}, el::Byte{0x72U}, el::Byte{0x63U}};

        // Create independent containers suited to each receiving API.
        auto mutableBuffer = encodedBranch.toByteBuffer();
        const auto unsignedBytes = encodedBranch.toUInt8Vector();
        const auto characterBytes = encodedBranch.toCharVector();
        mutableBuffer.set(el::ByteIndex{0U}, el::Byte{0x61U});

        el::io::printLine("Skill branch       : Arc ancien"_el);
        el::io::printLine("Original block     : "_el, el::ByteFormat::separated(), encodedBranch);
        el::io::printLine(
            "Mutable buffer     : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(mutableBuffer.span()));
        el::io::printLine("Unsigned vector len: "_el, unsignedBytes.size());
        el::io::printLine("Character vector len: "_el, characterBytes.size());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Skill branch       : Arc ancien
    Original block     : 41 72 63
    Mutable buffer     : 61 72 63
    Unsigned vector len: 3
    Character vector len: 3

.. erbsland-demo-end::

Editing and Assembling Small Blocks
===================================

:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` supports both structural changes and in-place updates.
:cpp:func:`append() <erbsland::mem::ByteBlockEditor::append>`,
:cpp:func:`insert() <erbsland::mem::ByteBlockEditor::insert>`, and
:cpp:func:`appendInteger() <erbsland::mem::ByteBlockEditor::appendInteger>` grow an assembled record.
``appendRepeated()`` efficiently appends a cyclic repetition of an existing range, including overlapping back references
used by compression formats.
:cpp:func:`replace() <erbsland::mem::ByteBlockEditor::replace>` removes a clamped range and inserts replacement bytes,
so its result may have a different length.

:cpp:func:`overwrite() <erbsland::mem::ByteBlockEditor::overwrite>` and
:cpp:func:`fill() <erbsland::mem::ByteBlockEditor::fill>` never resize the editor.
They change only the part of the selected range that exists, which makes them suitable for fixed fields in an already
allocated record.
:cpp:func:`remove() <erbsland::mem::ByteBlockEditor::remove>` deletes a range, while
:cpp:func:`keep() <erbsland::mem::ByteBlockEditor::keep>` discards everything outside it.

:cpp:func:`xorWith() <erbsland::mem::ByteBlockEditor::xorWith>` returns ``false`` without changing the editor when a
whole-block source has a different length.
:cpp:func:`xorWithOrThrow() <erbsland::mem::ByteBlockEditor::xorWithOrThrow>` expresses equal length as an invariant.
The ranged XOR overload remains a clamped, partial operation.

.. erbsland-demo::
    :source: mem/ByteBlock/EditingBlocks.cpp
    :exec: mem/byte_block --demo EditingBlocks
    :source-sha256: de9b923324f05c649d19490a89cf1fe7ac38eb627f83e788fa46ac065433a62d

.. code-block:: cpp

    /// Assemble and edit a small binary record in place.
    ///
    /// `ByteBlockEditor` combines growing operations such as `append()` and
    /// `insert()` with non-growing `overwrite()` and `fill()`, structural changes,
    /// and whole-block XOR for compact binary transformations.
    void editingBlocks() {
        auto build = el::ByteBlockEditor{el::Byte{0x10U}, el::Byte{0x20U}};

        // Assemble the skill record from bytes, a block, and a fixed-width integer.
        build.append(el::Byte{0x30U})
            .insert(el::ByteIndex{1U}, el::ByteBlock{el::Byte{0x15U}})
            .append(el::ByteBlock{el::Byte{0x40U}, el::Byte{0x50U}})
            .appendInteger(uint16_t{0x1234U}, el::Endianness::Big);

        // Adjust selected fields without changing the overall size.
        build.replace(el::ByteRange{el::ByteIndex{2U}, el::ByteLength{1U}}, el::ByteBlock{el::Byte{0x22U}});
        build.overwrite(el::ByteIndex{3U}, el::ByteArray{el::Byte{0x33U}, el::Byte{0x44U}}.span());
        build.fill(el::ByteRange{el::ByteIndex{5U}, el::ByteLength{1U}}, el::Byte{0x55U});

        // Remove a retired node, keep the record payload, and apply an equal-size mask.
        build.remove(el::ByteRange{el::ByteIndex{1U}, el::ByteLength{1U}});
        build.keep(el::ByteRange{el::ByteIndex{0U}, el::ByteLength{6U}});
        build.xorWithOrThrow(el::ByteBlock{el::ByteLength{6U}, el::Byte{0x0fU}});

        el::io::printLine("Skill build        : Gardien des étoiles"_el);
        el::io::printLine("Encoded build      : "_el, el::ByteFormat::separated(), el::ByteBlock{build});
    }

.. erbsland-ansi::
    :escape-char: ␛

    Skill build        : Gardien des étoiles
    Encoded build      : 1f 2d 3c 4b 5a 1d

.. erbsland-demo-end::

Controlling Editor Storage
==========================

Capacity management matters when an editor is built incrementally.
:cpp:func:`reserve() <erbsland::mem::ByteBlockEditor::reserve>` prepares room without changing the visible length, and
:cpp:func:`resize() <erbsland::mem::ByteBlockEditor::resize>` changes that length, filling newly visible bytes with
zero.
:cpp:func:`capacity() <erbsland::mem::ByteBlockEditor::capacity>` reports the current allocation, while
:cpp:func:`shrinkToFit() <erbsland::mem::ByteBlockEditor::shrinkToFit>` reduces it to the visible length.

Usually copy-on-write detaches automatically at the first real mutation.
:cpp:func:`detach() <erbsland::mem::ByteBlockEditor::detach>` lets you pay that cost at a deliberate point before a
series of changes.
:cpp:func:`clear() <erbsland::mem::ByteBlockEditor::clear>` removes all visible bytes but retains capacity for reuse;
:cpp:func:`reset() <erbsland::mem::ByteBlockEditor::reset>` also releases the allocation.

.. erbsland-demo::
    :source: mem/ByteBlock/ManagingStorage.cpp
    :exec: mem/byte_block --demo ManagingStorage
    :source-sha256: cb835b833a224d7f11e74dc88c03a6771fb4ee739a348017487808b6dbc5794f

.. code-block:: cpp

    /// Manage mutable byte-block length, capacity, and shared storage.
    ///
    /// Reserving avoids repeated growth while assembling data. Resizing changes the
    /// visible length, `detach()` eagerly establishes unique storage, and clearing or
    /// resetting chooses whether allocated capacity is retained.
    void managingStorage() {
        auto path = el::ByteBlockEditor{el::Byte{1U}, el::Byte{2U}, el::Byte{3U}};

        // Reserve working room and grow the visible block with zero-filled bytes.
        path.reserve(el::ByteLength{32U});
        const auto reservedCapacity = path.capacity();
        path.resize(el::ByteLength{6U});

        // Preserve a read-only snapshot and detach before a sequence of edits.
        const auto snapshot = el::ByteBlock{path};
        path.detach();
        path.fill(el::ByteRange{el::ByteIndex{3U}, el::ByteLength{3U}}, el::Byte{9U});
        path.shrinkToFit();

        el::io::printLine("Skill path         : Discipline astrale"_el);
        el::io::printLine("Reserved capacity  : "_el, reservedCapacity.toSizeT());
        el::io::printLine("Edited bytes       : "_el, el::ByteFormat::separated(), el::ByteBlock{path});
        el::io::printLine("Snapshot bytes     : "_el, el::ByteFormat::separated(), snapshot);
        el::io::printLine("Tight capacity     : "_el, path.capacity().toSizeT());

        // Clear retains the tight allocation; reset releases it.
        path.clear();
        el::io::printLine("Capacity after clear: "_el, path.capacity().toSizeT());
        path.reset();
        el::io::printLine("Capacity after reset: "_el, path.capacity().toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Skill path         : Discipline astrale
    Reserved capacity  : 48
    Edited bytes       : 01 02 03 09 09 09
    Snapshot bytes     : 01 02 03 00 00 00
    Tight capacity     : 6
    Capacity after clear: 6
    Capacity after reset: 0

.. erbsland-demo-end::

Comparing Blocks
================

The equality and three-way comparison operators compare visible bytes lexicographically across blocks and editors.
They are the right tools for ordinary equality, sorting, and ordered containers.

For authentication tags and similar equal-length values, use
:cpp:func:`isEqualConstTime() <erbsland::mem::ByteBlock::isEqualConstTime>`.
It inspects every byte when lengths match, avoiding a content-dependent early exit.
The length comparison remains observable, so callers working with secret values should use a format with a fixed,
already validated length.

.. erbsland-demo::
    :source: mem/ByteBlock/ComparingBlocks.cpp
    :exec: mem/byte_block --demo ComparingBlocks
    :source-sha256: 3da1a046ab7aa8fc611695c357ec65f75af081f87813edbb26bb33309a1cd150

.. code-block:: cpp

    /// Compare byte blocks lexicographically or without content-dependent exits.
    ///
    /// The ordinary comparison operators are appropriate for ordering and general
    /// equality checks. `isEqualConstTime()` inspects every byte for equal-length
    /// inputs and is the safer content comparison for authentication-related data.
    void comparingBlocks() {
        const auto novice = el::ByteBlock{el::Byte{1U}, el::Byte{3U}, el::Byte{5U}};
        const auto sameNovice = el::ByteBlockEditor{el::Byte{1U}, el::Byte{3U}, el::Byte{5U}};
        const auto adept = el::ByteBlock{el::Byte{1U}, el::Byte{4U}, el::Byte{1U}};

        // Use regular comparisons for equality and skill-tree ordering.
        const auto sameBuild = novice == sameNovice;
        const auto noviceComesFirst = (novice <=> adept) == std::strong_ordering::less;

        // Use constant-time content comparison for equal-length verification tokens.
        const auto matchingToken = novice.isEqualConstTime(sameNovice.span());
        const auto differentToken = novice.isEqualConstTime(adept);

        el::io::printLine("Build              : Apprentie des runes"_el);
        el::io::printLine("Same build         : "_el, el::BooleanFormat::yesNo(), sameBuild);
        el::io::printLine("Sorts before adept : "_el, el::BooleanFormat::yesNo(), noviceComesFirst);
        el::io::printLine("Matching token     : "_el, el::BooleanFormat::yesNo(), matchingToken);
        el::io::printLine("Different token    : "_el, el::BooleanFormat::yesNo(), differentToken);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Build              : Apprentie des runes
    Same build         : yes
    Sorts before adept : yes
    Matching token     : yes
    Different token    : no

.. erbsland-demo-end::
