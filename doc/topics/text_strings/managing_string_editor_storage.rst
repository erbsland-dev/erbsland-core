..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: StringEditor Storage
    single: String Capacity
    single: Copy on Write
    single: reserve
    single: shrinkToFit
    single: detach

*****************************
Managing StringEditor Storage
*****************************

For most edits, :cpp:type:`StringEditor <erbsland::text::StringEditor>` can manage its own storage and you never need to
think about capacity.
Storage becomes interesting when you already know how large the result will be, reuse one editor many times, or discover
that a small visible value retains more memory than expected.

This page explains those deliberate exceptions.
It connects capacity to the native unit of each encoding, shows when copy-on-write performs a real copy, and helps you
decide whether reserving, detaching, clearing, or compacting will actually improve the workflow.

Native Capacity Units and Reservation
=====================================

``capacity()`` and ``reserve()`` use the editor's native storage unit:

* ``StringEditor`` and ``U8StringEditor`` use bytes.
* ``U16StringEditor`` uses UTF-16 data units, not bytes and not decoded code points.
* ``U32StringEditor`` uses code points, which are also its native data units.

Reserve the final native amount once when it can be calculated cheaply.
Reserving before every append can repeatedly materialize exact-sized storage and removes the benefit of amortized
growth.

.. erbsland-demo::
    :source: text/StringEditor/ReserveForAppend.cpp
    :exec: text/string_editor --demo ReserveForAppend
    :sha256: 7f6da6306640396bb250d768bf56a0c738faa2454d622ba38710d829bbf0130f

.. code-block:: cpp

    /// Reserve string storage once when the final native size is already known.
    ///
    /// Reserving before every append step can repeatedly materialize new storage.
    /// The better pattern is to calculate the final native size, reserve once, and
    /// then append the fragments. Unreserved growth may otherwise reallocate and
    /// copy the existing text several times.
    void reserveForAppend() {
        const auto fragments = std::array{
            "Aurora station: céu limpo"_el,
            "Wind tunnel: brise légère"_el,
            "Observatory: 星が明るい"_el,
        };

        auto requiredBytes = std::size_t{0};
        for (const auto &fragment : fragments) {
            requiredBytes += fragment.length().toSizeT() + 1U;
        }

        auto planned = el::StringEditor{};
        planned.reserve(el::ByteCount::fromSizeT(requiredBytes));

        el::io::printLine("Planned reservation"_el);
        el::io::printLine("  required bytes : "_el, requiredBytes);
        el::io::printLine("  length after reserve : "_el, planned.length());
        el::io::printLine("  capacity after reserve : "_el, planned.capacity());

        for (const auto &fragment : fragments) {
            planned.append(fragment).append(U'\n');
        }

        el::io::printLine("  length after append : "_el, planned.length());
        el::io::printLine("  capacity after append : "_el, planned.capacity());

        auto repeated = el::StringEditor{};
        el::io::printLine();
        el::io::printLine("Repeated exact reservations"_el);
        for (const auto &fragment : fragments) {
            const auto nextLength = repeated.length().toSizeT() + fragment.length().toSizeT() + 1U;
            repeated.reserve(el::ByteCount::fromSizeT(nextLength));
            repeated.append(fragment).append(U'\n');
            el::io::printLine("  after step: length "_el, repeated.length(), ", capacity "_el, repeated.capacity());
        }

        el::io::printLine();
        el::io::printLine("Final report:"_el);
        el::io::print(planned);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mPlanned reservation
          required bytes : 84
          length after reserve : 0
          capacity after reserve : 84
          length after append : 84
          capacity after append : 84

        Repeated exact reservations
          after step: length 27, capacity 27
          after step: length 55, capacity 55
          after step: length 84, capacity 84

        Final report:
        Aurora station: céu limpo
        Wind tunnel: brise légère
        Observatory: 星が明るい␛[0m

.. erbsland-demo-end::

Automatic Copy-on-Write Detachment
==================================

Editor copies can share a backing store.
The first mutation of a shared editor detaches that editor so the other values remain unchanged.
This provides value semantics, but the detachment must copy existing character data.

Constructing an editor from a read-only ``String`` copies the text into editable storage.
Once an editor owns unique storage, later mutations can reuse that allocation and any spare capacity until an operation
requires more room.

.. erbsland-demo::
    :source: text/StringEditor/CopyOnWrite.cpp
    :exec: text/string_editor --demo CopyOnWrite
    :sha256: bbca72594f6d4c220285c392127605a5123bc6e98f3445f826cdf69e4f0a0607

.. code-block:: cpp

    /// Copies of an editor share their backing store until one editor is mutated.
    /// The first write detaches that editor, preserving the other values.
    ///
    /// From the user's perspective, every string behaves like an independent value.
    /// The sharing and copying happens automatically in the background.
    ///
    /// Constructing an editor from a read-only `String` creates editable storage.
    /// Later edits can reuse that storage and its spare capacity while it remains
    /// unique.
    void copyOnWrite() {
        const auto source = el::String{"The treasure is hidden under the old oak tree."_el};
        auto a = el::StringEditor{source};
        auto b = a;
        auto c = b;

        const auto printAll = [&]() -> void {
            el::io::printLine("a: ", a);
            el::io::printLine(el::toDebugString(a, cDebugFlags));

            el::io::printLine("b: ", b);
            el::io::printLine(el::toDebugString(b, cDebugFlags));

            el::io::printLine("c: ", c);
            el::io::printLine(el::toDebugString(c, cDebugFlags));

            el::io::printLine();
        };

        // All three strings share the same backing store.
        el::io::printLine("After copying a -> b and b -> c"_el);
        printAll();

        // Mutating b detaches it from the shared editor storage.
        b.replaceAll("treasure"_el, "secret"_el);

        el::io::printLine("After modifying b"_el);
        printAll();

        // Mutating a detaches it too; c still preserves the original value.
        a.append(" Nobody has found it yet."_el);

        el::io::printLine("After modifying a"_el);
        printAll();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mAfter copying a -> b and b -> c
        a: The treasure is hidden under the old oak tree.
        U8StringEditor:
            backingStorageId: 0x26235705d200308e:0x1a0ad4772eee5fc0
        b: The treasure is hidden under the old oak tree.
        U8StringEditor:
            backingStorageId: 0x26235705d200308e:0x1a0ad4772eee5fc0
        c: The treasure is hidden under the old oak tree.
        U8StringEditor:
            backingStorageId: 0x26235705d200308e:0x1a0ad4772eee5fc0

        After modifying b
        a: The treasure is hidden under the old oak tree.
        U8StringEditor:
            backingStorageId: 0x26235705d200308e:0x1a0ad4772eee5fc0
        b: The secret is hidden under the old oak tree.
        U8StringEditor:
            backingStorageId: 0x2623577da0fcd55e:0x1a0ad40f5c12ba12
        c: The treasure is hidden under the old oak tree.
        U8StringEditor:
            backingStorageId: 0x26235705d200308e:0x1a0ad4772eee5fc0

        After modifying a
        a: The treasure is hidden under the old oak tree. Nobody has found it yet.
        U8StringEditor:
            backingStorageId: 0x2623577da0fc7a5e:0x1a0ad40f5c121577
        b: The secret is hidden under the old oak tree.
        U8StringEditor:
            backingStorageId: 0x2623577da0fcd55e:0x1a0ad40f5c12ba12
        c: The treasure is hidden under the old oak tree.
        U8StringEditor:
            backingStorageId: 0x26235705d200308e:0x1a0ad4772eee5fc0␛[0m

.. erbsland-demo-end::

Explicit Copy-on-Write Detachment
=================================

Mutating operations detach automatically.
Call ``detach()`` only when low-level code deliberately wants to pay the copy cost before a group of edits or when
diagnostics must make the transition from shared to unique storage explicit.

.. erbsland-demo::
    :source: text/StringEditor/ManualDetach.cpp
    :exec: text/string_editor --demo ManualDetach
    :sha256: 40899e30b599fc4664c57d573ee8a3188ee23d26dc8d89e0cbb2e83c83e06d96

.. code-block:: cpp

    /// Manual detaching makes an automatic copy-on-write step explicit.
    ///
    /// A write operation detaches shared string storage automatically. Calling
    /// `detach()` directly is therefore rare. It is mainly useful when low-level
    /// code wants to make exclusive storage visible before a group of edits, or
    /// when diagnostics need to show exactly where sharing ends.
    void manualDetach() {
        auto fieldNote = el::StringEditor{"Fjord station: lichen sample 17, lumière froide"_el};
        auto archiveCopy = fieldNote;
        const auto yesNo = el::BooleanFormat::yesNo();

        el::io::printLine("Before detach:"_el);
        el::io::printLine("  same visible storage : "_el, yesNo, fieldNote.storageId() == archiveCopy.storageId());

        fieldNote.detach();

        el::io::printLine("After detach:"_el);
        el::io::printLine("  same visible storage : "_el, yesNo, fieldNote.storageId() == archiveCopy.storageId());

        fieldNote.replaceAll("sample 17"_el, "sample 17A"_el);
        fieldNote.append(" | checked"_el);

        el::io::printLine();
        el::io::printLine("Edited note : "_el, fieldNote);
        el::io::printLine("Archive copy: "_el, archiveCopy);

        constexpr auto debugDetails =
            el::DebugViewDetail::BackingStore | el::DebugViewDetail::Size | el::DebugViewDetail::Range;

        el::io::printLine();
        el::io::printLine("Debug view after editing:"_el);
        el::io::printLine(el::toDebugString(fieldNote, debugDetails));
        el::io::printLine(el::toDebugString(archiveCopy, debugDetails));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mBefore detach:
          same visible storage : yes
        After detach:
          same visible storage : no

        Edited note : Fjord station: lichen sample 17A, lumière froide | checked
        Archive copy: Fjord station: lichen sample 17, lumière froide

        Debug view after editing:
        U8StringEditor:
            length: 59
            characterLength: 58
            backingStorageId: 0x26235705d24d5e0e:0x1a0ad4772ea33153
            backingLength: 60
            selectedRange: index: 0 - 59 (length: 59)
        U8StringEditor:
            length: 48
            characterLength: 47
            backingStorageId: 0x26235705d24d37de:0x1a0ad4772ea358ee
            backingLength: 49
            selectedRange: index: 0 - 48 (length: 48)␛[0m

.. erbsland-demo-end::

Capacity Retention, clear(), reset(), and shrinkToFit()
=======================================================

``clear()`` keeps capacity for the next editing pass.
``reset()`` releases the editor state.
``shrinkToFit()`` compacts a completed editor when retained spare capacity is materially larger than the value that must
remain alive.

Compaction allocates and copies, so it is not routine cleanup.
Use it at a storage boundary after measuring or observing meaningful retention, not after every edit.

.. erbsland-demo::
    :source: text/StringEditor/ShrinkSlices.cpp
    :exec: text/string_editor --demo ShrinkSlices
    :sha256: e37f86a6c5832f70d4fd737f89560c743f911e5bb71aef3cc88af71f03878990

.. code-block:: cpp

    /// `shrinkToFit()` is a deliberate compaction step, not routine cleanup.
    ///
    /// A small construction can deliberately reserve its final maximum size.
    /// Calling `shrinkToFit()` after construction releases unused capacity before
    /// the editor is converted to a stored read-only string.
    void shrinkSlices() {
        auto note = el::StringEditor{};
        note.reserve(el::ByteCount{96U});
        note.append("Specimen: "_el);
        note.append("Luzula sylvatica"_el);

        el::io::printLine("Before compaction:"_el);
        el::io::printLine("  text ........: "_el, note);
        el::io::printLine("  length ......: "_el, note.length());
        el::io::printLine("  capacity ....: "_el, note.capacity());
        el::io::printLine("  memory usage : "_el, note.memoryUsage());

        note.shrinkToFit();
        el::io::printLine();
        el::io::printLine("After shrinkToFit:"_el);
        el::io::printLine("  text ........: "_el, note);
        el::io::printLine("  length ......: "_el, note.length());
        el::io::printLine("  capacity ....: "_el, note.capacity());
        el::io::printLine("  memory usage : "_el, note.memoryUsage());

        const auto storedNote = el::String{note};
        el::io::printLine("Stored result .: "_el, storedNote);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mBefore compaction:
          text ........: Specimen: Luzula sylvatica
          length ......: 26
          capacity ....: 96
          memory usage : 113

        After shrinkToFit:
          text ........: Specimen: Luzula sylvatica
          length ......: 26
          capacity ....: 26
          memory usage : 43
        Stored result .: Specimen: Luzula sylvatica␛[0m

.. erbsland-demo-end::

Storage Capacity, Memory Usage, and Sharing Diagnostics
=======================================================

``capacity()``, ``memoryUsage()``, and ``storageId()`` are useful diagnostics.
They help distinguish visible text length from retained allocation and show when values share storage.
Do not use ``storageId()`` as application identity; it describes an implementation-level backing store that can change
after detachment or compaction.
