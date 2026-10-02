// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>

#include <typeinfo>
#include <utility>

namespace app::constant_time {

using namespace el::text::literals;

/// Concrete backend identity independent of native host architecture and emulation.
/// @notest{Factory identities checked by explicit platform listings and fixture smoke runs.}
class BackendInfo final {
public:
    /// Name the concrete factory worker, independently of the native host architecture or emulation.
    /// @tparam tWorker Polymorphic worker interface.
    /// @param worker Selected implementation.
    /// @param portable Whether this operation follows the portable path.
    template <typename tWorker>
    explicit BackendInfo(const tWorker &worker, const bool portable) : _name{identify(worker, portable)} {}
    /// Get the concrete implementation identifier.
    [[nodiscard]] auto name() const noexcept -> const el::String & { return _name; }

private:
    /// Resolve a worker to a stable label or an explicit compiler type name.
    template <typename tWorker>
    [[nodiscard]] static auto identify(const tWorker &worker, const bool portable) -> el::String {
        if (portable) {
            return "portable"_el;
        }
        const auto type = el::String{typeid(worker).name()};
        for (
            const auto &[name, label] :
            {std::pair{"ArmAesBlockCipher"_el, "arm-aes"_el},
                std::pair{"X86AesBlockCipher"_el, "x86-aes"_el},
                std::pair{"ArmGaloisMultiplier"_el, "arm-pmull"_el},
                std::pair{"X86GaloisMultiplier"_el, "x86-pclmul"_el},
                std::pair{"ArmChaCha20Backend"_el, "arm-neon"_el},
                std::pair{"X86ChaCha20Backend"_el, "x86-sse2"_el},
                std::pair{"ArmPoly1305"_el, "arm-neon"_el},
                std::pair{"X86Poly1305"_el, "x86-sse2"_el}}) {
            if (type.contains(name)) {
                return label;
            }
        }
        return el::StringFormat{"implementation:{}"_el}.build(type);
    }

public:
    /// Identify an exact concrete worker without mistaking a derived accelerated worker for its portable base.
    /// @tparam tExpected Expected concrete implementation.
    /// @tparam tWorker Polymorphic worker interface.
    /// @param worker Factory-selected worker.
    /// @return Whether the implementation is exactly the expected type.
    template <typename tExpected, typename tWorker>
    [[nodiscard]] static auto isImplementation(const tWorker &worker) noexcept -> bool {
        return typeid(worker) == typeid(tExpected);
    }

private:
    el::String _name; ///< Stable implementation label.
};

}
