// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/stream/StreamError.hpp>
#include <erbsland/stream/StreamErrorSource.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <utility>

using namespace el::text::literals;

TESTED_TARGETS(StreamErrorSource)
class StreamErrorSourceTest final : public el::UnitTest {
    class ContextualErrorSource final : public el::stream::StreamErrorSource {
    public:
        auto createErrorContext() const noexcept -> el::stream::StreamErrorContext override {
            auto context = StreamErrorSource::createErrorContext();
            context.setPath("test/source.dat"_el);
            return context;
        }
    };

public:
    void testContextFactoryEnrichesConvenienceError() {
        const auto source = ContextualErrorSource{};
        const auto &errorSource = static_cast<const el::stream::StreamErrorSource &>(source);
        try {
            errorSource.throwError("Failed to process test data."_el, "The test data is unavailable."_el);
            REQUIRE(false);
        } catch (const el::stream::StreamError &error) {
            REQUIRE_EQUAL(error.title(), "Failed to process test data."_el);
            REQUIRE_EQUAL(error.description(), "The test data is unavailable."_el);
            REQUIRE_EQUAL(error.path(), "test/source.dat"_el);
        }
    }
};
