..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Command-Line Options; Reference
    single: Command Line Tools

********************
Command-Line Options
********************

Introduction
============

Option
------

:cpp:class:`Option <erbsland::options::Option>` describes one command line value.
The names you pass to ``addOption()`` decide whether the definition is a regular option or a positional argument.

Name Normalization and Validation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Option names, module names, positional names, and internal lookup aliases use canonical ASCII names.
Except for short options, ASCII letters are folded to lowercase and underscores within name bodies are replaced with
dashes.
For example, ``--One_Two`` becomes ``--one-two`` and ``Run_One`` becomes ``run-one``.
Short names such as ``-A`` and ``-a`` remain distinct.

The original prefix determines the name form.
Short and long option prefixes must be literal ``-`` and ``--``; normalization never creates or changes a prefix.
Leading underscores cannot introduce an option, and ``-_`` cannot become the end-of-options marker.

A bare or long name body starts with an ASCII letter and consists of non-empty alphanumeric word segments separated by
single dashes.
Underscores are accepted as separator spellings before normalization.
The body has at most 100 characters, excluding the optional ``--`` prefix.
A short option has exactly one ASCII letter or digit after its literal dash.
Modules, positional names, and internal aliases require the bare form.
Empty supplied names, non-ASCII text, leading digits in bare or long names, trailing separators, and consecutive
separators are invalid.

Definition constructors and name setters validate immediately and throw ``err::ParameterFailure`` for malformed names.
Failed name-list updates preserve the existing definition.
Empty default objects and empty option-name lists remain available for staged construction.
Parsed value-name setters use the same validation; an empty selected module name represents root scope.

Developer lookups, command-line name matching, detailed help, and name suggestions use the same canonical spelling.
Both ``--one-two`` and ``--one_two`` select a definition stored as ``--one-two``.
Invalid lookup queries return the usual missing result.
The original arguments and indexes remain available for diagnostics: normalization does not rewrite option values,
choice values, executable names, paths, or positional data, including text after ``--``.

Regular Options and Aliases
~~~~~~~~~~~~~~~~~~~~~~~~~~~

An option is a regular option as soon as at least one of its names starts with ``-`` or ``--``.
Regular options are read from the command line by their dashed names and may appear in any order.
Any additional dashless names are value aliases.
They are not accepted as command line arguments; they only provide stable and readable lookup names after parsing.

.. code-block:: cpp

    options->addOption({"--demo"_el, "demo"_el});

    const auto values = manager.parseOrThrow(arguments);
    if (values->getFlag("demo"_el)) {
        runDemo();
    }

Because this definition has the dashed name ``--demo``, it is a regular option.
Because no explicit type is set, it keeps the regular-option default type
:cpp:enumerator:`OptionType::Flag <erbsland::options::OptionType::Flag>`.
The command line accepts ``--demo``.
The name ``demo`` is only an alias for lookup through :cpp:class:`OptionValues <erbsland::options::OptionValues>`.

Aliases are especially useful when an option has several command line spellings but your code should use one clear
internal name.

.. code-block:: cpp

    options->addOption({"-n"_el, "--name"_el, "name"_el})
        .setType(el::OptionType::Text);

    const auto name = values->getText("name"_el);

Here, ``-n`` and ``--name`` are accepted on the command line.
The alias ``name`` is only used in user code.

Flags, Boolean Values, and Optional Values
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:cpp:enumerator:`OptionType::Flag <erbsland::options::OptionType::Flag>` is a valueless occurrence marker.
It may repeat, and :cpp:func:`getFlagCount() <erbsland::options::OptionValues::getFlagCount>` reports the number of
occurrences.
A flag never accepts a command-line value and cannot have a configured default.

:cpp:enumerator:`OptionType::Boolean <erbsland::options::OptionType::Boolean>` is a typed value.
It accepts the ASCII-case-insensitive true values ``true``, ``on``, ``yes``, and ``enabled``, and the false values
``false``, ``off``, ``no``, and ``disabled``.
Boolean values use the same scalar and list behavior as integers and text and are read with ``getBoolean()`` or
``getBooleanList()``.

Set :cpp:enumerator:`OptionFlag::AcceptAsFlag <erbsland::options::OptionFlag::AcceptAsFlag>` on a named scalar value
option when it may also occur without a value:

.. code-block:: cpp

    options->addOption({"--color"_el, "color"_el})
        .setType(el::OptionType::Choice)
        .addChoice("auto"_el)
        .addChoice("always"_el)
        .setFlag(el::OptionFlag::AcceptAsFlag);

A bare occurrence stores an internal flag marker: ``getFlag()`` returns true and the declared typed getter returns its
fallback.
A valued occurrence stores only the declared type.
Definitions using ``AcceptAsFlag`` must be named, non-positional, and accept exactly one value.
The flag has no additional effect on ``OptionType::Flag`` and no option-set behavior.

Built-in Requests
~~~~~~~~~~~~~~~~~

The parser provides ``-h`` and ``--help[=<name>]`` for help output and the pure flag ``--version`` for version output by
default.
Bare help displays the ordinary document.
The attached long form ``--help=<name>`` finds detailed help for a module, option alias, or choice help alias.
The keyword must use a valid ASCII option-name form: a bare or long name starts with a letter, followed by letters,
digits and single word separators, with at most 100 characters excluding the optional ``--`` prefix.
Underscores normalize to dashes; consecutive and trailing separators are invalid.
A short name is ``-`` followed by one ASCII letter or digit and remains case-sensitive.
A qualified choice selector, such as ``--help=--color/auto``, contains one slash: the option alias on the left and a
bare choice help alias on the right.
Each component follows its own name rules; the complete selector has at most 203 ASCII bytes.
Literal dash prefixes remain mandatory, including in qualified selectors.
Empty, oversized, non-ASCII, or otherwise malformed keywords produce a syntax error without echoing the keyword,
generating suggestions, or including the command-line argument snapshot.
The same validation applies when calling ``detailedHelpDocument()`` directly.
Exact repeated option aliases prefer the selected module, then globals, then other modules in registration order.
``See Also`` lists distinct alternatives without linking to the displayed target or repeating registrations.
Collisions between modules, options and choices, or between choices in the same scope, produce a compact results list.
Without an exact match, help first tries fuzzy matching using the existing edit-distance thresholds and then prefixes if
there are no fuzzy matches.
For example, ``--help=generate`` lists modules whose names start with ``generate``.
Results show up to ten distinct targets with their names and brief titles; additional matches are reported.
The heading identifies the searched keyword.
Modules appear in a ``Modules`` section, global options in ``Options``, and module options in ``Options for <module>``.
Option rows use their short and long aliases and value placeholders without repeating help commands or generated
value/default annotations.
Matching choices appear under ``Choices for <option>`` or ``Choices for <module> <option>``, showing their accepted
values and brief titles; only matching choices are listed.
Ranking and the ten-target limit apply before grouping, with existing scope priority retained between option and choice
sections and ranking retained within each section.
Even a single partial match produces a results list.
When nothing matches, the error title is ``No matching help found`` and the description directs the user to ``--help``.
Set :cpp:enumerator:`OptionParserFlag::NoHelpDetails <erbsland::options::OptionParserFlag::NoHelpDetails>` to make help
a pure flag and reject an attached target.
Use :cpp:enumerator:`OptionParserFlag::DisableHelp <erbsland::options::OptionParserFlag::DisableHelp>` or
:cpp:enumerator:`OptionParserFlag::DisableVersion <erbsland::options::OptionParserFlag::DisableVersion>` when an
application protocol needs to use one of these names as an ordinary option.
Disabling a built-in request also removes it from generated help and releases its names for application definitions.

.. code-block:: cpp

    options->setParserFlag(el::OptionParserFlag::DisableVersion);
    options->addOption({"--version"_el, "language-version"_el})
        .setType(el::OptionType::Text);

Value Tokens Beginning With a Dash
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A separate value token beginning with ``-`` is always rejected.
Use attached syntax such as ``--count=-22`` or ``--name=-draft``.
Shell quoting is processed before ``argv`` reaches the application, so the parser cannot determine whether a separate
argument was quoted and does not make quoting an exception to this rule.

Positional Arguments
~~~~~~~~~~~~~~~~~~~~

An option is positional only when none of its names starts with ``-`` or ``--``.
Positional arguments are assigned from non-option command line values in definition order.
Their default type is :cpp:enumerator:`OptionType::Text <erbsland::options::OptionType::Text>`.

.. code-block:: cpp

    options->addOption({"path"_el});

    const auto values = manager.parseOrThrow(arguments);
    const auto path = values->getText("path"_el);

This definition has no dashed name, so ``path`` receives the next positional command line value.
The command line does not accept ``--path`` unless you define a separate regular option with that dashed name.

If an active positional argument is required, invoking the command with no arguments produces the normal help output.
This lets a user see the expected input before encountering a missing-value error.
Set
:cpp:enumerator:`OptionParserFlag::ErrorOnEmptyRequiredPositionals <erbsland::options::OptionParserFlag::ErrorOnEmptyRequiredPositionals>`
to retain an error result for an empty command.
The behavior applies only to required positional arguments; required regular options continue to report a missing-value
error.

Choices and Explicit Types
~~~~~~~~~~~~~~~~~~~~~~~~~~

Calling :cpp:func:`setChoices() <erbsland::options::OptionEditor::setChoices>` or
:cpp:func:`addChoice() <erbsland::options::OptionEditor::addChoice>` promotes the option type to
:cpp:enumerator:`OptionType::Choice <erbsland::options::OptionType::Choice>`.
This works for regular options and positional arguments.

.. code-block:: cpp

    options->addOption({"--mode"_el, "mode"_el})
        .setChoices(el::OptionChoices::create({"fast"_el, "safe"_el}));

    const auto mode = values->getText("mode"_el);

The command line accepts values through ``--mode fast`` or ``--mode=safe``.
The dashless name ``mode`` remains a lookup alias because the definition has ``--mode``.

If you explicitly set an inconsistent type after adding choices, parsing fails with a definition error that points back
to the affected option.
For example, choices require :cpp:enumerator:`OptionType::Choice <erbsland::options::OptionType::Choice>`, and a choice
option must have at least one configured choice.

Sensitive Text
~~~~~~~~~~~~~~

Use :cpp:enumerator:`OptionType::SensitiveText <erbsland::options::OptionType::SensitiveText>` for a password, token, or
other command-line value that should enter protected text storage:

.. code-block:: cpp

    options->addOption({"-s"_el, "--secret"_el, "secret"_el})
        .setType(el::OptionType::SensitiveText);

    auto arguments = el::OptionManager::convertCommandLineArguments(argc, argv);
    const auto result = manager.parse(arguments);
    const auto secret = result.values()->getText("secret"_el);

Sensitive text accepts exactly one value.
Definitions with a default value or a maximum other than one are rejected, and repeating the option is an error.
The parsed value is stored as a marked :cpp:type:`String <erbsland::text::String>` and is available through
:cpp:func:`OptionValues::getText() <erbsland::options::OptionValues::getText>` or
:cpp:func:`OptionValue::getText() <erbsland::options::OptionValue::getText>`.

The converted argument list passed to ``parse()`` or ``parseOrThrow()`` must be a mutable lvalue.
Before either method returns, each sensitive suffix is replaced with exactly five stars.
Thus ``--secret=value`` becomes ``--secret=*****``, while a separate or positional value becomes ``*****``.
The previous copy-on-write allocation is securely erased when it is writable shared storage; literal-backed strings are
never modified.
Error contexts built by the parser contain the masked argument list.

:cpp:func:`OptionResult::sensitiveTextLocations() <erbsland::options::OptionResult::sensitiveTextLocations>` exposes
the read-only list of :cpp:class:`OptionSensitiveTextLocation <erbsland::options::OptionSensitiveTextLocation>` values
for every result status.
Each location contains the argument index and UTF-8 byte index at which the sensitive suffix began.
Attached long and short values start after ``=``, while separate and positional values start at byte zero.

Command-line masking reduces later accidental exposure but cannot retract values already visible to the shell, process
listings, operating-system facilities, logs, or earlier application code.

Custom Value Names
~~~~~~~~~~~~~~~~~~

Help output normally derives value placeholders from the option type.
Text options display ``<value>``, integer options display ``<integer>``, and choice options display ``<choice>``.
Use :cpp:func:`setValueName() <erbsland::options::OptionEditor::setValueName>` when a domain-specific name is clearer.
Pass the bare name; generated help adds the angle brackets.

.. code-block:: cpp

    options->addOption({"--config"_el, "config"_el})
        .setType(el::OptionType::Text)
        .setValueName("path"_el);

    options->addOption("input"_el)
        .setValueName("source"_el)
        .setRequired();

These definitions are displayed as ``--config <path>`` and ``<source>`` in usage and option-list help.
An empty value name clears the override and restores the type-derived or positional default.

Help Titles and Descriptions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Help metadata separates brief text from details without falling back between fields.

.. list-table:: Help field meanings
    :header-rows: 1

    * - Definition
      - ``title``
      - ``description``
    * - Option
      - Brief text beside its aliases in overview output
      - Details on its help page
    * - OptionSet
      - Section heading
      - Introduction immediately below the heading
    * - OptionModule
      - Brief text in the module overview
      - Details in full module help
    * - OptionChoice
      - Brief text beside the accepted choice value
      - Details on its help page

``OptionHelp(String)`` and the string ``setHelp()`` editor overloads set the title.
Use ``setHelpDescription()`` separately for detailed text.
Sets sharing a section heading combine their option rows and display each distinct description once in registration
order.
Only sets with visible option rows contribute headings or descriptions; empty, description-only and fully filtered sets
are omitted, including their introductions when another set shares the same heading.
Root ``Options`` descriptions continue to introduce root help.

Detail pages start with the brief title, or a localized identifying heading when no title is supplied, followed by a
separate identity line showing the option aliases, module name, or choice value and owning option.
Usage, nonempty descriptions, examples and epilogs follow.
Option and choice details retain relevant generated value and default information.
``--help=<module>`` and ``<module> --help`` produce the same full module page, including its option sections.

Choice Help Aliases
~~~~~~~~~~~~~~~~~~~

Choice values are not restricted to option-name syntax and remain unchanged during parsing.
For detailed help, configure independent bare aliases through
:cpp:func:`setHelpNames() <erbsland::options::OptionChoice::setHelpNames>` or the choice editor.
Aliases normalize ASCII case and underscores, follow the bare-name grammar, and have at most 100 characters each.
Malformed aliases throw ``err::ParameterFailure`` immediately and leave previous aliases unchanged.
An empty explicit alias list uses the normalized choice value as its help name when that value satisfies the bare-name
rules; other values need an explicit alias for direct lookup.

.. code-block:: cpp

    auto choices = el::OptionChoices::create();
    choices->addChoice("Deep Blue!"_el)
        .setHelp("Use deep blue"_el)
        .setHelpDescription("Choose this shade for the presentation."_el)
        .setHelpNames({"deep-blue"_el, "blue"_el});
    options->addOption("--color"_el).setChoices(choices);

Use ``--help=blue`` or ``--help=--color/blue`` for this choice.
Qualified selectors distinguish repeated choice aliases under different options.
Search results identify the owning option and optional module in their choice-section headings; detailed pages and
``See Also`` retain runnable help commands.

Help, Version and Error Documents
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:cpp:class:`OptionManager <erbsland::options::OptionManager>` builds neutral
:cpp:class:`TextDocument <erbsland::text::TextDocument>` trees for help, version and option-error output.
The ``options.help_results_title`` display-text format accepts the searched keyword.
``options.options_for_module_heading`` and ``options.choices_for_option_heading`` accept one name;
``options.choices_for_module_option_heading`` accepts the module name followed by the preferred option alias.
Applications can render these documents through
:cpp:class:`PlainTextRenderer <erbsland::text::PlainTextRenderer>` or
:cpp:class:`TerminalDocumentRenderer <erbsland::cterm::TerminalDocumentRenderer>`.
``DisplayText`` configures wording only; terminal colors and layout belong to
:cpp:class:`TerminalDocumentStyle <erbsland::cterm::TerminalDocumentStyle>`.

:cpp:class:`OptionErrorContext <erbsland::options::OptionErrorContext>` keeps the structured objects needed to build a
complete diagnostic.
The options root, selected module, option set and option are retained with shared pointers, while the command-line
arguments and argument index preserve the source snapshot.
Usage, contextual help and the full-help command are derived when the diagnostic is rendered instead of being copied
into the context as plain text.
Diagnostic titles describe the user-facing problem, while descriptions explain it and suggest a corrective action.
Default wording comes from ``DisplayText``; application-authored callback titles remain unchanged.
Malformed help selectors use the title ``Invalid help keyword`` and are never echoed in a diagnostic.
The built-in parser escapes command-line values before inserting them into explanatory messages.
Command-line snippets escape every displayed argument and calculate marker ranges from that escaped representation, so a
visible sequence such as ``\\033`` is covered by the marker in full.
The executable name derived from ``argv[0]`` is escaped in usage and full-help commands as well.

Unknown long options, modules, and visible choice values receive up to three ranked error suggestions.
Suggestions use decoded Unicode code points, adjacent-transposition-aware Damerau-Levenshtein distance, and conservative
length-dependent thresholds.
They are retained in ``OptionErrorContext::suggestions()`` and rendered in a localized ``Did You Mean`` section before
usage and contextual help.

Use :cpp:func:`OptionErrorContext::setTitle() <erbsland::options::OptionErrorContext::setTitle>` for the short error
heading and :cpp:func:`OptionErrorContext::setDescription() <erbsland::options::OptionErrorContext::setDescription>` for
the explanatory paragraph.
If a callback omits the title, the parser supplies a title based on
:cpp:enum:`OptionErrorReason <erbsland::options::OptionErrorReason>`.

Interface
=========

.. doxygenclass:: erbsland::options::Option
    :members:
.. doxygentypedef:: erbsland::options::OptionValidateFn

.. doxygentypedef:: erbsland::options::PreOptionSetParsingFn

.. doxygentypedef:: erbsland::options::PreOptionModuleParsingFn

.. doxygentypedef:: erbsland::options::PostParsingFn

.. doxygentypedef:: erbsland::options::ModuleMainFn
.. doxygenclass:: erbsland::options::OptionChoice
    :members:
.. doxygenclass:: erbsland::options::OptionChoiceEditor
    :members:
.. doxygenclass:: erbsland::options::OptionChoices
    :members:
.. doxygenclass:: erbsland::options::OptionEditor
    :members:
.. doxygenclass:: erbsland::options::OptionError
    :members:
.. doxygenclass:: erbsland::options::OptionErrorContext
    :members:
.. doxygenenum:: erbsland::options::OptionErrorReason
.. doxygenenum:: erbsland::options::OptionFlag

.. doxygentypedef:: erbsland::options::OptionFlags
.. doxygenclass:: erbsland::options::OptionHelp
    :members:
.. doxygenenum:: erbsland::options::OptionHelpVisibility
.. doxygentypedef:: erbsland::options::OptionInteger
.. doxygenclass:: erbsland::options::OptionManager
    :members:
.. doxygenclass:: erbsland::options::OptionModule
    :members:
.. doxygenenum:: erbsland::options::OptionParserFlag

.. doxygentypedef:: erbsland::options::OptionParserFlags
.. doxygenclass:: erbsland::options::OptionResult
    :members:
.. doxygenenum:: erbsland::options::OptionResultStatus
.. doxygenclass:: erbsland::options::Options
    :members:
.. doxygenclass:: erbsland::options::OptionSensitiveTextLocation
    :members:

.. doxygentypedef:: erbsland::options::OptionSensitiveTextLocations
.. doxygenclass:: erbsland::options::OptionSet
    :members:
.. doxygenclass:: erbsland::options::OptionSetManager
    :members:
.. doxygenclass:: erbsland::options::OptionType
    :members:
.. doxygenclass:: erbsland::options::OptionValue
    :members:
.. doxygenclass:: erbsland::options::OptionValueNameHash
    :members:
.. doxygenclass:: erbsland::options::OptionValues
    :members:
.. doxygentypedef:: erbsland::options::OptionValueStorage
.. doxygenenum:: erbsland::options::OptionValueType
