// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HuffmanTree.hpp"

#include <erbsland/ParseError.hpp>

#include <algorithm>
#include <limits>
#include <span>
#include <tuple>
#include <utility>

namespace tool::psl {

using namespace el::text::literals;

HuffmanTree::HuffmanTree(const std::array<std::uint64_t, 128> &frequencies) {
    auto pending = std::vector<int>{};
    for (auto token = 0U; token < frequencies.size(); ++token) {
        if (frequencies[token] == 0U) {
            continue;
        }
        pending.push_back(static_cast<int>(_nodes.size()));
        _nodes.push_back(Node{frequencies[token], token, static_cast<unsigned>(_nodes.size())});
    }
    if (pending.empty()) {
        throw el::ParseError{"An empty Huffman alphabet cannot be encoded."_el};
    }
    build(std::move(pending));
    visit(_root, 0U, 0U);
}

void HuffmanTree::build(std::vector<int> pending) {
    const auto compare = [this](const int left, const int right) -> bool {
        const auto &a = _nodes[left];
        const auto &b = _nodes[right];
        return std::tie(a.frequency, a.minimumToken, a.serial) < std::tie(b.frequency, b.minimumToken, b.serial);
    };
    if (pending.size() == 1U) {
        const auto leaf = pending.front();
        _root = static_cast<int>(_nodes.size());
        _nodes.push_back(
            Node{_nodes[leaf].frequency, _nodes[leaf].minimumToken, static_cast<unsigned>(_nodes.size()), leaf, leaf});
    } else {
        while (pending.size() > 1U) {
            std::ranges::sort(pending, compare);
            const auto left = pending[0];
            const auto right = pending[1];
            pending.erase(pending.begin(), pending.begin() + 2);
            if (_nodes[left].frequency > std::numeric_limits<std::uint64_t>::max() - _nodes[right].frequency) {
                throw el::ParseError{"Huffman frequency overflow."_el};
            }
            pending.push_back(static_cast<int>(_nodes.size()));
            _nodes.push_back(
                Node{
                    _nodes[left].frequency + _nodes[right].frequency,
                    std::min(_nodes[left].minimumToken, _nodes[right].minimumToken),
                    static_cast<unsigned>(_nodes.size()),
                    left,
                    right});
        }
        _root = pending.front();
    }
}

void HuffmanTree::visit(const int node, const std::uint64_t bits, const unsigned depth) {
    const auto &value = _nodes[node];
    if (value.left < 0) {
        _codes[value.minimumToken] = Code{bits, depth};
        return;
    }
    if (depth >= 64U) {
        throw el::ParseError{"A Huffman code exceeds the supported bit width."_el};
    }
    visit(value.left, bits << 1U, depth + 1U);
    if (value.right != value.left) {
        visit(value.right, (bits << 1U) | 1U, depth + 1U);
    }
}

void HuffmanTree::write(el::BitWriter &writer, const unsigned token) const {
    if (token >= _codes.size() || _codes[token].length == 0U) {
        throw el::ParseError{"Unknown Huffman symbol."_el};
    }
    writer.writeBits(_codes[token].bits, _codes[token].length);
}

auto HuffmanTree::flatten(const int node, std::vector<std::uint8_t> &bytes) const -> unsigned {
    const auto &value = _nodes[node];
    if (value.left < 0) {
        return 0x80U | value.minimumToken;
    }
    const auto index = bytes.size() / 2U;
    if (index >= 0x80U) {
        throw el::ParseError{"A Huffman branch index exceeds the representation."_el};
    }
    bytes.resize(bytes.size() + 2U);
    const auto left = flatten(value.left, bytes);
    const auto right = flatten(value.right, bytes);
    bytes[index * 2U] = static_cast<std::uint8_t>(left);
    bytes[index * 2U + 1U] = static_cast<std::uint8_t>(right);
    return static_cast<unsigned>(index);
}

auto HuffmanTree::flattened() const -> el::ByteBlock {
    auto bytes = std::vector<std::uint8_t>{};
    flatten(_root, bytes);
    return el::ByteBlock::fromSpan(std::span<const std::uint8_t>{bytes});
}

}
