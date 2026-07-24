// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Testing subtree/sub-branch validation as described in the validation-rules documentation.
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrSubBranchValidationTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void requireParsedDocumentLines(const std::vector<std::string_view> &lines) {
        lastError = {};
        Parser parser;
        try {
            document = parser.parseTextOrThrow(linesToString(lines));
        } catch (const ConfError &e) {
            setLastError(e);
            REQUIRE(false);
        }
    }

    void requireBranchPass(const std::string_view namePath, const Integer version = 0) {
        lastError = {};
        auto branch = document->valueOrThrow(el::text::String{namePath});
        try {
            rules->validate(branch, version);
        } catch (const ConfError &e) {
            setLastError(e);
            REQUIRE(false);
        }
    }

    void requireBranchFail(const std::string_view namePath, const Integer version = 0) {
        lastError = {};
        auto branch = document->valueOrThrow(el::text::String{namePath});
        try {
            rules->validate(branch, version);
            REQUIRE(false);
        } catch (const ConfError &e) {
            REQUIRE_EQUAL(e.category(), ConfErrorCategory::Validation);
            setLastError(e);
        }
    }

    static auto buildRulesFromLines(const std::vector<std::string_view> &lines) -> vr::RulesPtr {
        Parser parser;
        auto rulesDocument = parser.parseTextOrThrow(linesToString(lines));
        return vr::Rules::createFromDocument(rulesDocument);
    }

    void testPathsAreRelativeToValidatedSubBranchRoot() {
        // Document model: if rules are applied to a subtree, name paths are interpreted relative to that root.
        WITH_CONTEXT(requireRulesPassLines({
            "[enabled]",
            "type: \"boolean\"",
            "[port]",
            "type: \"integer\"",
            "minimum: 1",
            "maximum: 65535",
        }));
        WITH_CONTEXT(requireParsedDocumentLines({
            "[extension1]",
            "enabled: yes",
            "port: 8080",
            "[extension2]",
            "enabled: no",
            "port: 9000",
        }));
        WITH_CONTEXT(requireBranchPass("extension1"));
        WITH_CONTEXT(requireBranchPass("extension2"));
    }

    void testValidationScopeIsLimitedToTheValidatedSubBranch() {
        // Introduction/model: rules can be applied to a subtree. Siblings outside that subtree must not interfere.
        WITH_CONTEXT(requireRulesPassLines({
            "[enabled]",
            "type: \"boolean\"",
            "[port]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireParsedDocumentLines({
            "[extension1]",
            "enabled: yes",
            "port: 8080",
            "[extension2]",
            "enabled: yes",
            "port: \"not-an-integer\"",
        }));
        WITH_CONTEXT(requireBranchPass("extension1"));
        WITH_CONTEXT(requireBranchFail("extension2"));
        WITH_CONTEXT(requireError("Expected an integer value but got a text value"));
    }

    void testClosedByDefaultStillAppliesInsideTheSubBranch() {
        // Document model: unknown nodes are forbidden unless covered by rules.
        WITH_CONTEXT(requireRulesPassLines({
            "[port]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireParsedDocumentLines({
            "[extension1]",
            "port: 8080",
            "debug: yes",
        }));
        WITH_CONTEXT(requireBranchFail("extension1"));
        WITH_CONTEXT(requireError("unexpected boolean value"));
    }

    void testImplicitAncestorsAreResolvedRelativeToSubBranchRoot() {
        // Document model: implicit ancestors are created from missing path elements.
        WITH_CONTEXT(requireRulesPassLines({
            "[server.http.port]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireParsedDocumentLines({
            "[extension]",
            "[extension.server.http]",
            "port: 443",
        }));
        WITH_CONTEXT(requireBranchPass("extension"));

        WITH_CONTEXT(requireParsedDocumentLines({
            "[extension]",
        }));
        WITH_CONTEXT(requireBranchFail("extension"));
        WITH_CONTEXT(requireError("expected a section with the name 'server'"));
    }

    void testDefaultsAreAppliedToTheValidatedBranchOnly() {
        // Public API contract: defaults are inserted into the validated value/document branch.
        WITH_CONTEXT(requireRulesPassLines({
            "[timeout]",
            "type: \"integer\"",
            "default: 30",
        }));
        WITH_CONTEXT(requireParsedDocumentLines({
            "[extension1]",
            "[extension2]",
        }));

        WITH_CONTEXT(requireBranchPass("extension1"));
        REQUIRE_EQUAL(document->getOrThrow<Integer>(el::text::String{"extension1.timeout"}), 30);
        REQUIRE_FALSE(document->hasValue(el::text::String{"extension2.timeout"}));
        REQUIRE(document->valueOrThrow(el::text::String{"extension1.timeout"})->isDefaultValue());
    }

    void testSubBranchValidationRequiresSectionOrDocumentRoot() {
        WITH_CONTEXT(requireRulesPassLines({
            "[port]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireParsedDocumentLines({
            "[extension1]",
            "port: 8080",
        }));

        try {
            rules->validate(document->valueOrThrow(el::text::String{"extension1.port"}), 0);
            REQUIRE(false);
        } catch (const ConfError &e) {
            REQUIRE_EQUAL(e.category(), ConfErrorCategory::Validation);
            setLastError(e);
        }
        WITH_CONTEXT(requireError("must be a document or a section with names"));
    }

    void testOverlappingValidationOnRootAndSubBranchIsSupported() {
        // Design goal: applications may validate a root with one rules set and a subtree with another.
        auto coreRules = buildRulesFromLines({
            "[app]",
            "type: \"section\"",
            "[.extensions]",
            "type: \"value_list\"",
            "[.extensions.vr_entry]",
            "type: \"text\"",
            "[extension1]",
            "type: \"not_validated\"",
        });
        auto extensionRules = buildRulesFromLines({
            "[enabled]",
            "type: \"boolean\"",
            "[port]",
            "type: \"integer\"",
            "minimum: 1",
            "maximum: 65535",
        });
        WITH_CONTEXT(requireParsedDocumentLines({
            "[app]",
            "extensions: \"extension1\"",
            "[extension1]",
            "enabled: yes",
            "port: 8080",
        }));

        REQUIRE_NOTHROW(coreRules->validate(document, 0));
        REQUIRE_EQUAL(
            document->valueOrThrow(el::text::String{"extension1.enabled"})->validationRule()->type(),
            vr::RuleType::NotValidated);
        REQUIRE_NOTHROW(extensionRules->validate(document->valueOrThrow(el::text::String{"extension1"}), 0));
        REQUIRE_EQUAL(
            document->valueOrThrow(el::text::String{"extension1.enabled"})->validationRule()->type(),
            vr::RuleType::Boolean);
        REQUIRE_EQUAL(
            document->valueOrThrow(el::text::String{"extension1.port"})->validationRule()->type(),
            vr::RuleType::Integer);
    }

    void testOverlappingValidationIgnoresExistingDefaultValues() {
        // Existing default values from an earlier validation must not count as configured values in later validations.
        WITH_CONTEXT(requireParsedDocumentLines({
            "[extension1]",
        }));

        auto defaultingRules = buildRulesFromLines({
            "[port]",
            "type: \"integer\"",
            "default: 8080",
        });
        REQUIRE_NOTHROW(defaultingRules->validate(document->valueOrThrow(el::text::String{"extension1"}), 0));
        REQUIRE(document->valueOrThrow(el::text::String{"extension1.port"})->isDefaultValue());

        auto strictRules = buildRulesFromLines({
            "[port]",
            "type: \"integer\"",
        });
        try {
            strictRules->validate(document->valueOrThrow(el::text::String{"extension1"}), 0);
            REQUIRE(false);
        } catch (const ConfError &e) {
            REQUIRE_EQUAL(e.category(), ConfErrorCategory::Validation);
            setLastError(e);
        }
        WITH_CONTEXT(requireError("expected an integer value with the name 'port'"));
    }

    void testOverlappingDefaultsAreReplacedByLaterRulesDocument() {
        WITH_CONTEXT(requireParsedDocumentLines({
            "[app]",
            "[.sub]",
        }));
        auto rulesA = buildRulesFromLines({
            "[sub.x]",
            "type: \"integer\"",
            "default: 1",
        });
        auto rulesB = buildRulesFromLines({
            "[sub.x]",
            "type: \"integer\"",
            "default: 2",
        });

        REQUIRE_NOTHROW(rulesA->validate(document->valueOrThrow(el::text::String{"app"}), 0));
        REQUIRE_EQUAL(document->getOrThrow<Integer>(el::text::String{"app.sub.x"}), 1);
        REQUIRE(document->valueOrThrow(el::text::String{"app.sub.x"})->isDefaultValue());

        REQUIRE_NOTHROW(rulesB->validate(document->valueOrThrow(el::text::String{"app"}), 0));
        REQUIRE_EQUAL(document->getOrThrow<Integer>(el::text::String{"app.sub.x"}), 2);
        REQUIRE(document->valueOrThrow(el::text::String{"app.sub.x"})->isDefaultValue());
    }

    void testOverlappingOptionalRuleRemovesEarlierDefaultValue() {
        WITH_CONTEXT(requireParsedDocumentLines({
            "[app]",
            "[.sub]",
        }));
        auto rulesA = buildRulesFromLines({
            "[sub.x]",
            "type: \"integer\"",
            "default: 1",
        });
        auto rulesC = buildRulesFromLines({
            "[sub.x]",
            "type: \"integer\"",
            "is_optional: yes",
        });

        REQUIRE_NOTHROW(rulesA->validate(document->valueOrThrow(el::text::String{"app"}), 0));
        REQUIRE_EQUAL(document->getOrThrow<Integer>(el::text::String{"app.sub.x"}), 1);
        REQUIRE(document->valueOrThrow(el::text::String{"app.sub.x"})->isDefaultValue());

        REQUIRE_NOTHROW(rulesC->validate(document->valueOrThrow(el::text::String{"app"}), 0));
        REQUIRE_FALSE(document->hasValue(el::text::String{"app.sub.x"}));
    }

    void testOverlappingValidationRemovesNestedDefaultsBeforeConstraints() {
        WITH_CONTEXT(requireParsedDocumentLines({
            "[app]",
            "[.sub]",
        }));
        auto defaultingRules = buildRulesFromLines({
            "[sub.x]",
            "type: \"integer\"",
            "default: 1",
        });
        auto strictRules = buildRulesFromLines({
            "[sub]",
            "type: \"section\"",
            "minimum: 1",
            "[sub.vr_any]",
            "type: \"integer\"",
        });

        REQUIRE_NOTHROW(defaultingRules->validate(document->valueOrThrow(el::text::String{"app"}), 0));
        REQUIRE(document->valueOrThrow(el::text::String{"app.sub.x"})->isDefaultValue());

        try {
            strictRules->validate(document->valueOrThrow(el::text::String{"app"}), 0);
            REQUIRE(false);
        } catch (const ConfError &e) {
            REQUIRE_EQUAL(e.category(), ConfErrorCategory::Validation);
            setLastError(e);
        }
        WITH_CONTEXT(requireError("number of entries in this section must be at least 1"));
    }
};
