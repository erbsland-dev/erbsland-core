..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration; Validated Values
    single: Validation; Metadata
    single: Validation; Secret Values
    single: Validation; Default Values

*****************************
Using Values after Validation
*****************************

Validation does more than accept or reject a document.
It connects each checked value to the rule that accepted it, marks values inserted from defaults, and identifies
settings that need secret-aware handling.
This metadata is useful when you build configuration summaries, editors, diagnostics, or persistence tools around the
same schema that protects application startup.

The examples on this page assume that a document has already been validated as described in
:doc:`validation-rules`.
Always test the metadata on the :cpp:class:`Value <erbsland::conf::Value>` itself; extracting a native value such as an
integer or string intentionally leaves the configuration-tree context behind.

Use Rule Metadata in User Interfaces
====================================

After successful validation, :cpp:func:`Value::validationRule() <erbsland::conf::Value::validationRule>` returns the
immutable :cpp:class:`Rule <erbsland::conf::vr::Rule>` that matched the value.
The rule can carry a title and description written by the schema author.
These labels are often better for a configuration editor or diagnostic than a normalized name path such as
``survey.forest``.

.. erbsland-demo::
    :source: conf/ValidatedDocuments/RuleMetadata.cpp
    :exec: conf/validated_documents --demo RuleMetadata
    :source-sha256: 2feb16e3032181d34d4316c1cae3dcb9a3e26c5a53d0bbedaacd86e31f8e755d

.. code-block:: cpp

    /// Read presentation metadata from the rule that validated a value.
    ///
    /// Successful validation attaches the matching immutable rule to every validated value. Its title and description can
    /// give configuration editors and diagnostic tools useful, application-owned labels without duplicating schema text.
    void ruleMetadata() {
        using el::conf::vr::RuleType;
        using el::conf::vr::builder::Description;
        using el::conf::vr::builder::Title;

        auto builder = el::conf::vr::RulesBuilder{};
        builder.addRule("survey"_el, RuleType::Section);
        builder.addRule(
            "survey.forest"_el,
            RuleType::Text,
            Title{"Floresta observada"_el},
            Description{"Nome da área florestal incluída no levantamento."_el});
        const auto rules = builder.takeRules();

        const auto document = el::conf::Parser{}.parseTextOrThrow(
            "[survey]\n"
            "forest: \"Parque Nacional da Tijuca\"\n"_el);
        rules->validate(document, 1);

        // The attached rule is available after validation and can supply display metadata.
        const auto forest = document->valueOrThrow("survey.forest"_el);
        const auto rule = forest->validationRule();
        el::io::printLine("Title: "_el, rule->title());
        el::io::printLine("Description: "_el, rule->description());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Title: Floresta observada
    Description: Nome da área florestal incluída no levantamento.

.. erbsland-demo-end::

The pointer is null when the value was not validated.
If code may receive either a validated or an unvalidated document, check
:cpp:func:`Value::wasValidated() <erbsland::conf::Value::wasValidated>` before dereferencing the rule.
Code that owns the complete loading workflow can instead treat successful validation as a precondition for reading rule
metadata.

Titles and descriptions are optional and may be empty.
Use the value's name or name path as a fallback rather than requiring every rule to contain presentation text.
Other rule properties can support richer tools, but application behavior should still read the validated value rather
than trying to reproduce the validator from its constraints.

Handle Secret Values Deliberately
=================================

A rule marked with ``is_secret`` causes :cpp:func:`Value::isSecret() <erbsland::conf::Value::isSecret>` to return
``true`` on the validated value.
This marker lets generic code omit the value from a configuration dump, replace it with a fixed placeholder, or route it
into secret-aware storage without knowing every sensitive name path in advance.

The marker is a handling policy, not redaction or access control.
Calling ``asTextOrThrow()`` still returns the real text, and passing that text to an ordinary log or output stream can
still disclose it.
When extracted text must remain in memory, mark its shared storage as sensitive before retaining it.

.. erbsland-demo::
    :source: conf/ValidatedDocuments/SecretValues.cpp
    :exec: conf/validated_documents --demo SecretValues
    :source-sha256: 4e57296038cb6a1320f62e1228b97b45b345a54d4dfc007d5574ce10fd135df0

.. code-block:: cpp

    /// Apply the secret marker from a validation rule when configuration text leaves the document.
    ///
    /// `isSecret()` communicates handling policy; it does not hide the value or make logging safe by itself. Mark an
    /// extracted string as sensitive before retaining it so its shared storage receives the library's guarded cleanup
    /// behavior.
    void secretValues() {
        using el::conf::vr::RuleType;
        using el::conf::vr::builder::IsSecret;

        auto builder = el::conf::vr::RulesBuilder{};
        builder.addRule("survey"_el, RuleType::Section);
        builder.addRule("survey.api_token"_el, RuleType::Text, IsSecret{});
        const auto rules = builder.takeRules();

        const auto document = el::conf::Parser{}.parseTextOrThrow(
            "[survey]\n"
            "api_token: \"bosque-7a91\"\n"_el);
        rules->validate(document, 1);

        const auto tokenValue = document->valueOrThrow("survey.api_token"_el);
        auto token = tokenValue->asTextOrThrow();
        if (tokenValue->isSecret()) {
            token.markAsSensitive();
        }

        el::io::printLine("Rule marks value as secret: "_el, el::BooleanFormat::yesNo(), tokenValue->isSecret());
        el::io::printLine("Extracted text is sensitive: "_el, el::BooleanFormat::yesNo(), token.isSensitive());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Rule marks value as secret: yes
    Extracted text is sensitive: yes

.. erbsland-demo-end::

The sensitive mark adds guarded cleanup behavior to the string's shared allocation and propagates through supported
string operations.
It does not hide text from a debugger, prevent an explicit print, or replace application-level secret management.
See :doc:`/topics/security/about_sensitive_strings_and_byte_blocks` for the exact guarantees and limitations.

For configuration summaries, make the decision while you still have the ``Value`` object: write a fixed marker such as
``<secret>`` when ``isSecret()`` is true, and only convert ordinary values to their text representation.
This avoids creating an unnecessary plaintext copy merely to decide whether it should be displayed.

Recognize Values Supplied by Defaults
=====================================

Validation can insert a value declared by a rule when the input omits that path.
:cpp:func:`Value::isDefaultValue() <erbsland::conf::Value::isDefaultValue>` distinguishes such a value from one that
the user supplied explicitly.
Both values have the same type and are read through the same getters, so normal application logic rarely needs to care
where they came from.

The distinction becomes useful at a presentation or persistence boundary.
A configuration dump can label defaults so operators know which settings remain implicit.
A writer can omit them to keep a generated file concise, while an audit view may include them to show the complete
effective configuration.
Special migration logic can also distinguish an absent setting that received today's default from an explicit choice
that must be preserved.

The same example also shows :cpp:func:`Value::wasValidated() <erbsland::conf::Value::wasValidated>` before and after a
rule set is applied, and inspects a branch accepted with
:cpp:enumerator:`RuleType::NotValidated <erbsland::conf::vr::RuleType::NotValidated>`.

.. erbsland-demo::
    :source: conf/ValidatedDocuments/DefaultsAndValidation.cpp
    :exec: conf/validated_documents --demo DefaultsAndValidation
    :source-sha256: fa596a5ab11fc489dc2fdce3e6f51f0def20344408870e3b59324faea0a27aed

.. code-block:: cpp

    /// Distinguish supplied values, rule defaults, and deliberately unvalidated branches.
    ///
    /// `isDefaultValue()` identifies values inserted by validation. `wasValidated()` distinguishes a parsed value from one
    /// that has passed through a rule set; a `NotValidated` rule is still attached as an explicit schema decision.
    void defaultsAndValidation() {
        using el::conf::vr::RuleType;
        using el::conf::vr::builder::Default;

        auto builder = el::conf::vr::RulesBuilder{};
        builder.addRule("survey"_el, RuleType::Section);
        builder.addRule("survey.forest"_el, RuleType::Text);
        builder.addRule("survey.region"_el, RuleType::Text, Default{"Mata Atlântica"_el});
        builder.addRule("survey.extension"_el, RuleType::NotValidated);
        const auto rules = builder.takeRules();

        const auto configuration = "[survey]\n"
                                   "forest: \"Floresta Nacional de Ipanema\"\n"
                                   "extension: \"vendor-owned value\"\n"_el;
        const auto uncheckedDocument = el::conf::Parser{}.parseTextOrThrow(configuration);
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
        rules->validate(document, 1);

        const auto forest = document->valueOrThrow("survey.forest"_el);
        const auto region = document->valueOrThrow("survey.region"_el);
        const auto extension = document->valueOrThrow("survey.extension"_el);
        const auto yesNo = el::BooleanFormat::yesNo();

        el::io::printLine(
            "Forest validated before rules: "_el,
            yesNo,
            uncheckedDocument->valueOrThrow("survey.forest"_el)->wasValidated());
        el::io::printLine("Forest validated after rules: "_el, yesNo, forest->wasValidated());
        el::io::printLine("Region is a default: "_el, yesNo, region->isDefaultValue());
        el::io::printLine("Extension rule: "_el, extension->validationRule()->type().toText());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Forest validated before rules: no
    Forest validated after rules: yes
    Region is a default: yes
    Extension rule: NotValidated

.. erbsland-demo-end::

``isDefaultValue()`` only describes insertion by validation; ``false`` does not mean that a value was validated.
Use ``wasValidated()`` for that separate question.
A ``NotValidated`` branch still receives its ``NotValidated`` rule, so ``wasValidated()`` returns ``true`` there: the
schema explicitly accepted the branch without checking its type or content.
Inspect the attached rule's type when tooling needs to distinguish that decision from ordinary validation.

Keep Metadata with the Tree
===========================

Validation metadata belongs to the configuration tree.
If an application copies the final settings into its own structure and releases the document, copy any metadata the
application still needs at the same time.
For example, a settings loader can copy a secret string into sensitive storage and omit the source value entirely, while
a configuration editor may retain the document because it needs titles, descriptions, default markers, and locations.

This separation keeps ordinary runtime settings small without losing the richer information needed by diagnostics and
configuration tooling.
