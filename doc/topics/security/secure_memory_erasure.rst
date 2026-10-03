.. index::
    !single: Secure Memory Erasure
    single: Memory; Secure erasure
    single: Secrets; Memory erasure
    single: ByteBlock; Secure erasure
    single: ByteArray; Secure erasure
    single: ByteBuffer; Secure erasure
    single: RingBuffer; Secure erasure

******************************
Erasing Memory in a Secure Way
******************************

Overwriting a secret as soon as it is no longer needed reduces the time that plaintext remains in process memory.
Erbsland Core provides explicit erasure for borrowed memory, fixed arrays, shared byte blocks, dynamic buffers, and ring
buffers.
This page shows how to choose the matching operation and, just as importantly, how ownership and copies affect what an
erase can reach.

Why and When to Erase Memory
============================

An ordinary assignment to zero is not a reliable security operation.
A compiler may prove that the bytes are never read again and remove the assignment, while a container may retain old
bytes in unused capacity or in an allocation abandoned during growth.
:cpp:func:`secureErase() <erbsland::mem::secureErase>` uses a platform-backed operation that is kept even when the
erased storage is no longer observed by normal program logic.

Secure erasure is useful for passwords, private keys, session material, authentication tokens, and temporary
cryptographic state after their final use.
Apply it at the earliest point where the value is no longer needed, and protect dynamic containers throughout their
lifetime when they can reallocate or discard ranges.
For the allocation-level protection available to strings and byte containers, also read
:doc:`about_sensitive_strings_and_byte_blocks`.

.. important::

    Erasure is defense in depth, not a promise that no copy exists.
    Values may also have lived in registers, stack temporaries, crash dumps, swap, operating-system buffers, or
    third-party storage.
    Avoid unnecessary copies and use higher-level protected storage when the threat model requires stronger isolation.

Shared Byte Blocks Need Shared-Ownership Awareness
==================================================

:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` and
:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` can share a copy-on-write allocation.
Their :cpp:func:`ByteBlock::secureErase() <erbsland::mem::ByteBlock::secureErase>` and
:cpp:func:`ByteBlockEditor::secureErase() <erbsland::mem::ByteBlockEditor::secureErase>` methods preserve the visible
length and replace its bytes with zeros.
When the allocation has one owner, the operation overwrites its complete capacity immediately.

Mark secret storage with
:cpp:func:`ByteBlockEditor::markAsSensitive() <erbsland::mem::ByteBlockEditor::markAsSensitive>` or
:cpp:func:`ByteBlock::markAsSensitive() <erbsland::mem::ByteBlock::markAsSensitive>` before writing the secret or
creating aliases.
The mark belongs to the allocation and ensures that old allocations are erased during replacement or after their final
owner releases them.
It cannot retroactively erase earlier copies made elsewhere.

.. erbsland-demo::
    :source: mem/SecureErase/EraseSharedBlocks.cpp
    :exec: mem/secure_erase --demo EraseSharedBlocks
    :sha256: db06b31e6b91cb7caf8fe803e6d9e3c07b8eec07e43f9a3f96f05b8a1ec2401f

.. code-block:: cpp

    /// Securely erase copy-on-write byte blocks without overlooking aliases.
    ///
    /// Marking the allocation as sensitive before it receives secret bytes ensures
    /// final cleanup. An explicit erase zeros the invoking value immediately, but a
    /// shared alias remains readable until it is erased or released.
    void eraseSharedBlocks() {
        auto workingKey = el::ByteBlockEditor{el::ByteCount{6U}};
        workingKey.markAsSensitive();

        // Mark allocated storage before writing the sensitive value.
        workingKey.set(el::ByteIndex{0U}, el::Byte{0x61U});
        workingKey.set(el::ByteIndex{1U}, el::Byte{0x73U});
        workingKey.set(el::ByteIndex{2U}, el::Byte{0x74U});
        workingKey.set(el::ByteIndex{3U}, el::Byte{0x72U});
        workingKey.set(el::ByteIndex{4U}, el::Byte{0x6fU});
        workingKey.set(el::ByteIndex{5U}, el::Byte{0x69U});

        // This read-only block shares the marked allocation with the editor.
        auto retainedKey = el::ByteBlock{workingKey};
        workingKey.secureErase();
        const auto workingCopyIsZero = workingKey.isEqualConstTime(el::ByteBlock{workingKey.length()});
        const auto retainedCopyStillExists = retainedKey.get(el::ByteIndex::zero()) != el::Byte{};

        // Erase the remaining owner when it is no longer needed.
        retainedKey.secureErase();
        const auto retainedCopyIsZero = retainedKey.isEqualConstTime(el::ByteBlock{retainedKey.length()});

        el::io::printLine("Nøgle              : Asteroide-session"_el);
        el::io::printLine("Editor erased      : "_el, el::BooleanFormat::yesNo(), workingCopyIsZero);
        el::io::printLine("Alias retained data: "_el, el::BooleanFormat::yesNo(), retainedCopyStillExists);
        el::io::printLine("Alias erased       : "_el, el::BooleanFormat::yesNo(), retainedCopyIsZero);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mNøgle              : Asteroide-session
        Editor erased      : yes
        Alias retained data: yes
        Alias erased       : yes␛[0m

.. erbsland-demo-end::

The shared case deserves special care.
If one wrapper calls ``secureErase()`` while another wrapper still exposes the same bytes, erasing the shared allocation
would unexpectedly change the other value.
The caller therefore receives zero-filled replacement storage, while the other owner continues to see the original
allocation.
That is correct copy-on-write behavior, but it means that erasing one alias does not erase all aliases.

Keep ownership simple, mark the allocation before it becomes shared, and account for every copy and slice.
If immediate erasure of one physical allocation is required, avoid creating aliases or release them before erasing the
last owner.
Constructing an editor from a read-only block creates a separate editable copy, so both allocations must be considered
independently.

Fixed Arrays and Borrowed Spans
===============================

A :cpp:class:`ByteArray <erbsland::mem::ByteArray>` owns a fixed number of bytes directly and provides
:cpp:func:`ByteArray::secureErase() <erbsland::mem::ByteArray::secureErase>` for the complete array.
This operation is explicit: the array does not remember a sensitive mode and will not erase itself automatically on
scope exit.
Arrange control flow so cleanup also happens on early returns and exceptions when those paths are possible.

For writable memory owned by another object, pass a :cpp:type:`ByteSpan <erbsland::mem::ByteSpan>` to the free
``secureErase()`` function.
The span is only a view, so the operation overwrites exactly the referenced range and does not manage the owner's
lifetime.
Make sure the span covers every byte that may contain the value, including relevant scratch capacity rather than only a
short logical prefix.
The overload for writable ``std::span`` values can similarly erase trivially copyable native word arrays used as
mathematical state.

.. erbsland-demo::
    :source: mem/SecureErase/EraseFixedStorage.cpp
    :exec: mem/secure_erase --demo EraseFixedStorage
    :sha256: e583ec709998a8c9e532ba034a1ec13746f6b8f02ff8cc98a47ac9c74f8e8e80

.. code-block:: cpp

    /// Securely erase fixed byte storage and a borrowed writable span.
    ///
    /// `ByteArray::secureErase()` handles the complete array. The free
    /// `secureErase()` function applies the same platform-backed operation to any
    /// writable `ByteSpan` without taking ownership of its storage.
    void eraseFixedStorage() {
        auto authenticationTag = el::ByteArray{
            el::Byte{0x41U},
            el::Byte{0x70U},
            el::Byte{0x6fU},
            el::Byte{0x66U},
            el::Byte{0x69U},
            el::Byte{0x73U},
        };
        auto decoderScratch = std::array{
            el::Byte{0x53U},
            el::Byte{0x70U},
            el::Byte{0x65U},
            el::Byte{0x6bU},
            el::Byte{0x74U},
            el::Byte{0x72U},
        };

        // Erase owned fixed storage through its member function.
        authenticationTag.secureErase();
        const auto tagIsZero = authenticationTag == el::ByteArray<6>{};

        // Erase caller-owned storage through a writable borrowed span.
        el::mem::secureErase(el::ByteSpan{decoderScratch});
        const auto scratchIsZero = decoderScratch == std::array<el::Byte, 6>{};

        el::io::printLine("Observation        : Apofis-spektrum"_el);
        el::io::printLine("Tag erased         : "_el, el::BooleanFormat::yesNo(), tagIsZero);
        el::io::printLine("Scratch erased     : "_el, el::BooleanFormat::yesNo(), scratchIsZero);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mObservation        : Apofis-spektrum
        Tag erased         : yes
        Scratch erased     : yes␛[0m

.. erbsland-demo-end::

Uniquely Owned Dynamic Buffers
==============================

:cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` owns its allocation without copy-on-write sharing.
Calling :cpp:func:`ByteBuffer::secureErase() <erbsland::mem::ByteBuffer::secureErase>` overwrites the complete capacity
immediately, including bytes beyond the visible length, and preserves the buffer's length and capacity for reuse.

For a buffer that remains in use across several operations, call
:cpp:func:`ByteBuffer::setSensitive() <erbsland::mem::ByteBuffer::setSensitive>` before placing secrets in it.
Sensitive mode also erases removed ranges and replaced allocations during resizing, clearing, and growth.
An explicit ``secureErase()`` is still useful at a known lifecycle boundary because it overwrites the current allocation
without waiting for destruction.
Disabling sensitive mode is intentionally destructive: it securely erases the allocation, discards all visible bytes,
and retains the capacity.

.. erbsland-demo::
    :source: mem/SecureErase/EraseDynamicBuffer.cpp
    :exec: mem/secure_erase --demo EraseDynamicBuffer
    :sha256: 7638dad5f6e1725b7f6c2e849821aa50a2d74a4af594488a9ebf252651312faa

.. code-block:: cpp

    /// Protect and explicitly erase a uniquely owned dynamic byte buffer.
    ///
    /// Sensitive mode erases discarded ranges and replaced allocations throughout
    /// the buffer's lifetime. `secureErase()` immediately overwrites its complete
    /// capacity while preserving length, capacity, and sensitive mode.
    void eraseDynamicBuffer() {
        auto sessionMaterial = el::ByteBuffer{el::ByteCount{6U}};
        sessionMaterial.setSensitive(true);
        sessionMaterial.set(el::ByteIndex{0U}, el::Byte{0x4eU});
        sessionMaterial.set(el::ByteIndex{1U}, el::Byte{0x45U});
        sessionMaterial.set(el::ByteIndex{2U}, el::Byte{0x4fU});
        sessionMaterial.set(el::ByteIndex{3U}, el::Byte{0x2dU});
        sessionMaterial.set(el::ByteIndex{4U}, el::Byte{0x31U});
        sessionMaterial.set(el::ByteIndex{5U}, el::Byte{0x37U});
        sessionMaterial.reserve(el::ByteCount{32U});
        const auto lengthBeforeErase = sessionMaterial.length();
        const auto capacityBeforeErase = sessionMaterial.capacity();

        // Overwrite visible bytes and unused capacity as soon as the session ends.
        sessionMaterial.secureErase();
        const auto zeros = el::ByteArray<6>{};

        el::io::printLine("Session            : NEO-17"_el);
        el::io::printLine(
            "Buffer erased      : "_el, el::BooleanFormat::yesNo(), sessionMaterial.isEqualConstTime(zeros.span()));
        el::io::printLine(
            "Length preserved   : "_el, el::BooleanFormat::yesNo(), sessionMaterial.length() == lengthBeforeErase);
        el::io::printLine(
            "Capacity preserved : "_el, el::BooleanFormat::yesNo(), sessionMaterial.capacity() == capacityBeforeErase);
        el::io::printLine("Sensitive mode     : "_el, el::BooleanFormat::yesNo(), sessionMaterial.isSensitive());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mSession            : NEO-17
        Buffer erased      : yes
        Length preserved   : yes
        Capacity preserved : yes
        Sensitive mode     : yes␛[0m

.. erbsland-demo-end::

Sensitive Byte Ring Buffers and Consumed-Data Erasure
=====================================================

:cpp:class:`RingBuffer <erbsland::mem::RingBuffer>` and its integer-aware subclass
:cpp:class:`ByteRingBuffer <erbsland::mem::ByteRingBuffer>` reuse storage as bytes are read and overwritten.
Enable :cpp:func:`RingBuffer::setSensitive() <erbsland::mem::RingBuffer::setSensitive>` before queueing secrets so
consumed bytes, discarded data, old growth allocations, and the final allocation are securely erased at the appropriate
lifecycle points.

Calling :cpp:func:`RingBuffer::secureErase() <erbsland::mem::RingBuffer::secureErase>` provides an immediate session
boundary: it overwrites the complete allocation and empties the queue while preserving its capacity and sensitive mode.
The operation must not overlap an active unsafe-access lease; violating that exclusive-access rule terminates the
program.
Ordinary safe reads and writes need no special coordination.

.. erbsland-demo::
    :source: mem/SecureErase/EraseRingBuffer.cpp
    :exec: mem/secure_erase --demo EraseRingBuffer
    :sha256: 5f58290eeac000548a125b7f28eb0ab693f768580c0db76683978d0552543c81

.. code-block:: cpp

    /// Securely discard queued bytes from ring buffers.
    ///
    /// Sensitive mode erases bytes as they are consumed or discarded. An explicit
    /// `secureErase()` overwrites the complete ring allocation immediately and
    /// leaves the buffer empty without changing its capacity.
    void eraseRingBuffer() {
        auto records = el::ByteRingBuffer{el::ByteCount{8U}};
        records.setSensitive(true);
        const auto writeResult = records.writeInteger<uint32_t>(0x4e454f31U);
        const auto capacityBeforeErase = records.capacity();
        const auto lengthBeforeErase = records.length();

        // End the protocol session by erasing all queued and unused storage.
        records.secureErase();

        el::io::printLine("Kø               : Asteroide-poster"_el);
        el::io::printLine("Record accepted   : "_el, el::BooleanFormat::yesNo(), writeResult.isSuccessful());
        el::io::printLine("Bytes before erase: "_el, lengthBeforeErase.toSizeT());
        el::io::printLine("Queue empty       : "_el, el::BooleanFormat::yesNo(), records.isEmpty());
        el::io::printLine("Capacity preserved: "_el, el::BooleanFormat::yesNo(), records.capacity() == capacityBeforeErase);
        el::io::printLine("Sensitive mode    : "_el, el::BooleanFormat::yesNo(), records.isSensitive());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mKø               : Asteroide-poster
        Record accepted   : yes
        Bytes before erase: 4
        Queue empty       : yes
        Capacity preserved: yes
        Sensitive mode    : yes␛[0m

.. erbsland-demo-end::

Sensitive Storage versus Explicit Memory Erasure
================================================

The most reliable design starts with the owner of the sensitive bytes.
Use an owning container with sensitive lifecycle behavior for values that grow, move, or remain alive across several
operations.
Use direct erasure for fixed scratch state or for a borrowed region whose owner cannot provide that behavior.

Whichever type you choose, erase the complete relevant allocation, keep aliases visible in the design, and avoid
creating ordinary converted or formatted copies.
Erasure then becomes a deliberate boundary around the data you actually control rather than a last-minute attempt to
recover copies that have already escaped.
