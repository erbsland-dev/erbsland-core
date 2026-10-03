..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Application; Function Based
    single: Application; setInitializeFn
    single: Application; setMainFn

*****************************************
Writing Small Applications with Functions
*****************************************

A dedicated application class is unnecessary when an executable has one short initialization step and one short body.
:cpp:func:`setInitializeFn() <erbsland::core::Application::setInitializeFn>` and
:cpp:func:`setMainFn() <erbsland::core::Application::setMainFn>` keep that program in one function while preserving the
complete application lifecycle.

Configuring the Complete Lifecycle
==================================

The initialization function runs first inside ``Application::run()``.
It can set application metadata, enable terminal support, and add command-line options through the application's public
services.
The main function runs only after parsing succeeds, so it can read ``optionValues()`` without handling help or parser
errors itself.

.. erbsland-demo::
    :source: core/FunctionApplication/main.cpp
    :exec: core/core_function_application --environment production
    :exec-2: core/core_function_application --help
    :sha256: cfecb6202cd0d39910a3c2a132b1cade9ecab2854f17cbe87b59a5d6adc62118

.. code-block:: cpp

    /// Configure a small application with functions when a dedicated class would add no useful structure.
    ///
    /// The initialization function runs at the beginning of `Application::run()`.
    /// The main function runs after command-line parsing succeeds and can use the parsed option values.
    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.setInitializeFn([&app]() -> void {
            app.info().setApplicationName("Deployment Label"_el);
            app.info().setApplicationVersion(el::Version{1, 0, 0});
            app.options()
                ->addOption({"-e"_el, "--environment"_el, "environment"_el})
                .setType(el::OptionType::Text)
                .setDefaultValue("staging"_el)
                .setHelpTitle("Environment written into the deployment label."_el);
        });
        app.setMainFn([&app]() -> el::ExitCode {
            el::io::printLine("deployment environment: "_el, app.optionValues()->getText("environment"_el));
            return el::ExitCode::success();
        });
        return app.run();
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ core/core_function_application --environment production

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mdeployment environment: production␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ core/core_function_application --help

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mUsage:
        core_function_application [options]
        Options:
        -e, --environment <value> Environment written into the deployment label.
        -h, --help[=<keyword>]    Display this help.
            --version             Display version information.␛[0m

.. erbsland-demo-end::

Capturing the Application Safely
================================

The application object outlives both callbacks because ``run()`` completes before the object leaves ``main()``.
Capturing it by reference is therefore a direct way to reach metadata, options, parsed values, or shared services.
Objects captured by either callback need the same lifetime guarantee.

The registered initialization function belongs to the default virtual ``initialize()`` implementation.
A derived override replaces it unless that override calls ``Application::initialize()``.
Mixing both mechanisms is rarely helpful; a subclass should normally express initialization directly through its
override.

Knowing When to Introduce a Class
=================================

Function customization deliberately covers only initialization and main execution.
When the program needs separate command-line registration, parsing transformation, cleanup state, several helper
methods, or asynchronous callbacks that share members, an application subclass makes those relationships clearer.

Exceptions from either callback remain inside the normal ``Application::run()`` boundary.
An Erbsland Core exception is reported after cleanup, while the returned ``ExitCode`` becomes the process result.
