// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/impl/vr/EqualsBooleanConstraint.hpp>
#include <erbsland/conf/impl/vr/EqualsBytesConstraint.hpp>
#include <erbsland/conf/impl/vr/EqualsConstraint.hpp>
#include <erbsland/conf/impl/vr/EqualsFloatConstraint.hpp>
#include <erbsland/conf/impl/vr/EqualsIntegerConstraint.hpp>
#include <erbsland/conf/impl/vr/EqualsMatrixConstraint.hpp>
#include <erbsland/conf/impl/vr/EqualsTextConstraint.hpp>
#include <erbsland/conf/impl/vr/InBytesConstraint.hpp>
#include <erbsland/conf/impl/vr/InConstraint.hpp>
#include <erbsland/conf/impl/vr/InFloatConstraint.hpp>
#include <erbsland/conf/impl/vr/InIntegerConstraint.hpp>
#include <erbsland/conf/impl/vr/InTextConstraint.hpp>
#include <erbsland/conf/impl/vr/KeyConstraint.hpp>
#include <erbsland/conf/impl/vr/MinMaxConstraint.hpp>
#include <erbsland/conf/impl/vr/MinMaxDateConstraint.hpp>
#include <erbsland/conf/impl/vr/MinMaxDateTimeConstraint.hpp>
#include <erbsland/conf/impl/vr/MinMaxFloatConstraint.hpp>
#include <erbsland/conf/impl/vr/MinMaxIntegerConstraint.hpp>
#include <erbsland/conf/impl/vr/MinMaxMatrixConstraint.hpp>
#include <erbsland/conf/impl/vr/MultipleConstraint.hpp>
#include <erbsland/conf/impl/vr/MultipleFloatConstraint.hpp>
#include <erbsland/conf/impl/vr/MultipleIntegerConstraint.hpp>
#include <erbsland/conf/impl/vr/MultipleMatrixConstraint.hpp>
#include <erbsland/conf/impl/vr/Rule.hpp>
#include <erbsland/conf/impl/vr/Rules.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/conf/vr/RuleType.hpp>
#include <erbsland/text/StringList.hpp>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(RulesBuilder RuleDefinition)
TAGS(ValidationRules)
class VrBuilderApiTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    [[nodiscard]] static auto makeRule(const vr::RuleType type = vr::RuleType::Text) -> el::conf::impl::Rule {
        auto rule = el::conf::impl::Rule{};
        rule.setRuleNamePath(NamePath::fromText("app.value"_el));
        rule.setTargetNamePath(NamePath::fromText("app.value"_el));
        rule.setType(type);
        return rule;
    }

    [[nodiscard]] auto takeRulesImpl(vr::RulesBuilder &builder) -> std::shared_ptr<el::conf::impl::Rules> {
        auto rules = builder.takeRules();
        REQUIRE(rules);
        auto rulesImpl = std::dynamic_pointer_cast<el::conf::impl::Rules>(rules);
        REQUIRE(rulesImpl);
        return rulesImpl;
    }

    void testAddRuleSupportsAllNamePathLikeFormsAndErrors() {
        vr::RulesBuilder builder;

        REQUIRE_NOTHROW(builder.addRule(NamePathLike{el::text::String{"app"_el}}, vr::RuleType::Section));
        REQUIRE_NOTHROW(builder.addRule(NamePathLike{NamePath::fromText("app.int_value"_el)}, vr::RuleType::Integer));
        REQUIRE_NOTHROW(builder.addRule(NamePathLike{Name::createRegular("root_text"_el)}, vr::RuleType::Text));
        REQUIRE_NOTHROW(builder.takeRules());

        REQUIRE_THROWS_AS(el::conf::ConfError, builder.addRule(NamePathLike{std::size_t{2}}, vr::RuleType::Integer));
        REQUIRE_THROWS_AS(el::conf::ConfError, builder.addRule(NamePathLike{NamePath{}}, vr::RuleType::Integer));
        REQUIRE_THROWS_AS(
            el::conf::ConfError, builder.addRule(NamePathLike{el::text::String{"app[0]"_el}}, vr::RuleType::Integer));
        REQUIRE_THROWS_AS(
            el::conf::ConfError,
            builder.addRule(NamePathLike{el::text::String{"app.\"key\""_el}}, vr::RuleType::Integer));
        REQUIRE_THROWS_AS(
            el::conf::ConfError,
            builder.addRule(NamePathLike{el::text::String{"unknown.parent.child"_el}}, vr::RuleType::Integer));
    }

    void testAddAlternativeBranches() {
        vr::RulesBuilder builder;

        REQUIRE_NOTHROW(builder.addRule("app"_el, vr::RuleType::Section));
        REQUIRE_NOTHROW(builder.addAlternative("app.variant"_el, vr::RuleType::Integer));
        REQUIRE_NOTHROW(builder.addAlternative("app.variant"_el, vr::RuleType::Text));

        auto rulesImpl = takeRulesImpl(builder);
        const auto alternativeRule = rulesImpl->ruleForNamePath(NamePath::fromText("app.variant"_el));
        REQUIRE(alternativeRule);
        REQUIRE_EQUAL(alternativeRule->type(), vr::RuleType::Alternatives);
        REQUIRE_EQUAL(alternativeRule->childrenImpl().size(), 2U);

        const auto firstAlternative = alternativeRule->childrenImpl().rule(Name::createIndex(0));
        const auto secondAlternative = alternativeRule->childrenImpl().rule(Name::createIndex(1));
        REQUIRE(firstAlternative);
        REQUIRE(secondAlternative);
        REQUIRE_EQUAL(firstAlternative->type(), vr::RuleType::Integer);
        REQUIRE_EQUAL(secondAlternative->type(), vr::RuleType::Text);

        builder.reset();
        REQUIRE_NOTHROW(builder.addRule("app"_el, vr::RuleType::Section));
        REQUIRE_NOTHROW(builder.addRule("app.variant"_el, vr::RuleType::Integer));
        REQUIRE_THROWS_AS(el::conf::ConfError, builder.addAlternative("app.variant"_el, vr::RuleType::Text));
        REQUIRE_THROWS_AS(el::conf::ConfError, builder.addAlternative("missing.parent.variant"_el, vr::RuleType::Text));
    }

    void testAddMethodsValidateRuleTypeAndConstraintAlignment() {
        vr::RulesBuilder builder;

        REQUIRE_THROWS_AS(el::conf::ConfError, builder.addRule("app"_el, vr::RuleType::Undefined));
        REQUIRE_NOTHROW(builder.addRule("app"_el, vr::RuleType::Section));

        REQUIRE_THROWS_AS(
            el::conf::ConfError,
            builder.addRule("app.port"_el, vr::RuleType::Integer, vr::builder::Default("text"_el)));
        REQUIRE_THROWS_AS(
            el::conf::ConfError,
            builder.addRule("app.port"_el, vr::RuleType::Integer, vr::builder::Minimum(makeDate(2026, 1, 1))));
        REQUIRE_THROWS_AS(
            el::conf::ConfError,
            builder.addAlternative("app.kind"_el, vr::RuleType::Text, vr::builder::In(std::vector<Integer>{1, 2})));

        REQUIRE_NOTHROW(builder.addRule("app.port"_el, vr::RuleType::Integer, vr::builder::Default(Integer{443})));
        REQUIRE_NOTHROW(
            builder.addAlternative("app.kind"_el, vr::RuleType::Text, vr::builder::In({"dev"_el, "prod"_el})));
    }

    void testSimpleAttributes() {
        auto rule = makeRule();
        vr::builder::Title("My Title"_el)(rule);
        REQUIRE_EQUAL(rule.title(), "My Title"_el);

        vr::builder::Description("Description"_el)(rule);
        REQUIRE_EQUAL(rule.description(), "Description"_el);

        vr::builder::CustomError("Rule error"_el)(rule);
        REQUIRE(rule.hasCustomError());
        REQUIRE_EQUAL(rule.customError(), "Rule error"_el);

        vr::builder::IsOptional()(rule);
        REQUIRE(rule.isOptional());
        vr::builder::IsOptional(false)(rule);
        REQUIRE_FALSE(rule.isOptional());

        vr::builder::IsSecret()(rule);
        REQUIRE(rule.isSecret());
        vr::builder::IsSecret(false)(rule);
        REQUIRE_FALSE(rule.isSecret());

        auto defaultCaseSensitive = vr::builder::CaseSensitive{};
        defaultCaseSensitive(rule);
        REQUIRE_EQUAL(rule.caseSensitivity(), CaseSensitivity::CaseSensitive);
        auto caseInsensitive = vr::builder::CaseSensitive{CaseSensitivity::CaseInsensitive};
        caseInsensitive(rule);
        REQUIRE_EQUAL(rule.caseSensitivity(), CaseSensitivity::CaseInsensitive);
    }

    void testDefaultAttributeConstructors() {
        REQUIRE_THROWS(vr::builder::Default(el::conf::ValuePtr{}));
        auto defaultFromValue = vr::builder::Default(el::conf::impl::Value::createInteger(7));
        REQUIRE(defaultFromValue.value());
        REQUIRE_EQUAL(defaultFromValue.value()->type(), ValueType::Integer);

        REQUIRE_EQUAL(vr::builder::Default(Integer{7}).value()->type(), ValueType::Integer);
        REQUIRE_EQUAL(vr::builder::Default(true).value()->type(), ValueType::Boolean);
        REQUIRE_EQUAL(vr::builder::Default(Float{1.5}).value()->type(), ValueType::Float);
        REQUIRE_EQUAL(vr::builder::Default(el::text::String{"text"_el}).value()->type(), ValueType::Text);
        REQUIRE_EQUAL(vr::builder::Default("text"_el).value()->type(), ValueType::Text);
        REQUIRE_EQUAL(vr::builder::Default(makeDate(2026, 1, 1)).value()->type(), ValueType::Date);
        REQUIRE_EQUAL(vr::builder::Default(makeTime(12, 0, 0, 0)).value()->type(), ValueType::Time);
        REQUIRE_EQUAL(
            vr::builder::Default(el::time::DateTime{makeDate(2026, 1, 1), makeTime(12, 0, 0, 0)}).value()->type(),
            ValueType::DateTime);
        REQUIRE_EQUAL(vr::builder::Default(bytesFromHex("DE AD"_el)).value()->type(), ValueType::Bytes);
        REQUIRE_EQUAL(
            vr::builder::Default(el::time::CalendarDelta{el::time::Hours{2}}).value()->type(), ValueType::TimeDelta);
        REQUIRE_EQUAL(vr::builder::Default(el::re::RegEx::compile("a.*"_el)).value()->type(), ValueType::RegEx);
        REQUIRE_THROWS(vr::builder::Default(el::re::RegExPtr{}));

        REQUIRE_EQUAL(vr::builder::Default(std::vector<Integer>{1, 2}).value()->type(), ValueType::ValueList);
        REQUIRE_EQUAL(vr::builder::Default(std::vector<bool>{true, false}).value()->type(), ValueType::ValueList);
        REQUIRE_EQUAL(vr::builder::Default(std::vector<Float>{1.0, 2.0}).value()->type(), ValueType::ValueList);
        REQUIRE_EQUAL(vr::builder::Default(el::text::StringList{"a"_el, "b"_el}).value()->type(), ValueType::ValueList);
        REQUIRE_EQUAL(
            vr::builder::Default(std::vector<el::mem::ByteBlock>{bytesFromHex("AA"_el), bytesFromHex("BB"_el)})
                .value()
                ->type(),
            ValueType::ValueList);
        REQUIRE_EQUAL(
            vr::builder::Default(std::vector<std::vector<Integer>>{{1, 2}, {3, 4}}).value()->type(),
            ValueType::ValueList);
        REQUIRE_EQUAL(
            vr::builder::Default(std::vector<std::vector<Float>>{{1.0, 2.0}}).value()->type(), ValueType::ValueList);

        auto rule = makeRule(vr::RuleType::Integer);
        vr::builder::Default(Integer{42})(rule);
        REQUIRE(rule.hasDefault());
        REQUIRE(rule.defaultValue());
        REQUIRE_EQUAL(rule.defaultValue()->type(), ValueType::Integer);
    }

    void testKeyIndexConstructors() {
        auto rule = makeRule(vr::RuleType::Section);

        vr::builder::KeyIndex(std::vector<NamePathLike>{el::text::String{"users.vr_entry.id"_el}})(rule);
        vr::builder::KeyIndex(
            Name::createRegular("ids"_el), std::vector<NamePathLike>{el::text::String{"users.vr_entry.id"_el}})(rule);
        vr::builder::KeyIndex(
            el::text::String{"names"_el}, std::vector<NamePathLike>{el::text::String{"users.vr_entry.name"_el}})(rule);
        vr::builder::KeyIndex(NamePathLike{el::text::String{"users.vr_entry.id"_el}})(rule);
        vr::builder::KeyIndex(
            Name::createRegular("one"_el),
            NamePathLike{el::text::String{"users.vr_entry.id"_el}},
            CaseSensitivity::CaseSensitive)(rule);
        vr::builder::KeyIndex(
            el::text::String{"two"_el},
            NamePathLike{el::text::String{"users.vr_entry.id"_el}},
            CaseSensitivity::CaseInsensitive)(rule);
        vr::builder::KeyIndex({el::text::String{"users.vr_entry.id"_el}, el::text::String{"users.vr_entry.name"_el}})(
            rule);
        vr::builder::KeyIndex(Name::createRegular("three"_el), {el::text::String{"users.vr_entry.id"_el}})(rule);
        vr::builder::KeyIndex(
            el::text::String{"four"_el}, {el::text::String{"users.vr_entry.id"_el}}, CaseSensitivity::CaseSensitive)(
            rule);

        REQUIRE(rule.hasKeyDefinitions());
        REQUIRE_EQUAL(rule.keyDefinitions().size(), 9U);
        REQUIRE_EQUAL(rule.keyDefinitions().back()->caseSensitivity(), CaseSensitivity::CaseSensitive);
    }

    void testDependencyConstructors() {
        auto rule = makeRule(vr::RuleType::Section);

        vr::builder::Dependency(
            el::conf::vr::DependencyMode::If,
            std::vector<NamePathLike>{el::text::String{"a"_el}},
            std::vector<NamePathLike>{el::text::String{"b"_el}},
            "dep"_el)(rule);
        vr::builder::Dependency(
            el::conf::vr::DependencyMode::XOR, {el::text::String{"x"_el}}, {el::text::String{"y"_el}})(rule);

        REQUIRE(rule.hasDependencyDefinitions());
        REQUIRE_EQUAL(rule.dependencyDefinitions().size(), 2U);
        REQUIRE_EQUAL(rule.dependencyDefinitions().front()->mode(), el::conf::vr::DependencyMode::If);
        REQUIRE(rule.dependencyDefinitions().front()->hasErrorMessage());
        REQUIRE_EQUAL(rule.dependencyDefinitions().back()->mode(), el::conf::vr::DependencyMode::XOR);
    }

    void testVersionAttributesAndBranches() {
        auto rule = makeRule(vr::RuleType::Integer);

        vr::builder::ConfVersion(std::vector<Integer>{1, 1, 3})(rule);
        REQUIRE(rule.versionMask().matches(1));
        REQUIRE(rule.versionMask().matches(3));

        vr::builder::ConfVersion(Integer{9}, true)(rule);
        REQUIRE_FALSE(rule.versionMask().matches(9));

        auto initializerRule = makeRule(vr::RuleType::Integer);
        vr::builder::ConfVersion({2, 4})(initializerRule);
        REQUIRE_FALSE(initializerRule.versionMask().matches(1));
        REQUIRE(initializerRule.versionMask().matches(2));
        REQUIRE(initializerRule.versionMask().matches(4));

        vr::builder::MinimumVersion(2)(rule);
        REQUIRE(rule.versionMask().matches(3));
        REQUIRE_FALSE(rule.versionMask().matches(1));

        vr::builder::MaximumVersion(10)(rule);
        REQUIRE(rule.versionMask().matches(3));
        REQUIRE_FALSE(rule.versionMask().matches(50));

        auto negatedRule = makeRule(vr::RuleType::Integer);
        vr::builder::MinimumVersion(5, true)(negatedRule);
        REQUIRE(negatedRule.versionMask().matches(2));
        REQUIRE_FALSE(negatedRule.versionMask().matches(5));

        auto negatedMaxRule = makeRule(vr::RuleType::Integer);
        vr::builder::MaximumVersion(7, true)(negatedMaxRule);
        REQUIRE_FALSE(negatedMaxRule.versionMask().matches(6));
        REQUIRE(negatedMaxRule.versionMask().matches(9));

        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::ConfVersion(std::vector<Integer>{})(rule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::ConfVersion(std::vector<Integer>{1, -1})(rule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::MinimumVersion(-1)(rule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::MaximumVersion(-1)(rule));
    }

    void testCharsConstraintConstructorsAndOptions() {
        auto rule = makeRule(vr::RuleType::Text);

        vr::builder::Chars(el::text::StringList{"[ab]"_el})(rule);
        vr::builder::Chars(el::text::String{"[cd]"_el})(rule);
        vr::builder::Chars("[ef]"_el)(rule);
        vr::builder::Chars({el::text::String{"[ij]"_el}, el::text::String{"[kl]"_el}})(rule);
        vr::builder::Chars({"[mn]"_el, "[op]"_el}, {.isNegated = true, .errorMessage = "chars error"_el})(rule);

        const auto constraint = rule.constraint("not_chars"_el);
        REQUIRE(constraint);
        REQUIRE_EQUAL(constraint->type(), vr::ConstraintType::Chars);
        REQUIRE(constraint->isNegated());
        REQUIRE(constraint->hasCustomError());

        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Chars(el::text::StringList{})(rule));

        auto integerRule = makeRule(vr::RuleType::Integer);
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Chars("[ab]"_el)(integerRule));
    }

    void testStringPartConstraintConstructorsAndOptions() {
        auto rule = makeRule(vr::RuleType::Text);

        vr::builder::Starts(el::text::StringList{"a"_el})(rule);
        vr::builder::Starts(el::text::String{"b"_el})(rule);
        vr::builder::Starts("c"_el)(rule);
        vr::builder::Starts({el::text::String{"e"_el}})(rule);
        vr::builder::Starts({"f"_el, "g"_el}, {.isNegated = true, .errorMessage = "starts error"_el})(rule);

        vr::builder::Ends(el::text::StringList{"a"_el})(rule);
        vr::builder::Ends(el::text::String{"b"_el})(rule);
        vr::builder::Ends("c"_el)(rule);
        vr::builder::Ends({el::text::String{"e"_el}})(rule);
        vr::builder::Ends({"f"_el, "g"_el}, {.isNegated = true, .errorMessage = "ends error"_el})(rule);

        vr::builder::Contains(el::text::StringList{"a"_el})(rule);
        vr::builder::Contains(el::text::String{"b"_el})(rule);
        vr::builder::Contains("c"_el)(rule);
        vr::builder::Contains({el::text::String{"e"_el}})(rule);
        vr::builder::Contains({"f"_el, "g"_el}, {.isNegated = true, .errorMessage = "contains error"_el})(rule);

        REQUIRE(rule.constraint("not_starts"_el));
        REQUIRE(rule.constraint("not_ends"_el));
        REQUIRE(rule.constraint("not_contains"_el));

        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Starts(el::text::StringList{})(rule));

        auto integerRule = makeRule(vr::RuleType::Integer);
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Ends("x"_el)(integerRule));
    }

    void testEqualsConstraintConstructorsAndBranches() {
        auto textRule = makeRule(vr::RuleType::Text);
        vr::builder::Equals(Integer{5})(textRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::EqualsIntegerConstraint>(textRule.constraint("equals"_el)));
        vr::builder::Equals(el::text::String{"abc"_el})(textRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::EqualsTextConstraint>(textRule.constraint("equals"_el)));
        vr::builder::Equals("def"_el)(textRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::EqualsTextConstraint>(textRule.constraint("equals"_el)));
        auto booleanRule = makeRule(vr::RuleType::Boolean);
        vr::builder::Equals(true)(booleanRule);
        REQUIRE_NOT_EQUAL(
            std::dynamic_pointer_cast<el::conf::impl::EqualsBooleanConstraint>(booleanRule.constraint("equals"_el)),
            nullptr);

        auto floatRule = makeRule(vr::RuleType::Float);
        vr::builder::Equals(Float{1.25})(floatRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::EqualsFloatConstraint>(floatRule.constraint("equals"_el)));

        auto bytesRule = makeRule(vr::RuleType::Bytes);
        vr::builder::Equals(bytesFromHex("AA"_el))(bytesRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::EqualsBytesConstraint>(bytesRule.constraint("equals"_el)));
        vr::builder::Equals(Integer{7})(bytesRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::EqualsIntegerConstraint>(bytesRule.constraint("equals"_el)));

        auto matrixRule = makeRule(vr::RuleType::ValueMatrix);
        vr::builder::Equals(std::pair<Integer, Integer>{2, 3})(matrixRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::EqualsMatrixConstraint>(matrixRule.constraint("equals"_el)));

        vr::builder::Equals(Integer{4}, Integer{5}, {.isNegated = true, .errorMessage = "eq error"_el})(matrixRule);
        const auto negated = matrixRule.constraint("not_equals"_el);
        REQUIRE(negated);
        REQUIRE(negated->isNegated());
        REQUIRE(negated->hasCustomError());

        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Equals(true)(textRule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Equals(Float{1.0})(textRule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Equals(bytesFromHex("AA"_el))(textRule));
    }

    void testInConstraintConstructorsAndBranches() {
        auto integerRule = makeRule(vr::RuleType::Integer);
        vr::builder::In(std::vector<Integer>{1, 2})(integerRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::InIntegerConstraint>(integerRule.constraint("in"_el)));
        vr::builder::In(std::initializer_list<Integer>{1, 2})(integerRule);
        vr::builder::In(Integer{5})(integerRule);

        auto floatRule = makeRule(vr::RuleType::Float);
        vr::builder::In(std::vector<Float>{1.0, 2.0})(floatRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::InFloatConstraint>(floatRule.constraint("in"_el)));
        vr::builder::In({1.0, 2.0})(floatRule);
        vr::builder::In(Float{9.0})(floatRule);

        auto textRule = makeRule(vr::RuleType::Text);
        vr::builder::In(el::text::StringList{"a"_el, "b"_el})(textRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::InTextConstraint>(textRule.constraint("in"_el)));
        vr::builder::In({el::text::String{"x"_el}, el::text::String{"y"_el}})(textRule);
        vr::builder::In({"a"_el, "b"_el})(textRule);
        vr::builder::In(el::text::String{"x"_el})(textRule);
        vr::builder::In("x"_el)(textRule);

        auto bytesRule = makeRule(vr::RuleType::Bytes);
        vr::builder::In(std::vector<el::mem::ByteBlock>{bytesFromHex("AA"_el)})(bytesRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::InBytesConstraint>(bytesRule.constraint("in"_el)));
        vr::builder::In({bytesFromHex("01"_el), bytesFromHex("02"_el)})(bytesRule);
        vr::builder::In(bytesFromHex("FF"_el), {.isNegated = true, .errorMessage = "in error"_el})(bytesRule);

        const auto negated = bytesRule.constraint("not_in"_el);
        REQUIRE(negated);
        REQUIRE(negated->isNegated());
        REQUIRE(negated->hasCustomError());

        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::In(std::vector<Integer>{})(integerRule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::In(std::vector<Integer>{1, 1})(integerRule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::In(std::vector<Integer>{1, 2})(textRule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::In(el::text::StringList{"A"_el, "a"_el})(textRule));
    }

    void testKeyConstraintConstructorsAndOptions() {
        auto rule = makeRule(vr::RuleType::Integer);

        vr::builder::ConfKey(NamePathLike{el::text::String{"ids"_el}})(rule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::KeyConstraint>(rule.constraint("key"_el)));

        vr::builder::ConfKey(std::vector<NamePathLike>{el::text::String{"ids"_el}, el::text::String{"other"_el}})(rule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::KeyConstraint>(rule.constraint("key"_el)));

        vr::builder::ConfKey({el::text::String{"ids"_el}}, {.isNegated = true, .errorMessage = "key error"_el})(rule);
        const auto negated = rule.constraint("not_key"_el);
        REQUIRE(negated);
        REQUIRE(negated->isNegated());
        REQUIRE(negated->hasCustomError());

        auto floatRule = makeRule(vr::RuleType::Float);
        REQUIRE_THROWS_AS(
            el::conf::ConfError, vr::builder::ConfKey(NamePathLike{el::text::String{"ids"_el}})(floatRule));
    }

    void testMatchesConstraintConstructorsAndOptions() {
        auto rule = makeRule(vr::RuleType::Text);

        vr::builder::Matches(el::text::String{"^[a-z]+$"_el})(rule);
        REQUIRE(rule.constraint("matches"_el));

        vr::builder::Matches("^[0-9]+$"_el, true)(rule);
        REQUIRE(rule.constraint("matches"_el));

        vr::builder::Matches(
            el::re::RegEx::compile("^a+$"_el, el::re::Flags{el::re::Flag::Verbose}),
            {.isNegated = true, .errorMessage = "match error"_el})(rule);
        const auto negated = rule.constraint("not_matches"_el);
        REQUIRE(negated);
        REQUIRE(negated->isNegated());
        REQUIRE(negated->hasCustomError());

        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Matches(el::text::String{})(rule));
        REQUIRE_THROWS(vr::builder::Matches(el::re::RegExPtr{}));

        auto integerRule = makeRule(vr::RuleType::Integer);
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Matches("^[0-9]+$"_el)(integerRule));
    }

    void testMinimumAndMaximumConstraintConstructorsAndBranches() {
        auto integerRule = makeRule(vr::RuleType::Integer);
        vr::builder::Minimum(Integer{1})(integerRule);
        REQUIRE_NOT_EQUAL(
            std::dynamic_pointer_cast<el::conf::impl::MinMaxIntegerConstraint>(integerRule.constraint("minimum"_el)),
            nullptr);
        vr::builder::Maximum(Integer{1})(integerRule);
        REQUIRE_NOT_EQUAL(
            std::dynamic_pointer_cast<el::conf::impl::MinMaxIntegerConstraint>(integerRule.constraint("maximum"_el)),
            nullptr);

        auto floatRule = makeRule(vr::RuleType::Float);
        vr::builder::Minimum(Float{1.5})(floatRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::MinMaxFloatConstraint>(floatRule.constraint("minimum"_el)));
        vr::builder::Maximum(Float{1.5})(floatRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::MinMaxFloatConstraint>(floatRule.constraint("maximum"_el)));

        auto dateRule = makeRule(vr::RuleType::Date);
        vr::builder::Minimum(makeDate(2026, 1, 1))(dateRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::MinMaxDateConstraint>(dateRule.constraint("minimum"_el)));
        vr::builder::Maximum(makeDate(2026, 1, 1))(dateRule);
        REQUIRE(std::dynamic_pointer_cast<el::conf::impl::MinMaxDateConstraint>(dateRule.constraint("maximum"_el)));

        auto dateTimeRule = makeRule(vr::RuleType::DateTime);
        vr::builder::Minimum(el::time::DateTime{makeDate(2026, 1, 1), makeTime(12, 0, 0, 0)})(dateTimeRule);
        REQUIRE_NOT_EQUAL(
            std::dynamic_pointer_cast<el::conf::impl::MinMaxDateTimeConstraint>(dateTimeRule.constraint("minimum"_el)),
            nullptr);
        vr::builder::Maximum(el::time::DateTime{makeDate(2026, 1, 1), makeTime(12, 0, 0, 0)})(dateTimeRule);
        REQUIRE_NOT_EQUAL(
            std::dynamic_pointer_cast<el::conf::impl::MinMaxDateTimeConstraint>(dateTimeRule.constraint("maximum"_el)),
            nullptr);

        auto matrixRule = makeRule(vr::RuleType::ValueMatrix);
        vr::builder::Minimum(std::pair<Integer, Integer>{2, 3})(matrixRule);
        REQUIRE_NOT_EQUAL(
            std::dynamic_pointer_cast<el::conf::impl::MinMaxMatrixConstraint>(matrixRule.constraint("minimum"_el)),
            nullptr);
        vr::builder::Maximum(std::pair<Integer, Integer>{2, 3})(matrixRule);
        REQUIRE_NOT_EQUAL(
            std::dynamic_pointer_cast<el::conf::impl::MinMaxMatrixConstraint>(matrixRule.constraint("maximum"_el)),
            nullptr);

        vr::builder::Minimum(Integer{2}, Integer{4}, {.isNegated = true, .errorMessage = "min error"_el})(matrixRule);
        const auto negMin = matrixRule.constraint("not_minimum"_el);
        REQUIRE(negMin);
        REQUIRE(negMin->isNegated());
        REQUIRE(negMin->hasCustomError());

        vr::builder::Maximum(Integer{3}, Integer{5}, {.isNegated = true, .errorMessage = "max error"_el})(matrixRule);
        const auto negMax = matrixRule.constraint("not_maximum"_el);
        REQUIRE(negMax);
        REQUIRE(negMax->isNegated());
        REQUIRE(negMax->hasCustomError());

        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Minimum(Float{1.5})(integerRule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Minimum(makeDate(2026, 1, 1))(integerRule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Maximum(Integer{1})(matrixRule));
    }

    void testMultipleConstraintConstructorsAndBranches() {
        auto integerRule = makeRule(vr::RuleType::Integer);
        vr::builder::Multiple(Integer{2})(integerRule);
        REQUIRE_NOT_EQUAL(
            std::dynamic_pointer_cast<el::conf::impl::MultipleIntegerConstraint>(integerRule.constraint("multiple"_el)),
            nullptr);

        auto floatRule = makeRule(vr::RuleType::Float);
        vr::builder::Multiple(Float{0.5})(floatRule);
        REQUIRE_NOT_EQUAL(
            std::dynamic_pointer_cast<el::conf::impl::MultipleFloatConstraint>(floatRule.constraint("multiple"_el)),
            nullptr);

        auto matrixRule = makeRule(vr::RuleType::ValueMatrix);
        vr::builder::Multiple(std::pair<Integer, Integer>{2, 3})(matrixRule);
        REQUIRE_NOT_EQUAL(
            std::dynamic_pointer_cast<el::conf::impl::MultipleMatrixConstraint>(matrixRule.constraint("multiple"_el)),
            nullptr);

        vr::builder::Multiple(Integer{4}, Integer{5}, {.isNegated = true, .errorMessage = "mul error"_el})(matrixRule);
        const auto negated = matrixRule.constraint("not_multiple"_el);
        REQUIRE(negated);
        REQUIRE(negated->isNegated());
        REQUIRE(negated->hasCustomError());

        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Multiple(Integer{0})(integerRule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Multiple(Float{0.0})(floatRule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Multiple(Integer{1}, Integer{0})(matrixRule));
        REQUIRE_THROWS_AS(el::conf::ConfError, vr::builder::Multiple(Float{1.0})(integerRule));
    }
};
