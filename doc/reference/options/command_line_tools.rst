.. index::
    single: Command Line Tools

******************
Command Line Tools
******************

Introduction
============

Option
------

:cpp:class:`Option <erbsland::options::Option>` describes one command line value.
The names you pass to ``addOption()`` decide whether the definition is a regular option or a positional argument.

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

Boolean Flag Values
~~~~~~~~~~~~~~~~~~~

Flags are false when absent, true when written without a value, and accept explicit ELCL boolean literals by default.
The recognized ASCII-case-insensitive true values are ``true``, ``on``, ``yes``, and ``enabled``; the false values are
``false``, ``off``, ``no``, and ``disabled``.
Both ``--flag=false`` and ``--flag false`` are accepted.
The separate form consumes the following argument only when it is a recognized literal, preserving all other text for
positional parsing.

Implicit repetitions such as ``-vv`` are counted normally.
An explicit value makes the flag a single assignment, so explicit repetitions and mixtures such as
``--verbose=false -v`` are rejected.
An explicit false value remains a source occurrence even though
:cpp:func:`getFlag() <erbsland::options::OptionValues::getFlag>` returns false.

Set :cpp:enumerator:`OptionParserFlag::DisableBooleanValues <erbsland::options::OptionParserFlag::DisableBooleanValues>`
on the root options object when an application needs the legacy valueless grammar:

.. code-block:: cpp

    options->setParserFlag(el::OptionParserFlag::DisableBooleanValues);

The setting applies to every flag in the options tree, including built-in help and version.
Attached flag values are then errors, following boolean-looking words remain positional, and generated help omits the
optional boolean placeholder.

Built-in Requests
~~~~~~~~~~~~~~~~~

The parser provides ``-h`` and ``--help`` for help output and ``--version`` for version output by default.
Use :cpp:enumerator:`OptionParserFlag::DisableHelp <erbsland::options::OptionParserFlag::DisableHelp>` or
:cpp:enumerator:`OptionParserFlag::DisableVersion <erbsland::options::OptionParserFlag::DisableVersion>` when an
application protocol needs to use one of these names as an ordinary option.
Disabling a built-in request also removes it from generated help and releases its names for application definitions.

.. code-block:: cpp

    options->setParserFlag(el::OptionParserFlag::DisableVersion);
    options->addOption({"--version"_el, "language-version"_el})
        .setType(el::OptionType::Text);

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

Help, Version and Error Documents
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:cpp:class:`OptionManager <erbsland::options::OptionManager>` builds neutral
:cpp:class:`TextDocument <erbsland::text::TextDocument>` trees for help, version and option-error output.
Applications can render these documents through
:cpp:class:`PlainTextRenderer <erbsland::text::PlainTextRenderer>` or
:cpp:class:`TerminalDocumentRenderer <erbsland::cterm::TerminalDocumentRenderer>`.
``DisplayTextMap`` configures wording only; terminal colors and layout belong to
:cpp:class:`TerminalDocumentStyle <erbsland::cterm::TerminalDocumentStyle>`.

:cpp:class:`OptionErrorContext <erbsland::options::OptionErrorContext>` keeps the structured objects needed to build a
complete diagnostic.
The options root, selected module, option set and option are retained with shared pointers, while the command-line
arguments and argument index preserve the source snapshot.
Usage, contextual help and the full-help command are derived when the diagnostic is rendered instead of being copied
into the context as plain text.
The built-in parser escapes command-line values before inserting them into explanatory messages.
Command-line snippets escape every displayed argument and calculate marker ranges from that escaped representation, so a
visible sequence such as ``\\033`` is covered by the marker in full.
The executable name derived from ``argv[0]`` is escaped in usage and full-help commands as well.

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
