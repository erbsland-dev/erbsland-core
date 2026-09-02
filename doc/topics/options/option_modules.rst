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
    :source-sha256: f01d1bc1017506215848f19244267e999500fad41945952646a70ad1fc54111a

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
                .setHelpDescription("Prints additional progress details."_el)
                .setHelpVisibility(el::OptionHelpVisibility::Overview);

            auto scan = el::OptionModule::create("scan"_el);
            scan->setHelpTitle("Scan Instrument"_el);
            scan->setHelpDescription("Scans an instrument and stores its readings."_el);
            scan->addOption({"-r"_el, "--range"_el, "range"_el})
                .setType(el::OptionType::Integer)
                .setDefaultValue(el::OptionInteger{5})
                .setHelpDescription("Reading range around the starting point."_el);
            scan->addOption("instrument"_el).setRequired().setHelpDescription("Instrument to scan."_el);
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
                .setHelpDescription("Output format for the record."_el);
            render->addOption("instrument"_el).setRequired().setHelpDescription("Instrument to render."_el);
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

.. erbsland-ansi::
    :escape-char: ␛

    module: scan
    instrument: microscopio
    range: 7
    verbose: yes

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
    :source-sha256: ca4e644128468349786e2b6c5feb28d7a7ae29a7a0fa6534787a0f83eb440a2a

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
                .setHelpDescription("Prints extra progress information."_el)
                .setHelpVisibility(el::OptionHelpVisibility::Overview);
            options->addModule(createObserveModule());
            options->addModule(createExportModule());

            auto debug = el::OptionModule::create("debug"_el);
            debug->setHelpDescription("Hidden diagnostic module for developers."_el);
            debug->setHelpVisibility(el::OptionHelpVisibility::Hidden);
            debug->setMainFn([](el::OptionValuesPtr) -> el::ExitCode {
                el::io::printLine("debug module"_el);
                return el::ExitCode::success();
            });
            options->addModule(debug);
        }
    };

.. rubric:: ``$ option/option_module_callbacks --help``

.. erbsland-ansi::
    :escape-char: ␛

    Manages observation rounds through separate command modules.

    ␛[1mUsage:
      ␛[22;92moption_module_callbacks␛[39m ␛[90m<␛[92mmodule␛[90m>␛[39m ␛[90m[␛[96moptions␛[90m]

    ␛[39;1mModules:
      ␛[22;92mobserve␛[39m  Starts an observation round in a nighttime area.
      ␛[92mexport␛[39m   Writes an existing observation as text or a table.

    ␛[1mOptions:
      ␛[22;93m-h␛[39m, ␛[96m--help␛[39m     Display this help.
      ␛[93m-l␛[39m, ␛[96m--loud␛[39m     Prints extra progress information.
          ␛[96m--version␛[39m  Display version information.

.. rubric:: ``$ option/option_module_callbacks observe --help``

.. erbsland-ansi::
    :escape-char: ␛

    Starts an observation round in a nighttime area.

    ␛[1mUsage:
      ␛[22;92moption_module_callbacks␛[39m ␛[92mobserve␛[39m ␛[90m[␛[96moptions␛[90m]␛[39m ␛[90m<␛[95marea␛[90m>

    ␛[39;1mOptions:
      ␛[22m    ␛[90m<␛[95marea␛[90m>␛[39m                Area to observe.
      ␛[93m-d␛[39m, ␛[96m--duration␛[39m ␛[90m<␛[92minteger␛[90m>␛[39m  Duration of the round in minutes.
      ␛[93m-h␛[39m, ␛[96m--help␛[39m                Display this help.
      ␛[93m-l␛[39m, ␛[96m--loud␛[39m                Prints extra progress information.
          ␛[96m--version␛[39m             Display version information.

    Example: night-watch observe --duration 45 dune-edge

.. rubric:: ``$ option/option_module_callbacks observe --duration 45 dune-edge``

.. erbsland-ansi::
    :escape-char: ␛

    module: observe
    area: dune-edge
    duration: 45
    loud: no

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
