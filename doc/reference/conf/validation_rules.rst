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
    builder.addRule("server"_el, RuleType::Section);
    builder.addRule("server.port"_el, RuleType::Integer, Minimum{1}, Maximum{65535});
    builder.addRule("server.name"_el, RuleType::Text, IsOptional{}, Default{"localhost"_el});
    auto rules = builder.takeRules();

    rules->validate(document, 1);

Validation follows document order and rule ordering.
It can attach rule metadata and insert declared defaults into the validated branch.
It throws a validation-category ``erbsland::conf::ConfError`` on the first failure.
Reuse finalized rules for multiple documents, passing the document version explicitly.

Builder attributes check their compatibility with the selected rule type while rules are finalized.
This catches invalid rule definitions before application configuration is validated.

Interface
=========

.. doxygenstruct:: erbsland::conf::vr::builder::Attribute
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::CaseSensitive
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::ConfVersion
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::CustomError
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Default
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Dependency
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Description
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::IsOptional
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::IsSecret
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::KeyIndex
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::MaximumVersion
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::MinimumVersion
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Title
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Type
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Chars
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::ConfKey
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::ConstraintAttribute
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::ConstraintOptions
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Contains
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Ends
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Equals
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::In
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Matches
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Maximum
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Minimum
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Multiple
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::Starts
    :members:
.. doxygenstruct:: erbsland::conf::vr::builder::StringPartConstraint
    :members:
.. doxygenclass:: erbsland::conf::vr::Constraint
    :members:
.. doxygenclass:: erbsland::conf::vr::Rules
    :members:
.. doxygenclass:: erbsland::conf::vr::RulesBuilder
    :members:
