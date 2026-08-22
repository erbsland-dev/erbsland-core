// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/system/EnvironmentVariables.hpp>
#include <erbsland/system/PlatformError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

using namespace el::text::literals;

TESTED_TARGETS(EnvironmentVariables EnvironmentVariableBackend)
class EnvironmentVariablesTest final : public el::UnitTest {
    class TestBackend final : public el::system::impl::EnvironmentVariableBackend {
    public:
        [[nodiscard]] auto get(const el::text::String &) const -> std::optional<el::text::String> override {
            if (failGet) {
                throw el::system::PlatformError{"Injected environment-variable lookup failure."_el};
            }
            return value;
        }

        void set(const el::text::String &, const el::text::String &newValue) override {
            if (failSet) {
                throw el::system::PlatformError{"Injected environment-variable set failure."_el};
            }
            value = newValue;
        }

        void remove(const el::text::String &) override {
            if (failRemove) {
                throw el::system::PlatformError{"Injected environment-variable removal failure."_el};
            }
            value = std::nullopt;
        }

        mutable std::optional<el::text::String> value;
        bool failGet{};
        bool failSet{};
        bool failRemove{};
    };

    class VariableGuard final {
    public:
        VariableGuard(el::system::EnvironmentVariables &environment, el::text::String name) :
            _environment{environment}, _name{std::move(name)}, _previousValue{_environment.get(_name)} {}

        ~VariableGuard() {
            if (_previousValue.has_value()) {
                _environment.set(_name, *_previousValue);
            } else {
                _environment.remove(_name);
            }
        }

        // deletions
        VariableGuard(const VariableGuard &) = delete;
        VariableGuard(VariableGuard &&) = delete;
        auto operator=(const VariableGuard &) -> VariableGuard & = delete;
        auto operator=(VariableGuard &&) -> VariableGuard & = delete;

    private:
        el::system::EnvironmentVariables &_environment;
        el::text::String _name;
        std::optional<el::text::String> _previousValue;
    };

public:
    void testGetVariants() {
        auto backend = std::make_unique<TestBackend>();
        auto *backendPtr = backend.get();
        auto environment = el::system::EnvironmentVariables{std::move(backend)};

        REQUIRE_FALSE(environment.get("TEST"_el).has_value());
        REQUIRE_EQUAL(environment.get("TEST"_el, "fallback"_el), "fallback"_el);
        REQUIRE_THROWS_AS(el::system::PlatformError, environment.getOrThrow("TEST"_el));

        backendPtr->value = el::text::String{};
        const auto emptyValue = environment.get("TEST"_el);
        REQUIRE(emptyValue.has_value());
        REQUIRE(emptyValue->isEmpty());
        REQUIRE(environment.get("TEST"_el, "fallback"_el).isEmpty());
        REQUIRE(environment.getOrThrow("TEST"_el).isEmpty());

        backendPtr->value = "stored"_el;
        REQUIRE_EQUAL(*environment.get("TEST"_el), "stored"_el);
        REQUIRE_EQUAL(environment.get("TEST"_el, "fallback"_el), "stored"_el);
        REQUIRE_EQUAL(environment.getOrThrow("TEST"_el), "stored"_el);
    }

    void testNonThrowingFailures() {
        auto backend = std::make_unique<TestBackend>();
        auto *backendPtr = backend.get();
        auto environment = el::system::EnvironmentVariables{std::move(backend)};

        backendPtr->failGet = true;
        REQUIRE_FALSE(environment.get("TEST"_el).has_value());
        REQUIRE_EQUAL(environment.get("TEST"_el, "fallback"_el), "fallback"_el);
        REQUIRE_THROWS_AS(el::system::PlatformError, environment.getOrThrow("TEST"_el));

        backendPtr->failSet = true;
        REQUIRE_FALSE(environment.set("TEST"_el, "value"_el));
        REQUIRE_THROWS_AS(el::system::PlatformError, environment.setOrThrow("TEST"_el, "value"_el));

        backendPtr->failRemove = true;
        REQUIRE_FALSE(environment.remove("TEST"_el));
        REQUIRE_THROWS_AS(el::system::PlatformError, environment.removeOrThrow("TEST"_el));
    }

    void testSetAndRemove() {
        auto backend = std::make_unique<TestBackend>();
        auto *backendPtr = backend.get();
        auto environment = el::system::EnvironmentVariables{std::move(backend)};

        REQUIRE(environment.set("TEST"_el, "first"_el));
        REQUIRE_EQUAL(*backendPtr->value, "first"_el);
        environment.setOrThrow("TEST"_el, "second"_el);
        REQUIRE_EQUAL(*backendPtr->value, "second"_el);
        REQUIRE(environment.remove("TEST"_el));
        REQUIRE_FALSE(backendPtr->value.has_value());
        environment.removeOrThrow("TEST"_el);
        REQUIRE_FALSE(backendPtr->value.has_value());
    }

    void testValidation() {
        auto environment = el::system::EnvironmentVariables{std::make_unique<TestBackend>()};
        const auto nullName = el::text::String{std::string_view{"A\0B", 3U}};
        const auto nullValue = el::text::String{std::string_view{"A\0B", 3U}};

        REQUIRE_FALSE(environment.get({}).has_value());
        REQUIRE_FALSE(environment.get("A=B"_el).has_value());
        REQUIRE_FALSE(environment.get(nullName).has_value());
        REQUIRE_THROWS_AS(el::err::ParameterError, environment.getOrThrow({}));
        REQUIRE_THROWS_AS(el::err::ParameterError, environment.getOrThrow("A=B"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, environment.getOrThrow(nullName));

        REQUIRE_FALSE(environment.set({}, "value"_el));
        REQUIRE_FALSE(environment.set("A=B"_el, "value"_el));
        REQUIRE_FALSE(environment.set(nullName, "value"_el));
        REQUIRE_FALSE(environment.set("TEST"_el, nullValue));
        REQUIRE_THROWS_AS(el::err::ParameterError, environment.setOrThrow({}, "value"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, environment.setOrThrow("A=B"_el, "value"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, environment.setOrThrow(nullName, "value"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, environment.setOrThrow("TEST"_el, nullValue));

        REQUIRE_FALSE(environment.remove({}));
        REQUIRE_THROWS_AS(el::err::ParameterError, environment.removeOrThrow({}));
    }

    void testRejectsNullBackend() {
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            el::system::EnvironmentVariables{el::system::impl::EnvironmentVariableBackendPtr{}});
    }

    void testDefaultBackend() {
        auto environment = el::system::EnvironmentVariables{};
        const auto name = el::text::String{"ERBSLAND_CORE_ENVIRONMENT_VARIABLES_TEST"_el};
        const auto guard = VariableGuard{environment, name};

        environment.removeOrThrow(name);
        REQUIRE_FALSE(environment.get(name).has_value());
        environment.removeOrThrow(name);

        environment.setOrThrow(name, "first"_el);
        REQUIRE_EQUAL(environment.getOrThrow(name), "first"_el);
        environment.setOrThrow(name, "second"_el);
        REQUIRE_EQUAL(environment.getOrThrow(name), "second"_el);
        environment.setOrThrow(name, {});
        const auto emptyValue = environment.get(name);
        REQUIRE(emptyValue.has_value());
        REQUIRE(emptyValue->isEmpty());

        environment.removeOrThrow(name);
        REQUIRE_FALSE(environment.get(name).has_value());
    }

    void testDefaultBackendWithSlicedInputs() {
        constexpr auto nameText = std::string_view{"ERBSLAND_CORE_SLICED_ENVIRONMENT_VARIABLE_TEST"};
        constexpr auto valueText = std::string_view{"sliced-value"};
        const auto nameSource = el::text::String{"xERBSLAND_CORE_SLICED_ENVIRONMENT_VARIABLE_TEST-tail"_el};
        const auto valueSource = el::text::String{"xsliced-value-tail"_el};
        const auto name = nameSource.slice(
            el::unit::ByteRange{el::unit::ByteIndex{1U}, el::unit::ByteLength::fromSizeT(nameText.size())});
        const auto value = valueSource.slice(
            el::unit::ByteRange{el::unit::ByteIndex{1U}, el::unit::ByteLength::fromSizeT(valueText.size())});
        auto environment = el::system::EnvironmentVariables{};
        const auto canonicalName = el::text::String{nameText};
        const auto guard = VariableGuard{environment, canonicalName};

        environment.setOrThrow(name, value);
        REQUIRE_EQUAL(environment.getOrThrow(canonicalName), el::text::String{valueText});
        environment.removeOrThrow(name);
        REQUIRE_FALSE(environment.get(canonicalName).has_value());
    }
};
