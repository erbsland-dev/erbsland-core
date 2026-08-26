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

Measure Capacity in the Encoding's Native Unit
==============================================

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
    :source-sha256: 55eeca6e1313604f86b348e182d27cdefd6e87b8eeddcf2dcd98107c8c384480

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
        planned.reserve(el::ByteLength::fromSizeT(requiredBytes));

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
            repeated.reserve(el::ByteLength::fromSizeT(nextLength));
            repeated.append(fragment).append(U'\n');
            el::io::printLine("  after step: length "_el, repeated.length(), ", capacity "_el, repeated.capacity());
        }

        el::io::printLine();
        el::io::printLine("Final report:"_el);
        el::io::print(planned);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Planned reservation
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
    Observatory: 星が明るい

.. erbsland-demo-end::

The First Write to Shared Storage
=================================

Editor copies can share a backing store.
The first mutation of a shared editor detaches that editor so the other values remain unchanged.
This provides value semantics, but the detachment must copy existing character data.

Constructing an editor from a read-only ``String`` copies the text into editable storage.
Once an editor owns unique storage, later mutations can reuse that allocation and any spare capacity until an operation
requires more room.

.. erbsland-demo::
    :source: text/StringEditor/CopyOnWrite.cpp
    :exec: text/string_editor --demo CopyOnWrite
    :source-sha256: 4979f3fcb07f02077ef376b286ff493bfcd4eb9705e44b0126e83789021a3674

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

.. erbsland-ansi::
    :escape-char: ␛

    After copying a -> b and b -> c
    a: The treasure is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570f0afd9b9e:0x1a0ad47df613f4d0
    b: The treasure is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570f0afd9b9e:0x1a0ad47df613f4d0
    c: The treasure is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570f0afd9b9e:0x1a0ad47df613f4d0

    After modifying b
    a: The treasure is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570f0afd9b9e:0x1a0ad47df613f4d0
    b: The secret is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570f0afd9a9e:0x1a0ad47df613f5d2
    c: The treasure is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570f0afd9b9e:0x1a0ad47df613f4d0

    After modifying a
    a: The treasure is hidden under the old oak tree. Nobody has found it yet.
    U8StringEditor:
        backingStorageId: 0x2623570f0afc39de:0x1a0ad47df61256f7
    b: The secret is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570f0afd9a9e:0x1a0ad47df613f5d2
    c: The treasure is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570f0afd9b9e:0x1a0ad47df613f4d0

.. erbsland-demo-end::

Pay for Detachment at a Deliberate Point
========================================

Mutating operations detach automatically.
Call ``detach()`` only when low-level code deliberately wants to pay the copy cost before a group of edits or when
diagnostics must make the transition from shared to unique storage explicit.

.. erbsland-demo::
    :source: text/StringEditor/ManualDetach.cpp
    :exec: text/string_editor --demo ManualDetach
    :source-sha256: 4761192cf6765cf028b4c259c77d8a2d0784f49f421889167075abc489e31225

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

.. erbsland-ansi::
    :escape-char: ␛

    Before detach:
      same visible storage : yes
    After detach:
      same visible storage : no

    Edited note : Fjord station: lichen sample 17A, lumière froide | checked
    Archive copy: Fjord station: lichen sample 17, lumière froide

    Debug view after editing:
    U8StringEditor:
        length: 59
        characterLength: 58
        backingStorageId: 0x262357030cfc395e:0x1a0ad471f0125663
        backingLength: 60
        selectedRange: index: 0 - 59 (length: 59)
    U8StringEditor:
        length: 48
        characterLength: 47
        backingStorageId: 0x262357030cfddb0e:0x1a0ad471f013b45e
        backingLength: 49
        selectedRange: index: 0 - 48 (length: 48)

.. erbsland-demo-end::

Keep Capacity for Reuse or Give It Back
=======================================

``clear()`` keeps capacity for the next editing pass.
``reset()`` releases the editor state.
``shrinkToFit()`` compacts a completed editor when retained spare capacity is materially larger than the value that must
remain alive.

Compaction allocates and copies, so it is not routine cleanup.
Use it at a storage boundary after measuring or observing meaningful retention, not after every edit.

.. erbsland-demo::
    :source: text/StringEditor/ShrinkSlices.cpp
    :exec: text/string_editor --demo ShrinkSlices
    :source-sha256: c95dc2995d3240be1d7a8273994f2eb2ee60671ee154122f137ccf189a9f10a8

.. code-block:: cpp

    /// `shrinkToFit()` is a deliberate compaction step, not routine cleanup.
    ///
    /// A small construction can deliberately reserve its final maximum size.
    /// Calling `shrinkToFit()` after construction releases unused capacity before
    /// the editor is converted to a stored read-only string.
    void shrinkSlices() {
        auto note = el::StringEditor{};
        note.reserve(el::ByteLength{96U});
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

.. erbsland-ansi::
    :escape-char: ␛

    Before compaction:
      text ........: Specimen: Luzula sylvatica
      length ......: 26
      capacity ....: 96
      memory usage : 113

    After shrinkToFit:
      text ........: Specimen: Luzula sylvatica
      length ......: 26
      capacity ....: 26
      memory usage : 43
    Stored result .: Specimen: Luzula sylvatica

.. erbsland-demo-end::

Diagnose Retained and Shared Storage
====================================

``capacity()``, ``memoryUsage()``, and ``storageId()`` are useful diagnostics.
They help distinguish visible text length from retained allocation and show when values share storage.
Do not use ``storageId()`` as application identity; it describes an implementation-level backing store that can change
after detachment or compaction.
