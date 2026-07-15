..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: String Memory Management
    single: String
    single: U8String
    single: U16String
    single: U32String
    single: reserve
    single: shrinkToFit
    single: capacity
    single: memoryUsage
    single: detach
    single: storageId
    single: Copy on Write
    single: String Debug View

************************************
Managing Memory when Editing Strings
************************************

Most string memory management in Erbsland Core happens automatically.
Editable strings are owning value types with copy-on-write storage, so copying a string is cheap until one copy is
modified.
When you append, insert, replace, or remove text, the string first makes sure that it has writable storage and then
performs the edit.

The low-level memory functions are useful when automatic behavior is not enough information or control.
Use them when you already know the final native size of an editable string, when a small slice must survive after a
large source should be released, or when diagnostic code has to reason about shared backing storage.
Most application code does not need to call these functions directly.

The examples on this page use :cpp:type:`String <erbsland::text::String>`, the common UTF-8 editable string type.
The same ideas apply to :cpp:class:`U8String <erbsland::text::U8String>`,
:cpp:class:`U16String <erbsland::text::U16String>`, and
:cpp:class:`U32String <erbsland::text::U32String>`.
Only the native capacity unit changes: UTF-8 reserves bytes, UTF-16 reserves UTF-16 data units, and UTF-32 reserves code
points.

You will learn when manual reservation is useful, why shrinking belongs at lifetime boundaries, how copy-on-write
affects storage sharing, and how to inspect string storage without a debugger.

Choose the Right Construction Tool
==================================

Before reaching for manual memory control, choose the construction API that matches the work you are doing.

Use :cpp:class:`StringBuilder <erbsland::text::StringBuilder>` when text is produced piece by piece, especially when the
final size is unknown or fragments arrive from other code.
Use :cpp:type:`String <erbsland::text::String>` when you edit an existing value, or when you can calculate the final
native size before appending.

Manual reservation is most useful when you can follow this pattern:

*   calculate the final native size once,
*   call :cpp:func:`reserve() <erbsland::text::U8String::reserve>` once,
*   append or edit the string without repeatedly growing it.

Calling :cpp:func:`reserve() <erbsland::text::U8String::reserve>` before every append step is usually a performance
smell.
Each larger reservation may allocate new storage and copy the current text.
When you cannot know the final size, let :cpp:class:`StringBuilder <erbsland::text::StringBuilder>` manage growth for
you.

.. erbsland-demo::
    :source: text/String/ReserveForAppend.cpp
    :exec: text/string --demo ReserveForAppend
    :source-sha256: 10f72d2b6e37368d06ed470abaad2c2286b78ae7b82f43068578305255f3a244

.. code-block:: cpp

    /// Reserve string storage once when the final native size is already known.
    ///
    /// Reserving before every append step can repeatedly materialize new storage.
    /// The better pattern is to calculate the final native size, reserve once, and
    /// then append the fragments. For unknown or streaming text, prefer
    /// `StringBuilder` because it is designed for incremental construction.
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

        auto planned = el::String{};
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

        auto repeated = el::String{};
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

Understand Capacity and Memory Usage
====================================

:cpp:func:`capacity() <erbsland::text::U8String::capacity>` reports how much native string data can be stored in the
current visible string value without another reservation.
For UTF-8, this is a byte count.
For UTF-16, this is a count of UTF-16 data units.
For UTF-32, this is a count of code points.

:cpp:func:`memoryUsage() <erbsland::text::U8String::memoryUsage>` estimates the retained heap memory for the backing
storage, including internal management data.
It is intended for monitoring, cache accounting, and diagnostics.
It is not a promise about the exact number of bytes kept by the allocator.

Capacity and memory usage answer different questions.
Capacity tells you whether the current string can grow without reallocating.
Memory usage tells you how much backing storage is retained.
For sliced strings, this difference matters: a small visible string can still keep a larger backing store alive.

Shrink Only at Lifetime Boundaries
==================================

:cpp:func:`shrinkToFit() <erbsland::text::U8String::shrinkToFit>` materializes the visible string value into exact-sized
storage.
This can be useful before storing a compact result in a long-lived object or cache.
It is usually not useful after every edit, because the next edit may have to allocate again.

Treat shrinking as a boundary operation.
It belongs after a large temporary buffer has been reduced to the value you will keep, or after a slice has selected the
small part of a larger string that must survive.
It should not be part of ordinary append, replace, or formatting loops.

.. erbsland-demo::
    :source: text/String/ShrinkSlices.cpp
    :exec: text/string --demo ShrinkSlices
    :source-sha256: ca426b720fbeb11b93e4b16752e2f8329a53ef4d7f8dd17746f579efac0461a4

.. code-block:: cpp

    /// `shrinkToFit()` is a deliberate compaction step, not routine cleanup.
    ///
    /// A sliced string can keep the original backing store alive. Calling
    /// `shrinkToFit()` materializes the visible range into exact-sized storage.
    /// This is useful before keeping a small slice for a long time, but it should
    /// not be used after every edit.
    void shrinkSlices() {
        auto archiveLine =
            el::String{"Observatory log | Luzula sylvatica | vallée alpine | cielo sereno | 2026-06-07"_el};
        archiveLine.reserve(el::ByteLength{180U});

        const auto marker = "Luzula sylvatica"_el;
        auto specimen = archiveLine.slice(el::ByteRange{archiveLine.find(marker), marker.length()});

        el::io::printLine("Original line:"_el);
        el::io::printLine(archiveLine);
        el::io::printLine();

        el::io::printLine("Slice before compaction:"_el);
        el::io::printLine("  text ........: "_el, specimen);
        el::io::printLine("  length ......: "_el, specimen.length());
        el::io::printLine("  capacity ....: "_el, specimen.capacity());
        el::io::printLine("  memory usage : "_el, specimen.memoryUsage());

        specimen.shrinkToFit();

        el::io::printLine();
        el::io::printLine("Slice after shrinkToFit:"_el);
        el::io::printLine("  text ........: "_el, specimen);
        el::io::printLine("  length ......: "_el, specimen.length());
        el::io::printLine("  capacity ....: "_el, specimen.capacity());
        el::io::printLine("  memory usage : "_el, specimen.memoryUsage());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Original line:
    Observatory log | Luzula sylvatica | vallée alpine | cielo sereno | 2026-06-07

    Slice before compaction:
      text ........: Luzula sylvatica
      length ......: 16
      capacity ....: 16
      memory usage : 193

    Slice after shrinkToFit:
      text ........: Luzula sylvatica

.. erbsland-demo-end::

Let Copy-on-Write Do the Usual Work
===================================

Editable strings share storage after ordinary copies.
This makes passing and returning strings cheap while keeping value semantics: when one copy is modified, it detaches
from the shared storage and the other copies remain unchanged.

In normal code, you do not have to call :cpp:func:`detach() <erbsland::text::U8String::detach>`.
Any mutating operation detaches automatically when needed.
Manual detaching is useful only when you want to make that copy-on-write point explicit before a group of low-level
operations, or when diagnostic output should show that two strings are independent before editing begins.

The storage identifier returned by :cpp:func:`storageId() <erbsland::text::U8String::storageId>` describes the visible
storage range, not merely the decoded text.
Two strings can contain the same text and still have different storage identifiers.
Conversely, two copies can have the same identifier until one of them detaches.

Use storage identifiers only in low-level code that caches native indexes or validates saved reader state.
They are not stable across edits, reservations, compaction, or view range changes.
Do not serialize them and do not use them as content hashes.

.. erbsland-demo::
    :source: text/String/ManualDetach.cpp
    :exec: text/string --demo ManualDetach
    :source-sha256: 1872b059e4be14a11a1cd9dd5b0fff432380ae1fc37282e87f737ddc07c219b7

.. code-block:: cpp

    /// Manual detaching makes an automatic copy-on-write step explicit.
    ///
    /// A write operation detaches shared string storage automatically. Calling
    /// `detach()` directly is therefore rare. It is mainly useful when low-level
    /// code wants to make exclusive storage visible before a group of edits, or
    /// when diagnostics need to show exactly where sharing ends.
    void manualDetach() {
        auto fieldNote = el::String{"Fjord station: lichen sample 17, lumière froide"_el};
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
    U8String:
        length: 59
        characterLength: 58
        backingStorageId: 0x2623570d46bcd942:0x1a0ad47fba52b667
        backingLength: 60
        selectedRange: index: 0 - 59 (length: 59)
    U8String:
        length: 48
        characterLength: 47
        backingStorageId: 0x2623570d473c9b42:0x1a0ad47fbbd2f412
        backingLength: 49
        selectedRange: index: 0 - 48 (length: 48)

.. erbsland-demo-end::

Diagnose Storage Problems without a Debugger
============================================

The string debug helpers in :cpp:func:`toDebugString() <erbsland::debug::toDebugString>` expose information that is
usually hidden behind copy-on-write storage.
With :cpp:enum:`DebugViewDetail <erbsland::debug::DebugViewDetail>` flags, diagnostic code can include the backing
storage identity, the selected native range, and the native length.

This output is meant for investigation, logging, and documentation.
It helps answer questions such as:

*   Does this string still share storage with a copy?
*   Did a slice keep a larger backing store alive?
*   Did a reservation, shrink, or edit move the visible string to new storage?
*   Does a cached native index still belong to the same visible range?

The debug output should not be parsed by application logic.
For program logic, compare :cpp:func:`storageId() <erbsland::text::U8String::storageId>` directly and store native
indexes together with the matching identifier.
