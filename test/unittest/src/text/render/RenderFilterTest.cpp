// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MemoryLayoutLoader.hpp"

#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/RenderError.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(Environment ExpressionCompiler Engine)
class RenderFilterTest final : public el::UnitTest {
public:
    void testFilterChains() {
        const auto environment = environmentFor("{{ value | prefix | suffix | true }}"_el);
        environment->addFilter("prefix"_el, [](const ValueList &values) -> Value {
            return el::text::StringList{"["_el, values.getRefOrThrow(el::unit::ItemIndex::zero()).asText()}.join();
        });
        environment->addFilter("suffix"_el, [](const ValueList &values) -> Value {
            return el::text::StringList{values.getRefOrThrow(el::unit::ItemIndex::zero()).asText(), "]"_el}.join();
        });
        environment->addFilter("true"_el, [](const ValueList &values) -> Value {
            return values.getRefOrThrow(el::unit::ItemIndex::zero());
        });

        REQUIRE_EQUAL(environment->render("page"_el, Context{}.set("value"_el, "x"_el)), "[x]"_el);
    }

    void testUnknownArgumentsAndShortCircuit() {
        REQUIRE_THROWS_AS(RenderError, environmentFor("{{ value | missing }}"_el)->render("page"_el));

        const auto arguments = environmentFor("{{ value | known() }}"_el);
        arguments->addFilter("known"_el, [](const ValueList &values) -> Value {
            return values.getRefOrThrow(el::unit::ItemIndex::zero());
        });
        REQUIRE_EQUAL(arguments->render("page"_el, Context{}.set("value"_el, "x"_el)), "x"_el);

        auto calls = std::atomic_size_t{0U};
        const auto skipped = environmentFor("{{ true or \"x\" | hit }}|{{ false and \"x\" | hit }}"_el);
        skipped->addFilter("hit"_el, [&calls](const ValueList &values) -> Value {
            ++calls;
            return values.getRefOrThrow(el::unit::ItemIndex::zero());
        });
        REQUIRE_EQUAL(skipped->render("page"_el), "true|false"_el);
        REQUIRE_EQUAL(calls.load(), std::size_t{0U});
    }

    void testRegistrationValidation() {
        const auto environment = environmentFor("page"_el);
        const auto identity = [](const ValueList &values) -> Value {
            return values.getRefOrThrow(el::unit::ItemIndex::zero());
        };
        REQUIRE_THROWS_AS(el::err::ParameterError, environment->addFilter(""_el, identity));
        REQUIRE_THROWS_AS(el::err::ParameterError, environment->addFilter("bad-name"_el, identity));
        REQUIRE_THROWS_AS(el::err::ParameterError, environment->addFilter("empty"_el, FilterFn{}));
        REQUIRE_THROWS_AS(el::err::ParameterError, environment->addFilter("escape"_el, identity));
        REQUIRE_THROWS_AS(el::err::ParameterError, environment->addFilter("e"_el, identity));
        REQUIRE_THROWS_AS(el::err::ParameterError, environment->addFilter("safe"_el, identity));
        environment->addFilter("valid_2"_el, identity);
        REQUIRE_THROWS_AS(el::err::ParameterError, environment->addFilter("valid_2"_el, identity));
        REQUIRE_EQUAL(environment->render("page"_el), "page"_el);
        REQUIRE_THROWS_AS(el::err::LogicError, environment->addFilter("late"_el, identity));
    }

    void testCallbackArgumentContractAndValidation() {
        const auto environment =
            environmentFor("{{ 'x' | inspect }}|{{ 'x' | inspect('a') }}|{{ 'x' | inspect('a', 'b') }}"_el);
        auto argumentCounts = std::vector<el::unit::ItemCount>{};
        environment->addFilter("inspect"_el, [&argumentCounts](const ValueList &values) -> Value {
            argumentCounts.push_back(values.count());
            auto text = el::text::StringList{};
            values.forEach([&text](const Value &value) -> void { text.append(value.toString()); });
            return text.join(","_el);
        });
        REQUIRE_EQUAL(environment->render("page"_el), "x|x,a|x,a,b"_el);
        REQUIRE_EQUAL(argumentCounts.size(), std::size_t{3U});
        REQUIRE_EQUAL(argumentCounts[0U], el::unit::ItemCount{1U});
        REQUIRE_EQUAL(argumentCounts[1U], el::unit::ItemCount{2U});
        REQUIRE_EQUAL(argumentCounts[2U], el::unit::ItemCount{3U});

        const auto strict = environmentFor("{{ 'x' | strict }}"_el);
        strict->addFilter("strict"_el, [](const ValueList &values) -> Value {
            if (values.count() != el::unit::ItemCount{2U}) {
                throw el::err::ParameterError{"The strict filter requires one argument."_el, "values"_el};
            }
            return values.getRefOrThrow(el::unit::ItemIndex::zero());
        });
        REQUIRE_THROWS_AS(RenderError, strict->render("page"_el));
    }

    void testCallbackResultAndException() {
        const auto callback = environmentFor("{{ \"input\" | late }}"_el);
        callback->addFilter("late"_el, [](const ValueList &) -> Value {
            return ValueCallbackFn{[]() -> Value { return "resolved"_el; }};
        });
        REQUIRE_EQUAL(callback->render("page"_el), "resolved"_el);

        const auto failing = environmentFor("{{ \"input\" | fail }}"_el);
        failing->addFilter("fail"_el, [](const ValueList &) -> Value { throw std::runtime_error{"filter failure"}; });
        try {
            static_cast<void>(failing->render("page"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Runtime);
            REQUIRE_EQUAL(error.context().location().column(), el::unit::ColumnIndex{13U});
        }
    }

    void testConcurrentInvocation() {
        auto calls = std::atomic_size_t{0U};
        const auto environment = environmentFor("{{ value | count }}"_el);
        environment->addFilter("count"_el, [&calls](const ValueList &values) -> Value {
            ++calls;
            return values.getRefOrThrow(el::unit::ItemIndex::zero());
        });
        auto workers = std::vector<std::thread>{};
        for (auto worker = 0; worker < 4; ++worker) {
            workers.emplace_back([environment]() -> void {
                for (auto iteration = 0; iteration < 20; ++iteration) {
                    static_cast<void>(environment->render("page"_el, Context{}.set("value"_el, "x"_el)));
                }
            });
        }
        for (auto &worker : workers) {
            worker.join();
        }
        REQUIRE_EQUAL(calls.load(), std::size_t{80U});
    }

private:
    [[nodiscard]] static auto environmentFor(const el::text::String &text) -> EnvironmentPtr {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, text);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        return environment;
    }
};
