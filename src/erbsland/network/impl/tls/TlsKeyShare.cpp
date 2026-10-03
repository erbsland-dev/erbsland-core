#include "TlsKeyShare.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../text/Literals.hpp"

#include <array>
#include <set>

namespace erbsland::network::impl::tls_key_share {

using namespace text::literals;

auto helloRetryRequestRandom() -> mem::ByteBlock {
    static constexpr auto cRandom = std::array<uint8_t, 32U>{
        0xcfU,
        0x21U,
        0xadU,
        0x74U,
        0xe5U,
        0x9aU,
        0x61U,
        0x11U,
        0xbeU,
        0x1dU,
        0x8cU,
        0x02U,
        0x1eU,
        0x65U,
        0xb8U,
        0x91U,
        0xc2U,
        0xa2U,
        0x11U,
        0x16U,
        0x7aU,
        0xbbU,
        0x8cU,
        0x5eU,
        0x07U,
        0x9eU,
        0x09U,
        0xe2U,
        0xc8U,
        0xa8U,
        0x33U,
        0x9cU};
    return mem::ByteBlock::fromSpan(std::span{cRandom});
}

auto group(const cryptology::KeyAgreementAlgorithm algorithm) -> uint16_t {
    switch (algorithm.toRawValue()) {
    case cryptology::KeyAgreementAlgorithm::X25519:
        return 0x001dU;
    case cryptology::KeyAgreementAlgorithm::P256:
        return 0x0017U;
    }
    throw err::ParameterFailure{"Unsupported TLS key-agreement group."_el, "groups"_el};
}

auto algorithm(const uint16_t group) noexcept -> std::optional<cryptology::KeyAgreementAlgorithm> {
    if (group == 0x001dU) {
        return cryptology::KeyAgreementAlgorithm::X25519;
    }
    if (group == 0x0017U) {
        return cryptology::KeyAgreementAlgorithm::P256;
    }
    return std::nullopt;
}

void validate(const std::vector<cryptology::KeyAgreementAlgorithm> &groups) {
    if (groups.empty() || groups.size() > cryptology::KeyAgreementAlgorithm::all().size()) {
        throw err::ParameterFailure{"TLS requires a nonempty bounded key-agreement group list."_el, "groups"_el};
    }
    auto seen = std::set<uint16_t>{};
    for (const auto algorithm : groups) {
        if (!seen.insert(group(algorithm)).second) {
            throw err::ParameterFailure{"TLS key-agreement groups must not contain duplicates."_el, "groups"_el};
        }
    }
}

}
