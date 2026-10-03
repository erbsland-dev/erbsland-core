// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MemoryLayoutLoader.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/render/RenderError.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <exception>
#include <new>
#include <thread>
#include <vector>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(Environment ExpressionCompiler Engine)
class RenderFilterTest final : public el::UnitTest {
private:
    static constexpr auto cPrefixKey = "prefix"_el;
    static constexpr auto cValueKey = "value"_el;

public:
    void testFilterChains() {
        const auto environment = environmentFor("{{ value | prefix | suffix | true }}"_el);
        environment->addFilter(cPrefixKey, [](const ValueList &values) -> Value {
            return el::text::StringList{"["_el, values.getRefOrThrow(el::unit::ItemIndex::zero()).asText()}.join();
        });
        environment->addFilter("suffix"_el, [](const ValueList &values) -> Value {
            return el::text::StringList{values.getRefOrThrow(el::unit::ItemIndex::zero()).asText(), "]"_el}.join();
        });
        environment->addFilter("true"_el, [](const ValueList &values) -> Value {
            return values.getRefOrThrow(el::unit::ItemIndex::zero());
        });

        REQUIRE_EQUAL(environment->render("page"_el, Context{}.set(cValueKey, "x"_el)), "[x]"_el);
    }

    void testUnknownArgumentsAndShortCircuit() {
        REQUIRE_THROWS_AS(RenderError, environmentFor("{{ value | missing }}"_el)->render("page"_el));

        const auto arguments = environmentFor("{{ value | known() }}"_el);
        arguments->addFilter("known"_el, [](const ValueList &values) -> Value {
            return values.getRefOrThrow(el::unit::ItemIndex::zero());
        });
        REQUIRE_EQUAL(arguments->render("page"_el, Context{}.set(cValueKey, "x"_el)), "x"_el);

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
        REQUIRE_THROWS_AS(el::err::ParameterFailure, environment->addFilter(""_el, identity));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, environment->addFilter("bad-name"_el, identity));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, environment->addFilter("empty"_el, FilterFn{}));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, environment->addFilter("escape"_el, identity));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, environment->addFilter("e"_el, identity));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, environment->addFilter("safe"_el, identity));
        environment->addFilter("valid_2"_el, identity);
        REQUIRE_THROWS_AS(el::err::ParameterFailure, environment->addFilter("valid_2"_el, identity));
        REQUIRE_EQUAL(environment->render("page"_el), "page"_el);
        REQUIRE_THROWS_AS(el::err::LogicFailure, environment->addFilter("late"_el, identity));
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
                throw el::err::ParseError{"The strict filter requires one argument."_el};
            }
            return values.getRefOrThrow(el::unit::ItemIndex::zero());
        });
        try {
            static_cast<void>(strict->render("page"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Runtime);
            REQUIRE_EQUAL(error.context().location().column(), el::unit::ColumnIndex{9U});
            REQUIRE(error.hasCause());
            REQUIRE_THROWS_AS(el::err::ParseError, std::rethrow_exception(error.cause()));
        }
    }

    void testCallbackRejectsContextTypesWithoutPoisoningTheEnvironment() {
        const auto environment = environmentFor("{{ value | label(prefix) }}"_el);
        environment->addFilter("label"_el, [](const ValueList &values) -> Value {
            if (values.count() != el::unit::ItemCount{2U} || !values.getRef(el::unit::ItemIndex{0U}).isText() ||
                !values.getRef(el::unit::ItemIndex{1U}).isText()) {
                throw RenderError{RenderErrorContext{
                    RenderErrorCategory::Runtime,
                    "Invalid label filter arguments"_el,
                    "The label filter needs text and one text prefix."_el}};
            }
            return el::text::StringList{
                values.getRef(el::unit::ItemIndex{1U}).asText(), values.getRef(el::unit::ItemIndex{0U}).asText()}
                .join();
        });
        REQUIRE_THROWS_AS(RenderError, environment->render("page"_el, Context{}.set(cValueKey, "x"_el)));
        REQUIRE_THROWS_AS(
            RenderError, environment->render("page"_el, Context{}.set(cValueKey, 42).set(cPrefixKey, "label: "_el)));
        REQUIRE_THROWS_AS(
            RenderError, environment->render("page"_el, Context{}.set(cValueKey, "x"_el).set(cPrefixKey, 42)));
        REQUIRE_EQUAL(
            environment->render("page"_el, Context{}.set(cValueKey, "x"_el).set(cPrefixKey, "label: "_el)),
            "label: x"_el);
    }

    void testDirectRenderErrorsGainCallbackLocations() {
        const auto cause = std::make_exception_ptr(el::err::ParseError{"Invalid upstream data."_el});
        const auto fail = [cause]() -> Value {
            throw RenderError{
                RenderErrorContext{
                    RenderErrorCategory::Runtime, "Value unavailable"_el, "The source rejected input."_el},
                cause};
        };
        const auto filter = environmentFor("{{ 'x' | fail }}"_el);
        filter->addFilter("fail"_el, [fail](const ValueList &) -> Value { return fail(); });
        WITH_CONTEXT(requireCallbackError(filter, {}, el::unit::ColumnIndex{9U}, cause));

        const auto source = environmentFor("{{ fail }}"_el);
        WITH_CONTEXT(requireCallbackError(
            source, Context{}.set("fail"_el, ValueCallbackFn{fail}), el::unit::ColumnIndex{3U}, cause));
    }

    void testDirectRenderErrorsPreserveExistingSourceContext() {
        const auto environment = environmentFor("{{ 'x' | fail }}"_el);
        environment->addFilter("fail"_el, [](const ValueList &) -> Value {
            throw RenderError{
                RenderErrorContext{RenderErrorCategory::Load, "External source failed"_el, "Cannot load record."_el}
                    .setLayout("external"_el)
                    .setOrigin("database"_el)
                    .setLocation(el::unit::CodeLocation{el::unit::LineIndex{7U}})
                    .addFrame("original frame"_el)};
        });
        try {
            static_cast<void>(environment->render("page"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Load);
            REQUIRE_EQUAL(error.context().layout(), "external"_el);
            REQUIRE_EQUAL(error.context().origin(), "database"_el);
            REQUIRE_EQUAL(error.context().location().line(), el::unit::LineIndex{7U});
            REQUIRE_EQUAL(error.context().frames().join(), "original frame"_el);
        }
    }

    void testCallbackResultAndException() {
        const auto callback = environmentFor("{{ \"input\" | late }}"_el);
        callback->addFilter("late"_el, [](const ValueList &) -> Value {
            return ValueCallbackFn{[]() -> Value { return "resolved"_el; }};
        });
        REQUIRE_EQUAL(callback->render("page"_el), "resolved"_el);

        const auto failing = environmentFor("{{ \"input\" | fail }}"_el);
        failing->addFilter(
            "fail"_el, [](const ValueList &) -> Value { throw el::err::RuntimeError{"filter failure"}; });
        try {
            static_cast<void>(failing->render("page"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Runtime);
            REQUIRE_EQUAL(error.context().location().column(), el::unit::ColumnIndex{13U});
        }
    }

    void testFatalFilterFailureIsNotWrapped() {
        const auto broken = environmentFor("{{ \"input\" | fail }}"_el);
        broken->addFilter("fail"_el, [](const ValueList &) -> Value {
            throw el::err::LogicFailure{"The filter's internal invariant is broken."_el};
        });
        REQUIRE_THROWS_AS(el::err::LogicFailure, broken->render("page"_el));

        const auto environment = environmentFor("{{ \"input\" | fail }}"_el);
        environment->addFilter("fail"_el, [](const ValueList &) -> Value { throw std::bad_alloc{}; });
        REQUIRE_THROWS_AS(std::bad_alloc, environment->render("page"_el));
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
                    static_cast<void>(environment->render("page"_el, Context{}.set(cValueKey, "x"_el)));
                }
            });
        }
        for (auto &worker : workers) {
            worker.join();
        }
        REQUIRE_EQUAL(calls.load(), std::size_t{80U});
    }

private:
    void requireCallbackError(
        const EnvironmentPtr &environment,
        const Context &context,
        const el::unit::ColumnIndex column,
        const std::exception_ptr &cause) {
        try {
            static_cast<void>(environment->render("page"_el, context));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Runtime);
            REQUIRE_EQUAL(error.context().title(), "Value unavailable"_el);
            REQUIRE_EQUAL(error.context().description(), "The source rejected input."_el);
            REQUIRE_EQUAL(error.context().layout(), "page"_el);
            REQUIRE_EQUAL(error.context().origin(), "memory:page"_el);
            REQUIRE_EQUAL(error.context().location().line(), el::unit::LineIndex{0U});
            REQUIRE_EQUAL(error.context().location().column(), column);
            REQUIRE(error.cause() == cause);
        }
    }

    [[nodiscard]] static auto environmentFor(const el::text::String &text) -> EnvironmentPtr {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, text);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        return environment;
    }
};
