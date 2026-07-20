// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../Integer.hpp"

#include <vector>

namespace erbsland::conf::impl {
class VersionMask;
}

namespace erbsland::conf::vr::builder {

/// Restricts a rule to specific versions.
struct ConfVersion : Attribute {
    explicit ConfVersion(std::vector<Integer> versions, const bool isNegated = false) :
        _versions{std::move(versions)}, _isNegated{isNegated} {}
    explicit ConfVersion(const std::initializer_list<Integer> versions, const bool isNegated = false) :
        _versions{versions}, _isNegated{isNegated} {}
    explicit ConfVersion(const Integer version, const bool isNegated = false) :
        _versions{version}, _isNegated{isNegated} {}

    void operator()(impl::Rule &rule) override;

    [[nodiscard]] static auto toVersionMask(const std::vector<Integer> &versions) -> impl::VersionMask;

    std::vector<Integer> _versions;
    bool _isNegated{false};
};

}
