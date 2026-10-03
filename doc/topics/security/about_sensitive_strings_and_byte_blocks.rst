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

Passwords, tokens, and other secrets can remain in heap memory after your code stops using them.
Erbsland Core's sensitive-storage mark tells its string and byte containers to erase their allocations before releasing
them.
You keep the ordinary container interface while reducing the lifetime of those remnants.

This page explains how to choose a container, how the mark follows shared storage and copied data, and when erasure
happens.
It also shows where conversions and borrowed views require you to take responsibility for a separate copy.

.. important::

    Sensitive storage is defense in depth, not an access-control boundary.
    Plaintext remains readable in the process address space.
    A debugger, memory-disclosure bug, injected code, crash dump, or compromised process can still expose it.

Choosing Storage for a Secret
=============================

For passwords and other UTF-8 text, :cpp:type:`String <erbsland::text::String>` and
:cpp:type:`StringEditor <erbsland::text::StringEditor>` provide ``markAsSensitive()`` and ``isSensitive()``.
The mark controls cleanup of their shared allocation and cannot be cleared.
It does not prevent ordinary operations such as reading, comparing, or displaying the text.

For binary keys or tokens, :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` provides the same allocation-level mark.
When you need to assemble or modify the bytes, a
:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` keeps that mark while you edit.
Both types share storage, so the last owner determines when the allocation can be erased.

:cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` is useful when you need independently owned mutable storage.
Its ``setSensitive(true)`` mode also erases vacated bytes during mutation.
Copies own independent storage and copy the mode; moves transfer storage and mode.
Disabling the mode erases the complete allocation and discards its visible bytes while retaining capacity.

Receiving Sensitive Values
==========================

Whenever possible, choose a producer that marks storage before it writes secret bytes.
For example, :cpp:class:`SecureRandom <erbsland::random::SecureRandom>` creates sensitive results through
``buildByteBlock()``, ``buildString()``, and ``buildByteBuffer()``.
An empty result that has no storage has no allocation to erase.

For file or stream input, enable
:cpp:func:`InputStreamSettings::setSensitive() <erbsland::stream::InputStreamSettings::setSensitive>` when opening
the stream.
Byte-stream reads, including their coroutine variants, then return marked blocks and protect the library's retained and
transport buffers.
Text streams return marked strings from their generic read methods with the same setting.
This keeps the cleanup policy attached to input as you pass it through your application.

Secure-storage placeholders also return marked UTF-8 text, which keeps its mark through built-in filters and
replacement.
See :doc:`/topics/cryptology/configuring_tls` for an example that reads a signing-key password this way.
Memory sensitivity and configuration secrecy serve different purposes: mark secret configuration fields in your
validation rules as well, so logging and export can recognize them.

Allocation-Level Marking
========================

The sensitivity mark belongs to the shared allocation, not to one wrapper object.
Copies and non-empty slices share the same allocation and observe the same mark.
Marking any alias therefore marks the complete allocation for every alias.

Copy-on-write detachment, cloning, capacity growth, shrinking, and replacement allocations preserve the mark.
Assigning or resetting a complete value can replace its allocation with an ordinary or empty one.
An empty byte block without allocated storage cannot retain metadata, so marking it is a no-op and ``isSensitive()``
returns ``false``.
An explicitly marked empty UTF-8 string does retain the mark.
This matters when an empty password passes through a placeholder filter: an empty value can still be sensitive.
An empty editor that still has reserved storage can be marked because it still owns an allocation.

This allocation-level behavior is important when reviewing ownership.
A small slice can keep a larger allocation alive, and that allocation is erased only after its final alias is released.

Building and Copying Sensitive Values
=====================================

When you build a message from marked UTF-8 text, the destination becomes sensitive too.
Appending, inserting, replacing, and joining mark the destination before copying the source bytes.
Copies, slices, trims, escaped text, and modified results preserve the source mark, and a marked destination stays
marked during mutation.
UTF-8 builders and formatters also preserve marked string arguments.
For example, a formatted string containing a marked password receives sensitive storage, but it still contains a
readable password; the mark does not redact output.

Owning byte blocks carry the same cleanup information into deep copies, appends, inserts, replacements, overwrites, XOR
operations, and joins.
The destination keeps the mark once it has received sensitive data.

Borrowed Views and Conversions
------------------------------

A borrowed span carries bytes without their ownership metadata.
If you pass ``sensitiveBlock.span()`` to an overload accepting ``ConstByteSpan``, the destination cannot detect the
source's mark.
Pass the owning block when the operation supports it, or mark the destination before copying the bytes.
This distinction also matters when passing data to native or third-party APIs.

String conversions to another character width, encoded bytes, or standard-library strings produce ordinary unmarked
results.
Similarly, a marked byte block can be converted to a byte buffer, formatted, or copied into a vector without protecting
that separate destination.
Custom formatters and diagnostic code are responsible for the copies they create.

Treat each such conversion as a new place where the secret is stored.
Keep its lifetime short, arrange cleanup where the destination supports it, and redact secrets before logging them.
Sensitive storage governs memory cleanup; persisted and transmitted data need their own protection.

Copies Made Before Marking
--------------------------

Marking an existing allocation does not erase earlier copies.
A command-line parser, terminal editor, configuration parser, file reader, network stack, or third-party library may
already have stored the same secret in ordinary memory.
Moving or releasing one wrapper does not clear unrelated allocations.

Prefer producers that mark storage before sensitive bytes are written.
When an ordinary boundary is unavoidable, shorten the ordinary value's lifetime, avoid additional copies, and erase it
explicitly when its type provides a reliable operation.

When Erasure Happens
====================

When a marked allocation is replaced during reallocation or finally destroyed, Erbsland Core securely erases the
complete allocation before deallocation.
Erasure covers the allocation's full capacity, including unused space that may contain earlier values.

Removing bytes, truncating, clearing, keeping a smaller range, or making a shorter replacement does not immediately
erase the vacated part of an allocation.
Those bytes remain covered by the final full-allocation erasure.
Call ``secureErase()`` when the current capacity must be overwritten immediately.

With unique storage, ``secureErase()`` overwrites the complete capacity while preserving logical length and the
sensitivity mark.
With shared storage, one alias cannot erase data still visible through another alias.
The invoking value receives zero-filled replacement storage, while the original marked allocation remains available to
its other owners and is erased after the final owner releases it.

Comparison Timing
=================

Marked byte blocks use the same equality and ordering operations as ordinary byte blocks.
These comparisons are not constant-time.
Byte arrays, blocks, block editors, and buffers provide ``isEqualConstTime()`` for explicit equality checks without
content-dependent short-circuiting.
For equal-length operands, this operation inspects every byte.
A length mismatch returns immediately, so operand lengths remain observable.
Where available, prefer a higher-level password, message-authentication, signature, or protocol verification API.

String comparisons are also ordinary and do not provide a constant-time guarantee.

Limits of the Guarantee
=======================

Sensitive storage does not:

*   lock pages or prevent swapping, suspension, or hibernation copies;
*   sanitize CPU registers, stacks, or temporary buffers created by called code;
*   erase earlier copies or copies held in unmarked output, vectors, or converted buffers;
*   protect crash dumps or guarantee cleanup after forced termination;
*   encrypt persisted or transmitted data;
*   prevent privileged inspection, memory-disclosure bugs, or a compromised process.

For long-lived application keys, prefer a secret manager, operating-system key store, hardware security module, or an
opaque operation that keeps plaintext outside general application memory.
When software-held plaintext is unavoidable, load it late, keep ownership simple, release it promptly, and apply the
process and deployment controls required by the threat model.
