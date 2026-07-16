// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GroupFlags.hpp"
#include "NodeData.hpp"

#include "../engine/CaptureGroupTypes.hpp"

#include "../../../text/String.hpp"

namespace erbsland::re::impl::node_data {

/// A group that provides alternative branches.
/// It optionally captures text and sets flags.
class Group : public NodeData {
public:
    /// Create a new capturing group.
    [[nodiscard]] static auto createCapture(const GroupFlags flags, const std::size_t index) noexcept -> Group {
        Group result;
        result.flags = flags;
        result.index = index;
        return result;
    }

    /// Create a new, named capturing group.
    [[nodiscard]] static auto createCapture(
        const GroupFlags flags, const std::size_t index, text::String &&name) noexcept -> Group {
        Group result;
        result.flags = flags;
        result.index = index;
        result.name = std::move(name);
        return result;
    }

    /// Create a new non-capturing group.
    [[nodiscard]] static auto createNonCapturing(const GroupFlags flags) noexcept -> Group {
        Group result;
        result.flags = flags;
        return result;
    }

    /// Create a new atomic group.
    [[nodiscard]] static auto createAtomic(const GroupFlags flags, const AtomicGroupId atomicGroupId) noexcept
        -> Group {
        Group result;
        result.flags = flags | GroupFlag::Atomic;
        result.atomicGroupId = atomicGroupId;
        return result;
    }

    /// Default constructor for aggregate-style initialization and fast zero-init.
    constexpr Group() = default;

private:
    /// Create a new non-capturing group with the given flags.
    explicit Group(const GroupFlags flags) noexcept : flags{flags} {}

    /// Create a new capturing group with the given flags and index.
    /// @param flags The group flags.
    /// @param index The capture group index (1-based).
    Group(const GroupFlags flags, const std::size_t index) noexcept : index{index}, flags{flags} {}

    /// Create a new, named capturing group.
    /// @param flags The group flags.
    /// @param index The capture group index (1-based).
    /// @param name The name of the group.
    Group(const GroupFlags flags, const std::size_t index, text::String &&name) noexcept :
        index{index}, name{std::move(name)}, flags{flags} {}

public:
    /// Add a child node to this group.
    /// The order of insertion defines the evaluation order.
    /// @param node The child to append.
    void addNode(const PatternNodePtr &node) { nodes.push_back(node); }

    /// Create a stable string used for validating node trees in tests.
    /// Note: To avoid string encoding conversions, the name length is reported instead of the actual name.
    [[nodiscard]] auto toTestString() const -> text::String {
        using namespace text::literals;
        auto result = text::StringFormat{"Group(size={}"}.build(nodes.size());
        if (index != 0) {
            result.append(text::StringFormat{",index={}"}.build(index));
        }
        if (atomicGroupId != cNoAtomicGroupId) {
            result.append(text::StringFormat{",atomicGroupId={}"}.build(atomicGroupId));
        }
        if (!name.isEmpty()) {
            const auto safeName = name.toSafeString(unit::CpLength{200U});
            result.append(",name=\""_el);
            result.append(safeName);
            result.append(U'"');
        }
        if (!flags.isEmpty()) {
            auto flagsStr = flags.toString();
            if (!flagsStr.isEmpty()) {
                result.append(text::StringFormat{",flags={}"}.build(flagsStr));
            }
            if (flags.isSet(GroupFlag::Atomic)) {
                result.append(",atomic"_el);
            }
        }
        result.append(U')');
        return result;
    }

    /// Access the children of this node as a zero‑overhead view.
    [[nodiscard]] auto children() const noexcept -> std::span<const PatternNodePtr> { return nodes; }

    /// Access the size of this data block.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return nodes.size(); }

public:
    std::vector<PatternNodePtr> nodes; ///< The list of alternative branches.
    std::size_t index{0};              ///< The index of this capture group. 0 = no capture, 1+ = capture group.
    AtomicGroupId atomicGroupId{cNoAtomicGroupId}; ///< The unique ID of the atomic group.
    text::String name;                             ///< An optional name for this capture group.
    GroupFlags flags;                              ///< Flags
};

}
