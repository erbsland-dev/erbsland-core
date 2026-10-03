..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Custom Implementation
    single: Streams; Subclass Contract
    single: Streams; Testing

**********************
Writing Custom Streams
**********************

.. erbsland-draft::

Custom streams connect a new source or destination to the public bounded stream contract.
This page builds small byte streams and explains settings, short reads, atomic output, lifecycle, errors, positioning,
thread safety, coroutine ownership, and contract tests.

Choose the Narrowest Base
=========================

Derive from ``ByteInputStream`` or ``ByteOutputStream`` when the source is fundamentally binary.
Build text behavior as a decoder or encoder around a byte boundary only when the custom format owns an encoding.
Do not derive a text stream merely because the current payload happens to contain UTF-8 bytes.

The sample types keep state with their data and expose shared factories for coroutine use.

.. erbsland-demo::
    :source: stream/CustomStreams/SkillTreeStreams.hpp
    :sha256: 02982ddc42467cb7597190fc8472807d07000b20be80135d8d62e2a2ae6f7811

.. code-block:: cpp

    /// A bounded byte-input stream for serialized character skill trees.
    class SkillTreeInputStream final : public el::ByteInputStream {
    public:
        [[nodiscard]] static auto create(std::vector<uint8_t> bytes) -> std::shared_ptr<SkillTreeInputStream>;
        explicit SkillTreeInputStream(std::vector<uint8_t> bytes);

    public: // implement InputStream
        [[nodiscard]] auto inputSettings() const noexcept -> const el::InputStreamSettings & override { return _settings; }
        [[nodiscard]] auto state() const noexcept -> el::StreamState override { return _state; }
        [[nodiscard]] auto isReady() const noexcept -> bool override { return _state == el::StreamState::Open; }
        [[nodiscard]] auto waitForReady() -> el::StreamWaitStatus override;
        auto close() -> el::StreamCloseStatus override;
        void abort() noexcept override;

    protected: // implement ByteInputStream
        [[nodiscard]] auto readFromSource(std::span<el::Byte> destination, ReadDeadline deadline)
            -> el::StreamReadResult<el::ByteCount> override;

    private:
        std::vector<uint8_t> _bytes;
        std::size_t _position{0U};
        el::InputStreamSettings _settings;
        el::StreamState _state{el::StreamState::Open};
    };

    /// An atomic in-memory byte-output stream for serialized skill trees.
    class SkillTreeOutputStream final : public el::ByteOutputStream {
    public:
        [[nodiscard]] static auto create() -> std::shared_ptr<SkillTreeOutputStream>;

    public: // accessors
        [[nodiscard]] auto bytes() const noexcept -> const std::vector<uint8_t> & { return _bytes; }

    public: // implement OutputStream
        using el::ByteOutputStream::write;

        [[nodiscard]] auto outputSettings() const noexcept -> const el::OutputStreamSettings & override {
            return _settings;
        }
        [[nodiscard]] auto state() const noexcept -> el::StreamState override { return _state; }
        [[nodiscard]] auto isReady() const noexcept -> bool override { return _state == el::StreamState::Open; }
        [[nodiscard]] auto waitForReady() -> el::StreamWaitStatus override;
        auto flush() -> el::StreamWriteStatus override;
        auto close() -> el::StreamCloseStatus override;
        void abort() noexcept override;
        auto write(std::span<const el::Byte> bytes) -> el::StreamWriteStatus override;

    private:
        std::vector<uint8_t> _bytes;
        el::OutputStreamSettings _settings;
        el::StreamState _state{el::StreamState::Open};
    };

.. erbsland-demo-end::

Implement Bounded Input
=======================

A byte input subclass implements immutable settings, lifecycle, readiness, and
``readFromSource(destination, deadline)``.
Return a short ``Data`` result whenever some bytes are available, ``Finished`` only at normal end, and ``Timeout`` when
the deadline expires before progress.

The base class serializes logical reads and supplies exact reads, aggregate reads, integer helpers, retained input, and
coroutine wrappers.
Call ``discardRetainedInput()`` when closing or aborting.

.. erbsland-demo::
    :source: stream/CustomStreams/ReadCustomStream.cpp
    :exec: stream/custom_streams --demo ReadCustomStream
    :sha256: 9784547e015ef421fe8090b6cce6f0270963a0b0c8c6dedf955e43d5e2d93326

.. code-block:: cpp

    /// Derive a custom byte input from `ByteInputStream` and implement one bounded `readFromSource()` operation.
    /// The base class supplies exact reads, aggregate reads, integer helpers, retained partial input, and coroutine
    /// wrappers.
    void readCustomStream() {
        auto input = SkillTreeInputStream{{1U, 4U, 2U, 8U, 5U, 7U}};
        const auto root = input.readExact(el::ByteCount{4U});
        el::io::printLine("Skill-tree root-node bytes: "_el, root.data().length().toSizeT());
        el::io::printLine("The base class combines the underlying short reads."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mSkill-tree root-node bytes: 4
        The base class combines the underlying short reads.␛[0m

.. erbsland-demo-end::

Implement Atomic Output
=======================

An output subclass must accept a complete span or none of it.
Validate state and request limits first, then copy or reserve all storage before publishing the request.
Use a mutex or another explicit serialization design when multiple producers can call the stream.

``flush()`` confirms native flushing, graceful ``close()`` drains accepted data, and ``abort()`` returns immediately.
The destructor must call the non-blocking abort path rather than waiting for close.

.. erbsland-demo::
    :source: stream/CustomStreams/WriteCustomStream.cpp
    :exec: stream/custom_streams --demo WriteCustomStream
    :sha256: 58637d8b197874fa183756c2fe185d07ade900f0c93510d820290fb0acdcd5df

.. code-block:: cpp

    /// A custom output stream must accept each write completely or return timeout without accepting anything.
    /// Keep settings immutable, make lifecycle transitions explicit, and serialize access when multiple threads can write.
    void writeCustomStream() {
        const auto output = SkillTreeOutputStream::create();
        output->setEndianness(el::Endianness::Little);
        output->writeUInt16(12U);
        output->writeUInt16(3U);
        output->flush();

        el::io::printLine("Atomically written bytes: "_el, output->bytes().size());
        output->close();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mAtomically written bytes: 4␛[0m

.. erbsland-demo-end::

Forward Errors and Positioning Honestly
=======================================

Call ``throwError(title, description)`` for local failures.
Decorators should override ``createErrorContext()`` and return the backing stream's context, adding their own context
data when needed, so paths and native details are preserved.
Move the state to ``Failed`` before throwing when the stream cannot continue.

Leave the default positioning methods in place unless the complete implementation can honor logical byte positions.
An input implementation must discard read-ahead and retained state after movement.
An output implementation must serialize the change with queued output and leave the position unchanged on timeout.

Support Coroutine Ownership Deliberately
========================================

Inherited ``co...`` methods require the object to be owned by ``std::shared_ptr``.
Provide a factory when asynchronous use is part of the type's contract; retain stack construction only for synchronous
algorithms and tests.

.. erbsland-demo::
    :source: stream/CustomStreams/UseCustomStreamAsynchronously.cpp
    :exec: stream/custom_streams --demo UseCustomStreamAsynchronously
    :sha256: 6b07a4c83797ba78e636a6cbe668ffb368ff810e6c4f12e2850c6f5d77aa7399

.. code-block:: cpp

    /// Provide a shared factory when callers should use inherited coroutine methods.
    /// The coroutine wrapper retains the custom stream while its bounded source operation runs on the worker service.
    void useCustomStreamAsynchronously() {
        const auto input = SkillTreeInputStream::create({9U, 8U, 7U, 6U});
        auto task = input->coReadExact(el::ByteCount{4U});
        waitForTask(task);
        el::io::printLine("Asynchronous read completed: "_el, task.result().hasData());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mAsynchronous read completed: true␛[0m

.. erbsland-demo-end::

Test the Complete Contract
==========================

Test custom streams with deliberately small buffers and controlled sources.
Cover short reads, exact-read retention across timeout, switching logical read operations, end-of-stream, request
atomicity, readiness, flush, repeated close, abort, stored failures, and optional positioning.
For coroutine-enabled types, also verify shared retention, exception propagation, single-consumer behavior, and task or
generator cancellation.

.. erbsland-demo::
    :source: stream/CustomStreams/VerifyCustomContracts.cpp
    :exec: stream/custom_streams --demo VerifyCustomContracts
    :sha256: 45eb02efe557792fa92961d3eb5a832172198e0bc3a5bdaadf9cf4ea3c9f45fa

.. code-block:: cpp

    /// Test custom streams through their public contract.
    /// Cover short reads, timeout retention, atomic writes, close and abort, failures, optional positioning, and the shared
    /// ownership requirement for coroutine methods.
    void verifyCustomContracts() {
        auto input = SkillTreeInputStream{{1U, 2U, 3U, 4U, 5U}};
        const auto exact = input.readExact(el::ByteCount{5U});
        const auto finished = input.readByte();
        auto output = SkillTreeOutputStream{};
        const auto write = output.writeUInt32(0x01020304U);
        const auto close = output.close();

        el::io::printLine("Exact read: "_el, exact.hasData());
        el::io::printLine("Stream finished: "_el, finished.isFinished());
        el::io::printLine("Atomic write: "_el, write.isSuccess());
        el::io::printLine("Closed normally: "_el, close.isClosed());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mExact read: true
        Stream finished: true
        Atomic write: true
        Closed normally: true␛[0m

.. erbsland-demo-end::
