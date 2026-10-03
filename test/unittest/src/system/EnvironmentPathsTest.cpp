// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/EnvironmentChanges.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/PlatformError.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/system/impl/environment/EnvironmentChangesProcessor.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(EnvironmentPaths EnvironmentVariables EnvironmentVariableBackend)
class EnvironmentPathsTest final : public el::UnitTest {
private:
    static constexpr auto cPathVariable = "PATH"_el;

    struct State {
        el::StringMap<el::String> values;
        bool failGet{};
        bool failSet{};
    };
    class Backend final : public el::system::impl::EnvironmentVariableBackend {
    public:
        explicit Backend(std::shared_ptr<State> state) : _state{std::move(state)} {}
        auto get(const el::String &name) const -> std::optional<el::String> override {
            if (_state->failGet) {
                throw el::PlatformError{"Injected lookup failure"_el};
            }
            for (const auto &[key, value] : _state->values) {
                if (compare(key, name) == 0) {
                    return value;
                }
            }
            return {};
        }
        void set(const el::String &name, const el::String &value) override {
            if (_state->failSet) {
                throw el::PlatformError{"Injected assignment failure"_el};
            }
            _state->values.set(name, value);
        }
        void remove(const el::String &name) override { _state->values.remove(name); }

    private:
        std::shared_ptr<State> _state;
    };

public:
    void testPortableEditingAndInsertionPositions() {
        auto state = std::make_shared<State>();
        auto environment = el::EnvironmentVariables{std::make_unique<Backend>(state)};
        REQUIRE(environment.paths().isEmpty());
        REQUIRE(environment.removePath(el::Path{"absent"_el}));
        REQUIRE(!environment.get(cPathVariable));
        REQUIRE(environment.appendPath(el::Path{"first"_el}));
        environment.appendPathOrThrow(el::Path{"tail"_el});
        REQUIRE(environment.insertPath(el::ItemIndex{1}, el::Path{"middle"_el}));
        environment.prependPathOrThrow(el::Path{"tail"_el});
        REQUIRE_EQUAL(
            environment.paths(), (el::PathList{el::Path{"tail"_el}, el::Path{"first"_el}, el::Path{"middle"_el}}));
        environment.insertPathOrThrow(el::ItemIndex{3}, el::Path{"tail"_el});
        environment.appendPathOrThrow(el::Path{"first/."_el});
        REQUIRE_EQUAL(
            environment.pathsOrThrow(),
            (el::PathList{el::Path{"first"_el}, el::Path{"middle"_el}, el::Path{"tail"_el}}));
        environment.removePathOrThrow(el::Path{"middle"_el});
        environment.appendPathOrThrow(el::Path{"sdk/bin"_el});
        REQUIRE(environment.appendPath(el::Path{"sdk-old"_el}));
        REQUIRE(environment.removePathsIn(el::Path{"sdk"_el}));
        REQUIRE_EQUAL(
            environment.paths(), (el::PathList{el::Path{"first"_el}, el::Path{"tail"_el}, el::Path{"sdk-old"_el}}));
        REQUIRE_FALSE(environment.insertPath(el::ItemIndex{9}, el::Path{"invalid"_el}));
        REQUIRE_THROWS_AS(
            el::ParameterFailure, environment.insertPathOrThrow(el::ItemIndex::noIndex(), el::Path{"invalid"_el}));
        REQUIRE_FALSE(environment.prependPath(el::Path{}));
        REQUIRE_FALSE(environment.appendPath(el::Path{}));
        REQUIRE_FALSE(environment.removePath(el::Path{}));
        REQUIRE_FALSE(environment.removePathsIn(el::Path{}));
    }

    void testMalformedEntriesAndCurrentDirectory() {
        auto state = std::make_shared<State>();
        auto environment = el::EnvironmentVariables{std::make_unique<Backend>(state)};
        const auto bytes = el::unittest::th::stdStringFromHex("FF 61 FE");
        const auto opaque = el::String{std::string_view{bytes}};
        const auto separator =
            el::String::fromCharacter(el::system::impl::EnvironmentVariableBackend::native().pathSeparator());
        const auto source = el::StringList{"a"_el, ""_el, "."_el, "a/."_el, opaque, "b"_el, ""_el}.join(separator);
        environment.setOrThrow(cPathVariable, source);
        REQUIRE_EQUAL(environment.paths(), (el::PathList{el::Path{"a"_el}, el::Path{"."_el}, el::Path{"b"_el}}));
        // Reading does not mutate the original environment.
        REQUIRE_EQUAL(environment.getOrThrow(cPathVariable).length(), source.length());
        environment.removePathOrThrow(el::Path{"missing"_el});
        REQUIRE_EQUAL(environment.getOrThrow(cPathVariable), (el::StringList{"a"_el, ""_el, "b"_el}.join(separator)));
        environment.setOrThrow(cPathVariable, ""_el);
        REQUIRE_EQUAL(environment.paths(), el::PathList{el::Path::currentElement()});
    }

    void testWindowsQuotedPathsPlaceholdersAndCase() {
#ifdef EL_OS_WINDOWS
        auto state = std::make_shared<State>();
        auto environment = el::EnvironmentVariables{std::make_unique<Backend>(state)};
        environment.setOrThrow("ROOT"_el, R"/(C:\Program Files)/"_el);
        const auto invalidBytes = el::unittest::th::stdStringFromHex("FF FE");
        environment.setOrThrow("BAD"_el, el::String{std::string_view{invalidBytes}});
        environment.setOrThrow("LOOP"_el, "%LOOP%"_el);
        environment.setOrThrow("LIST"_el, "C:/one;C:/two"_el);
        environment.setOrThrow(
            cPathVariable,
            R"/("%root%\bin";c:/PROGRAM FILES/bin/.;C:relative;%BAD%;%bad=name%;%MISSING%\bin;"unclosed;%LOOP%;%LIST%;\\server\share\bin;C:/Keep)/"_el);
        REQUIRE_EQUAL(
            environment.paths(),
            (el::PathList{
                el::Path::fromWindows("C:/Program Files/bin"_el),
                el::Path::fromWindows("//server/share/bin"_el),
                el::Path::fromWindows("C:/Keep"_el)}));
        environment.appendPathOrThrow(el::Path::fromWindows("c:/keep"_el));
        REQUIRE_EQUAL(environment.getOrThrow(cPathVariable), R"/("%root%\bin";\\server\share\bin;C:/Keep)/"_el);
        environment.removePathsInOrThrow(el::Path::fromWindows("c:/PROGRAM FILES"_el));
        REQUIRE_EQUAL(environment.paths().count(), el::ItemCount{2});
#endif
    }

    void testExecutableCandidatesDoNotProbeTheFilesystem() {
        auto state = std::make_shared<State>();
        auto environment = el::EnvironmentVariables{std::make_unique<Backend>(state)};
        environment.appendPathOrThrow(el::Path{"first"_el});
        environment.appendPathOrThrow(el::Path{"second"_el});
        auto suffix = el::String{};
#ifdef EL_OS_WINDOWS
        suffix = ".exe"_el;
#endif
        const auto name = el::join("compiler"_el, suffix);
        REQUIRE_EQUAL(
            environment.executableCandidates("compiler"_el),
            (el::PathList{el::Path{"first"_el} / name, el::Path{"second"_el} / name}));
        REQUIRE_EQUAL(environment.executableCandidates("compiler.exe"_el).first().name(), "compiler.exe"_el);
        REQUIRE(environment.executableCandidates("../compiler"_el).isEmpty());
        REQUIRE(environment.executableCandidates({}).isEmpty());
        REQUIRE_THROWS_AS(el::ParameterFailure, environment.executableCandidatesOrThrow("folder/compiler"_el));
        const auto absolute = el::system::impl::EnvironmentVariableBackend::native().pathKey("/tools/compiler"_el);
        REQUIRE_EQUAL(
            environment.executableCandidatesOrThrow("/tools/compiler"_el), el::PathList{absolute.parent() / name});
    }

    void testFailuresDoNotOverwritePath() {
        auto state = std::make_shared<State>();
        auto environment = el::EnvironmentVariables{std::make_unique<Backend>(state)};
        environment.setOrThrow(cPathVariable, "original"_el);
        state->failGet = true;
        REQUIRE(environment.paths().isEmpty());
        REQUIRE(environment.executableCandidates("program"_el).isEmpty());
        REQUIRE_FALSE(environment.prependPath(el::Path{"new"_el}));
        REQUIRE_FALSE(environment.appendPath(el::Path{"new"_el}));
        REQUIRE_FALSE(environment.insertPath(el::ItemIndex{0}, el::Path{"new"_el}));
        REQUIRE_FALSE(environment.removePath(el::Path{"original"_el}));
        REQUIRE_FALSE(environment.removePathsIn(el::Path{"original"_el}));
        REQUIRE_THROWS_AS(el::PlatformError, environment.pathsOrThrow());
        REQUIRE_EQUAL(state->values.get(cPathVariable).value(), "original"_el);
        state->failGet = false;
        state->failSet = true;
        REQUIRE_FALSE(environment.appendPath(el::Path{"new"_el}));
        REQUIRE_EQUAL(environment.getOrThrow(cPathVariable), "original"_el);
    }

    void testDeferredChangesExpandAgainstTheResultingEnvironment() {
#ifdef EL_OS_WINDOWS
        using Processor = el::system::impl::EnvironmentChangesProcessor;
        auto base = Processor::Environment<el::String>{};
        base.emplace(cPathVariable, R"/("%ROOT%\bin";%MISSING%;C:relative;C:/Keep)/"_el);
        base.emplace("ROOT"_el, "Old"_el);
        auto changes = el::EnvironmentChanges{}.setVariable("ROOT"_el, "New"_el).removePathsIn(el::Path{"New"_el});
        REQUIRE_EQUAL(Processor::apply(changes, base).at(cPathVariable), "C:/Keep"_el);
#endif
    }
};
