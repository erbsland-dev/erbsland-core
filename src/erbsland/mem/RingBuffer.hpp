// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteBlock.hpp"

#include "impl/UnsafeRingBufferAccess_fwd.hpp"

#include "../unit/ByteLength.hpp"
#include "../util/Result.hpp"

#include <array>
#include <span>
#include <vector>

namespace erbsland::mem {

/// A contiguous-storage byte ring with checked safe access.
/// The storage can optionally grow up to a fixed hard limit. Native APIs use `impl::UnsafeRingBufferAccess` to access
/// the two contiguous sections around the wrap point without copying.
/// @tested{RingBufferTest}
class RingBuffer {
    friend class impl::UnsafeRingBufferAccess;

public:
    /// Create a fixed-capacity ring.
    explicit RingBuffer(unit::ByteLength capacity);
    /// Create a growable ring.
    RingBuffer(unit::ByteLength initialCapacity, unit::ByteLength maximumCapacity);

    // defaults/deletions
    virtual ~RingBuffer() = default;
    RingBuffer(const RingBuffer &) = delete;
    RingBuffer(RingBuffer &&) = delete;
    auto operator=(const RingBuffer &) -> RingBuffer & = delete;
    auto operator=(RingBuffer &&) -> RingBuffer & = delete;

public: // state
    /// Get the current storage capacity.
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteLength;
    /// Get the hard storage limit.
    [[nodiscard]] auto maximumCapacity() const noexcept -> unit::ByteLength;
    /// Get the number of readable bytes.
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength;
    /// Get the writable space in the current storage.
    [[nodiscard]] auto available() const noexcept -> unit::ByteLength;
    /// Test if the ring contains no readable bytes.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _length == 0U; }
    /// Test if the current storage has no writable space.
    [[nodiscard]] auto isFull() const noexcept -> bool { return _length == _storage.size(); }
    /// Test if an atomic write can fit within the hard storage limit.
    [[nodiscard]] auto canWrite(unit::ByteLength length) const noexcept -> bool;

public: // safe access
    /// Reserve space for additional bytes without changing the visible data.
    /// @return `Success` if the requested space is available, `Failure` if it exceeds the hard limit.
    [[nodiscard]] auto reserveAdditional(unit::ByteLength length) -> util::Result;
    /// Copy as many bytes as possible into the ring.
    /// @return The number of copied bytes.
    auto write(std::span<const Byte> bytes) -> unit::ByteLength;
    /// Atomically copy all bytes into the ring.
    /// @return `Failure` without modification if the bytes exceed the hard limit.
    [[nodiscard]] auto writeExact(std::span<const Byte> bytes) -> util::Result;
    /// Copy as many buffered bytes as possible to the destination.
    /// @return The number of copied bytes.
    auto read(std::span<Byte> destination) -> unit::ByteLength;
    /// Read up to the requested number of bytes.
    [[nodiscard]] auto read(unit::ByteLength maximum) -> ByteBlock;
    /// Discard all buffered bytes.
    void clear() noexcept;
    /// Shrink empty storage back to its initial capacity.
    void shrinkToInitial();
    /// Swap complete ring state with another ring.
    void swap(RingBuffer &other) noexcept;

private:
    /// Get the readable sections around the wrap point.
    [[nodiscard]] auto readableSpans() const noexcept -> std::array<std::span<const Byte>, 2>;
    /// Get the writable sections around the wrap point.
    [[nodiscard]] auto writableSpans() noexcept -> std::array<std::span<Byte>, 2>;
    /// Make bytes written through unsafe access visible to readers.
    void commitWritten(std::size_t length);
    /// Consume bytes read through unsafe access.
    void consumeRead(std::size_t length);
    /// Start an exclusive unsafe access lease.
    void beginUnsafeAccess();
    /// End the current unsafe access lease.
    void endUnsafeAccess() noexcept;
    /// Verify that safe access is currently allowed.
    void verifySafeAccess() const;

private:
    std::vector<Byte> _storage;      ///< Contiguous ring storage.
    std::size_t _initialCapacity{};  ///< Capacity restored by `shrinkToInitial()`.
    std::size_t _maximumCapacity{};  ///< Hard storage limit.
    std::size_t _readPosition{};     ///< First readable byte.
    std::size_t _length{};           ///< Number of readable bytes.
    bool _unsafeAccessActive{false}; ///< True while an unsafe access lease exists.
};

}
