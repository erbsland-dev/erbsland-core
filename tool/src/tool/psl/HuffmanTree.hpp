// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/BitWriter.hpp>
#include <erbsland/ByteBlock.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace tool::psl {

/// Deterministic Huffman encoder with the runtime's compact branch representation.
/// @tested{PublicSuffixTest PublicSuffixFullTest}
class HuffmanTree final {
    /// One indexed leaf or branch in the working tree.
    struct Node final {
        std::uint64_t frequency{}; ///< Symbol or subtree weight.
        unsigned minimumToken{};   ///< Lowest token for deterministic ties.
        unsigned serial{};         ///< Creation order for deterministic ties.
        int left{-1};              ///< Left child, or -1 for a leaf.
        int right{-1};             ///< Right child, or -1 for a leaf.
    };
    /// One bounded MSB-first symbol code.
    struct Code final {
        std::uint64_t bits{}; ///< Low-order code bits.
        unsigned length{};    ///< Number of significant code bits.
    };

public:
    /// Build with ascending token/minimum-token/creation-order tie breaking.
    /// @param frequencies Weights indexed by the 7-bit token.
    /// @throws el::ParseError On an empty alphabet, overflow, or unsupported code width.
    explicit HuffmanTree(const std::array<std::uint64_t, 128> &frequencies);
    /// Write one symbol with this tree's code.
    /// @param writer Destination bit stream.
    /// @param token Present symbol in the range 0..127.
    /// @throws el::ParseError If the symbol is not in this alphabet.
    void write(el::BitWriter &writer, unsigned token) const;
    /// Get the preorder flattened tree, including a deterministic singleton branch.
    /// @throws el::ParseError If branch indices exceed the representation.
    [[nodiscard]] auto flattened() const -> el::ByteBlock;

private:
    /// Merge weighted leaves into the deterministic working tree.
    /// @param pending Nonempty leaf indices in token order.
    void build(std::vector<int> pending);
    /// Assign bounded symbol codes by visiting the working tree.
    /// @param node The node index to visit.
    /// @param bits The accumulated path bits.
    /// @param depth The accumulated code length.
    /// @throws el::ParseError If the code exceeds the supported width.
    void visit(int node, std::uint64_t bits, unsigned depth);
    /// Append preorder branches and return the encoded child reference.
    /// @param node The working node index.
    /// @param bytes The flattened tree being built.
    /// @return A branch index or a token with the leaf marker set.
    /// @throws el::ParseError If branch indices exceed the representation.
    auto flatten(int node, std::vector<std::uint8_t> &bytes) const -> unsigned;

private:
    std::vector<Node> _nodes;       ///< Index-addressed tree building storage.
    std::array<Code, 128> _codes{}; ///< Symbol codes.
    int _root{};                    ///< Root index.
};

}
