..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Application; Procedural
    single: Application; main Override
    single: Application; optionValues

********************************
Building Procedural Applications
********************************

A procedural application performs one synchronous sequence after its inputs have been validated.
Deriving from :cpp:class:`Application <erbsland::core::Application>` gives each lifecycle concern a natural method while
an overridden ``main()`` keeps the actual work easy to read.

Separating Setup, Options, and Work
===================================

Initialization establishes process-wide metadata and services.
Command-line registration describes input without parsing it manually.
After parsing succeeds, ``handleCommandLineOptions(values)`` can validate inputs and store option-dependent settings.
The main method then performs the synchronous work and returns the final ``ExitCode``.

.. erbsland-demo::
    :source: core/ProceduralApplication/main.cpp
    :exec: core/core_procedural_application --dry-run staging
    :exec-2: core/core_procedural_application --help
    :sha256: 1c9d938bbf743f48ddd08ab365098e310544f13bc639d05c161d8b1110a5770f

.. code-block:: cpp

    /// Override `Application::main()` for a synchronous workflow with one clear result.
    ///
    /// `Application` initializes the process, parses the command line, and handles framework errors before and after this
    /// method. The overridden main method reads the validated values and returns the process exit code.
    class ReportApplication final : public el::Application {
    public:
        using Application::Application;

    protected: // implement Application
        void initialize() override {
            info().setApplicationName("Deployment Report"_el);
            info().setApplicationVersion(el::Version{1, 0, 0});
        }
        void registerCommandLineOptions(const el::OptionsPtr &options) override {
            options->addOption("environment"_el).setRequired().setHelpTitle("Environment summarized by the report."_el);
            options->addOption({"-d"_el, "--dry-run"_el, "dry-run"_el})
                .setHelpTitle("Marks the report as a simulation."_el);
        }
        [[nodiscard]] auto main() -> el::ExitCode override {
            el::io::printLine("environment: "_el, optionValues()->getText("environment"_el));
            el::io::printLine("mode: "_el, optionValues()->getFlag("dry-run"_el) ? "simulation"_el : "deployment"_el);
            return el::ExitCode::success();
        }
    };

    /// Create the procedural application and run its complete lifecycle.
    auto main(const int argc, char *argv[]) -> int {
        auto app = ReportApplication{argc, argv};
        return app.run();
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ core/core_procedural_application --dry-run staging

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0menvironment: staging
        mode: simulation␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ core/core_procedural_application --help

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mUsage:
        core_procedural_application [options] <environment>
        Options:
        -d, --dry-run           Marks the report as a simulation.
        <environment>           Environment summarized by the report.
        -h, --help[=<keyword>]  Display this help.
            --version           Display version information.␛[0m

.. erbsland-demo-end::

Using Parsed Values
===================

:cpp:func:`optionValues() <erbsland::core::Application::optionValues>` contains validated values, defaults, and the
selected option module.
The framework has already handled ``--help``, ``--version``, missing required values, and validation failures before the
override is reached.

For a larger workflow, keep ``main()`` as the readable top-level sequence and move individual operations into private
methods or collaborating classes.
This preserves the lifecycle boundary without turning the application subclass into every subsystem at once.

Overriding Application Main Dispatch
====================================

The procedural override replaces the default main dispatch.
It therefore does not automatically call a selected option-module main function, a function installed with
``setMainFn()``, start application parts, or enter the event loop.

This is the desired behavior for a synchronous tool.
If the method needs one of those facilities, call the corresponding base or protected operation explicitly rather than
assuming it still happens in the background.

Cleanup and Errors
==================

``Application::run()`` calls ``cleanup()`` after the returned exit code and after handled Erbsland Core exceptions.
Cleanup must not block or throw.
Use ordinary local ownership within ``main()`` for resources that should be released before application cleanup, and use
the cleanup override only for application-lifetime state.
