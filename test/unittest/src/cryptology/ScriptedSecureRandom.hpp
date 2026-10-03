// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteBlock.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/Random.hpp>
#include <erbsland/RandomFailure.hpp>
#include <erbsland/SecureRandom.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace erbsland::test {

/// Scoped entropy fixture that scripts candidate draws and separately counts subsequent system entropy requests.
class ScriptedSecureRandom final : public random::Random {
public: // script
    /// Install candidate bytes, optionally repeating the final candidate indefinitely.
    /// @param candidates Candidate script consumed in order.
    /// @param repeatLast Keep returning the final candidate after consuming a nonempty script.
    void arm(std::vector<mem::ByteBlock> candidates, bool repeatLast = false) {
        _candidates = std::move(candidates);
        _repeatLast = repeatLast;
        _fail = false;
        _scriptedDraws = 0U;
        _otherDraws = 0U;
    }
    /// Make every subsequent draw fail with an identifiable source failure.
    void armFailure() {
        arm({});
        _fail = true;
    }
    /// Number of scripted candidate draws, including a failed source request.
    [[nodiscard]] auto scriptedDraws() const noexcept -> std::size_t { return _scriptedDraws; }
    /// Number of entropy requests delegated to the real source after the script.
    [[nodiscard]] auto otherDraws() const noexcept -> std::size_t { return _otherDraws; }

public: // implement Random
    [[nodiscard]] auto isSecure() const noexcept -> bool override { return true; }
    [[nodiscard]] auto getInt32(int32_t minimum, int32_t maximum) -> int32_t override {
        return _fallback.getInt32(minimum, maximum);
    }
    [[nodiscard]] auto getUInt32(uint32_t minimum, uint32_t maximum) -> uint32_t override {
        return _fallback.getUInt32(minimum, maximum);
    }
    [[nodiscard]] auto getInt64(int64_t minimum, int64_t maximum) -> int64_t override {
        return _fallback.getInt64(minimum, maximum);
    }
    [[nodiscard]] auto getUInt64(uint64_t minimum, uint64_t maximum) -> uint64_t override {
        return _fallback.getUInt64(minimum, maximum);
    }
    [[nodiscard]] auto getDouble(double minimum, double maximum) -> double override {
        return _fallback.getDouble(minimum, maximum);
    }
    [[nodiscard]] auto getBool() -> bool override { return _fallback.getBool(); }
    void fillBytes(std::span<std::byte> destination) override {
        if (_fail) {
            ++_scriptedDraws;
            throw random::RandomFailure{"Scripted entropy source failure"};
        }
        if (_scriptedDraws < _candidates.size() || (_repeatLast && !_candidates.empty())) {
            auto index = _scriptedDraws;
            if (index >= _candidates.size()) {
                index = _candidates.size() - 1U;
            }
            ++_scriptedDraws;
            const auto &candidate = _candidates[index];
            if (candidate.span().size() != destination.size()) {
                throw err::LogicFailure{"Unexpected scripted entropy request width"};
            }
            for (auto offset = std::size_t{}; offset < destination.size(); ++offset) {
                destination[offset] = static_cast<std::byte>(candidate.span()[offset].toUInt8());
            }
            return;
        }
        ++_otherDraws;
        _fallback.fillBytes(destination);
    }

private:
    random::SecureRandom _fallback;          ///< Source for unrelated protected-storage entropy requests.
    std::vector<mem::ByteBlock> _candidates; ///< Candidate script, containing only public test fixtures.
    std::size_t _scriptedDraws{};            ///< Requests served by the script.
    std::size_t _otherDraws{};               ///< Requests served by the fallback.
    bool _repeatLast{};                      ///< Repeat the last candidate when the script is consumed.
    bool _fail{};                            ///< Simulate an immediate entropy-source failure.
};

}
