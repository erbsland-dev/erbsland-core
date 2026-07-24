..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Command Line Options System
    single: Options
    single: Command Line Options
    single: OptionManager
    single: OptionSet
    single: OptionModule
    single: OptionValues
    single: Application; Command Line Options
    single: Help Output; Command Line Options

.. _options-overview:

***********************
Options System Overview
***********************

This page gives you a practical overview of the command line options system in Erbsland Core.
You will learn how to register options, how parsing works, how to structure larger option surfaces with sets and
modules, and how parsed values, help output, version output, and errors are handled.

The option system is designed for small tools as well as larger command-line applications.
A tiny program can register a few options directly on :cpp:class:`Options <erbsland::options::Options>`.
A larger application can let each component return an :cpp:class:`OptionSet <erbsland::options::OptionSet>`.
A command-style tool can use :cpp:class:`OptionModule <erbsland::options::OptionModule>` to model subcommands such as
``scan``, ``render``, or ``publish``.
All of these definitions are parsed by :cpp:class:`OptionManager <erbsland::options::OptionManager>` and produce an
:cpp:class:`OptionValues <erbsland::options::OptionValues>` object.

How the Pieces Fit Together
===========================

At startup, an application builds an option definition tree.
The root object is :cpp:class:`Options <erbsland::options::Options>`.
It owns built-in ``-h``, ``--help``, and ``--version`` options, global option sets, and optional modules.

Parsing then follows this sequence:

* The executable path from ``argv[0]`` is stored for usage text.
* If modules are registered, the first command-line argument must select one of them, except for root ``--help`` and
  ``--version``.
* The active option sets are collected from the root and, if selected, the module.
* Pre-parsing callbacks may adjust option definitions before command-line values are read.
* Options and positional arguments are parsed, defaults are applied, required options are checked, validators run, and
  post-parsing callbacks are called.
* On success, :cpp:class:`OptionValues <erbsland::options::OptionValues>` stores the parsed values under every accepted
  option name and lookup alias.

:cpp:class:`Application <erbsland::core::Application>` wraps this flow for normal command-line tools.
It calls ``registerCommandLineOptions()``, parses the command line, displays help/version/error output when needed, and
then calls your application ``main()`` only after parsing succeeds.
Use :cpp:class:`OptionManager <erbsland::options::OptionManager>` directly when you need to handle the result yourself.

Command Line Syntax
===================

The parser accepts a small, predictable command-line grammar.
Long option names are case-insensitive and use ``--name`` or ``--name=value``.
Short option names are case-sensitive and use ``-n`` or ``-n=value``.
Short flags may be grouped, so ``-abc`` is equivalent to ``-a -b -c`` when all three options are flags.

Values can follow an option as the next argument or be attached with ``=``.
Use the attached form for values that start with a dash, for example ``--count=-1``.
Flags are false when absent and true when written without a value.
They also accept the ASCII-case-insensitive ELCL literals ``true``, ``on``, ``yes``, ``enabled``, ``false``, ``off``,
``no``, and ``disabled``.
Use either an attached value such as ``--cleanup=false`` or a separate recognized value such as ``--cleanup false``.
A separate argument is consumed only if it is one of these literals; other text remains available to positional
arguments.
The argument ``--`` stops option parsing; all later arguments are treated as positional values.
The built-in ``-h``, ``--help``, and ``--version`` requests stop normal parsing before user callbacks are called.
Applications that need one of these names for their own protocol can disable the help or version request individually
with :cpp:enum:`OptionParserFlag <erbsland::options::OptionParserFlag>` and then register an ordinary option under the
released name:

.. code-block:: cpp

    options->setParserFlag(el::OptionParserFlag::DisableVersion);
    options->addOption({"--version"_el, "language-version"_el})
        .setType(el::OptionType::Text);

Applications whose positional grammar may use boolean-looking words can restore valueless flag behavior for the complete
options tree:

.. code-block:: cpp

    options->setParserFlag(el::OptionParserFlag::DisableBooleanValues);

This compatibility flag applies to ordinary flags, modules, option sets, and built-in help/version requests.

If modules are present, no ordinary options may appear before the module name.
This makes module selection unambiguous:

.. code-block:: text

    forest-atlas --help
    forest-atlas collect --sensor north route-a
    forest-atlas publish --format atlas report-a

Using Application
=================

Use :cpp:class:`Application <erbsland::core::Application>` for regular executables.
Override ``initialize()`` for application metadata and terminal setup, override ``registerCommandLineOptions()`` to
register the command line interface, and read ``optionValues()`` in ``main()``.
When the user asks for help or version information, ``main()`` is not called.
When parsing fails, :cpp:class:`Application <erbsland::core::Application>` renders the diagnostic and exits with an
error code.

.. erbsland-demo::
    :source: option/ApplicationRegistration/main.cpp
    :exec: option/option_application --help
    :exec-2: option/option_application --version
    :exec-3: option/option_application --instrument Prisma-7 --gain 4 cristal-azul
    :source-sha256: 24a5eade0a60fa7eb85d01eb1045c8d475014ee84fe8dab9ed13c286737ae901

.. code-block:: cpp

    /// Derive from `Application` when command line options belong to the executable lifecycle.
    ///
    /// Register options in `registerCommandLineOptions()`, then read the parsed `OptionValues` in `main()`.
    /// `Application` handles `--help`, `-h`, `--version`, option errors, and terminal rendering before your main function
    /// is called.
    class SpectrometerApp final : public el::Application {
    public:
        using Application::Application;

    protected:
        void initialize() override {
            info().setApplicationName("Prism Laboratory"_el);
            info().setApplicationVersion(el::Version{0, 8, 0});
            info().setAuthorName("Erbsland DEV"_el);
            info().setLicenseText("Apache-2.0"_el);
        }
        void registerCommandLineOptions(const el::OptionsPtr &options) override {
            options->setHelpTitle("Prism Laboratory"_el);
            options->setHelpDescription("Configures a short measurement with a bench spectrometer."_el);
            options->setHelpEpilog(
                "Please use the correct names from the manual and the research datasheet "
                "to select the instruments and samples with the command line options."_el);
            options->addOption({"-i"_el, "--instrument"_el, "instrument"_el})
                .setType(el::OptionType::Text)
                .setRequired()
                .setHelpDescription("Name of the instrument that will perform the measurement."_el);
            options->addOption({"-g"_el, "--gain"_el, "gain"_el})
                .setType(el::OptionType::Integer)
                .setDefaultValue(el::OptionInteger{2})
                .setHelpDescription("Detector gain between 1 and 9."_el);
            options->addOption({"-d"_el, "--dark-frame"_el, "dark-frame"_el})
                .setType(el::OptionType::Flag)
                .setHelpDescription("Captures a dark frame before measuring the sample."_el);
            options->addOption("sample"_el)
                .setRequired()
                .setHelpDescription("Sample placed in front of the instrument."_el);
        }
        [[nodiscard]] auto main() -> el::ExitCode override {
            const auto values = optionValues();
            el::io::printLine("instrument: "_el, values->getText("instrument"_el));
            el::io::printLine("sample: "_el, values->getText("sample"_el));
            el::io::printLine("gain: "_el, values->getInteger("gain"_el));
            el::io::printLine("dark frame: "_el, el::BooleanFormat::yesNo(), values->getFlag("dark-frame"_el));
            return el::ExitCode::success();
        }
    };

    /// This simple main method creates the application and lets it run the full lifecycle.
    auto main(const int argc, char *argv[]) -> int {
        auto app = SpectrometerApp{argc, argv};
        app.enableTerminal();
        return app.run();
    }

.. rubric:: ``$ option/option_application --help``

.. erbsland-ansi::
    :escape-char: ␛

    Configures a short measurement with a bench spectrometer.

    ␛[1mUsage:
      ␛[22;92moption_application␛[39m ␛[90m[␛[96moptions␛[90m]␛[39m ␛[90m<␛[95msample␛[90m>

    ␛[39;1mOptions:
      ␛[22;93m-d␛[39m, ␛[96m--dark-frame␛[39m          Captures a dark frame before measuring the sample.
      ␛[93m-g␛[39m, ␛[96m--gain␛[39m ␛[90m<␛[92minteger␛[90m>␛[39m      Detector gain between 1 and 9.
      ␛[93m-h␛[39m, ␛[96m--help␛[39m                Display this help.
      ␛[93m-i␛[39m, ␛[96m--instrument␛[39m ␛[90m<␛[92mvalue␛[90m>␛[39m  Name of the instrument that will perform the measurement.
          ␛[90m<␛[95msample␛[90m>␛[39m              Sample placed in front of the instrument.
          ␛[96m--version␛[39m             Display version information.

    Please use the correct names from the manual and the research datasheet to select the
    instruments and samples with the command line options.

.. rubric:: ``$ option/option_application --version``

.. erbsland-ansi::
    :escape-char: ␛

    ␛[92mPrism Laboratory␛[39m 0.8.0
      Author:   Erbsland DEV
      License:  Apache-2.0

.. rubric:: ``$ option/option_application --instrument Prisma-7 --gain 4 cristal-azul``

.. erbsland-ansi::
    :escape-char: ␛

    instrument: Prisma-7
    sample: cristal-azul
    gain: 4
    dark frame: no

.. erbsland-demo-end::

Using OptionManager Directly
============================

:cpp:class:`OptionManager <erbsland::options::OptionManager>` is the lower-level parser and display orchestrator.
It is the right tool when parsing does not belong to the global application lifecycle, when you are testing option
definitions, or when your program needs custom routing for help and errors.

Call :cpp:func:`parse() <erbsland::options::OptionManager::parse>` when you want an
:cpp:class:`OptionResult <erbsland::options::OptionResult>` and will inspect the result status yourself.
Call :cpp:func:`parseOrThrow() <erbsland::options::OptionManager::parseOrThrow>` when errors should become
:cpp:class:`OptionError <erbsland::options::OptionError>` exceptions.
Both methods can parse already converted :cpp:type:`CommandLineArguments <erbsland::core::CommandLineArguments>` or raw
``argc`` /``argv`` pairs.
The converted-list overloads require mutable lvalues because sensitive option values are replaced with five stars before
parsing returns.
When :cpp:class:`Application <erbsland::core::Application>` owns parsing, it also masks the borrowed native ``argv``
buffers in place without changing their length.

.. erbsland-demo::
    :source: option/ManualParsing/main.cpp
    :exec: option/option_manual_parsing
    :source-sha256: f4a4abaae277484d4c26b91c768a867a69a95c62c017ec20dc4fc3645874e8ef

.. code-block:: cpp

    /// Use `OptionManager` directly when option parsing is only one part of a larger startup flow.
    ///
    /// Manual parsing returns an `OptionResult` instead of immediately exiting or throwing.
    /// This is useful for libraries, test tools, embedded command interpreters, or applications that need to route help,
    /// version, and error documents through their own output system.
    auto createManualOptions() -> el::OptionsPtr {
        auto options = el::Options::create();
        auto info = el::ApplicationInfo{};
        info.setApplicationName("Photometry Notebook"_el);
        info.setApplicationVersion(el::Version{0, 8, 0});
        options->setApplicationInfo(info);
        options->setHelpTitle("Photometry Notebook"_el);
        options->setHelpDescription("Records a short note from a photometer measurement."_el);
        options->addOption({"-q"_el, "--quiet"_el, "quiet"_el}).setHelpDescription("Reduces progress output."_el);
        options->addOption({"-n"_el, "--note"_el, "note"_el})
            .setType(el::OptionType::Text)
            .setRequired()
            .setHelpDescription("Text of the laboratory note."_el);
        options->addOption({"-r"_el, "--repeat"_el, "repeat"_el})
            .setType(el::OptionType::Integer)
            .setDefaultValue(el::OptionInteger{1})
            .setHelpDescription("Number of times to repeat the note in the report."_el);
        return options;
    }

    auto manualParsing() -> el::ExitCode {
        auto manager = el::OptionManager{createManualOptions()};
        auto args = makeArgs({"fotometria"_el, "--note"_el, "Lectura estable en lámpara azul"_el, "--repeat=2"_el});
        const auto result = manager.parse(args);

        if (result.status() != el::OptionResultStatus::Success) {
            if (result.status() == el::OptionResultStatus::DisplayHelp) {
                el::io::printLine(manager.helpDocument(result.values()->moduleName()).toString());
            } else if (result.status() == el::OptionResultStatus::DisplayVersion) {
                el::io::printLine(manager.versionDocument(result.values()->moduleName()).toString());
            } else if (result.errorContext().has_value()) {
                el::io::printLine(manager.errorDocument(result.errorContext().value()).toString());
            }
            return el::ExitCode::failure();
        }

        const auto values = result.values();
        el::io::printLine("note: "_el, values->getText("note"_el));
        el::io::printLine("repetitions: "_el, values->getInteger("repeat"_el));
        el::io::printLine("quiet: "_el, el::BooleanFormat::yesNo(), values->getFlag("quiet"_el));
        return el::ExitCode::success();
    }

.. erbsland-ansi::
    :escape-char: ␛

    note: Lectura estable en lámpara azul
    repetitions: 2
    quiet: no

.. erbsland-demo-end::

Where to Go Next
================

The remaining topic pages focus on one concept at a time:

* :doc:`options` explains individual option definitions, names, value types, choices, defaults, validation, flags, and
  help visibility.
* :doc:`option_sets` explains how larger applications let individual components provide their own options and
  callbacks.
* :doc:`option_modules` explains command-style tools where the first argument selects a module.
* :doc:`option_values` explains the parsed value map returned by the parser.
* :doc:`customizing_help_output` explains generated help documents, terminal rendering, custom styles, and display
  wording.
