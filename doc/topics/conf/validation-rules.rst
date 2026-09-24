..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration; Validation Rules
    single: Validation Rules; Embedded Documents
    single: Validation Rules; C++ Builder

**************************************
Validating Documents with Rule Schemas
**************************************

Typed getters can verify one setting at a time.
Validation rules describe the contract for an entire configuration tree: which paths may appear, which types they hold,
which values are optional or secret, and which constraints each value must satisfy.
Validation can also insert declared defaults and attach rule metadata for later diagnostics or presentation.

Erbsland Core supports the same validation model from two sources.
You can write a readable ELCL rules document and compile it into the application, or construct the rules directly in C++
with :cpp:class:`RulesBuilder <erbsland::conf::vr::RulesBuilder>`.
Both approaches produce an immutable :cpp:class:`Rules <erbsland::conf::vr::Rules>` object that can validate many
documents.

The `ELCL validation-rules documentation <https://config-lang.erbsland.dev/validation-rules/index.html>`_ is the
language reference for rule-document structure, rule types, constraints, alternatives, versions, dependencies, and
custom diagnostics.
This page focuses on loading and applying those rules with Erbsland Core.

Embed a Rules Document
======================

A separate ``.elcl`` file keeps a substantial schema approachable in reviews and lets tools process it without
understanding C++.
For an application-owned schema, compile that file into the executable with the resource system.
This preserves the source file as a normal project artifact while avoiding a deployment-time search path or a mismatch
between the executable and its rules.

The demo embeds this rules document:

.. code-block:: elcl

    [survey]
    type: "section"
    title: "Levantamento da floresta"

    [survey.forest]
    type: "text"
    minimum: 3
    maximum: 80

    [survey.observers]
    type: "integer"
    minimum: 1
    maximum: 12

    [survey.region]
    type: "text"
    default: "Mata Atlântica"

    [survey.api_token]
    type: "text"
    is_optional: yes
    is_secret: yes

Each section path identifies a node in the application document.
The ``type`` attribute chooses its ELCL type; other attributes add metadata or behavior.
Constraints such as ``minimum`` and ``maximum`` narrow accepted content according to the rule type, so here they bound
text length for ``forest`` and numeric value for ``observers``.

Add the directory to the demo target with ``erbsland_core_add_resources`` as described in
:doc:`/topics/resource/compiled_resources`:

.. code-block:: cmake

    erbsland_core_add_resources(
            TARGET validation_rules
            DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data"
            IDENTIFIER "conf-validation-rules"
            SUFFIXES ".elcl"
    )

At runtime, read the text from :cpp:func:`Application::resources() <erbsland::core::Application::resources>`, parse it
as a normal configuration document, and pass the result to
:cpp:func:`Rules::createFromDocument() <erbsland::conf::vr::Rules::createFromDocument>`.
The factory checks that the document itself is a valid rule schema.
A malformed definition therefore fails during application setup, before it is applied to user configuration.

.. erbsland-demo::
    :source: conf/ValidationRules/EmbeddedRules.cpp
    :exec: conf/validation_rules --demo EmbeddedRules
    :source-sha256: a2c9b3f5106dfbcc6437fff4683405c03cbedd289943898a818930aac3333f06

.. code-block:: cpp

    /// Load and apply an ELCL validation-rules document embedded in the executable.
    ///
    /// Compiled resources keep a readable rules file beside the source code without introducing a deployment-time path.
    /// Parse that text, create a reusable `Rules` instance, and validate each application document before reading values.
    void embeddedRules() {
        // Load the rules document from the application's compiled resources.
        const auto ruleText =
            el::application().resources().getTextOrThrow("conf-validation-rules"_el, "survey-rules.elcl"_el);
        const auto ruleDocument = el::conf::Parser{}.parseTextOrThrow(ruleText);
        const auto rules = el::conf::vr::Rules::createFromDocument(ruleDocument);

        const auto configuration = "[survey]\n"
                                   "forest: \"Parque Nacional da Tijuca\"\n"
                                   "observers: 8\n"_el;
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);

        // Validation checks the shape and constraints, then adds configured default values.
        rules->validate(document, 1);
        el::io::printLine("Forest: "_el, document->getTextOrThrow("survey.forest"_el));
        el::io::printLine("Region: "_el, document->getTextOrThrow("survey.region"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Forest: Parque Nacional da Tijuca
    Region: Mata Atlântica

.. erbsland-demo-end::

Call :cpp:func:`Rules::validate() <erbsland::conf::vr::Rules::validate>` only after the application document has been
parsed and before its values are interpreted.
The integer argument selects the application's configuration-schema version.
Even when the first schema has no version-specific rules, passing its explicit version keeps the loading contract ready
for later evolution.

Validation stops at the first violation and throws ``ConfError`` with the relevant path and location.
On success, defaults such as ``survey.region`` are present in the document and ordinary typed getters can read the
validated tree.
Create the rules once during setup and reuse them when the application loads or reloads multiple documents.

Build Rules into C++
====================

A schema that closely follows compiled application logic can be expressed with ``RulesBuilder``.
This approach makes rule types and builder arguments visible to the C++ compiler and removes the parsing step.
It is especially convenient for compact schemas, library-owned configuration branches, or generated rule definitions.

:cpp:func:`RulesBuilder::addRule() <erbsland::conf::vr::RulesBuilder::addRule>` associates a name path with a
:cpp:class:`RuleType <erbsland::conf::vr::RuleType>` and any number of builder attributes.
Attributes such as :cpp:class:`Title <erbsland::conf::vr::builder::Title>`,
:cpp:class:`Default <erbsland::conf::vr::builder::Default>`,
:cpp:class:`IsOptional <erbsland::conf::vr::builder::IsOptional>`, and
:cpp:class:`IsSecret <erbsland::conf::vr::builder::IsSecret>` describe metadata or behavior.
Constraint attributes such as :cpp:class:`Minimum <erbsland::conf::vr::builder::Minimum>` and
:cpp:class:`Maximum <erbsland::conf::vr::builder::Maximum>` restrict accepted values.

.. erbsland-demo::
    :source: conf/ValidationRules/BuiltRules.cpp
    :exec: conf/validation_rules --demo BuiltRules
    :source-sha256: 8ef2de13f36755de9ebf18ab55dddc07f89fe5c11da09127de4808fb5e72a1a1

.. code-block:: cpp

    /// Build and apply validation rules directly in C++.
    ///
    /// `RulesBuilder` combines each rule type with attributes and constraints. Attributes describe behavior or metadata;
    /// constraints narrow the accepted value. Finalization checks the definition and returns an immutable, reusable rule
    /// set.
    void builtRules() {
        using el::conf::vr::RuleType;
        using el::conf::vr::builder::Default;
        using el::conf::vr::builder::Description;
        using el::conf::vr::builder::IsOptional;
        using el::conf::vr::builder::IsSecret;
        using el::conf::vr::builder::Maximum;
        using el::conf::vr::builder::Minimum;
        using el::conf::vr::builder::Title;

        auto builder = el::conf::vr::RulesBuilder{};
        builder.configureRoot(Title{"Levantamento florestal"_el});
        builder.addRule("survey"_el, RuleType::Section, Title{"Levantamento da floresta"_el});
        builder.addRule(
            "survey.forest"_el, RuleType::Text, Description{"Nome da floresta observada"_el}, Minimum{3}, Maximum{80});
        builder.addRule("survey.observers"_el, RuleType::Integer, Minimum{1}, Maximum{12});
        builder.addRule("survey.region"_el, RuleType::Text, Default{"Mata Atlântica"_el});
        builder.addRule("survey.api_token"_el, RuleType::Text, IsOptional{}, IsSecret{});
        const auto rules = builder.takeRules();

        const auto configuration = "[survey]\n"
                                   "forest: \"Floresta Nacional de Ipanema\"\n"
                                   "observers: 6\n"_el;
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
        rules->validate(document, 1);

        el::io::printLine("Validated observers: "_el, document->getIntegerOrThrow("survey.observers"_el));
        el::io::printLine("Default region: "_el, document->getTextOrThrow("survey.region"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Validated observers: 6
    Default region: Mata Atlântica

.. erbsland-demo-end::

The document root has an implicit rule.
Use :cpp:func:`configureRoot() <erbsland::conf::vr::RulesBuilder::configureRoot>` when the root itself needs a title,
description, dependency, version range, or another compatible attribute.
Use :cpp:func:`addAlternative() <erbsland::conf::vr::RulesBuilder::addAlternative>` when one path deliberately accepts
several rule definitions.

:cpp:func:`takeRules() <erbsland::conf::vr::RulesBuilder::takeRules>` finalizes the schema and resets the builder for
reuse.
Finalization checks the relationships between definitions and rejects incompatible attributes, invalid references, and
other logical errors before configuration validation begins.
The resulting ``Rules`` object is applied exactly like one created from a rules document, so the choice of source does
not spread into the rest of the loading code.

Choose the Rule Source
======================

Prefer an embedded ELCL document when people need to read or maintain a substantial schema independently of C++.
It remains normal text in the source tree, while compiled resources keep deployment reliable.
Prefer ``RulesBuilder`` when a compact schema belongs directly to a component's implementation or when C++ code
generates definitions from another model.

Applications can also combine these approaches at subsystem boundaries.
For example, the main executable can own an embedded application schema while a reusable component exposes a built rule
set for the configuration branch it understands.
Whichever source you choose, finalize the rules during setup, validate the complete input, and only then interpret the
document.

Continue with Validated Values
==============================

Successful validation leaves useful information on each value: the matching rule, secret status, default status, and
whether the value passed through validation at all.
The next topic explains how application diagnostics, configuration dumps, and editors can use this metadata safely.
