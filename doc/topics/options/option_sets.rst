..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Options; Option Sets
    single: OptionSet

.. _options-option-sets:

************************
Command-Line Option Sets
************************

An :cpp:class:`OptionSet <erbsland::options::OptionSet>` groups options that belong to one part of an application.
This page explains how sets keep responsibilities separated, how set callbacks work, and how set help appears in
generated output.

Grouping Component Options with Option Sets
===========================================

Larger tools often have components that can explain their own settings better than a central command-line file can.
A sensor reader may own acquisition options, a report writer may own output options, and a diagnostics component may own
tracing switches.

An option set lets each component return a self-contained definition.
The application root collects the sets, and :cpp:class:`OptionManager <erbsland::options::OptionManager>` parses them in
one pass.
After parsing, each component can read the values it owns in its own post-parsing callback.

.. erbsland-demo::
    :source: option/OptionSets/main.cpp
    :exec: option/option_sets
    :sha256: c55743586265b13046b2a597fef468099b8d3bffc3366520ab5bb622512a61fb

.. code-block:: cpp

    /// Use `OptionSet` to let each component own its command line options and callbacks.
    ///
    /// Sets are registered together for one parse operation, but their callbacks stay separate.
    /// A set can prepare its definitions in a pre-parsing callback and can read all parsed values in a post-parsing
    /// callback, including values from other sets.
    auto createInstrumentSet() -> el::OptionSetPtr {
        auto instrumentSet = el::OptionSet::create();
        instrumentSet->setHelpTitle("Instrument"_el);
        instrumentSet->setHelpDescription("Options for the component that selects the instrument."_el);
        instrumentSet->addOption({"-q"_el, "--quiet"_el, "quiet"_el}).setHelpTitle("Reduces progress output."_el);
        instrumentSet->addOption({"-i"_el, "--instrument"_el, "instrument"_el})
            .setType(el::OptionType::Text)
            .setRequired()
            .setHelpTitle("Name of the laboratory instrument."_el);
        instrumentSet->setPostParsingFn([](const el::OptionValuesPtr &values) -> void {
            appSettings.quiet = values->getFlag("quiet"_el);
            appSettings.instrument = values->getText("instrument"_el);
        });
        return instrumentSet;
    }

    auto createReportSet() -> el::OptionSetPtr {
        auto reportSet = el::OptionSet::create();
        reportSet->setHelpTitle("Report"_el);
        reportSet->setHelpDescription("Options for the component that writes the report."_el);
        reportSet->addOption({"-t"_el, "--title"_el, "title"_el})
            .setType(el::OptionType::Text)
            .setDefaultValue("Calibration report"_el)
            .setHelpTitle("Title of the generated report."_el);
        reportSet->addOption({"-s"_el, "--samples"_el, "samples"_el})
            .setType(el::OptionType::Integer)
            .setDefaultValue(el::OptionInteger{4})
            .setHelpTitle("Number of samples to include."_el);
        reportSet->setPostParsingFn([](const el::OptionValuesPtr &values) -> void {
            appSettings.reportTitle = values->getText("title"_el);
            appSettings.sampleCount = values->getInteger("samples"_el);
        });
        return reportSet;
    }

    auto optionSets() -> el::ExitCode {
        auto options = el::Options::create();
        auto info = el::ApplicationInfo{};
        info.setApplicationName("Instrument Report"_el);
        info.setApplicationVersion(el::Version{0, 8, 0});
        options->setApplicationInfo(info);
        options->setHelpTitle("Instrument Report"_el);
        options->setHelpDescription("Builds a short report from independent option sets."_el);
        options->addSet(createInstrumentSet());
        options->addSet(createReportSet());

        auto manager = el::OptionManager{options};
        auto arguments =
            makeArgs({"instrumentos"_el, "--instrument"_el, "Interferómetro Norte"_el, "--title"_el, "Prueba matinal"_el});
        const auto values = manager.parseOrThrow(arguments);
        if (values == nullptr) {
            return el::ExitCode::success();
        }

        el::io::printLine("instrument: "_el, appSettings.instrument);
        el::io::printLine("title: "_el, appSettings.reportTitle);
        el::io::printLine("samples: "_el, appSettings.sampleCount);
        el::io::printLine("quiet: "_el, el::BooleanFormat::yesNo(), appSettings.quiet);
        return el::ExitCode::success();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0minstrument: Interferómetro Norte
        title: Prueba matinal
        samples: 4
        quiet: no␛[0m

.. erbsland-demo-end::

Option-Set Help Groups and Metadata
===================================

Set help metadata gives generated help a logical structure.
The set title becomes the option group title.
The description documents the group for renderers that expose set descriptions.
The epilog is available for renderers that support trailing group text.

Sets with the same non-empty title are displayed together.
Untitled sets use the default options heading.
Set visibility is inherited by options whose own visibility is ``Inherit``.

.. erbsland-demo::
    :source: option/OptionSetCallbacks/main.cpp
    :exec: option/option_set_callbacks --help
    :exec-2: option/option_set_callbacks
    :sha256: 4e65863bc586664878249ed9953e92f265c005ed56588ad41f7731496a45e2e7

.. code-block:: cpp

    /// Option sets let independent application parts own their command-line definitions and callbacks.
    ///
    /// A pre-parsing callback can adjust a set after configuration is loaded but before arguments are read.
    /// A post-parsing callback can copy parsed values into the component that owns the set.
    /// Disabled sets are removed from parsing and help, while ordinary help metadata becomes a group in generated help.
    [[nodiscard]] auto createObservationSet() -> el::OptionSetPtr {
        auto set = el::OptionSet::create();
        set->setHelpTitle("Observation"_el);
        set->setHelpDescription("Values the field team needs to plan a night round."_el);
        set->addOption({"-g"_el, "--area"_el, "area"_el})
            .setType(el::OptionType::Text)
            .setRequired()
            .setHelpTitle("Name of the observation area."_el);
        set->addOption({"-i"_el, "--interval"_el, "interval"_el})
            .setType(el::OptionType::Integer)
            .setHelpTitle("Minutes between two observations."_el);
        set->setPreParsingFn([](const el::OptionSetPtr &optionSet) -> void {
            optionSet->editOption("interval"_el).setDefaultValue(el::OptionInteger{15});
        });
        set->setPostParsingFn([](const el::OptionValuesPtr &values) -> void {
            settings.gebied = values->getText("area"_el);
            settings.interval = values->getInteger("interval"_el);
        });
        return set;
    }

    [[nodiscard]] auto createReportSet() -> el::OptionSetPtr {
        auto set = el::OptionSet::create();
        set->setHelpTitle("Report"_el);
        set->setHelpDescription("Values used only by the reporting component."_el);
        set->addOption({"-n"_el, "--note"_el, "note"_el})
            .setType(el::OptionType::Text)
            .setDefaultValue("no observations"_el)
            .setHelpTitle("Short text for the field report."_el);
        set->addOption({"-q"_el, "--quiet"_el, "quiet"_el}).setHelpTitle("Suppresses progress lines."_el);
        set->setPostParsingFn([](const el::OptionValuesPtr &values) -> void {
            settings.notitie = values->getText("note"_el);
            settings.stil = values->getFlag("quiet"_el);
        });
        return set;
    }

    [[nodiscard]] auto createLegacySet() -> el::OptionSetPtr {
        auto set = el::OptionSet::create();
        set->setHelpTitle("Legacy"_el);
        set->setFlags(el::OptionFlag::Disabled);
        set->addOption({"--old-route"_el, "old-route"_el}).setHelpTitle("Deprecated option that is no longer accepted."_el);
        return set;
    }

    void configureOptions(const el::OptionsPtr &options) {
        options->setHelpTitle("Night Logbook"_el);
        options->setHelpDescription("Combines options from several parts of an observation app."_el);
        options->addSet(createObservationSet());
        options->addSet(createReportSet());
        options->addSet(createLegacySet());
        options->addOption("round"_el).setRequired().setHelpTitle("Name of the planned night round."_el);
    }

    void printSettings() {
        el::io::printLine("area: "_el, settings.gebied);
        el::io::printLine("interval: "_el, settings.interval);
        el::io::printLine("note: "_el, settings.notitie);
        el::io::printLine("quiet: "_el, el::BooleanFormat::yesNo(), settings.stil);
    }

    class NightLogbookApp final : public el::Application {
    public:
        using Application::Application;

    protected:
        void initialize() override {
            enableTerminal();
            info().setApplicationName("Night Logbook"_el);
            info().setApplicationVersion(el::Version{0, 8, 0});
        }
        void registerCommandLineOptions(const el::OptionsPtr &options) override { configureOptions(options); }
        [[nodiscard]] auto main() -> el::ExitCode override {
            printSettings();
            return el::ExitCode::success();
        }
    };

    auto optionSetCallbacks() -> el::ExitCode {
        auto options = el::Options::create();
        auto info = el::ApplicationInfo{};
        info.setApplicationName("Night Logbook"_el);
        info.setApplicationVersion(el::Version{0, 8, 0});
        options->setApplicationInfo(info);
        options->setExecutablePath("night-logbook"_el);
        configureOptions(options);

        auto manager = el::OptionManager{options};
        auto arguments =
            makeArgs({"night-logbook"_el, "--area"_el, "duinrand"_el, "--note"_el, "vleermuizen actief"_el, "round-a"_el});
        const auto values = manager.parseOrThrow(arguments);
        if (values == nullptr) {
            return el::ExitCode::success();
        }

        printSettings();
        return el::ExitCode::success();
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ option/option_set_callbacks --help

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mCombines options from several parts of an observation app.

        ␛[1mUsage:
          ␛[92;49m␛[22moption_set_callbacks␛[39;49m ␛[90;49m[␛[96;49moptions␛[90;49m]␛[39;49m ␛[90;49m<␛[95;49mround␛[90;49m>

        ␛[39;49m␛[1mOptions:
          ␛[93;49m␛[22m-h␛[39;49m, ␛[96;49m--help␛[39;49m[=␛[90;49m<␛[92;49mkeyword␛[90;49m>␛[39;49m]  Display this help.
              ␛[96;49m--version␛[39;49m           Display version information.

        ␛[1mObservation:
        ␛[22mValues the field team needs to plan a night round.
          ␛[93;49m-g␛[39;49m, ␛[96;49m--area␛[39;49m ␛[90;49m<␛[92;49mvalue␛[90;49m>␛[39;49m        Name of the observation area.
          ␛[93;49m-i␛[39;49m, ␛[96;49m--interval␛[39;49m ␛[90;49m<␛[92;49minteger␛[90;49m>␛[39;49m  Minutes between two observations.
              ␛[90;49m<␛[95;49mround␛[90;49m>␛[39;49m               Name of the planned night round.

        ␛[1mReport:
        ␛[22mValues used only by the reporting component.
          ␛[93;49m-n␛[39;49m, ␛[96;49m--note␛[39;49m ␛[90;49m<␛[92;49mvalue␛[90;49m>␛[39;49m  Short text for the field report.
          ␛[93;49m-q␛[39;49m, ␛[96;49m--quiet␛[39;49m         Suppresses progress lines.␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ option/option_set_callbacks

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0marea: duinrand
        interval: 15
        note: vleermuizen actief
        quiet: no␛[0m

.. erbsland-demo-end::

Pre-Parsing Callbacks
=====================

:cpp:func:`setPreParsingFn() <erbsland::options::OptionSet::setPreParsingFn>` installs a callback that receives the set
before command-line values are read.
This is the right place to apply configuration that affects the definitions themselves.

Useful pre-parsing tasks include:

* setting defaults from a configuration file,
* enabling or disabling options based on a feature profile,
* adding validation callbacks that depend on runtime configuration,
* or hiding advanced options unless a component is active.

The callback should adjust definitions, not read parsed values.
No user values have been assigned yet.

Post-Parsing Callbacks
======================

:cpp:func:`setPostParsingFn() <erbsland::options::OptionSet::setPostParsingFn>` installs a callback that receives the
complete :cpp:class:`OptionValues <erbsland::options::OptionValues>` object after successful parsing.
Use it to copy settings into the component that owns the set, perform semantic checks that require multiple options, or
prepare application state before the main function starts.

Post callbacks can read values from any active set.
That is useful when one component needs a global switch such as ``--quiet`` while still keeping its own option
definitions local.

Set Flags
=========

An option set currently uses :cpp:enum:`OptionFlag <erbsland::options::OptionFlag>` for set-level behavior.
The meaningful set flag is :cpp:enumerator:`OptionFlag::Disabled <erbsland::options::OptionFlag::Disabled>`.

A disabled set is removed from parsing and generated help.
Use it when a whole component is unavailable in the current build, profile, or runtime environment.
Do not use a disabled set merely to hide options from help; use help visibility for that.

Help Visibility on Sets
=======================

Set help visibility controls the default visibility for options in that set.
Options can still override their own visibility.

This is especially useful for module overview help.
A global set marked ``Overview`` can keep important options visible at the root level even when most options are only
shown after a module is selected.
Use ``Hidden`` on a set when the whole group should stay parseable but should not appear in normal generated help.
