..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Options; Help Output
    single: DisplayText
    single: Terminal Help
    single: TextDocument; Option Help

.. _options-customizing-help-output:

***********************
Customizing Help Output
***********************

The option system builds help, version, and error output as neutral
:cpp:class:`TextDocument <erbsland::text::TextDocument>` trees.
This page explains how :cpp:class:`Application <erbsland::core::Application>` renders these documents, how to render
them manually, and how to customize help ordering, wording, and terminal style.

Order Help Entries
==================

Help orders commands and options alphabetically by default.
Use ``setHelpSorting()`` on ``Options`` to change the root policy.
Modules and sets inherit their owner's policy unless you set an override.

For example, if the order in which you define commands and options reflects the order in which users need them, call
``options->setHelpSorting(el::OptionHelpSorting::None)``.
To give a few entries priority within an otherwise alphabetical list, select ``ByWeight`` and give those entries
negative weights with ``setHelpWeight()``.

See :ref:`options-help-sorting` for the sorting modes and the rules for shared section headings.

Detailed Option Help
====================

The built-in ``--help=<name>`` form displays one visible option in detail.
The name may be a case-insensitive long or internal alias, or a case-sensitive short alias.
For a root request, global options are searched before modules in registration order.
For a module request, that module is searched first, then global options, then the remaining modules.
When the same exact alias exists in another scope, the document adds a ``See Also`` command for that scope.

Detailed documents generate one usage line for each dashed alias and use attached value syntax.
They show the option title, description, an optional example configured with ``setHelpExample()``, and any related scope
commands.
Use ``OptionManager::detailedHelpDocument()`` or ``displayDetailedHelp()`` when routing display requests manually.
An unresolved direct request throws :cpp:class:`OptionError <erbsland::options::OptionError>` with an ``UnknownName``
context and fuzzy suggestions.

Plain Output Is the Default
===========================

Without terminal support, :cpp:class:`Application <erbsland::core::Application>` renders generated help as plain text.
This is predictable for redirected output, build logs, tests, and simple command-line tools.

.. erbsland-demo::
    :source: option/HelpCustomization/main.cpp
    :exec: option/option_help_customization --help
    :exec-2: option/option_help_customization --terminal --help
    :exec-3: option/option_help_customization manual
    :sha256: 3f1eac6a0a1711ed9a02ec9e783f8cb584a03a8b3bf3b4c1f777d897db033ae0

.. code-block:: cpp

    /// `Application::enableTerminal()` switches generated help from plain text to styled terminal output.
    ///
    /// This application checks for `--terminal` before parsing and enables terminal rendering early enough for `--help`.
    /// It also sets a predefined terminal document style for system output.
    class HelpCustomizationApp final : public el::Application {
    public:
        using Application::Application;

    protected:
        void initialize() override {
            info().setApplicationName("Night Watch Help"_el);
            info().setApplicationVersion(el::Version{0, 8, 0});
            for (const auto &arg : commandLineArguments()) {
                if (arg == "--terminal"_el) {
                    enableTerminal();
                    setSystemOutputStyle(el::cterm::TerminalDocumentStyle::defaultStyled());
                    break;
                }
            }
        }
        void registerCommandLineOptions(const el::OptionsPtr &options) override {
            addSharedOptions(options);
            options->addOption({"--terminal"_el, "terminal"_el})
                .setHelpTitle("Renders help with terminal colors."_el)
                .setHelpVisibility(el::OptionHelpVisibility::Hidden);
        }
    };

    /// `OptionManager` exposes neutral help documents and compiled display-text layers for manual rendering and wording
    /// changes.
    ///
    /// The manager can build a `TextDocument` without owning the application lifecycle.
    /// Change compiled display-text layers when generated labels such as `Usage`, `Options`, placeholders, or built-in
    /// option descriptions need application-specific wording.
    auto manualHelpRendering() -> el::ExitCode {
        auto options = el::Options::create();
        auto info = el::ApplicationInfo{};
        info.setApplicationName("Manual Help"_el);
        info.setApplicationVersion(el::Version{0, 8, 0});
        options->setApplicationInfo(info);
        options->setExecutablePath("manual-help"_el);
        addSharedOptions(options);

        el::application().displayText()->addData(DisplayTextDemo::create());

        auto manager = el::OptionManager{options};
        el::io::printLine(manager.helpDocument({}).toString());
        return el::ExitCode::success();
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ option/option_help_customization --help

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mShows how option help can be rendered as a document.
        Usage:
        option_help_customization [options]
        Options:
        -g, --area <area>       Area for which to view help.
        -f, --format <choice>   Output detail level. Choices: short, full.
        -h, --help[=<keyword>]  Display this help.
            --version           Display version information.␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ option/option_help_customization --terminal --help

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m

              ␛[37;40mShows how option help can be rendered as a document.


           ␛[36;40m␛[1m-◆ ␛[96;40mUsage␛[36;40m ◆-␛[90;40m──────────────────────────────────────────────────────────────────────────

        ␛[37;40m␛[22moption_help_customization options


           ␛[36;40m␛[1m-◆ ␛[96;40mOptions␛[36;40m ◆-␛[90;40m────────────────────────────────────────────────────────────────────────

        ␛[37;40m␛[22m-g, --area area␛[39;49m       ␛[37;40mArea for which to view help.
        -f, --format choice␛[39;49m   ␛[37;40mOutput detail level. Choices: short, full.
        -h, --help[=keyword]␛[39;49m  ␛[37;40mDisplay this help.
        ␛[39;49m    ␛[37;40m--version␛[39;49m         ␛[37;40mDisplay version information.␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ option/option_help_customization manual

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mShows how option help can be rendered as a document.
        Usage:
        manual-help [options]
        Settings:
        -g, --area <area>       Area for which to view help.
        -f, --format <choice>   Output detail level. Choices: short, full.
        -h, --help[=<keyword>]  Display this help.
            --version           Display version information.␛[0m

.. erbsland-demo-end::

Enable Terminal Rendering in Application
========================================

Call :cpp:func:`enableTerminal() <erbsland::core::Application::enableTerminal>` before command-line parsing when help
should use terminal styling.
The usual place is at the start of ``initialize()``.
If styling should depend on an early command-line switch, inspect
:cpp:func:`commandLineArguments() <erbsland::core::Application::commandLineArguments>` in ``initialize()`` and enable
the terminal before parsing begins.

When terminal support is enabled, :cpp:class:`Application <erbsland::core::Application>` renders help, version, errors,
and other system output through its terminal document renderer.
Use :cpp:func:`setSystemOutputStyle() <erbsland::core::Application::setSystemOutputStyle>` to choose a predefined or
custom :cpp:class:`TerminalDocumentStyle <erbsland::cterm::TerminalDocumentStyle>`.

Render Help Manually
====================

Use :cpp:class:`OptionManager <erbsland::options::OptionManager>` directly when help or diagnostics are part of a larger
workflow.
The manager can build neutral documents without displaying them:

* :cpp:func:`helpDocument() <erbsland::options::OptionManager::helpDocument>` builds help for the root or a module,
* :cpp:func:`versionDocument() <erbsland::options::OptionManager::versionDocument>` builds version output,
* :cpp:func:`errorDocument() <erbsland::options::OptionManager::errorDocument>` builds an option diagnostic.

The document can be converted to plain text with ``toString()`` or passed to a terminal document renderer.
Manual rendering is useful for tests, embedded command interpreters, graphical shells, and tools that collect
diagnostics before printing them.

Customize Display Text
======================

``DisplayText`` controls generated words such as labels, headings, placeholders, built-in option descriptions, and
diagnostic labels.
It does not change parser behavior.

Define the labels you want to replace in an ELCL catalog, compile it with ``erbsland_core_display_text()``, and register
its generated data with ``application().displayText()->addData(...)``.
The application registry supplies wording to existing managers and diagnostic renderers.
See :doc:`/reference/i18n/internationalization` for catalog generation, layer precedence, and translator integration.

This is the right tool for localization, house style, or embedding option help into an application that already has its
own vocabulary.
Use option and set help metadata for user-facing descriptions of your own options.
Use ``DisplayText`` for generated framework wording.
