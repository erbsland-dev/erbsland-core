..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Options
    single: Option
    single: OptionEditor
    single: OptionType
    single: OptionFlag
    single: OptionHelpVisibility

.. _options-options:

******************
Option Definitions
******************

An :cpp:class:`Option <erbsland::options::Option>` describes one accepted command-line value.
This page explains how option names, value types, repeated values, defaults, validation, flags, choices, and help
metadata work together.

Start With the User-Facing Value
================================

The most useful way to design an option is to start with the value the user wants to provide.
Then decide how that value should be spelled on the command line and how application code should read it.

An option with at least one dashed name is a regular option.
Long names such as ``--species`` are case-insensitive and are usually the most readable spelling.
Short names such as ``-s`` are case-sensitive and are useful for common options.
Dashless names on a regular option are lookup aliases only; they are not accepted on the command line.

An option without dashed names is a positional argument.
Positionals are assigned in definition order and appear as placeholders in the generated usage line.

Use OptionEditor for Compact Definitions
========================================

Most code creates options through :cpp:func:`addOption() <erbsland::options::OptionSetManager::addOption>`.
The returned :cpp:class:`OptionEditor <erbsland::options::OptionEditor>` edits the new option and returns itself from
each setter.
This makes complete option declarations compact while still keeping the important details next to the name.

.. erbsland-demo::
    :source: option/DefiningOptions/main.cpp
    :exec: option/option_definitions
    :source-sha256: b6d84a6d8284de4aaee946c220eb65e81ea767bdd19703bd24d5e183bfc69fa7

.. code-block:: cpp

    /// An `Option` definition describes names, type, defaults, list limits, choices, validation, and help text.
    ///
    /// A dashed name such as `--instrument` is accepted on the command line.
    /// A dashless name on the same regular option, such as `instrument`, is a lookup alias for `OptionValues`.
    /// An option with only dashless names is a positional argument.
    auto createDefinitionOptions() -> el::OptionsPtr {
        auto options = el::Options::create();
        auto info = el::ApplicationInfo{};
        info.setApplicationName("Optical Planner"_el);
        info.setApplicationVersion(el::Version{0, 8, 0});
        options->setApplicationInfo(info);
        options->setHelpTitle("Optical Planner"_el);
        options->setHelpDescription("Prepares a small measurement sequence for optical instruments."_el);
        options->addOption({"-i"_el, "--instrument"_el, "instrument"_el})
            .setType(el::OptionType::Text)
            .setRequired()
            .setHelpDescription("Instrument used for the sequence."_el);
        options->addOption({"-l"_el, "--level"_el, "level"_el})
            .setType(el::OptionType::Integer)
            .setMaximum(el::ArgumentCount{3U})
            .setHelpDescription("Intensity levels to test. Can be repeated up to three times."_el);
        options->addOption({"-m"_el, "--mode"_el, "mode"_el})
            .addChoice("fast"_el)
            .addChoice("precise"_el)
            .addChoice("night"_el)
            .setDefaultValue(el::String{"precise"_el})
            .setHelpDescription("Measurement mode for the output."_el);
        options->addOption({"--minimum-signal"_el, "minimum-signal"_el})
            .setType(el::OptionType::Integer)
            .setDefaultValue(el::OptionInteger{3})
            .setValidateFn([](const el::OptionValuePtr &value, el::OptionValuesPtr) -> void {
                if (value->getInteger() < 1 || value->getInteger() > 9) {
                    auto context = el::OptionErrorContext{};
                    context.setDescription("The signal level must be between 1 and 9."_el);
                    throw el::OptionError{context};
                }
            })
            .setHelpDescription("Minimum accepted signal on a scale from 1 to 9."_el);
        options->addOption("sample"_el).setHelpDescription("Sample placed in the optical holder."_el);
        return options;
    }

    auto definingOptions() -> el::ExitCode {
        auto manager = el::OptionManager{createDefinitionOptions()};
        const auto args = makeArgs(
            {"optica"_el,
                "--instrument"_el,
                "Prisma-7"_el,
                "-l"_el,
                "1"_el,
                "--level=3"_el,
                "--mode=night"_el,
                "vidrio azul"_el});
        const auto values = manager.parseOrThrow(args);

        el::io::printLine("instrument: "_el, values->getText("instrument"_el));
        el::io::printLine("mode: "_el, values->getText("mode"_el));
        el::io::printLine("sample: "_el, values->getText("sample"_el));
        el::io::printLine("levels: "_el, values->getIntegerList("level"_el).size());
        el::io::printLine("minimum signal: "_el, values->getInteger("minimum-signal"_el));
        return el::ExitCode::success();
    }

.. erbsland-ansi::
    :escape-char: ␛

    instrument: Prisma-7
    mode: night
    sample: vidrio azul
    levels: 2
    minimum signal: 3

.. erbsland-demo-end::

Choose the Right Value Type
===========================

The parser supports four option types:

* :cpp:enumerator:`OptionType::Flag <erbsland::options::OptionType::Flag>` stores whether a switch appeared, and also
  remembers how often it appeared.
* :cpp:enumerator:`OptionType::Integer <erbsland::options::OptionType::Integer>` parses a signed decimal integer.
* :cpp:enumerator:`OptionType::Text <erbsland::options::OptionType::Text>` stores one text value exactly as the command
  line supplied it after argument conversion.
* :cpp:enumerator:`OptionType::Choice <erbsland::options::OptionType::Choice>` accepts text from a configured choice
  list and stores the configured spelling.

Regular options default to ``Flag`` because a dashed option without a value is normally a switch.
Positionals default to ``Text`` because a positional argument always consumes a value.
Calling :cpp:func:`setType() <erbsland::options::OptionEditor::setType>` makes the intent explicit.
Calling :cpp:func:`addChoice() <erbsland::options::OptionEditor::addChoice>` or
:cpp:func:`setChoices() <erbsland::options::OptionEditor::setChoices>` promotes the option to ``Choice``.

Work With Choices
=================

Choices are matched case-insensitively, but the parsed value uses the configured choice text.
Use simple :cpp:func:`addChoice() <erbsland::options::OptionEditor::addChoice>` calls when all choices only need a
label.
Use :cpp:class:`OptionChoices <erbsland::options::OptionChoices>` and
:cpp:class:`OptionChoice <erbsland::options::OptionChoice>` when choices need their own help text or visibility.

Choice help is rendered as detail rows below the option.
Hidden choices remain parseable, but do not appear in the help output.

.. erbsland-demo::
    :source: option/OptionHelpDetails/main.cpp
    :exec: option/option_help_details --help
    :exec-2: option/option_help_details --species bat --phase dusk --quiet dune-edge
    :exec-3: option/option_help_details --species bat --round 9 dune-edge
    :exec-3-exit-code: 1
    :source-sha256: 3e8c843f0e1c8c8f0642758dac3205ef2290ed98de445b05f1e2c26b4952d268

.. code-block:: cpp

    /// Build a choice list when individual choices need their own help metadata.
    ///
    /// `OptionEditor::addChoice()` is enough for simple choices.
    /// Create `OptionChoice` objects when choices need descriptions or visibility settings of their own.
    /// Hidden choices remain accepted by the parser, but are omitted from generated help output.
    [[nodiscard]] auto activityChoices() -> el::OptionChoicesPtr {
        auto choices = el::OptionChoices::create();

        choices->addChoice(el::OptionChoice::create("dusk"_el, el::OptionHelp{"Activity around dusk."_el}));
        choices->addChoice(el::OptionChoice::create("night"_el, el::OptionHelp{"Activity in full darkness."_el}));
        choices->addChoice(el::OptionChoice::create("dawn"_el, el::OptionHelp{"Activity shortly before sunrise."_el}));

        auto internalHelp = el::OptionHelp{"Internal test choice that does not appear in regular help."_el};
        internalHelp.setVisibility(el::OptionHelpVisibility::Hidden);
        choices->addChoice(el::OptionChoice::create("internal"_el, internalHelp));
        return choices;
    }

    /// Option definitions combine command-line names, value types, flags, choices, defaults, validation, and help text.
    ///
    /// `OptionEditor` is returned from `addOption()`, so each option can be declared in one fluent expression.
    /// Dashed names are accepted on the command line, while dashless names are lookup aliases for `OptionValues`.
    /// Choices may carry their own help text, and visibility controls where an enabled option appears in generated help.
    class NightWatchApp final : public el::Application {
    public:
        using Application::Application;

    protected:
        void initialize() override {
            enableTerminal();
            info().setApplicationName("Night Watch"_el);
            info().setApplicationVersion(el::Version{0, 8, 0});
        }
        void registerCommandLineOptions(const el::OptionsPtr &options) override {
            options->setHelpTitle("Night Watch"_el);
            options->setHelpDescription("Records short observations of nocturnal animal behavior."_el);
            options->setHelpEpilog("Hidden options remain usable, but do not appear in this help."_el);
            options->addOption({"-s"_el, "--species"_el, "species"_el})
                .setType(el::OptionType::Text)
                .setValueName("animal"_el)
                .setRequired()
                .setHelpDescription("Animal species tracked during the round."_el)
                .setHelpVisibility(el::OptionHelpVisibility::Usage);
            options->addOption({"-f"_el, "--phase"_el, "phase"_el})
                .setChoices(activityChoices())
                .setDefaultValue(el::String{"night"_el})
                .setHelpDescription("Time window in which the observation occurs."_el);
            options->addOption({"-r"_el, "--round"_el, "round"_el})
                .setType(el::OptionType::Integer)
                .setDefaultValue(el::OptionInteger{2})
                .setValidateFn([](const el::OptionValuePtr &value, el::OptionValuesPtr) -> void {
                    if (value->getInteger() < 1 || value->getInteger() > 6) {
                        auto context = el::OptionErrorContext{};
                        context.setTitle("Invalid round count"_el)
                            .setDescription("The round count must be between 1 and 6."_el);
                        throw el::OptionError{context};
                    }
                })
                .setHelpDescription("Number of observation rounds in this area."_el);
            options->addOption({"-q"_el, "--quiet"_el, "quiet"_el})
                .setHelpDescription("Writes only the final result."_el)
                .setHelpVisibility(el::OptionHelpVisibility::Hidden);
            options->addOption({"--old-log"_el, "old-log"_el})
                .setFlag(el::OptionFlag::Disabled)
                .setHelpDescription("Old logging option that is no longer accepted."_el);
            options->addOption("area"_el)
                .setRequired()
                .setHelpDescription("Area where the observation takes place."_el);
        }
        [[nodiscard]] auto main() -> el::ExitCode override {
            const auto values = optionValues();
            el::io::printLine("species: "_el, values->getText("species"_el));
            el::io::printLine("area: "_el, values->getText("area"_el));
            el::io::printLine("phase: "_el, values->getText("phase"_el));
            el::io::printLine("rounds: "_el, values->getInteger("round"_el));
            el::io::printLine("quiet: "_el, el::BooleanFormat::yesNo(), values->getFlag("quiet"_el));
            return el::ExitCode::success();
        }
    };

.. rubric:: ``$ option/option_help_details --help``

.. erbsland-ansi::
    :escape-char: ␛

    Records short observations of nocturnal animal behavior.

    ␛[1mUsage:
      ␛[22;92moption_help_details␛[39m ␛[90m[␛[96moptions␛[90m]␛[39m ␛[90m[␛[96m--species ␛[90m<␛[92manimal␛[90m>]␛[39m ␛[90m<␛[95marea␛[90m>

    ␛[39;1mOptions:
      ␛[22m    ␛[90m<␛[95marea␛[90m>␛[39m          Area where the observation takes place.
      ␛[93m-h␛[39m, ␛[96m--help␛[39m          Display this help.
      ␛[93m-f␛[39m, ␛[96m--phase␛[39m ␛[90m<␛[92mchoice␛[90m>
      ␛[39m                    Time window in which the observation occurs. Choices: dusk, night,
                          dawn.
          ␛[92mdusk␛[39m   Activity around dusk.
          ␛[92mnight␛[39m  Activity in full darkness.
          ␛[92mdawn␛[39m   Activity shortly before sunrise.
      ␛[93m-r␛[39m, ␛[96m--round␛[39m ␛[90m<␛[92minteger␛[90m>
      ␛[39m                    Number of observation rounds in this area.
      ␛[93m-s␛[39m, ␛[96m--species␛[39m ␛[90m<␛[92manimal␛[90m>
      ␛[39m                    Animal species tracked during the round.
          ␛[96m--version␛[39m       Display version information.

    Hidden options remain usable, but do not appear in this help.

.. rubric:: ``$ option/option_help_details --species bat --phase dusk --quiet dune-edge``

.. erbsland-ansi::
    :escape-char: ␛

    species: bat
    area: dune-edge
    phase: dusk
    rounds: 2
    quiet: yes

.. rubric:: ``$ option/option_help_details --species bat --round 9 dune-edge``

.. erbsland-ansi::
    :escape-char: ␛


      ␛[1;91mInvalid␛[22m ␛[1mround␛[22m ␛[1mcount

      ␛[22;39mThe round count must be between 1 and 6.

    ␛[1mError␛[22m ␛[1mSource:
      ␛[22;92mCommand Line Arguments
      ␛[93mArgument: 4

    ␛[39;1mCommand␛[22m ␛[1mLine␛[22m ␛[1mArguments:

      ␛[22;90m   0 │ ␛[39mcmake-build-debug/demo-apps/option/option_help_details
      ␛[90m   1 │ ␛[39m--species
      ␛[90m   2 │ ␛[39mbat
      ␛[90m   3 │ ␛[39m--round
      ␛[90m   4 │ ␛[91m9
      ␛[90m     │ ␛[91m▔
      ␛[90m   5 │ ␛[39mdune-edge

    ␛[1mUsage:
      ␛[22;92moption_help_details␛[39m ␛[90m[␛[96moptions␛[90m]␛[39m ␛[90m[␛[96m--species ␛[90m<␛[92manimal␛[90m>]␛[39m ␛[90m<␛[95marea␛[90m>

    ␛[39;1mOption␛[22m ␛[1mHelp:
      ␛[22;93m-r␛[39m, ␛[96m--round␛[39m ␛[90m<␛[92minteger␛[90m>␛[39m  Number of observation rounds in this area.

    ␛[1mView␛[22m ␛[1mFull␛[22m ␛[1mHelp:
      ␛[22;92moption_help_details␛[39m ␛[96m--help␛[0m

.. erbsland-demo-end::

Repeated Values and Maximum
===========================

:cpp:func:`setMaximum() <erbsland::options::OptionEditor::setMaximum>` controls how many values an option may store.
For a text or integer option, this turns the parsed result into a list.
For a flag, repeated occurrences increase the flag count.

Repeated regular options may be supplied by repeating the option name.
For positionals, the maximum controls how many positional values can be consumed for that definition.
If a positional list should consume values greedily before later positionals are considered, add
:cpp:enumerator:`OptionFlag::Greedy <erbsland::options::OptionFlag::Greedy>`.
Use that deliberately, because greedy positionals make the grammar less self-evident.

Defaults and Validation
=======================

:cpp:func:`setDefaultValue() <erbsland::options::OptionEditor::setDefaultValue>` stores a static default in the option
definition.
When the user omits the option, the parser inserts that default into the result before application code reads
:cpp:class:`OptionValues <erbsland::options::OptionValues>`.

:cpp:func:`setValidateFn() <erbsland::options::OptionEditor::setValidateFn>` installs a callback that runs after basic
type parsing and default insertion, but before post-parsing callbacks are called.
Throw :cpp:class:`OptionError <erbsland::options::OptionError>` with an
:cpp:class:`OptionErrorContext <erbsland::options::OptionErrorContext>` when the value is syntactically valid but not
valid for your domain.

Set a short title and a longer description when the distinction makes the error easier to scan.
The parser supplies a reason-based title when a callback only provides a description.
It also adds the current options, module, set, option, arguments and source index when they are known, so custom
validation errors receive the same usage and contextual help as built-in parser errors.

Use validation for rules such as allowed ranges, relationships between options, or values that must match an external
configuration.
Keep basic type expectations in the option type itself.

Flags
=====

:cpp:enum:`OptionFlag <erbsland::options::OptionFlag>` changes parser behavior.

``Disabled``
    Removes the option from parsing and generated help.
    A disabled option is different from a hidden option: hidden options still parse, disabled options do not.

``Required``
    Requires the option or positional argument to be present after defaults are applied.
    :cpp:func:`OptionEditor::setRequired() <erbsland::options::OptionEditor::setRequired>` is the common shorthand.

``Greedy``
    Allows a positional list option to consume values greedily.
    It is meaningful for positional list definitions; it is not a general replacement for explicit command syntax.

Customize Option Help
=====================

Every option stores :cpp:class:`OptionHelp <erbsland::options::OptionHelp>`.
For ordinary definitions, use the direct editor methods:
:cpp:func:`setHelpDescription() <erbsland::options::OptionEditor::setHelpDescription>`,
:cpp:func:`setHelpTitle() <erbsland::options::OptionEditor::setHelpTitle>`,
:cpp:func:`setHelpEpilog() <erbsland::options::OptionEditor::setHelpEpilog>`, and
:cpp:func:`setHelpVisibility() <erbsland::options::OptionEditor::setHelpVisibility>`.

The description is the main text shown next to the option.
The title is used by renderers when a short label is needed.
The epilog is available for renderers that expose trailing option details.
The value name changes the placeholder shown in help, for example ``<dier>`` instead of the generic ``<value>``.

Help Visibility
===============

:cpp:enum:`OptionHelpVisibility <erbsland::options::OptionHelpVisibility>` controls where enabled options appear.
It never disables parsing.

``Inherit``
    Uses the owning option set visibility, or ``Normal`` when no owner provides one.

``Hidden``
    Keeps the option parseable while hiding it from generated help.
    Use this for compatibility switches, debug toggles, or integration options that should not distract regular users.

``Normal``
    Shows the option in help for the active root or module.

``Overview``
    Also shows the option in root overview help when modules exist.
    Use this for global options that users may need before choosing a module.

``Usage``
    Behaves like ``Overview`` and renders regular options explicitly in the usage line.
    Use it sparingly for options that are central to understanding the command.
