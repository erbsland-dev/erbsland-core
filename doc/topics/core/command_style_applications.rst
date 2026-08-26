..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Application; Command Style
    single: Application; Option Modules
    single: OptionModule; Application Dispatch

***********************************
Building Command-Style Applications
***********************************

A command-style executable chooses an action such as ``list``, ``add``, or ``remove`` before parsing the options that
belong to that action.
:cpp:class:`OptionModule <erbsland::options::OptionModule>` models this shape directly and lets the default application
main dispatch the selected command.

Giving Every Action Its Own Module
==================================

An action object can create its option module and connect the module's main function to its own behavior.
The application owns the action objects and adds their modules during command-line registration.
This keeps option definitions, help text, and execution close together without building a manual command switch.

.. erbsland-demo::
    :source: core/CommandApplication/main.cpp
    :exec: core/core_command_application add api
    :exec-2: core/core_command_application list
    :exec-3: core/core_command_application --help
    :source-sha256: 5185c6c831c65747cf98839ddb0eb23402e226ffa78851f24d1cd816462fd7c9

.. code-block:: cpp

    /// Keep the behavior of the `list` command next to the module that describes its command line.
    class ListAction final {
    public:
        [[nodiscard]] auto module() -> el::OptionModulePtr {
            auto result = el::OptionModule::create("list"_el);
            result->setHelpDescription("Lists the entries in the deployment catalog."_el);
            result->setMainFn([this](el::OptionValuesPtr values) -> el::ExitCode { return main(std::move(values)); });
            return result;
        }

    private:
        [[nodiscard]] auto main([[maybe_unused]] const el::OptionValuesPtr &values) -> el::ExitCode {
            el::io::printLine("catalog entries: api, worker"_el);
            return el::ExitCode::success();
        }
    };

    /// Keep the behavior and options of the `add` command in one action object.
    class AddAction final {
    public:
        [[nodiscard]] auto module() -> el::OptionModulePtr {
            auto result = el::OptionModule::create("add"_el);
            result->setHelpDescription("Adds an entry to the deployment catalog."_el);
            result->addOption("name"_el).setRequired().setHelpDescription("Name of the new catalog entry."_el);
            result->setMainFn([this](el::OptionValuesPtr values) -> el::ExitCode { return main(std::move(values)); });
            return result;
        }

    private:
        [[nodiscard]] auto main(const el::OptionValuesPtr &values) -> el::ExitCode {
            el::io::printLine("adding catalog entry: "_el, values->getText("name"_el));
            return el::ExitCode::success();
        }
    };

    /// Keep the behavior and options of the `remove` command in one action object.
    class RemoveAction final {
    public:
        [[nodiscard]] auto module() -> el::OptionModulePtr {
            auto result = el::OptionModule::create("remove"_el);
            result->setHelpDescription("Removes an entry from the deployment catalog."_el);
            result->addOption("name"_el).setRequired().setHelpDescription("Name of the catalog entry to remove."_el);
            result->setMainFn([this](el::OptionValuesPtr values) -> el::ExitCode { return main(std::move(values)); });
            return result;
        }

    private:
        [[nodiscard]] auto main(const el::OptionValuesPtr &values) -> el::ExitCode {
            el::io::printLine("removing catalog entry: "_el, values->getText("name"_el));
            return el::ExitCode::success();
        }
    };

    /// Use option modules to dispatch a command-style application without a manual command switch.
    ///
    /// The application owns every action object so callbacks that capture `this` remain valid for the complete run.
    /// It deliberately keeps the base `Application::main()` implementation, which calls the selected module main function.
    class CatalogApplication final : public el::Application {
    public:
        using Application::Application;

    protected: // implement Application
        void initialize() override {
            info().setApplicationName("Deployment Catalog"_el);
            info().setApplicationVersion(el::Version{1, 0, 0});
        }
        void registerCommandLineOptions(const el::OptionsPtr &options) override {
            options->setHelpDescription("Manages a small catalog of deployable services."_el);
            options->addModule(_listAction.module());
            options->addModule(_addAction.module());
            options->addModule(_removeAction.module());
        }

    private:
        ListAction _listAction;
        AddAction _addAction;
        RemoveAction _removeAction;
    };

    /// Create the command application and let the base main implementation dispatch the selected module.
    auto main(const int argc, char *argv[]) -> int {
        auto app = CatalogApplication{argc, argv};
        return app.run();
    }

.. rubric:: ``$ core/core_command_application add api``

.. erbsland-ansi::
    :escape-char: ␛

    adding catalog entry: api

.. rubric:: ``$ core/core_command_application list``

.. erbsland-ansi::
    :escape-char: ␛

    catalog entries: api, worker

.. rubric:: ``$ core/core_command_application --help``

.. erbsland-ansi::
    :escape-char: ␛

    Manages a small catalog of deployable services.
    Usage:
    core_command_application <module> [options]
    Modules:
    list        Lists the entries in the deployment catalog.
    add         Adds an entry to the deployment catalog.
    remove      Removes an entry from the deployment catalog.
    Options:
    -h, --help[=<boolean>]    Display this help.
        --version[=<boolean>] Display version information.

.. erbsland-demo-end::

Preserving Callback Lifetimes
=============================

The module callbacks capture their action object.
Making those objects application members keeps them alive until ``Application::run()`` and all module callbacks have
finished.
An action may instead use a value-only callback when no state needs to survive registration.

The callback receives the complete ``OptionValues`` object.
It can read module-local values as well as root options that apply to every command.
For the full option-module model, including module help, option sets, validation, and parsing callbacks, see
:doc:`/topics/options/option_modules`.

Leaving Main Dispatch to Application
====================================

The application deliberately does not override ``main()``.
The base implementation finds the selected module and calls its main function before considering ``setMainFn()`` or the
event loop.

Overriding ``main()`` would bypass that dispatch unless the override calls ``Application::main()``.
The same warning applies when a command application also registers application parts: module main functions do not
automatically start them, so a command that needs parts must explicitly start and wait for the required services.

Help, Errors, and Exit Codes
============================

Root help presents the available commands, while module help presents the options for one selected action.
Invalid module names and option values are reported before any action main function runs.
Each action returns its own ``ExitCode``, allowing command-specific failure results without changing the application
shell.
