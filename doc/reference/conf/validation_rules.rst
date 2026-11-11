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

Membership constraints built with ``vr::builder::In`` accept ``List<Integer>``, ``List<Float>``, ``StringList``, and
``List<ByteBlock>``.
Parsed validation rules use the same containers without converting through standard vectors.

Literal Names in Rules Documents
================================

A rule attribute and a child rule cannot share the same name in an ELCL section.
Use ``vr_literal_<name>`` in a section path to declare a child with a literal configuration name:

.. code-block:: elcl

    [server]
    type: "section"
    title: "Server settings"

    [server.vr_literal_type]
    type: "text"

    [server.vr_literal_title]
    type: "text"
    default: "Main server"

These rules validate values named ``type`` and ``title`` inside ``server``.
Exactly one ``vr_literal_`` prefix is removed; the remaining text must be a regular ELCL name and is not interpreted as
another escape or a structural rule name.
For example, ``vr_literal_vr_any`` targets a literal value named ``vr_any`` rather than arbitrary values.
The existing escape ``vr_vr_type`` targets ``vr_type``.
Both forms work in nested rules, templates, and alternatives, including required values and default insertion.
Sibling declarations that resolve to the same target name are rejected.

Dependency paths and key paths use actual configuration names, such as ``type`` or ``vr_type``, for both escape forms.
Within a key path, ``vr_entry`` identifies list entries only when the preceding path resolves to a list rule.
At other positions it can be an ordinary configuration name.
The programmatic builder takes literal names directly: ``server.type`` needs no escape, and ``server.vr_literal_type``
targets a value whose name is literally ``vr_literal_type``.

Rule Paths and Metadata
=======================

``vr::Rule::namePath()`` returns the rule's symbolic value path.
For a named scalar this is its configuration path, with document escapes already decoded.
For example, the declaration ``server.vr_literal_type`` has the value path ``server.type``.

A symbolic value path can describe many document values.
The path ``items.vr_entry.type`` describes ``type`` in every entry of ``items``; ``vr_entry`` is not a concrete list
index.
Similarly, ``vr_any`` represents names selected from the document, and ``vr_name`` identifies constraints on a value's
name.
Name constraints retain their full symbolic path in both document-loaded and programmatically built rules.
The root path is empty, and alternative branches share the value path of their alternatives container.

Internally, each rule also has a definition path identifying its position in the expanded rule tree.
Definition paths retain escape spellings and alternative indexes, such as ``items.vr_entry.vr_literal_type[0]``.
When a template is inserted, these paths use the insertion point; source locations still identify the template
declaration.
Explicit rule roles distinguish named values, list entries, wildcard rules, name constraints, alternatives, and the
root.
A named value whose decoded name is ``vr_entry`` therefore remains distinct from a list-entry rule.

Key paths use actual names and a list-entry placeholder, even when several components happen to be named ``vr_entry``.
For a list named ``vr_entry``, ``vr_entry.vr_entry.type`` selects the ``type`` field in each entry.
Rule finalization resolves the list and entry-relative paths once; repeated document validation reuses them.
Index names and index references are literal metadata and never undergo declaration-escape decoding.

Name-Constraint Templates
=========================

A ``vr_name`` section may use a template whose type is Text:

.. code-block:: elcl

    [vr_template.identifier]
    type: "text"
    starts: "u_"
    minimum: 3

    [users.vr_any]
    type: "integer"

    [users.vr_any.vr_name]
    use_template: "identifier"
    minimum: 5

Template constraints and case sensitivity are inherited; local settings override them using the ordinary template rules.
The constraints apply to the name, independently of the value's type.
Without ``type`` or ``use_template``, a name rule still has implicit Text type.
A name rule cannot specify both fields, use a non-text or alternative template, or contain subsections.
Templates cannot reference other templates.

Builder Branches
================

Use ``createBranch()`` to construct a subtree with relative paths:

.. code-block:: cpp

    builder.addRule("server"_el, RuleType::Section);
    auto server = builder.createBranch("server"_el);
    server.configureRoot(Title{"Server Settings"_el});
    server.addRule("port"_el, RuleType::Integer, Minimum{1}, Maximum{65535});
    server.addRule("tls"_el, RuleType::Section);
    auto tls = server.createBranch("tls"_el);
    tls.addRule("certificate"_el, RuleType::Text);

A branch only establishes a prefix; it does not declare its parent rule.
``parentPath()`` returns the complete prefix, or an empty path for the main builder.
Nested branches append paths, including reserved names such as ``vr_entry`` and ``vr_any``.
Empty paths, text names, and indexes are rejected using the same rules as ``addRule()``.

Branches derive from ``RulesBuilder`` and can be passed to helpers accepting ``RulesBuilder&``.
``configureRoot()`` configures the existing branch parent; dependencies and key paths remain relative to that rule.
A missing parent raises a validation-category ``ConfError``.
Only the main builder can call ``reset()`` or ``takeRules()``; either operation on a branch raises ``LogicFailure``.

Branches share the backend and may outlive the builder or branch that created them.
After the main builder resets or takes rules, existing branches retain their prefixes and operate on the new document.
Declare the necessary parent rules again before using them.
Builders and branches are neither copyable nor movable; ``createBranch()`` returns a branch directly by value.
They provide no synchronization for concurrent mutation.

Simple Value Lists
==================

Use ``EntryType`` when each value-list entry needs only a scalar type check:

.. code-block:: cpp

    builder.addRule(
        "server.names"_el, RuleType::ValueList, EntryType{RuleType::Text}, IsOptional{});

This creates the same child rule as an explicit ``server.names.vr_entry`` rule of type ``Text``.
The existing value-list behavior still accepts a single scalar as a one-entry list.
``EntryType{RuleType::Value}`` accepts any scalar type, and ``addAlternative()`` supports independent entry types for
each alternative.
List attributes continue to apply to the list itself.

``EntryType`` accepts scalar rule types only and requires a ``ValueList`` parent.
Duplicate entry attributes or an additional explicit ``vr_entry`` are rejected with ``ConfError`` in the validation
category.
Use ``EntryConstraints``, ``EntryTitle``, and ``EntryDescription`` to configure an implicit scalar entry:

.. code-block:: cpp

    server.addRule(
        "names"_el,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        EntryConstraints{Minimum{2}, Matches{R"(\A[a-z]+\z)"_el}},
        EntryTitle{"Server name"_el},
        EntryDescription{"A lower-case server name with at least two characters."_el},
        IsOptional{});

These attributes may appear before or after ``EntryType`` and require it in the same builder call.
``EntryConstraints`` owns its constraint arguments and accepts custom ``ConstraintAttribute`` subclasses.
Stored entry attributes can be reused across declarations.
An empty constraint pack is allowed; repeated constraints replace matching constraints, and repeated entry titles or
descriptions use the last value.
Entry constraints and metadata apply only to ``vr_entry``; ordinary attributes continue to configure the list.
Invalid entry attributes are rejected before a new rule or alternative is attached.
Each alternative has its own entry configuration.

A branch targeting a value list can configure its missing entry using ``configureRoot()`` with ``EntryType`` and entry
attributes in the same call.
An existing explicit or implicit ``vr_entry`` cannot be replaced this way.
For nested lists, matrices, section entries, or other entry attributes, declare ``vr_entry`` explicitly.

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
.. doxygenclass:: erbsland::conf::vr::builder::EntryConstraints
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::EntryDescription
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::EntryTitle
    :members:
.. doxygenclass:: erbsland::conf::vr::builder::EntryType
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
.. doxygenclass:: erbsland::conf::vr::RulesBuilderBranch
    :members:
.. doxygenclass:: erbsland::conf::vr::RuleType
    :members:
