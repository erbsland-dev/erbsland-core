.. index::
    single: Memory; Byte spans
    single: Byte span; Borrowed byte storage
    single: Span; Byte storage

**********************************
Working with Predefined Byte-Spans
**********************************

Many low-level operations need to examine or update bytes that already belong to another object.
Copying those bytes would add work and would sometimes hide the fact that an operation is meant to modify the original
storage.
The predefined byte spans provide a small, zero-copy view for these situations.
This page explains how to choose a span, how to adapt standard byte storage, and how to access integer fields while
keeping the lifetime of the borrowed memory clear.

A span is as low-level as a pointer and a length.
It does not own, retain, or protect the bytes it references.
Use one as a short-lived parameter or local view only when you can guarantee that the underlying storage remains alive
and does not move for the span's complete lifetime.

Borrowing a Block of Bytes
==========================

:cpp:type:`ByteSpan <erbsland::mem::ByteSpan>` is a writable view whose length is known at runtime, while
:cpp:type:`ConstByteSpan <erbsland::mem::ConstByteSpan>` provides a read-only view.
The const form is the natural parameter type for an operation that only consumes bytes: callers can provide storage
without a copy, and the signature promises that the operation will not change it.

When an algorithm requires an exact number of bytes, :cpp:type:`FixedByteSpan <erbsland::mem::FixedByteSpan>` and
:cpp:type:`FixedConstByteSpan <erbsland::mem::FixedConstByteSpan>` keep that extent in the type.
This lets the compiler reject a view with an incompatible fixed extent and makes the size requirement visible at the
call site.
You can still create a dynamic span from a fixed one when a later operation accepts a runtime length.

The following observation stays in a local array for the whole demo.
Its spans are used immediately and never outlive that array.
Notice how the writable variants are reserved for the steps that actually modify pixels, while the calculation receives
a read-only span.

.. erbsland-demo::
    :source: mem/ByteSpan/BorrowedViews.cpp
    :exec: mem/byte_span --demo BorrowedViews
    :source-sha256: b30cc7a0e4e3312af854987d572637eaf7558d69ff435271a304635570fcc029

.. code-block:: cpp

    /// Borrow a short-lived view of a local byte array.
    ///
    /// `FixedByteSpan` and `FixedConstByteSpan` preserve a known extent in the
    /// type, while `ByteSpan` and `ConstByteSpan` accept a size determined at
    /// runtime. All four are non-owning views, so the referenced storage must stay
    /// alive and in place for the complete use of the span.
    void borrowedViews() {
        const auto averageSignal = [](const el::ConstByteSpan samples) -> uint32_t {
            auto sum = uint32_t{};
            for (const auto sample : samples) {
                sum += sample.toUInt32();
            }
            return samples.empty() ? 0U : sum / static_cast<uint32_t>(samples.size());
        };

        auto observation =
            std::array{el::Byte{12U}, el::Byte{21U}, el::Byte{34U}, el::Byte{55U}, el::Byte{89U}, el::Byte{144U}};

        // Use a writable fixed span while the six-pixel extent is part of the algorithm.
        auto fixedPixels = el::FixedByteSpan<6>{observation};
        fixedPixels.front() = el::Byte{13U};

        // A dynamic span is useful when the relevant range is chosen at runtime.
        const auto visiblePixelCount = std::size_t{4U};
        auto visiblePixels = el::ByteSpan{fixedPixels}.first(visiblePixelCount);
        visiblePixels.back() = el::Byte{56U};

        // Read-only spans document that the receiving function cannot change the bytes.
        const auto corePixels = el::FixedConstByteSpan<4>{fixedPixels.first<4>()};
        const auto average = averageSignal(el::ConstByteSpan{corePixels});

        el::io::printLine("Observation        : Νεφέλωμα του Ωρίωνα"_el);
        el::io::printLine("Captured pixels    : "_el, fixedPixels.size());
        el::io::printLine("Visible pixels     : "_el, visiblePixels.size());
        el::io::printLine("Average signal     : "_el, average);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Observation        : Νεφέλωμα του Ωρίωνα
    Captured pixels    : 6

.. erbsland-demo-end::

The Lifetime Is Part of the Contract
====================================

A span remains valid only while its referenced byte range remains valid at the same address.
Returning a span into a local array, keeping one after its owner is destroyed, or retaining one while a resizable
container reallocates its storage leaves a dangling view.
The span cannot detect any of these mistakes.
Accessing such a view is undefined behavior, just like dereferencing an invalid raw pointer.

For the same reason, do not store spans in application state or pass them across thread boundaries.
Timing and ownership become much harder to verify once the span escapes the short operation that created it.
Use :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` for owned, read-only byte data and
:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` when that data must be edited.
A block or one of its owning slices is the safer unit for storage, queues, asynchronous work, and exchange between
threads.

Adapting Existing Byte Storage
==============================

External and standard C++ interfaces commonly expose bytes as ``std::byte``, ``uint8_t``, or ``char``.
:cpp:func:`toByteSpan() <erbsland::mem::toByteSpan>` creates a writable :cpp:any:`ByteSpan
<erbsland::mem::ByteSpan>` from a mutable ``std::span`` of any of these three element types.
:cpp:func:`toConstByteSpan() <erbsland::mem::toConstByteSpan>` creates the corresponding read-only view.

These conversions reinterpret the existing byte-sized elements; they do not allocate or copy storage.
A change made through the writable result is therefore visible in the original array, and both views have exactly the
same lifetime constraints as the source span.
Prefer ``toConstByteSpan()`` whenever mutation is not required, because constness makes the intended direction of data
flow explicit.

.. erbsland-demo::
    :source: mem/ByteSpan/SpanConversions.cpp
    :exec: mem/byte_span --demo SpanConversions
    :source-sha256: 9633462af3cd4b53152888e16b669386d695a4fc6e2f0f005c82f9935279ea6a

.. code-block:: cpp

    /// View standard byte storage through the Erbsland byte-span types.
    ///
    /// `toByteSpan()` and `toConstByteSpan()` adapt spans of `std::byte`,
    /// `uint8_t`, and `char` without copying their data. A writable view changes
    /// the original storage; a const view provides read-only access to it.
    void spanConversions() {
        auto filters = std::array{std::byte{1U}, std::byte{2U}, std::byte{3U}};
        auto intensities = std::array<uint8_t, 3>{31U, 47U, 63U};
        auto catalogId = std::array{'M', '4', '?'};

        // Adapt mutable standard storage and edit it through a byte view.
        auto filterBytes = el::toByteSpan(std::span{filters});
        auto intensityBytes = el::toByteSpan(std::span{intensities});
        auto catalogBytes = el::toByteSpan(std::span{catalogId});
        filterBytes.front() = el::Byte{7U};
        intensityBytes.back() = el::Byte{64U};
        catalogBytes.back() = el::Byte::fromChar('2');

        // Convert const spans when an operation only needs to inspect the bytes.
        const auto &constFilters = filters;
        const auto &constIntensities = intensities;
        const auto &constCatalogId = catalogId;
        const auto readOnlyFilters = el::toConstByteSpan(std::span{constFilters});
        const auto readOnlyIntensities = el::toConstByteSpan(std::span{constIntensities});
        const auto readOnlyCatalog = el::toConstByteSpan(std::span{constCatalogId});

        el::io::printLine("Observation        : Νεφέλωμα του Ωρίωνα"_el);
        el::io::printLine("First filter       : "_el, readOnlyFilters.front().toUInt32());
        el::io::printLine("Last intensity     : "_el, readOnlyIntensities.back().toUInt32());
        el::io::printLine(
            "Catalog ID         : "_el,
            readOnlyCatalog[0].toChar(),
            readOnlyCatalog[1].toChar(),
            readOnlyCatalog[2].toChar());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Observation        : Νεφέλωμα του Ωρίωνα
    First filter       : 7
    Last intensity     : 64
    Catalog ID         : M42

.. erbsland-demo-end::

Reading and Writing Integer Fields
==================================

Binary records often place native integer values at known byte offsets.
The functions in :file:`ByteIntegerAccess.hpp` read and write these fields directly in a byte span, with an explicit
:cpp:enum:`Endianness <erbsland::mem::Endianness>`.
Little-endian order is the default, but binary formats should pass the byte order explicitly when doing so helps the
code mirror the format description.

:cpp:func:`getInteger() <erbsland::mem::getInteger>` returns a chosen fallback if the requested integer does not fit.
:cpp:func:`getIntegerInto() <erbsland::mem::getIntegerInto>` reports success and leaves its destination unchanged on an
invalid range.
:cpp:func:`setInteger() <erbsland::mem::setInteger>` follows the same reporting style and leaves the bytes unchanged if
the write cannot fit.
These forms work well when an invalid range is expected input that the caller can handle.

Use :cpp:func:`getIntegerOrThrow() <erbsland::mem::getIntegerOrThrow>` or
:cpp:func:`setIntegerOrThrow() <erbsland::mem::setIntegerOrThrow>` when the field layout is an invariant and an invalid
offset means the program or format description is wrong.
All functions accept signed and unsigned native integer types except ``bool``.

.. erbsland-demo::
    :source: mem/ByteSpan/IntegerAccess.cpp
    :exec: mem/byte_span --demo IntegerAccess
    :source-sha256: db88c03f7071ad47820441f84b088e85465ad44a1c7dc5940b1c60387259c6f7

.. code-block:: cpp

    /// Read and write integer fields in a borrowed byte span.
    ///
    /// The integer-access functions handle signed and unsigned native integers in
    /// little- or big-endian byte order. Their checked variants either report an
    /// invalid range without changing data, return a chosen fallback value, or
    /// throw when an invalid layout is a programming error.
    void integerAccess() {
        auto record = std::array<el::Byte, 12>{};
        auto writable = el::ByteSpan{record};

        // Encode fields in the byte order defined by the observation format.
        const auto wavelengthStored = el::setInteger(writable, el::ByteIndex{0U}, uint16_t{656U}, el::Endianness::Big);
        el::setIntegerOrThrow(writable, el::ByteIndex{2U}, int16_t{-18}, el::Endianness::Little);
        el::setIntegerOrThrow(writable, el::ByteIndex{4U}, uint32_t{1'350'000U}, el::Endianness::Big);

        // Decode fields without copying the record into an intermediate structure.
        const auto readOnly = el::ConstByteSpan{record};
        const auto wavelength = el::getIntegerOrThrow<uint16_t>(readOnly, el::ByteIndex{0U}, el::Endianness::Big);
        auto temperature = int16_t{};
        const auto temperatureRead = el::getIntegerInto(readOnly, temperature, el::ByteIndex{2U}, el::Endianness::Little);
        const auto exposure = el::getInteger<uint32_t>(readOnly, el::ByteIndex{4U}, el::Endianness::Big);
        const auto unavailable =
            el::getInteger<uint32_t>(readOnly, el::ByteIndex{10U}, el::Endianness::Big, uint32_t{999U});

        el::io::printLine("Observation        : Νεφέλωμα του Ωρίωνα"_el);
        el::io::printLine("Wavelength stored  : "_el, el::BooleanFormat::yesNo(), wavelengthStored);
        el::io::printLine("Temperature read   : "_el, el::BooleanFormat::yesNo(), temperatureRead);
        el::io::printLine("Wavelength (nm)    : "_el, wavelength);
        el::io::printLine("Temperature (C)    : "_el, temperature);
        el::io::printLine("Exposure (us)      : "_el, exposure);
        el::io::printLine("Missing fallback   : "_el, unavailable);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Observation        : Νεφέλωμα του Ωρίωνα
    Wavelength stored  : yes
    Temperature read   : yes
    Wavelength (nm)    : 656
    Temperature (C)    : -18
    Exposure (us)      : 1350000
    Missing fallback   : 999

.. erbsland-demo-end::

From Views to Purpose-Built Containers
======================================

Byte spans are deliberately minimal and should remain at low-level call boundaries.
For a fixed-size value that belongs inside one function or algorithm, :cpp:class:`ByteArray <erbsland::mem::ByteArray>`
adds bounds-aware byte and range operations while keeping the size in the type.
For data that must have an independent lifetime, a :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` or a specialized
container such as :cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` carries ownership explicitly.

This separation keeps the fast path available without letting borrowed storage quietly become long-lived application
state: spans describe a temporary view, while arrays, blocks, and buffers describe where bytes actually live.
