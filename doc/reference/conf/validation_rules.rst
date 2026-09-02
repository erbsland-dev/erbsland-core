.. index::
    single: Configuration Validation Rules

******************************
Configuration Validation Rules
******************************

Validation rules describe the expected shape and content of a configuration tree.
A rule selects a name path and type, then adds attributes such as optionality, defaults, version ranges, dependencies,
secrecy, and descriptive metadata.
Constraints narrow accepted values with equality, membership, bounds, multiples, character classes, regular expressions,
references, and text-part checks.

Rules can be read from a parsed rules document with
:cpp:func:`erbsland::conf::vr::Rules::createFromDocument <erbsland::conf::vr::Rules::createFromDocument>` or built
directly in C++ with :cpp:class:`erbsland::conf::vr::RulesBuilder <erbsland::conf::vr::RulesBuilder>`.

.. code-block:: cpp

    using namespace el::text::literals;
    using namespace el::conf::vr;
    using namespace el::conf::vr::builder;

    RulesBuilder builder;
    builder.configureRoot(
        Title{"Server Settings"_el},
        Dependency{
            DependencyMode::XNOR,
            {"server.certificate"_el},
            {"server.signing_key"_el},
            "Certificate and signing key must be configured together."_el});
    builder.addRule("server"_el, RuleType::Section);
    builder.addRule("server.port"_el, RuleType::Integer, Minimum{1}, Maximum{65535});
    builder.addRule("server.certificate"_el, RuleType::Text, IsOptional{});
    builder.addRule("server.signing_key"_el, RuleType::Text, IsOptional{}, IsSecret{});
    builder.addRule("server.name"_el, RuleType::Text, IsOptional{}, Default{"localhost"_el});
    auto rules = builder.takeRules();

    rules->validate(document, 1);

Validation follows document order and rule ordering.
It can attach rule metadata and insert declared defaults into the validated branch.
It throws a validation-category ``erbsland::conf::ConfError`` on the first failure.
Reuse finalized rules for multiple documents, passing the document version explicitly.

Builder attributes check their compatibility with the selected rule type while rules are finalized.
This catches invalid rule definitions before application configuration is validated.

The builder's implicit root rule represents the document or section passed to ``Rules::validate()``.
Use ``configureRoot()`` for root metadata, dependencies, key indexes, version restrictions, and other root attributes.
``DependencyMode`` is part of the public validation-rule API, so defining cross-value relationships never requires an
implementation header.

Rules written as ELCL documents use structural sections for rule definitions and reserved ``vr_dependency`` and
``vr_key`` section lists for relationships.
ELCL documents cannot contain scalar values directly at their document root, so root scalar metadata such as a title is
a programmatic-builder feature; dependencies and key indexes work in both forms.

Interface
=========

.. doxygenclass:: erbsland::conf::vr::builder::Attribute
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::CaseSensitive
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::ConfVersion
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::CustomError
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Default
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Dependency
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Description
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::IsOptional
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::IsSecret
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::KeyIndex
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::MaximumVersion
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::MinimumVersion
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Title
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Type
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Chars
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::ConfKey
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::ConstraintAttribute
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::ConstraintOptions
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Contains
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Ends
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Equals
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::In
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Matches
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Maximum
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Minimum
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Multiple
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::Starts
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::StringPartConstraint
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::RuleDefinition
    :members:
.. doxygenclass:: erbsland::conf::vr::Constraint
    :members:
.. doxygenclass:: erbsland::conf::vr::ConstraintType
    :members:
.. doxygenclass:: erbsland::conf::vr::DependencyMode
    :members:
.. doxygenclass:: erbsland::conf::vr::Rule
    :members:
.. doxygenclass:: erbsland::conf::vr::Rules
    :members:
.. doxygenclass:: erbsland::conf::vr::RulesBuilder
    :members:
.. doxygenclass:: erbsland::conf::vr::RuleType
    :members:
