..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Options; Option Modules
    single: OptionModule
    single: Command Modules

.. _options-option-modules:

**************
Option Modules
**************

An :cpp:class:`OptionModule <erbsland::options::OptionModule>` models a command inside one executable.
This page explains when modules are useful, how module parsing works, and how module callbacks and main functions keep
command dispatch close to the command definition.

Use Modules for Command-Style Tools
===================================

Modules are meant for tools where the first ordinary command-line argument selects an action.
Examples include commands such as ``scan``, ``render``, ``observe``, or ``export``.

Once any modules are registered, the command line must select a module before ordinary options are parsed.
Only root ``--help`` and ``--version`` are accepted before the module name.
This rule keeps parsing unambiguous and makes generated root help a clear command overview.
When no argument is supplied, parsing returns ``DisplayModuleOverview`` and displays only the module usage line and
visible module list, without callbacks or definition validation.
Set ``OptionParserFlag::ErrorOnMissingModule`` when an empty command must remain an error.

.. erbsland-demo::
    :source: option/OptionModules/main.cpp
    :exec: option/option_modules scan --range 7 --verbose microscopio
    :sha256: effae2f842e422c3e8662f3bcfc2de9a14a95404e5d6522febad832afa2f2683

.. code-block:: cpp

    /// Use `OptionModule` for command-style tools where the first argument selects an action.
    ///
    /// A module owns its own option sets and can provide a main function.
    /// `Application` calls the selected module main function automatically after parsing succeeds.
    class InstrumentArchiveApp final : public el::Application {
    public:
        using Application::Application;

    protected:
        void initialize() override {
            enableTerminal();
            info().setApplicationName("Instrument Archive"_el);
            info().setApplicationVersion(el::Version{0, 8, 0});
            info().setLicenseText("Apache-2.0"_el);
        }
        void registerCommandLineOptions(const el::OptionsPtr &options) override {
            options->setHelpTitle("Instrument Archive"_el);
            options->setHelpDescription("Manages small records for scientific instruments."_el);
            options->addOption({"-v"_el, "--verbose"_el, "verbose"_el})
                .setHelpTitle("Prints additional progress details."_el)
                .setHelpVisibility(el::OptionHelpVisibility::Overview);

            auto scan = el::OptionModule::create("scan"_el);
            scan->setHelpTitle("Scan Instrument"_el);
            scan->setHelpDescription("Scans an instrument and stores its readings."_el);
            scan->addOption({"-r"_el, "--range"_el, "range"_el})
                .setType(el::OptionType::Integer)
                .setDefaultValue(el::OptionInteger{5})
                .setHelpTitle("Reading range around the starting point."_el);
            scan->addOption("instrument"_el).setRequired().setHelpTitle("Instrument to scan."_el);
            scan->setMainFn([](const el::OptionValuesPtr &values) -> el::ExitCode {
                el::io::printLine("module: "_el, values->moduleName());
                el::io::printLine("instrument: "_el, values->getText("instrument"_el));
                el::io::printLine("range: "_el, values->getInteger("range"_el));
                el::io::printLine("verbose: "_el, el::BooleanFormat::yesNo(), values->getFlag("verbose"_el));
                return el::ExitCode::success();
            });
            options->addModule(scan);

            auto render = el::OptionModule::create("render"_el);
            render->setHelpTitle("Render Record"_el);
            render->setHelpDescription("Renders a record as text, a chart, or data."_el);
            render->addOption({"-f"_el, "--format"_el, "format"_el})
                .addChoice("text"_el)
                .addChoice("chart"_el)
                .addChoice("data"_el)
                .setDefaultValue("text"_el)
                .setHelpTitle("Output format for the record."_el);
            render->addOption("instrument"_el).setRequired().setHelpTitle("Instrument to render."_el);
            render->setMainFn([](const el::OptionValuesPtr &values) -> el::ExitCode {
                el::io::printLine("module: "_el, values->moduleName());
                el::io::printLine("instrument: "_el, values->getText("instrument"_el));
                el::io::printLine("format: "_el, values->getText("format"_el));
                return el::ExitCode::success();
            });
            options->addModule(render);
        }
    };

    /// This main method leaves module dispatch to `Application`.
    auto main(const int argc, char *argv[]) -> int {
        auto app = InstrumentArchiveApp{argc, argv};
        return app.run();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mmodule: scan
        instrument: microscopio
        range: 7
        verbose: yes␛[0m

.. erbsland-demo-end::

Root Help and Module Help
=========================

Root help lists available modules and global options that are visible at overview level.
Module help lists the selected module, global options, and module-local options.

Use root help to help users choose a command.
Use module help to explain how one command is configured.

.. erbsland-demo::
    :source: option/OptionModuleCallbacks/main.cpp
    :exec: option/option_module_callbacks --help
    :exec-2: option/option_module_callbacks observe --help
    :exec-3: option/option_module_callbacks observe --duration 45 dune-edge
    :sha256: fc8fc2d6f34af9f0f77c3a3a929a906be4f47df986a83fd58a1ceefb1d6411c6

.. code-block:: cpp

    /// Option modules model command-style applications where the first ordinary argument selects an action.
    ///
    /// Each module owns module-local options, callbacks, help metadata, and an optional main function.
    /// Root options stay available after the module is selected.
    /// `Application` automatically calls the selected module main function after parsing succeeds.
    class NightWatchApp final : public el::Application {
    public:
        using Application::Application;

    protected:
        void initialize() override {
            enableTerminal();
            info().setApplicationName("Night Watch Modules"_el);
            info().setApplicationVersion(el::Version{0, 8, 0});
        }
        void registerCommandLineOptions(const el::OptionsPtr &options) override {
            options->setHelpTitle("Night Watch Modules"_el);
            options->setHelpDescription("Manages observation rounds through separate command modules."_el);
            options->addOption({"-l"_el, "--loud"_el, "loud"_el})
                .setHelpTitle("Prints extra progress information."_el)
                .setHelpVisibility(el::OptionHelpVisibility::Overview);
            options->addModule(createObserveModule());
            options->addModule(createExportModule());

            auto debug = el::OptionModule::create("debug"_el);
            debug->setHelpTitle("Hidden diagnostic module for developers."_el);
            debug->setHelpVisibility(el::OptionHelpVisibility::Hidden);
            debug->setMainFn([](el::OptionValuesPtr) -> el::ExitCode {
                el::io::printLine("debug module"_el);
                return el::ExitCode::success();
            });
            options->addModule(debug);
        }
    };

.. erbsland-demo-run::

    .. code-block:: console

        $ option/option_module_callbacks --help

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mManages observation rounds through separate command modules.

        ␛[1mUsage:
          ␛[92;49m␛[22moption_module_callbacks␛[39;49m ␛[90;49m<␛[92;49mmodule␛[90;49m>␛[39;49m ␛[90;49m[␛[96;49moptions␛[90;49m]

        ␛[39;49m␛[1mModules:
          ␛[92;49m␛[22mexport␛[39;49m   Export Logbook
          ␛[92;49mobserve␛[39;49m  Observe Area

        ␛[1mOptions:
          ␛[93;49m␛[22m-h␛[39;49m, ␛[96;49m--help␛[39;49m[=␛[90;49m<␛[92;49mkeyword␛[90;49m>␛[39;49m]  Display this help.
          ␛[93;49m-l␛[39;49m, ␛[96;49m--loud␛[39;49m              Prints extra progress information.
              ␛[96;49m--version␛[39;49m           Display version information.␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ option/option_module_callbacks observe --help

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m
        ␛[1mObserve␛[22m ␛[1mArea
        ␛[92;49m␛[22mobserve

        ␛[39;49m␛[1mUsage:
          ␛[92;49m␛[22moption_module_callbacks␛[39;49m ␛[92;49mobserve␛[39;49m ␛[90;49m[␛[96;49moptions␛[90;49m]␛[39;49m ␛[90;49m<␛[95;49marea␛[90;49m>

        ␛[39;49m␛[1mDescription:
        ␛[22mStarts an observation round in a nighttime area.

        ␛[1mOptions:
          ␛[22m    ␛[90;49m<␛[95;49marea␛[90;49m>␛[39;49m                Area to observe.
          ␛[93;49m-d␛[39;49m, ␛[96;49m--duration␛[39;49m ␛[90;49m<␛[92;49minteger␛[90;49m>␛[39;49m  Duration of the round in minutes.
          ␛[93;49m-h␛[39;49m, ␛[96;49m--help␛[39;49m[=␛[90;49m<␛[92;49mkeyword␛[90;49m>␛[39;49m]    Display this help.
          ␛[93;49m-l␛[39;49m, ␛[96;49m--loud␛[39;49m                Prints extra progress information.
              ␛[96;49m--version␛[39;49m             Display version information.

        Example: night-watch observe --duration 45 dune-edge␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ option/option_module_callbacks observe --duration 45 dune-edge

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mmodule: observe
        area: dune-edge
        duration: 45
        loud: no␛[0m

.. erbsland-demo-end::

Add Options and Sets to a Module
================================

An option module can receive options directly through
:cpp:func:`addOption() <erbsland::options::OptionModule::addOption>`.
For small commands this keeps code compact.

For larger commands, add one or more :cpp:class:`OptionSet <erbsland::options::OptionSet>` objects with
:cpp:func:`addSet() <erbsland::options::OptionModule::addSet>`.
This gives module-local components the same ownership model as root-level components.

Root option sets remain active after a module is selected.
That lets global options such as ``--verbose`` or ``--config`` work consistently across commands.

Module Callbacks
================

:cpp:func:`setPreParsingFn() <erbsland::options::OptionModule::setPreParsingFn>` installs a callback that receives the
selected module before command-line values are parsed.
Use it to adjust module-local definitions based on configuration or runtime capabilities.

:cpp:func:`setPostParsingFn() <erbsland::options::OptionModule::setPostParsingFn>` receives the complete parsed
:cpp:class:`OptionValues <erbsland::options::OptionValues>` object after successful parsing.
Use it for module-local semantic checks or for preparing state shared by the module main function.

Module Main Functions
=====================

The module main function has this shape:

.. code-block:: cpp

    std::function<el::ExitCode(el::OptionValuesPtr)>

When a selected module has a main function, :cpp:class:`Application <erbsland::core::Application>` calls it from the
default application ``main()`` implementation.
This keeps command dispatch inside the module definition and avoids a separate ``if command == ...`` block.

If your application needs custom dispatch, omit module main functions and read
:cpp:func:`OptionValues::moduleName() <erbsland::options::OptionValues::moduleName>` yourself.

Module Help Metadata and Visibility
===================================

Module help metadata works like root help metadata:

* the description is shown in the root module list and as the module-help summary,
* the title is a short display title,
* the epilog appears after module-specific help output,
* and help visibility controls whether the module appears in root help.

There is currently no :cpp:enum:`OptionFlag <erbsland::options::OptionFlag>` field on
:cpp:class:`OptionModule <erbsland::options::OptionModule>`.
To hide a module from help while keeping it callable, use ``OptionHelpVisibility::Hidden``.
To make a command unavailable, do not add that module to the root options for the current configuration.
