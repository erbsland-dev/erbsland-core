// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HuffmanTree.hpp"
#include "Rule.hpp"

#include <erbsland/BitReader.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/List.hpp>

#include <array>
#include <cstdint>

namespace tool::psl {

/// Encode and independently verify the runtime PSL payload.
/// @tested{PublicSuffixTest PublicSuffixFullTest}
class EncodedData final {
    /// Symbol weights indexed by the seven-bit token.
    using Frequencies = std::array<std::uint64_t, 128>;

public:
    /// Number of rules encoded in each independently decodable public-suffix block.
    static constexpr unsigned cBlockSize = 32U;

public:
    /// Encode sorted reversed-label rules and validate the entire decoded result.
    /// @param rules Sorted canonical rules, using tokens 0..2.
    /// @throws el::ParseError On invalid rules, overflow, storage limits, or verification failure.
    explicit EncodedData(const el::List<Rule> &rules);

public: // accessors
    /// Get the sorted canonical rules.
    [[nodiscard]] auto rules() const -> const el::List<Rule> & { return _rules; }
    /// Get the MSB-first encoded stream.
    [[nodiscard]] auto encodedRules() const -> const el::ByteBlock & { return _encodedRules; }
    /// Get block starts and the final bit position.
    [[nodiscard]] auto blockOffsets() const -> const el::List<std::uint32_t> & { return _blockOffsets; }
    /// Get the flattened prefix code tree.
    [[nodiscard]] auto prefixTree() const -> const el::ByteBlock & { return _prefixTree; }
    /// Get the flattened content code tree.
    [[nodiscard]] auto contentTree() const -> const el::ByteBlock & { return _contentTree; }
    /// Get the maximum ASCII rule byte length.
    [[nodiscard]] auto maximumRuleLength() const -> el::ByteCount { return _maximumRuleLength; }
    /// Get the maximum number of labels per rule.
    [[nodiscard]] auto maximumLabelCount() const -> unsigned { return _maximumLabelCount; }
    /// Get the original text byte count including separators.
    [[nodiscard]] auto originalTextByteCount() const -> el::ByteCount { return _originalTextByteCount; }
    /// Get total persistent representation size.
    [[nodiscard]] auto storageByteCount() const -> el::ByteCount;

public: // verification
    /// Independently decode every block and compare it to the source rules.
    /// @throws el::ParseError If any tree, stream, block boundary, or decoded rule is invalid.
    void verify() const;

private:
    /// Validate the source rules and collect prefix lengths and symbol weights.
    /// @param prefixFrequencies Destination prefix token weights.
    /// @param contentFrequencies Destination content token weights.
    /// @return Common prefix lengths in source order.
    auto collectFrequencies(Frequencies &prefixFrequencies, Frequencies &contentFrequencies) -> el::List<el::ByteCount>;
    /// Validate one source rule and update the representation statistics.
    /// @param rule The current rule.
    /// @param previous The preceding rule, or an empty string at the start.
    void validateRule(const Rule &rule, const el::String &previous);
    /// Write the rules and block offsets with the selected Huffman codes.
    /// @param prefixes Common prefix lengths in source order.
    /// @param prefixEncoder The prefix token encoder.
    /// @param contentEncoder The content token encoder.
    void encode(
        const el::List<el::ByteCount> &prefixes, const HuffmanTree &prefixEncoder, const HuffmanTree &contentEncoder);
    /// Decode and compare one independent block.
    /// @param block The block index.
    /// @param index The next source rule index, advanced past the block.
    void verifyBlock(el::ItemIndex block, el::ItemIndex &index) const;
    /// Reconstruct one rule using the independent decoder.
    /// @param reader The block's bit reader.
    /// @param end The exclusive block end bit offset.
    /// @param previous The preceding decoded rule in this block.
    /// @return The decoded text and terminal token.
    [[nodiscard]] auto decodeRule(el::BitReader &reader, std::uint32_t end, const el::String &previous) const -> Rule;
    /// Decode one bounded Huffman symbol independently of the encoder.
    /// @param reader The block's bit reader.
    /// @param end The exclusive block end bit offset.
    /// @param tree The flattened tree to traverse.
    /// @return The decoded seven-bit token.
    [[nodiscard]] static auto decodeToken(el::BitReader &reader, std::uint32_t end, const el::ByteBlock &tree)
        -> unsigned;
    /// Count equal leading ASCII bytes in two rules.
    /// @param a The preceding canonical rule.
    /// @param b The current canonical rule.
    /// @return The common prefix length in bytes.
    [[nodiscard]] static auto prefixLength(const el::String &a, const el::String &b) -> el::ByteCount;
    /// Get the lazily constructed ASCII content alphabet.
    [[nodiscard]] static auto alphabet() -> const el::String &;
    /// Convert an ASCII content character to its Huffman token.
    /// @param character The character to encode.
    /// @return Its token in the fixed content alphabet.
    /// @throws el::ParseError If the character is outside the supported alphabet.
    [[nodiscard]] static auto characterToken(el::Char character) -> unsigned;

private:
    el::List<Rule> _rules;                 ///< Sorted canonical rules.
    el::ByteBlock _encodedRules;           ///< MSB-first encoded stream.
    el::List<std::uint32_t> _blockOffsets; ///< Block starts and final bit position.
    el::ByteBlock _prefixTree;             ///< Flattened prefix code tree.
    el::ByteBlock _contentTree;            ///< Flattened content code tree.
    el::ByteCount _maximumRuleLength;      ///< Maximum ASCII byte length.
    unsigned _maximumLabelCount{};         ///< Maximum labels per rule.
    el::ByteCount _originalTextByteCount;  ///< Text bytes including separators.
};

}
