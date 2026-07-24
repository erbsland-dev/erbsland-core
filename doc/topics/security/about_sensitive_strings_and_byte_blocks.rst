..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Sensitive Strings and Byte Blocks
    single: String; Sensitive
    single: ByteBuffer; Sensitive
    single: ByteBlock; Sensitive
    single: Sensitive Data
    single: Secrets; Process Memory
    single: Security; Sensitive Memory
    single: Secure Erasure
    single: Memory; Secure Erasure

***************************************
About Sensitive Strings and Byte Blocks
***************************************

Erbsland Core can reduce the lifetime of secret remnants in heap memory.
Text secrets use a marked :cpp:type:`String <erbsland::text::String>`.
Binary secrets use an ordinary :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` or
:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` whose shared allocation is marked as sensitive, or a
:cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` in sensitive mode.

.. important::

    Sensitive storage is defense in depth, not an access-control boundary.
    Plaintext remains readable in the process address space.
    A debugger, memory-disclosure bug, injected code, crash dump, or compromised process can still expose it.

Marked Strings and Byte Storage
===============================

``String`` and ``StringEditor`` are the UTF-8 aliases and provide ``markAsSensitive()`` and ``isSensitive()``.
The mark controls secure cleanup of their shared allocation and cannot be cleared.
It does not restrict the ordinary string interface.

Byte blocks use runtime metadata instead of a separate sensitive type.
Call ``ByteBlock::markAsSensitive()`` or ``ByteBlockEditor::markAsSensitive()`` to set the mark.
The mark cannot be cleared through an API.
It controls secure cleanup of the allocation; it does not restrict which ordinary byte-block operations are available.

``ByteBuffer`` uses the reversible object mode ``setSensitive(bool)`` following ``RingBuffer``.
Each copy owns independent storage and copies the mode; moves transfer storage and mode.
Disabling the mode erases the complete allocation, discards visible bytes, and retains capacity.

Allocation-Level Marking
========================

The sensitivity mark belongs to the shared allocation, not to one wrapper object.
Copies and non-empty slices share the same allocation and observe the same mark.
Marking any alias therefore marks the complete allocation for every alias.

Copy-on-write detachment, cloning, capacity growth, shrinking, and replacement allocations preserve the mark.
Assigning or resetting a complete value can replace its allocation with an ordinary or empty one.
An empty value without allocated storage cannot retain metadata, so marking it is a no-op and ``isSensitive()`` returns
``false``.
An empty editor that still has reserved storage can be marked because it still owns an allocation.

This allocation-level behavior is important when reviewing ownership.
A small slice can keep a larger allocation alive, and that allocation is erased only after its final alias is released.

Propagation Is Type-Specific
============================

UTF-8 copies, slices, trims, copy-on-write detachment, and same-string modified results preserve their source mark.
Appending, inserting, replacing, or joining marked source text into an unmarked destination does not mark that
destination.
A marked destination remains marked during mutation.
Conversions to another string width, encoded bytes, standard-library text, escaped text, formatted text, or diagnostic
text produce ordinary unmarked results.

Operations that receive an owning byte block can observe its sensitivity mark.
Successful deep copies, appends, inserts, replacements, overwrites, XOR operations, and joins propagate sensitivity to
their destination.
Once propagated, the destination mark cannot be removed.

Borrowed spans do not carry ownership metadata.
Passing ``sensitiveBlock.span()`` to an overload that accepts ``ConstByteSpan`` therefore does **not** mark the
destination.
Use an owning-block overload when propagation is required, or mark the destination explicitly before writing.
The same limitation applies at native and third-party API boundaries.

Formatting and Ordinary Conversions
===================================

A marked byte block retains the complete ordinary byte-block interface.
It can be formatted, compared, converted with ``toByteBuffer()``, or copied into ``uint8_t`` and ``char`` vectors.
These operations can create ordinary, unprotected copies.
The sensitivity mark is not a data-flow tracking system and does not follow bytes into arbitrary containers.

This is an accepted tradeoff of the unified API.
Code that logs, displays, serializes, or converts a marked block must treat the resulting destination as a separate
security boundary.
Redact before logging and protect persisted or transmitted data independently.

When Erasure Happens
====================

When a marked allocation is replaced during reallocation or finally destroyed, Erbsland Core securely erases the
complete allocation before deallocation.
The erased region includes shared metadata, visible bytes, unused capacity, and layout padding.
The platform-specific primitive is selected so the compiler must not optimize the overwrite away.

Removing bytes, truncating, clearing, keeping a smaller range, or making a shorter replacement does not immediately
erase the vacated part of an allocation.
Those bytes remain covered by the final full-allocation erasure.
Call ``secureErase()`` when the current capacity must be overwritten immediately.

With unique storage, ``secureErase()`` overwrites the complete capacity while preserving logical length and the
sensitivity mark.
With shared storage, one alias cannot erase data still visible through another alias.
The invoking value receives zero-filled replacement storage, while the original marked allocation remains available to
its other owners and is erased after the final owner releases it.

Comparisons Are Ordinary
========================

Marked byte blocks use the same equality and ordering operations as ordinary byte blocks.
These comparisons are not constant-time.
Do not use byte-block equality as a cryptographic verification primitive.
Use a higher-level password, message-authentication, signature, or protocol verification API.

String comparisons are also ordinary and do not provide a constant-time guarantee.

Creating Sensitive Values
=========================

:cpp:class:`SecureRandom <erbsland::random::SecureRandom>` reports ``isSecure() == true``.
Its inherited :cpp:func:`Random::buildByteBlock() <erbsland::random::Random::buildByteBlock>` marks storage before the
generator writes random bytes.
Its inherited ``buildString()`` and ``buildByteBuffer()`` likewise select sensitive storage before writing.
Other random generators return ordinary byte blocks unless their ``isSecure()`` override says otherwise.
Storage-less empty results remain unmarked.

Byte input streams opened with
:cpp:func:`InputStreamSettings::setSensitive() <erbsland::stream::InputStreamSettings::setSensitive>` automatically
return marked blocks from ``read()``, ``readExact()``, ``readAll()``, and their coroutine variants.
The setting also protects library-owned retained and transport buffers.
Ordinary streams return ordinary blocks.
Text streams return marked strings from their generic read methods when the same setting is enabled.

Import Boundaries
=================

Marking an existing allocation does not erase earlier copies.
A command-line parser, terminal editor, configuration parser, file reader, network stack, or third-party library may
already have stored the same secret in ordinary memory.
Moving or releasing one wrapper does not clear unrelated allocations.

Prefer producers that mark storage before sensitive bytes are written.
When an ordinary boundary is unavoidable, shorten the ordinary value's lifetime, avoid additional copies, and erase it
explicitly when its type provides a reliable operation.

Choosing a Type
===============

Use a marked ``String`` for passwords and other text secrets that benefit from UTF-8 validation and best-effort
allocation cleanup.
Use a marked ``ByteBlock`` for binary keys, tokens, peppers, salts, and secret cryptographic intermediates.
Use a marked ``ByteBlockEditor`` while those bytes must be assembled or modified, then share it as a read-only block.
Use ``ByteBuffer::setSensitive(true)`` for directly owned, independently copied byte storage with immediate cleanup of
vacated bytes and old allocations.

Limits of the Guarantee
=======================

Sensitive storage does not:

*   lock pages or prevent swapping, suspension, or hibernation copies;
*   sanitize CPU registers, stacks, or temporary buffers created by called code;
*   erase caller-owned inputs, formatted output, vectors, or converted buffers;
*   protect crash dumps or guarantee cleanup after forced termination;
*   encrypt persisted or transmitted data;
*   prevent privileged inspection, memory-disclosure bugs, or a compromised process.

For long-lived application keys, prefer a secret manager, operating-system key store, hardware security module, or an
opaque operation that keeps plaintext outside general application memory.
When software-held plaintext is unavoidable, load it late, keep ownership simple, release it promptly, and apply the
process and deployment controls required by the threat model.
