// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Blake2b.hpp"

#include "../SecureEraseGuard.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../math/IntegerBitOperations.hpp"
#include "../../../mem/ByteArray.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/ByteBuffer.hpp"
#include "../../../mem/ByteIntegerAccess.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../mem/impl/SecureErase.hpp"
#include "../../../unit/ByteLength.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace erbsland::cryptology::impl {

// algorithms are never included in the public API, therefore using these namespaces never leaks.
using namespace erbsland::unit;
using namespace erbsland::mem;

/// Parameters for the internal Argon2id primitive.
/// @tested{PasswordPrimitiveTest}
struct Argon2idParameters final {
    uint32_t memoryKiB;       ///< The requested memory size m in 1024-byte blocks.
    uint32_t passes;          ///< The number of passes t over memory.
    uint32_t lanes;           ///< The degree of parallelism p.
    std::size_t outputLength; ///< The requested tag length T in bytes.
};

/// The internal Argon2id version 1.3 primitive.
/// @tested{PasswordPrimitiveTest}
class Argon2id final {
public:
    /// Create an Argon2id primitive with validated parameters.
    /// @param parameters The validated memory, pass, lane, and output dimensions.
    /// @throws err::ParameterError If a parameter is invalid or exceeds the implementation's safety limits.
    explicit Argon2id(const Argon2idParameters parameters) : _parameters{parameters} {
        validateParameters();
        // RFC 9106 section 3.1 fixes the synchronization-point count SL at four. Round m down to a multiple of 4*p so
        // every lane has four equal segments.
        const auto alignment = 4U * _parameters.lanes;
        _memoryBlocks = (_parameters.memoryKiB / alignment) * alignment;
        _laneLength = _memoryBlocks / _parameters.lanes;
        _segmentLength = _laneLength / 4U;
    }

public:
    /// Derive bytes with Argon2id version 1.3.
    ///
    /// Argon2id uses data-independent addresses for the first half of pass zero, limiting cache-timing leakage while
    /// the password still dominates the state. The remainder uses data-dependent references for stronger resistance to
    /// time-memory tradeoffs.
    ///
    /// Source: https://www.rfc-editor.org/rfc/rfc9106.html
    /// @param password The password P.
    /// @param salt The nonce/salt S.
    /// @param secret The optional secret value K.
    /// @param associatedData The optional associated data X.
    /// @return The Argon2id tag in marked storage.
    /// @throws err::ParameterError If a parameter is invalid or exceeds the implementation's safety limits.
    /// Covered by PasswordPrimitiveTest.
    [[nodiscard]] auto derive(
        const ConstByteSpan password,
        const ConstByteSpan salt,
        const ConstByteSpan secret = {},
        const ConstByteSpan associatedData = {}) const -> ByteBlockEditor {
        auto initialHash = createInitialHash(password, salt, secret, associatedData);
        auto memory = initializeMemory(initialHash);
        [[maybe_unused]] const auto memoryErase = SecureEraseGuard{memory};
        fillMemory(memory);
        return finalize(memory);
    }

private:
    /// Argon2's variable-length hash H'.
    ///
    /// Outputs up to 64 bytes directly with BLAKE2b. Longer outputs form a chain of 64-byte BLAKE2b values, taking
    /// 32 bytes from each intermediate value and a variable-sized final value.
    /// @param input The input A.
    /// @param outputLength The requested output length T.
    /// @return H'^T(A) in marked storage.
    /// @throws err::ParameterError If the output length cannot be encoded as Argon2's 32-bit T.
    /// Covered by PasswordPrimitiveTest.
    [[nodiscard]] static auto variableHash(const ConstByteSpan input, const std::size_t outputLength)
        -> ByteBlockEditor {
        if (outputLength == 0U || outputLength > std::numeric_limits<uint32_t>::max()) {
            throw err::ParameterError{"Invalid Argon2 output length", "outputLength"};
        }
        // RFC 9106 section 3.3 prefixes every H' input with LE32(T).
        auto prefixed = ByteBlockEditor{};
        prefixed.appendInteger(static_cast<uint32_t>(outputLength));
        prefixed.markAsSensitive();
        prefixed.append(input);
        if (outputLength <= 64U) {
            // For T <= 64, H'^T(A) is one BLAKE2b invocation configured for T output bytes.
            return Blake2b{ByteLength::fromSizeT(outputLength)}.digest(prefixed.span());
        }
        auto result = ByteBlockEditor{ByteLength::fromSizeT(outputLength)};
        result.markAsSensitive();
        auto value = Blake2b{ByteLength{64U}}.digest(prefixed.span());
        // For longer output, r = ceil(T/32)-2. Each V1..Vr contributes its first 32 bytes.
        const auto rounds = (outputLength + 31U) / 32U - 2U;
        auto position = std::size_t{0};
        for (auto round = std::size_t{0}; round < rounds; ++round) {
            result.overwrite(
                {ByteIndex::fromSizeT(position), ByteLength{32U}}, value.slice(ByteIndex::zero(), ByteLength{32U}));
            position += 32U;
            if (round + 1U < rounds) {
                // RFC 9106 defines Vi = BLAKE2b-512(Vi-1) for intermediate chain values.
                value = Blake2b{ByteLength{64U}}.digest(value.span());
            }
        }
        // Vr+1 uses exactly T-32r output bytes, completing H' without truncating a longer final digest.
        value = Blake2b{ByteLength::fromSizeT(outputLength - position)}.digest(value.span());
        result.overwrite({ByteIndex::fromSizeT(position), ByteLength::fromSizeT(outputLength - position)}, value);
        return result;
    }

    /// Argon2's modified BLAKE2 round over sixteen 64-bit words.
    /// @param words The complete 1024-byte block represented as 128 little-endian words.
    /// @param index The sixteen word indices forming one row or paired column.
    /// Covered by PasswordPrimitiveTest.
    static void round(std::array<uint64_t, 128> &words, const std::array<std::size_t, 16> &index) noexcept {
        const auto combine = [](uint64_t &a, uint64_t &b, uint64_t &c, uint64_t &d) noexcept -> void {
            // RFC 9106 section 3.6 modifies BLAKE2b's addition with 2*low32(x)*low32(y), the BlaMka operation.
            const auto add = [](const uint64_t x, const uint64_t y) noexcept -> uint64_t {
                const auto product =
                    static_cast<uint64_t>(static_cast<uint64_t>(static_cast<uint32_t>(x)) * static_cast<uint32_t>(y));
                return static_cast<uint64_t>(x + y + 2U * product);
            };
            // The 32, 24, 16, and 63-bit rotations are the GB permutation constants from RFC 9106 section 3.6.
            a = add(a, b);
            d = math::rotateRight(d ^ a, 32);
            c = add(c, d);
            b = math::rotateRight(b ^ c, 24);
            a = add(a, b);
            d = math::rotateRight(d ^ a, 16);
            c = add(c, d);
            b = math::rotateRight(b ^ c, 63);
        };
        const auto g =
            [&](const std::size_t a, const std::size_t b, const std::size_t c, const std::size_t d) noexcept -> void {
            combine(words[index[a]], words[index[b]], words[index[c]], words[index[d]]);
        };
        // Four column GB calls followed by four diagonal GB calls implement one permutation P.
        g(0, 4, 8, 12);
        g(1, 5, 9, 13);
        g(2, 6, 10, 14);
        g(3, 7, 11, 15);
        g(0, 5, 10, 15);
        g(1, 6, 11, 12);
        g(2, 7, 8, 13);
        g(3, 4, 9, 14);
    }

    /// Argon2's 1024-byte compression function G.
    ///
    /// The input blocks are XORed, permuted first by rows and then by paired columns, and finally XORed with the
    /// original input difference. From pass two onward, version 1.3 also XORs the block previously stored at the output
    /// position.
    /// @param first The first 1024-byte input X.
    /// @param second The second 1024-byte input Y.
    /// @param output The output block, also carrying the old value when `withXor` is true.
    /// @param outputOffset The start of the output block in the contiguous memory buffer.
    /// @param withXor Whether Argon2 version 1.3 must XOR the previous output block on passes after the first.
    /// Covered by PasswordPrimitiveTest.
    static void fillBlock(
        const ConstByteSpan first,
        const ConstByteSpan second,
        ByteBuffer &output,
        const ByteIndex outputOffset,
        const bool withXor) noexcept {
        // RFC 9106 section 3.5 begins with R = X XOR Y, interpreted as 128 little-endian 64-bit words.
        auto original = std::array<uint64_t, 128>{};
        auto words = std::array<uint64_t, 128>{};
        for (auto i = std::size_t{0}; i < 128U; ++i) {
            const auto offset = i * sizeof(uint64_t);
            original[i] = getInteger<uint64_t>(first, ByteIndex::fromSizeT(offset)) ^
                getInteger<uint64_t>(second, ByteIndex::fromSizeT(offset));
            words[i] = original[i];
        }
        // View R as an 8x8 matrix of 16-byte registers and apply P to each row (sixteen words per row).
        for (auto row = std::size_t{0}; row < 8U; ++row) {
            auto indexes = std::array<std::size_t, 16>{};
            for (auto i = std::size_t{0}; i < 16U; ++i) {
                indexes[i] = row * 16U + i;
            }
            round(words, indexes);
        }
        // Apply P to paired 64-bit words from each register column, matching Figure 15 of RFC 9106.
        for (auto column = std::size_t{0}; column < 8U; ++column) {
            auto indexes = std::array<std::size_t, 16>{};
            for (auto row = std::size_t{0}; row < 8U; ++row) {
                indexes[row * 2U] = row * 16U + column * 2U;
                indexes[row * 2U + 1U] = row * 16U + column * 2U + 1U;
            }
            round(words, indexes);
        }
        // The compression output is Z XOR R. Starting with pass one, Argon2 v1.3 additionally XORs the old destination.
        for (auto i = std::size_t{0}; i < 128U; ++i) {
            auto value = words[i] ^ original[i];
            const auto offset = outputOffset + ByteLength::fromSizeT(i * sizeof(uint64_t));
            if (withXor) {
                value ^= output.getInteger<uint64_t>(offset);
            }
            static_cast<void>(output.setInteger(offset, value));
        }
        mem::impl::secureErase(std::as_writable_bytes(std::span{original}));
        mem::impl::secureErase(std::as_writable_bytes(std::span{words}));
    }

    /// Read one 64-bit word from an Argon2 block.
    /// @param block The readable block.
    /// @param index The 64-bit word index.
    /// @return The decoded little-endian word.
    /// Covered by PasswordPrimitiveTest.
    [[nodiscard]] static auto blockWord(const ConstByteSpan block, const std::size_t index) noexcept -> uint64_t {
        return getInteger<uint64_t>(block, ByteIndex::fromSizeT(index * sizeof(uint64_t)));
    }

    /// Calculate the reference-block index described by RFC 9106 section 3.4.1.3.
    /// @param pass The zero-based pass r.
    /// @param slice The zero-based slice sl.
    /// @param index The index within the current segment.
    /// @param sameLane Whether the reference lane equals the current lane.
    /// @param laneLength The number q of blocks in one lane.
    /// @param segmentLength The number q/4 of blocks in one segment.
    /// @param pseudoRandom The J1 value selecting a nonuniform position.
    /// @return The referenced block index z within the selected lane.
    /// Covered by PasswordPrimitiveTest.
    [[nodiscard]] static auto referenceIndex(
        const uint32_t pass,
        const uint32_t slice,
        const std::size_t index,
        const bool sameLane,
        const uint32_t laneLength,
        const uint32_t segmentLength,
        const uint32_t pseudoRandom) noexcept -> uint32_t {
        // Construct |W|, the size of the RFC 9106 reference area. The current predecessor is excluded; cross-lane
        // references also exclude an unfinished final block at a segment boundary.
        auto area = uint64_t{0};
        if (pass == 0U) {
            if (slice == 0U) {
                area = index - 1U;
            } else if (sameLane) {
                area = static_cast<uint64_t>(slice) * segmentLength + index - 1U;
            } else {
                area = static_cast<uint64_t>(slice) * segmentLength - (index == 0U ? 1U : 0U);
            }
        } else if (sameLane) {
            area = static_cast<uint64_t>(laneLength) - segmentLength + index - 1U;
        } else {
            area = static_cast<uint64_t>(laneLength) - segmentLength - (index == 0U ? 1U : 0U);
        }
        // RFC 9106 Figure 13 maps J1 nonuniformly without floating point:
        // x = J1^2 >> 32, y = |W|*x >> 32, zz = |W|-1-y.
        auto relative = static_cast<uint64_t>(pseudoRandom);
        relative = (relative * relative) >> 32U;
        relative = area - 1U - ((area * relative) >> 32U);
        // On later passes, the reference window wraps at the lane boundary and begins after the current slice.
        const auto start =
            pass == 0U ? uint64_t{0} : (slice == 3U ? uint64_t{0} : static_cast<uint64_t>(slice + 1U) * segmentLength);
        return static_cast<uint32_t>((start + relative) % laneLength);
    }

    /// Validate the configured Argon2id dimensions.
    /// @throws err::ParameterError If a parameter is invalid or exceeds the implementation's safety limits.
    void validateParameters() const {
        if (_parameters.lanes == 0U || _parameters.lanes > 16U || _parameters.passes == 0U ||
            _parameters.passes > 10U || _parameters.memoryKiB < 8U * _parameters.lanes ||
            _parameters.memoryKiB > 1024U * 1024U || _parameters.outputLength < 4U ||
            _parameters.outputLength > std::numeric_limits<uint32_t>::max()) {
            throw err::ParameterError{"Invalid or unsafe Argon2id parameters", "parameters"};
        }
    }

    /// Create the Argon2id initial hash H0.
    /// @param password The password P.
    /// @param salt The nonce/salt S.
    /// @param secret The optional secret value K.
    /// @param associatedData The optional associated data X.
    /// @return The 64-byte initial hash H0.
    [[nodiscard]] auto createInitialHash(
        const ConstByteSpan password,
        const ConstByteSpan salt,
        const ConstByteSpan secret,
        const ConstByteSpan associatedData) const -> ByteBlockEditor {
        // RFC 9106 section 3.2 step 1 / Figure 1: build H0's complete LE32-encoded parameter and input sequence.
        auto initialInput = ByteBlockEditor{};
        initialInput.appendInteger(_parameters.lanes)
            .appendInteger(static_cast<uint32_t>(_parameters.outputLength))
            .appendInteger(_parameters.memoryKiB)
            .appendInteger(_parameters.passes)
            .appendInteger(uint32_t{0x13U}) // Version 0x13 identifies Argon2 version 1.3.
            .appendInteger(uint32_t{2U});   // Type value 2 identifies Argon2id.
        initialInput.markAsSensitive();
        const auto appendField = [&](const ConstByteSpan field) -> void {
            initialInput.appendInteger(static_cast<uint32_t>(field.size())).append(field);
        };
        appendField(password);
        appendField(salt);
        appendField(secret);
        appendField(associatedData);
        // H0 is a 64-byte BLAKE2b digest of the encoded input.
        return Blake2b{ByteLength{64U}}.digest(initialInput.span());
    }

    /// Allocate and initialize the Argon2id memory.
    /// @param initialHash The 64-byte initial hash H0.
    /// @return The initialized memory blocks.
    [[nodiscard]] auto initializeMemory(const ByteBlockEditor &initialHash) const -> ByteBuffer {
        // Step 2: allocate m' blocks. Argon2 fixes each block at 1024 bytes.
        auto memory = ByteBuffer{ByteLength::fromSizeT(static_cast<std::size_t>(_memoryBlocks) * 1024U)};
        auto memoryErase = SecureEraseGuard{memory};
        auto blockInput = ByteBlockEditor{ByteBlock{initialHash}};
        // Steps 3 and 4 append LE32(block index) and LE32(lane) to the 64-byte H0, hence the 72-byte input.
        blockInput.appendInteger(uint32_t{}).appendInteger(uint32_t{});
        for (auto lane = uint32_t{0}; lane < _parameters.lanes; ++lane) {
            for (auto block = uint32_t{0}; block < 2U; ++block) {
                blockInput.setIntegerOrThrow(ByteIndex{64U}, block);
                blockInput.setIntegerOrThrow(ByteIndex{68U}, lane);
                auto generated = variableHash(blockInput.span(), 1024U);
                const auto offset = (static_cast<std::size_t>(lane) * _laneLength + block) * 1024U;
                memory.overwrite({ByteIndex::fromSizeT(offset), ByteLength{1024U}}, generated.span());
            }
        }
        memoryErase.release();
        return memory;
    }

    /// Fill every memory segment for every pass.
    /// @param memory The initialized memory blocks.
    void fillMemory(ByteBuffer &memory) const {
        // Steps 5 and 6 fill all four slices of every lane for each pass. The zero block is reused by Argon2i-style
        // address generation during the first half of pass zero.
        auto zeroBlock = ByteArray<1024U>{};
        for (auto pass = uint32_t{0}; pass < _parameters.passes; ++pass) {
            for (auto slice = uint32_t{0}; slice < 4U; ++slice) {
                for (auto lane = uint32_t{0}; lane < _parameters.lanes; ++lane) {
                    fillSegment(memory, zeroBlock.span(), pass, slice, lane);
                }
            }
        }
    }

    /// Fill one lane segment.
    /// @param memory All writable Argon2id memory blocks.
    /// @param zeroBytes The reusable zero block for data-independent addressing.
    /// @param pass The zero-based pass.
    /// @param slice The zero-based slice.
    /// @param lane The zero-based lane.
    void fillSegment(
        ByteBuffer &memory,
        const ConstByteSpan zeroBytes,
        const uint32_t pass,
        const uint32_t slice,
        const uint32_t lane) const {
        // RFC 9106 section 3.4.1.3: Argon2id is data-independent for slices 0 and 1 of pass zero, and
        // data-dependent everywhere else.
        const auto independent = pass == 0U && slice < 2U;
        auto inputBlock = ByteBuffer{ByteLength{1024U}};
        [[maybe_unused]] const auto inputBlockErase = SecureEraseGuard{inputBlock};
        auto addressBlock = ByteBuffer{ByteLength{1024U}};
        [[maybe_unused]] const auto addressBlockErase = SecureEraseGuard{addressBlock};
        inputBlock.setIntegerOrThrow(ByteIndex{0U}, static_cast<uint64_t>(pass));
        inputBlock.setIntegerOrThrow(ByteIndex{8U}, static_cast<uint64_t>(lane));
        inputBlock.setIntegerOrThrow(ByteIndex{16U}, static_cast<uint64_t>(slice));
        inputBlock.setIntegerOrThrow(ByteIndex{24U}, static_cast<uint64_t>(_memoryBlocks));
        inputBlock.setIntegerOrThrow(ByteIndex{32U}, static_cast<uint64_t>(_parameters.passes));
        inputBlock.setIntegerOrThrow(ByteIndex{40U}, uint64_t{2U}); // Argon2 type y=2 selects Argon2id.
        const auto start = pass == 0U && slice == 0U ? std::size_t{2U} : std::size_t{0U};
        const auto nextAddresses = [&]() -> void {
            // RFC 9106 section 3.4.1.2 increments the input counter, applies G(zero, G(zero, input)), and
            // interprets the resulting 1024 bytes as 128 consecutive J1/J2 address pairs.
            const auto counterOffset = ByteIndex{48U};
            inputBlock.setIntegerOrThrow(counterOffset, inputBlock.getInteger<uint64_t>(counterOffset) + 1U);
            fillBlock(zeroBytes, inputBlock.span(), addressBlock, ByteIndex::zero(), false);
            fillBlock(zeroBytes, addressBlock.span(), addressBlock, ByteIndex::zero(), false);
        };
        if (independent && start != 0U) {
            nextAddresses();
        }
        for (auto index = start; index < _segmentLength; ++index) {
            const auto current =
                static_cast<std::size_t>(lane) * _laneLength + static_cast<std::size_t>(slice) * _segmentLength + index;
            const auto laneStart = static_cast<std::size_t>(lane) * _laneLength;
            const auto previous = current == laneStart ? laneStart + _laneLength - 1U : current - 1U;
            auto pseudo = blockWord(memory.span(ByteIndex::fromSizeT(previous * 1024U), ByteLength{1024U}), 0U);
            if (independent) {
                // One 1024-byte address block supplies 128 little-endian 64-bit pseudo-random values.
                if (index % 128U == 0U) {
                    nextAddresses();
                }
                pseudo = blockWord(addressBlock.span(), index % 128U);
            }
            auto referenceLane = static_cast<uint32_t>(pseudo >> 32U) % _parameters.lanes;
            if (pass == 0U && slice == 0U) {
                referenceLane = lane;
            }
            const auto selectedIndex = referenceIndex(
                pass, slice, index, referenceLane == lane, _laneLength, _segmentLength, static_cast<uint32_t>(pseudo));
            const auto reference = static_cast<std::size_t>(referenceLane) * _laneLength + selectedIndex;
            // RFC 9106 Figures 5 and 6: combine the previous and selected reference blocks; later passes
            // also XOR the old block at the current position.
            fillBlock(
                memory.span(ByteIndex::fromSizeT(previous * 1024U), ByteLength{1024U}),
                memory.span(ByteIndex::fromSizeT(reference * 1024U), ByteLength{1024U}),
                memory,
                ByteIndex::fromSizeT(current * 1024U),
                pass != 0U);
        }
    }

    /// Combine the final lane blocks and produce the configured tag.
    /// @param memory The completely filled memory blocks.
    /// @return The Argon2id tag in marked storage.
    [[nodiscard]] auto finalize(const ByteBuffer &memory) const -> ByteBlockEditor {
        // Step 7 XORs the final block from every lane into C.
        auto finalBlock = ByteBuffer{memory.span(ByteIndex::fromSizeT((_laneLength - 1U) * 1024U), ByteLength{1024U})};
        [[maybe_unused]] const auto finalBlockErase = SecureEraseGuard{finalBlock};
        for (auto lane = uint32_t{1U}; lane < _parameters.lanes; ++lane) {
            const auto offset = (static_cast<std::size_t>(lane) * _laneLength + _laneLength - 1U) * 1024U;
            static_cast<void>(finalBlock.xorWith(memory.span(ByteIndex::fromSizeT(offset), ByteLength{1024U})));
        }
        // Step 8 applies H'^T(C) to produce the requested tag.
        return variableHash(finalBlock.span(), _parameters.outputLength);
    }

private:
    Argon2idParameters _parameters; ///< The validated memory, pass, lane, and output dimensions.
    uint32_t _memoryBlocks{};       ///< The aligned number m' of 1024-byte memory blocks.
    uint32_t _laneLength{};         ///< The number q of blocks in one lane.
    uint32_t _segmentLength{};      ///< The number q/4 of blocks in one segment.
};

}
