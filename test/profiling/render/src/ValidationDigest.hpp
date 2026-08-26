// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/text/render/impl/CompiledBlock.hpp>
#include <erbsland/text/render/impl/CompiledExtends.hpp>
#include <erbsland/text/render/impl/CompiledInclude.hpp>
#include <erbsland/text/render/impl/CompiledLayout.hpp>
#include <erbsland/text/render/impl/Token.hpp>

#include <cstdint>

namespace app::render {

/// Stable digest mixers shared by measured and validation paths.
/// @notest{Covered by the render profiler determinism and coverage tests.}
class ValidationDigest final {
public:
    /// Mix one token into a digest.
    [[nodiscard]] static auto token(std::uint64_t digest, const erbsland::text::render::impl::Token &token) noexcept
        -> std::uint64_t {
        const auto kind = token.kind == erbsland::text::render::impl::TokenKind::In
            ? erbsland::text::render::impl::TokenKind::Identifier
            : token.kind;
        return (digest * 131U) ^ static_cast<std::uint8_t>(kind) ^ token.text.length().toRawValue();
    }

    /// Mix one immutable compiled layout into a digest.
    [[nodiscard]] static auto compiled(
        std::uint64_t digest, const erbsland::text::render::impl::CompiledLayout &layout) noexcept -> std::uint64_t {
        digest = (digest * 131U) ^ stringHash(layout.name());
        digest = (digest * 131U) ^ layout.setupProgram().data().length().toRawValue();
        digest = (digest * 131U) ^ layout.bodyProgram().data().length().toRawValue();
        digest = (digest * 131U) ^ layout.constantCount().toRawValue();
        for (const auto &[name, block] : layout.blocks()) {
            digest = (digest * 131U) ^ stringHash(name);
            digest = (digest * 131U) ^ block->program().data().length().toRawValue();
        }
        if (layout.extendsDependency() != nullptr) {
            digest = (digest * 131U) ^ stringHash(layout.extendsDependency()->name());
        }
        for (const auto &include : layout.includes()) {
            digest = (digest * 131U) ^ stringHash(include->name());
            digest = (digest * 131U) ^ static_cast<std::uint64_t>(include->withContext());
            digest = (digest * 131U) ^ static_cast<std::uint64_t>(include->ignoreMissing());
        }
        return digest;
    }

    /// Mix one rendered output into a digest.
    [[nodiscard]] static auto rendered(std::uint64_t digest, const erbsland::String &output) noexcept -> std::uint64_t {
        return (digest * 131U) ^ output.length().toRawValue() ^ stringHash(output);
    }

private:
    /// Create the platform-independent equivalent of the library's ordinary string hash.
    /// Keep this mixer lightweight because it also runs inside the measured compile workload; a cryptographic hasher
    /// would make hash throughput and allocation overhead part of the renderer profile.
    [[nodiscard]] static auto stringHash(const erbsland::String &text) noexcept -> std::uint64_t {
        auto result = std::uint64_t{};
        text.forEach([&result](const erbsland::Char character) -> erbsland::util::LoopStatus {
            const auto value = static_cast<std::uint64_t>(character.toRawValue());
            result ^= value + 0x9e3779b9U + (result << 6U) + (result >> 2U);
            return erbsland::util::LoopStatus::Continue;
        });
        return result;
    }
};

}
