..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Options; Help Output
    single: DisplayTextMap
    single: Terminal Help
    single: TextDocument; Option Help

.. _options-customizing-help-output:

***********************
Customizing Help Output
***********************

The option system builds help, version, and error output as neutral
:cpp:class:`TextDocument <erbsland::text::TextDocument>` trees.
This page explains how :cpp:class:`Application <erbsland::core::Application>` renders these documents, how to render
them manually, and how to customize generated wording and terminal style.

Plain Output Is the Default
===========================

Without terminal support, :cpp:class:`Application <erbsland::core::Application>` renders generated help as plain text.
This is predictable for redirected output, build logs, tests, and simple command-line tools.

.. erbsland-demo::
    :source: option/HelpCustomization/main.cpp
    :exec: option/option_help_customization --help
    :exec-2: option/option_help_customization --terminal --help
    :exec-3: option/option_help_customization manual
    :source-sha256: 725671237bf9d1646469cc3bf248575257c12c8420cbe65bd3f7cada3d609975

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
                .setHelpDescription("Renders help with terminal colors."_el)
                .setHelpVisibility(el::OptionHelpVisibility::Hidden);
        }
    };

    /// `OptionManager` exposes neutral help documents and `DisplayTextMap` for manual rendering and wording changes.
    ///
    /// The manager can build a `TextDocument` without owning the application lifecycle.
    /// Change `DisplayTextMap` when generated labels such as `Usage`, `Options`, placeholders, or built-in option
    /// descriptions need application-specific wording.
    auto manualHelpRendering() -> el::ExitCode {
        auto options = el::Options::create();
        auto info = el::ApplicationInfo{};
        info.setApplicationName("Manual Help"_el);
        info.setApplicationVersion(el::Version{0, 8, 0});
        options->setApplicationInfo(info);
        options->setExecutablePath("manual-help"_el);
        addSharedOptions(options);

        auto displayText = el::DisplayTextMap::defaultMap()->clone();
        displayText->set("options.UsageLabel"_el, "Usage"_el)
            .set("options.OptionsHeading"_el, "Settings"_el)
            .set("options.OptionsPlaceholder"_el, "options"_el)
            .set("options.ChoicePlaceholder"_el, "choice"_el)
            .set("options.ChoicesLabel"_el, "Choices"_el);

        auto manager = el::OptionManager{options};
        manager.setDisplayTextMap(displayText);
        el::io::printLine(manager.helpDocument({}).toString());
        return el::ExitCode::success();
    }

.. rubric:: ``$ option/option_help_customization --help``

.. erbsland-ansi::
    :escape-char: ␛

    Shows how option help can be rendered as a document.
    Usage:
    option_help_customization [options]
    Options:
    -g, --area <area>      Area for which to view help.
    -f, --format <choice>  Output detail level. Choices: short, full.
    -h, --help             Display this help.
        --version          Display version information.

.. rubric:: ``$ option/option_help_customization --terminal --help``

.. erbsland-ansi::
    :escape-char: ␛



          ␛[37;40mShows how option help can be rendered as a document.


       ␛[1;36m-◆ ␛[96mUsage␛[36m ◆-␛[90m──────────────────────────────────────────────────────────────────────────

    ␛[22;37moption_help_customization options


       ␛[1;36m-◆ ␛[96mOptions␛[36m ◆-␛[90m────────────────────────────────────────────────────────────────────────

    ␛[22;37m-g, --area area␛[39;49m      ␛[37;40mArea for which to view help.
    -f, --format choice␛[39;49m  ␛[37;40mOutput detail level. Choices: short, full.
    -h, --help␛[39;49m           ␛[37;40mDisplay this help.
    ␛[39;49m    ␛[37;40m--version␛[39;49m        ␛[37;40mDisplay version information.␛[0m

.. rubric:: ``$ option/option_help_customization manual``

.. erbsland-ansi::
    :escape-char: ␛

    Shows how option help can be rendered as a document.
    Usage:
    manual-help [options]
    Settings:
    -g, --area <area>      Area for which to view help.
    -f, --format <choice>  Output detail level. Choices: short, full.
    -h, --help             Display this help.
        --version          Display version information.

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

``DisplayTextMap`` controls generated words such as labels, headings, placeholders, built-in option descriptions, and
diagnostic labels.
It does not change parser behavior.

Start from ``DisplayTextMap::defaultMap()``, clone it, adjust the keys you need, and pass it to
``OptionManager::setDisplayTextMap()``.

This is the right tool for localization, house style, or embedding option help into an application that already has its
own vocabulary.
Use option and set help metadata for user-facing descriptions of your own options.
Use ``DisplayTextMap`` for generated framework wording.
