// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormatForConf.hpp>
#include <erbsland/conf/vr/Rules.hpp>

#include <algorithm>
#include <ranges>

using namespace el::conf;
using namespace el::text::literals;

// Testing all requirements from chapter "Validation Rules"->"Dependencies".
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrDependenciesTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testSectionListRequired() {
        // #1: Section List Required: Dependencies must be defined using a section list named "vr_dependency".
        WITH_CONTEXT(requireRulesFailLines({
            "[vr_dependency]",
            "mode: \"if\"",
            "source: \"server.hostname\"",
            "target: \"server.ip_address\"",
        }));
        WITH_CONTEXT(requireError("vr_dependency"));
        WITH_CONTEXT(requireError("section list"));
    }

    void testPlacementAndScope() {
        // #2: Placement and Scope: vr_dependency may appear at root or within a section node-rules definition.
        // Dependencies apply only within the subtree of the section in which they are defined.
        WITH_CONTEXT(requireRulesPassLines({
            "[server]",
            "type: \"section\"",
            "is_optional: yes",
            "[server.hostname]",
            "type: \"text\"",
            "is_optional: yes",
            "[server.ip_address]",
            "type: \"text\"",
            "is_optional: yes",
            "*[server.vr_dependency]*",
            "mode: \"xor\"",
            "source: \"hostname\"",
            "target: \"ip_address\"",
            "[client]",
            "type: \"section\"",
            "is_optional: yes",
            "[client.hostname]",
            "type: \"text\"",
            "is_optional: yes",
            "[client.ip_address]",
            "type: \"text\"",
            "is_optional: yes",
        }));

        // Dependency is scoped to [server], so client should not be affected.
        WITH_CONTEXT(requirePassLines({
            "[client]",
            "hostname: \"client.local\"",
        }));

        // Dependency applies within [server].
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "hostname: \"server.local\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "ip_address: \"127.0.0.1\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "hostname: \"server.local\"",
            "ip_address: \"127.0.0.1\"",
        }));
        WITH_CONTEXT(requireError("either configure 'hostname' or configure 'ip_address'"));

        // vr_dependency must not be placed under non-section node rules.
        WITH_CONTEXT(requireRulesFailLines({
            "[client.username]",
            "type: \"text\"",
            "*[client.username.vr_dependency]*",
            "mode: \"if\"",
            "source: \"username\"",
            "target: \"password\"",
        }));
        WITH_CONTEXT(requireError("Dependency definitions can only be placed in node-rules definition of a section"));
    }

    void requireModePass(const std::string_view modeStr) {
        auto modeLine = std::format("mode: \"{}\"", modeStr);
        WITH_CONTEXT(requireRulesPassLines({
            "[app.a]",
            "type: \"integer\"",
            "is_optional: yes",
            "[app.b]",
            "type: \"integer\"",
            "is_optional: yes",
            "*[app.vr_dependency]*",
            std::string_view{modeLine},
            "source: \"a\"",
            "target: \"b\"",
        }));
    }

    void requireModeFail(const std::string_view modeStr) {
        auto modeLine = std::format("mode: \"{}\"", modeStr);
        WITH_CONTEXT(requireRulesFailLines({
            "[app.a]",
            "type: \"integer\"",
            "is_optional: yes",
            "[app.b]",
            "type: \"integer\"",
            "is_optional: yes",
            "*[client.vr_dependency]*",
            std::string_view{modeLine},
            "source: \"a\"",
            "target: \"b\"",
        }));
    }

    void testModeRequiredAndSupported() {
        // #3: Mode Required: Each dependency must define a mode entry.
        WITH_CONTEXT(requireRulesFailLines({
            "*[vr_dependency]*",
            "source: \"a\"",
            "target: \"b\"",
        }));
        WITH_CONTEXT(requireError("mode"));

        // valid modes
        WITH_CONTEXT(requireModePass("if"));
        WITH_CONTEXT(requireModePass("if_not"));
        WITH_CONTEXT(requireModePass("or"));
        WITH_CONTEXT(requireModePass("xor"));
        WITH_CONTEXT(requireModePass("xnor"));
        WITH_CONTEXT(requireModePass("and"));

        // name rules
        WITH_CONTEXT(requireModePass("If_Not"));
        WITH_CONTEXT(requireModePass("IF NOT"));
        WITH_CONTEXT(requireModePass("If Not"));

        // invalid modes
        WITH_CONTEXT(requireModeFail(""));
        WITH_CONTEXT(requireModeFail("ifif"));
        WITH_CONTEXT(requireModeFail("nand"));
    }

    void testSourceAndTargetRequired() {
        // #4: Source and Target Required: Each dependency must define both source and target.
        WITH_CONTEXT(requireRulesFailLines({
            "*[vr_dependency]*",
            "mode: \"if\"",
            "target: \"b\"",
        }));
        WITH_CONTEXT(requireError("source"));

        WITH_CONTEXT(requireRulesFailLines({
            "*[vr_dependency]*",
            "mode: \"if\"",
            "source: \"a\"",
        }));
        WITH_CONTEXT(requireError("target"));
    }

    void requireOrSemantics(const std::vector<std::string_view> &valueNames, const bool pass) {

        el::text::StringList doc;
        doc.append(el::text::String{"[app]"_el});
        for (const auto &name : valueNames) {
            auto line = el::text::StringEditor{};
            line.append(el::text::String{name});
            line.append(el::text::String{": 1"_el});
            doc.append(line);
        }
        auto docStr = doc.join(el::text::String{"\n"_el});
        if (pass) {
            requirePass(docStr);
        } else {
            requireFail(docStr);
        }
    }

    void testMultipleValuesOrSemantics() {
        // #5: Multiple Values (OR Semantics): Multiple name paths are treated as OR.
        WITH_CONTEXT(requireRulesPassLines({
            "[app.a]",
            "type: \"integer\"",
            "is_optional: yes",
            "[app.b]",
            "type: \"integer\"",
            "is_optional: yes",
            "[app.c]",
            "type: \"integer\"",
            "is_optional: yes",
            "[app.x]",
            "type: \"integer\"",
            "is_optional: yes",
            "[app.y]",
            "type: \"integer\"",
            "is_optional: yes",
            "[app.z]",
            "type: \"integer\"",
            "is_optional: yes",
            "*[app.vr_dependency]*",
            "mode: \"or\"",
            "source: \"a\", \"b\", \"c\"",
            "target: \"x\", \"y\", \"z\"",
        }));

        WITH_CONTEXT(requireOrSemantics({}, false));
        WITH_CONTEXT(requireError("You must configure at least one of 'a', 'b', 'c', 'x', 'y', or 'z'"));
        WITH_CONTEXT(requireOrSemantics({"a"}, true));
        WITH_CONTEXT(requireOrSemantics({"b"}, true));
        WITH_CONTEXT(requireOrSemantics({"c"}, true));
        WITH_CONTEXT(requireOrSemantics({"x"}, true));
        WITH_CONTEXT(requireOrSemantics({"y"}, true));
        WITH_CONTEXT(requireOrSemantics({"z"}, true));
        WITH_CONTEXT(requireOrSemantics({"a", "b"}, true));
        WITH_CONTEXT(requireOrSemantics({"a", "b", "c"}, true));
        WITH_CONTEXT(requireOrSemantics({"a", "b", "c", "x"}, true));
        WITH_CONTEXT(requireOrSemantics({"a", "b", "c", "x", "y"}, true));
        WITH_CONTEXT(requireOrSemantics({"a", "b", "c", "x", "y", "z"}, true));
        WITH_CONTEXT(requireOrSemantics({"b", "c"}, true));
        WITH_CONTEXT(requireOrSemantics({"b", "c", "x"}, true));
        WITH_CONTEXT(requireOrSemantics({"b", "c", "x", "y"}, true));
        WITH_CONTEXT(requireOrSemantics({"b", "c", "x", "y", "z"}, true));
        WITH_CONTEXT(requireOrSemantics({"c", "x"}, true));
        WITH_CONTEXT(requireOrSemantics({"c", "x", "y"}, true));
        WITH_CONTEXT(requireOrSemantics({"c", "x", "y", "z"}, true));
        WITH_CONTEXT(requireOrSemantics({"x", "y"}, true));
        WITH_CONTEXT(requireOrSemantics({"x", "y", "z"}, true));
        WITH_CONTEXT(requireOrSemantics({"y", "z"}, true));
    }

    void testCustomErrorMessage() {
        // #6: Custom ConfError Messages: The error entry provides a custom validation message.
        WITH_CONTEXT(requireRulesPassLines({
            "[server.hostname]",
            "type: \"text\"",
            "is_optional: yes",
            "[server.ip_address]",
            "type: \"text\"",
            "is_optional: yes",
            "*[server.vr_dependency]*",
            "mode: \"xor\"",
            "source: \"hostname\"",
            "target: \"ip_address\"",
            "error: \"Configure either 'hostname' or 'ip_address', not both.\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "hostname: \"server.local\"",
            "ip_address: \"127.0.0.1\"",
        }));
        WITH_CONTEXT(requireError("Configure either 'hostname' or 'ip_address', not both."));
    }

    void testDefaultsDoNotCountAsConfigured() {
        // Default values do not count as configured for dependency checks.
        WITH_CONTEXT(requireRulesPassLines({
            "[app]",
            "type: \"section\"",
            "[app.username]",
            "type: \"text\"",
            "default: \"user\"",
            "[app.password]",
            "type: \"text\"",
            "is_optional: yes",
            "*[app.vr_dependency]*",
            "mode: \"if\"",
            "source: \"username\"",
            "target: \"password\"",
        }));

        // Default applied to username must not trigger the dependency.
        WITH_CONTEXT(requirePassLines({
            "[app]",
        }));

        // When a username is explicitly configured, a password is required.
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "username: \"admin\"",
        }));
        WITH_CONTEXT(requireError("If 'username' is configured, you must also configure 'password'"));
    }

    enum class VC {
        None,
        S,
        T,
        ST,
    };

    void createLogicRules(const std::string_view modeStr) {
        auto modeLine = std::format("mode: \"{}\"", modeStr);
        WITH_CONTEXT(requireRulesPassLines({
            "[app.a]",
            "type: \"integer\"",
            "is_optional: yes",
            "[app.b]",
            "type: \"integer\"",
            "is_optional: yes",
            "*[app.vr_dependency]*",
            std::string_view{modeLine},
            "source: \"a\"",
            "target: \"b\"",
        }));
    }

    void requireLogicCombination(const std::vector<VC> &allowedCombinations) {
        static auto combiDoc = std::unordered_map<VC, std::string_view>{
            {VC::None, "[app]\n"},
            {VC::S, "[app]\na: 1\n"},
            {VC::T, "[app]\nb: 1\n"},
            {VC::ST, "[app]\na: 1\nb: 1\n"},
        };
        for (const auto combination : {VC::None, VC::S, VC::T, VC::ST}) {
            if (std::ranges::find(allowedCombinations, combination) != allowedCombinations.end()) {
                WITH_CONTEXT(requirePass(combiDoc.at(combination)));
            } else {
                WITH_CONTEXT(requireFail(combiDoc.at(combination)));
            }
        }
    }

    void testLogic() {
        struct Data {
            std::string_view mode;
            std::vector<VC> allowedCombinations;
        };
        auto testData = std::array<Data, 6>{
            Data{.mode = "if", .allowedCombinations = {VC::None, VC::T, VC::ST}},
            Data{.mode = "if_not", .allowedCombinations = {VC::None, VC::S, VC::T}},
            Data{.mode = "or", .allowedCombinations = {VC::S, VC::T, VC::ST}},
            Data{.mode = "xor", .allowedCombinations = {VC::S, VC::T}},
            Data{.mode = "xnor", .allowedCombinations = {VC::None, VC::ST}},
            Data{.mode = "and", .allowedCombinations = {VC::ST}}};
        for (const auto &data : testData) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    createLogicRules(data.mode);
                    WITH_CONTEXT(requireLogicCombination(data.allowedCombinations));
                },
                [&]() -> std::string { return std::format("Failed for mode {}.", data.mode); });
        }
    }
};
