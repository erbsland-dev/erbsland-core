// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ByteCount.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/SubprocessExitStatus.hpp>
#include <erbsland/SubprocessOptions.hpp>
#include <erbsland/system/impl/ProcessIdAccess.hpp>
#include <erbsland/system/impl/subprocess/SubprocessBackend.hpp>
#include <erbsland/system/impl/subprocess/SubprocessOutputSink.hpp>
#include <erbsland/system/impl/subprocess/SubprocessState.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(SubprocessState)
class SubprocessOutputTest final : public el::UnitTest {
    using State = el::system::impl::SubprocessState;
    using Status = el::system::SubprocessExitStatus;

    /// Completed child for deterministic pipe-fragment delivery.
    class Backend final : public el::system::impl::SubprocessBackend {
    public:
        auto processId() const noexcept -> el::system::ProcessId override {
            return el::system::impl::ProcessIdAccess::fromNative(123);
        }
        auto isRunning() -> bool override { return false; }
        auto wait() -> Status override { return Status::exited(0); }
        auto wait(el::time::TimeDelta) -> std::optional<Status> override { return Status::exited(0); }
        void terminate() override {}
        void kill() override {}
    };

private:
    void verifyOutput(
        const std::vector<std::string_view> &chunks,
        const std::string_view expectedHex,
        const el::unit::ByteCount limit = el::unit::ByteCount{256},
        const bool truncated = false,
        const bool removeBom = true) {
        const auto expected = el::text::String{el::unittest::th::stdStringFromHex(expectedHex)};
        for (const auto events : {false, true}) {
            auto state = State{
                el::path::Path{"fixture"_el},
                {},
                el::system::SubprocessOptions{}.setCaptureLimit(limit).setRemoveOutputBom(removeBom),
                [&](const auto &, const auto &, const auto &, const el::system::impl::SubprocessOutputSink &sink)
                    -> el::system::impl::SubprocessBackendPtr {
                    for (const auto chunk : chunks) {
                        const auto bytes = el::unittest::th::stdStringFromHex(chunk);
                        sink.output(false, bytes);
                        sink.output(true, bytes);
                    }
                    return std::make_unique<Backend>();
                }};
            auto output = el::text::StringEditor{};
            auto errorOutput = el::text::StringEditor{};
            auto overflowCount = 0;
            auto finished = false;
            if (events) {
                state.bind({});
                state.setOutput(false, [&](const el::text::String &block) -> void {
                    REQUIRE_FALSE(block.isEmpty());
                    output.append(block);
                });
                state.setOutput(true, [&](const el::text::String &block) -> void {
                    REQUIRE_FALSE(block.isEmpty());
                    errorOutput.append(block);
                });
                state.setOutputTruncated([&]() -> void { ++overflowCount; });
                state.setFinal([&]() -> void {
                    REQUIRE_EQUAL(el::text::String{output}, expected);
                    REQUIRE_EQUAL(el::text::String{errorOutput}, expected);
                    finished = true;
                });
            }
            state.start();
            REQUIRE(state.wait()->isSuccess());
            if (events) {
                state.dispatch();
                REQUIRE(finished);
                REQUIRE(state.captured(false).isEmpty());
                REQUIRE(state.captured(true).isEmpty());
                REQUIRE_EQUAL(overflowCount, static_cast<int>(truncated));
            } else {
                REQUIRE_EQUAL(state.captured(false), expected);
                REQUIRE_EQUAL(state.captured(true), expected);
            }
            REQUIRE_EQUAL(state.truncated(false), truncated);
            REQUIRE_EQUAL(state.truncated(true), truncated);
        }
    }

public:
    void testMachineOutputPreservesInitialBomAndNullBytes() {
        WITH_CONTEXT(verifyOutput({"ef", "bb", "bf0061ff"}, "efbbbf0061ff", el::unit::ByteCount{256}, false, false));
        WITH_CONTEXT(verifyOutput({"efbbbf"}, "efbbbf", el::unit::ByteCount{256}, false, false));
        WITH_CONTEXT(verifyOutput({"efbbbf61"}, "efbbbf", el::unit::ByteCount{3}, true, false));
        REQUIRE(el::system::SubprocessOptions{}.removesOutputBom());
    }

    void testInitialBomAcrossReadBoundaries() {
        WITH_CONTEXT(verifyOutput({"efbbbf616263"}, "616263"));
        WITH_CONTEXT(verifyOutput({"ef", "bbbf616263"}, "616263"));
        WITH_CONTEXT(verifyOutput({"efbb", "bf616263"}, "616263"));
        WITH_CONTEXT(verifyOutput({"", "ef", "", "bb", "bf", "616263"}, "616263"));
    }

    void testEmptyAndBomOnlyOutput() {
        WITH_CONTEXT(verifyOutput({}, ""));
        WITH_CONTEXT(verifyOutput({"", ""}, ""));
        WITH_CONTEXT(verifyOutput({"efbbbf"}, ""));
        WITH_CONTEXT(verifyOutput({"ef", "bb", "bf"}, ""));
    }

    void testPartialAndMismatchedSignaturesArePreserved() {
        WITH_CONTEXT(verifyOutput({"ef"}, "ef"));
        WITH_CONTEXT(verifyOutput({"ef", "bb"}, "efbb"));
        WITH_CONTEXT(verifyOutput({"ef", "61"}, "ef61"));
        WITH_CONTEXT(verifyOutput({"ef", "bb", "61"}, "efbb61"));
        WITH_CONTEXT(verifyOutput({"ef", "bc", "a1"}, "efbca1"));
    }

    void testPayloadBytesArePreserved() {
        WITH_CONTEXT(verifyOutput({"61ef", "bbbf62"}, "61efbbbf62"));
        WITH_CONTEXT(verifyOutput({"efbbbf", "ef", "bbbf"}, "efbbbf"));
        WITH_CONTEXT(verifyOutput({"efbbbf", "61f09f92a1ff0062e282ace2"}, "61f09f92a1ff0062e282ace2"));
    }

    void testBomDoesNotConsumeOutputBudget() {
        WITH_CONTEXT(verifyOutput({"ef", "bb", "bf616263"}, "616263", el::unit::ByteCount{3}));
        WITH_CONTEXT(verifyOutput({"efbbbf"}, "", el::unit::ByteCount{1}));
        WITH_CONTEXT(verifyOutput({"efbbbf616263"}, "6162", el::unit::ByteCount{2}, true));
        WITH_CONTEXT(verifyOutput({"ef", "bb"}, "ef", el::unit::ByteCount{1}, true));
        WITH_CONTEXT(verifyOutput({"ef", "bb", "61"}, "ef", el::unit::ByteCount{1}, true));
    }
};
