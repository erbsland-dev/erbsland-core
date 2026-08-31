.. index::
    single: Logging; Loading Configuration
    single: ELCL; Logging Configuration
    single: Log Configuration; Validation

*****************************
Loading Logging Configuration
*****************************

For many applications, it is useful to allow users to configure logging using a configuration file.
Usually, this configuration resides in one dedicated file or the main application configuration contains a fixed section
for the logging customization.

This page develops the two common arrangements separately.
In both, the application opens a known file, obtains the value that represents the root of the logging schema, validates
it, and installs the resulting
:cpp:class:`LogConfiguration <erbsland::log::LogConfiguration>` before ordinary work begins.

Give Logging One Fixed Place
============================

A dedicated ``log.elcl`` is a good fit when logging has its own deployment lifecycle.
Operations can replace or mount the file independently, and every value in the document belongs to logging.
The root of the parsed document is therefore the root of the logging schema.

An ``application.elcl`` with a root ``Log`` section is usually more convenient when one team owns a single application
configuration.
Other sections can describe the application, services, and storage, while the startup code always reads ``Log`` for
logging.
The section name is part of the application's schema just like any other required section.

With ELCL, both solutions work equally well.
In the second form, the log configuration can still be held in a separate file and be included from the main file.

Load a Dedicated ``log.elcl``
=============================

In a standalone logging file, ``Format``, ``Queue``, ``Trace Sections``, and ``Writers`` begin at the document root.
The compact fixture below configures a named console route.
A real application can resolve ``log.elcl`` through its platform-specific configuration directory; after that path has
been chosen, the loading sequence stays fixed.

.. literalinclude:: ../../../demos/log/LoggingTopics/data/log.elcl
    :language: erbsland-conf

:cpp:class:`Parser <erbsland::conf::Parser>` returns an ELCL document. Because the complete document follows the logging
schema, the application uses that document directly without performing a name lookup.
It parses the complete document, and installs it in the application-owned manager.

.. erbsland-demo::
    :source: log/LoggingTopics/LoadStandaloneLogConfiguration.cpp
    :exec: log/logging_topics --demo LoadStandaloneLogConfiguration
    :source-sha256: e1f6d4565f1196e6d4b8a37c790983d9d9730e61567a618d69188344c0745b99

.. code-block:: cpp

    /// Load logging from a dedicated `log.elcl` file.
    ///
    /// The whole document follows the logging schema. Validate the document root, parse the complete configuration, and
    /// install it before the application starts its ordinary work.
    /// @notest{Compiled and executed by the logging topics demo.}
    void loadStandaloneLogConfiguration() {
        const auto path = el::Path{"demos/log/LoggingTopics/data/log.elcl"_el};
        const auto document = el::conf::Parser{}.parseFileOrThrow(path);

        // The log schema is automatically validated by the parser.
        auto configuration = el::LogConfigurationParser{el::application().terminal()}.parse(document);
        el::application().log().setConfiguration(std::move(configuration));

        const auto log = el::application().log().createStream("log/startup"_el);
        log->info("Loaded logging from log.elcl."_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97mINF [␛[95mlog/startup␛[97m] Loaded logging from log.elcl.␛[0m

.. erbsland-demo-end::

Load ``Log`` From ``application.elcl``
======================================

When logging is part of the main application configuration, you reserve a subsection for it.
The example application schema reserves the root section ``Log``.
In your validation schema, you mark this value as ``NotValidated``, so you can validate it separately.

.. literalinclude:: ../../../demos/log/LoggingTopics/data/application.elcl
    :language: erbsland-conf

After parsing ``application.elcl``, call
:cpp:func:`valueOrThrow() <erbsland::conf::Value::valueOrThrow>` with the selected name ``log`` if the log configuration
is mandatory.
A missing section is then a configuration error.
The returned value becomes the logging-schema root for validation and parsing.

.. erbsland-demo::
    :source: log/LoggingTopics/LoadApplicationLogConfiguration.cpp
    :exec: log/logging_topics --demo LoadApplicationLogConfiguration
    :source-sha256: fccded8518e7b5daef3f5b5bc07f1b9d982565534216fc05a2303058b8283f68

.. code-block:: cpp

    /// Load logging from the fixed `Log` section of `application.elcl`.
    ///
    /// The application owns the section name as part of its configuration schema. Select that known section, validate it as
    /// a logging configuration, and install the parsed snapshot before ordinary application work begins.
    /// @notest{Compiled and executed by the logging topics demo.}
    void loadApplicationLogConfiguration() {
        const auto path = el::Path{"demos/log/LoggingTopics/data/application.elcl"_el};
        const auto document = el::conf::Parser{}.parseFileOrThrow(path);
        const auto logSection = document->valueOrThrow("log"_el);

        // You can validate the Log section independently from configuration parsing, in case you like to keep
        // the whole application configuration validation in one place and fail early.
        el::LogConfigurationParser::validationRules()->validate(logSection, el::LogConfigurationParser::version());

        // Calling `parse` will nevertheless validate the configuration again.
        auto configuration = el::LogConfigurationParser{el::application().terminal()}.parse(logSection);
        el::application().log().setConfiguration(std::move(configuration));

        const auto log = el::application().log().createStream("application/startup"_el);
        log->info("Loaded logging from application.elcl."_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97mINF [␛[95mapplication/startup␛[97m] Loaded logging from application.elcl.␛[0m

.. erbsland-demo-end::

Validate the Log Configuration Before Parsing
=============================================

:cpp:func:`validationRules() <erbsland::log::LogConfigurationParser::validationRules>` exposes the logging schema as
reusable ELCL validation rules.
Use its :cpp:func:`validate() <erbsland::conf::vr::Rules::validate>` function and pass the section value and version
number of the logging configuration for verification.
For a standalone file, the section is the document itself and if you embed it, use the value of the subsection.

This explicit step is particularly useful when startup validates every application section before it creates runtime
objects.
Logging errors then appear during the same preflight phase as errors in service or storage settings, before any writer
opens a file or starts network delivery.
The rules check the logging structure, value types, and documented bounds.
The following parse still performs conversions and creates the configured writers, so errors that depend on a specific
writer or an application resource can still arise there.

Calling the rules separately is optional for a smaller loader.
:cpp:func:`parse() <erbsland::log::LogConfigurationParser::parse>` validates the supplied value itself before building
the snapshot.
Explicit validation does not make parsing safer a second time; it gives a larger application a clean phase boundary and
a way to combine logging validation with the rest of its configuration checks.

Construct and Install the Complete Snapshot
===========================================

:cpp:class:`LogConfigurationParser <erbsland::log::LogConfigurationParser>` turns the validated value into one complete
configuration.
Supply the application's terminal when the schema permits a console writer.
That writer needs the terminal for styled, width-aware output.
Supplying it keeps console, file, last-error, and syslog choices available to the configuration without changing the
loading sequence.

Install the returned object with
:cpp:func:`setConfiguration() <erbsland::log::LogManager::setConfiguration>`. The manager replaces its writers, routes,
line format, limits, and trace sections together, so accepted messages never observe a partially applied configuration.

If initialization can produce important messages before the configuration is loaded, pause the manager at the beginning
of ``initialize()`` and resume it immediately after installation succeeds.
The retained startup entries then pass through the intended routes.
:doc:`configuring_logging` develops this lifecycle pattern in detail.

Use the Flexible Demo to Explore Scenarios
==========================================

The ``configured_logging`` executable serves a different purpose from the two application patterns above.
It is a scenario runner for this documentation: a command-line path selects a fixture, and an optional ``--branch`` lets
the same executable exercise a root logging document, a nested logging value, and an invalid configuration.
That flexibility keeps parser examples and error rendering easy to test.
It is not a template for choosing the logging location in a production application.

The complete scenario implementation is shown below so its special role is visible rather than hidden.
The first two runs demonstrate that :cpp:class:`LogConfigurationParser <erbsland::log::LogConfigurationParser>` accepts
the same schema at either value.
The third run deliberately fails validation and confirms that configuration diagnostics reach the application boundary.

.. erbsland-demo::
    :source: log/ConfiguredLogging/ConfiguredLoggingApp.cpp
    :exec: log/configured_logging demos/log/ConfiguredLogging/data/root.elcl
    :exec-2: log/configured_logging demos/log/ConfiguredLogging/data/nested.elcl --branch application.logging
    :exec-3: log/configured_logging demos/log/ConfiguredLogging/data/invalid.elcl
    :exec-3-exit-code: 1
    :source-sha256: 5dc977fa5b621977b81dd172b322dfa9eac3f51f3d7895487120b7cc1940eaa8

.. code-block:: cpp

    /// Load and install an ELCL logging configuration as part of application startup.
    ///
    /// The configuration can occupy the document root or a branch selected on the command line. Validation and parsing
    /// errors intentionally reach `Application::run()`, which presents the complete diagnostic and returns a failure exit
    /// code.
    void ConfiguredLoggingApp::initialize() {
        info().setApplicationName("Configured Explorer Guild"_el);
        enableTerminal();
    }

    void ConfiguredLoggingApp::registerCommandLineOptions(const el::OptionsPtr &options) {
        options->setHelpDescription("Load an application logging configuration from an ELCL file."_el);
        options->addOption("configuration"_el)
            .setRequired()
            .setHelpDescription("ELCL file containing the logging configuration."_el);
        options->addOption({"-b"_el, "--branch"_el, "branch"_el})
            .setType(el::OptionType::Text)
            .setHelpDescription("Optional dot-separated branch containing the logging configuration."_el);
    }

    void ConfiguredLoggingApp::parseCommandLine() {
        Application::parseCommandLine();
        if (optionValues() == nullptr) {
            return;
        }

        // Parse the application configuration file and select the requested branch.
        const auto path = el::Path::fromNativeOrThrow(optionValues()->getText("configuration"_el));
        const auto document = el::conf::Parser{}.parseFileOrThrow(path);
        auto branch = el::conf::ValuePtr{document};
        const auto branchName = optionValues()->getText("branch"_el);
        if (!branchName.isEmpty()) {
            branch = document->valueOrThrow(branchName);
        }

        // Pre-validation is useful when these rules are composed into broader application validation.
        el::LogConfigurationParser::validationRules()->validate(branch, 0);

        // A terminal is supplied because the selected configuration may create console writers.
        auto configuration = el::LogConfigurationParser{terminal()}.parse(branch);
        log().setConfiguration(std::move(configuration));
    }

    auto ConfiguredLoggingApp::main() -> el::ExitCode {
        const auto log = this->log().createStream("guild/configured"_el, el::LogTraceSection{"route-search"_el});
        if (log->traceEnabled()) {
            log->trace("Candidate route: Turku → Jääjärvi → Majakka"_el);
        }
        log->info("The explorer guild loaded its logging configuration."_el);
        log->warn("One route marker still needs confirmation."_el);
        return el::ExitCode::success();
    }

.. rubric:: ``$ log/configured_logging demos/log/ConfiguredLogging/data/root.elcl``

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97mINF [␛[95mguild/configured␛[97m] The explorer guild loaded its logging configuration.
    ␛[93mWRN [␛[95mguild/configured␛[93m] One route marker still needs confirmation.␛[0m

.. rubric:: ``$ log/configured_logging demos/log/ConfiguredLogging/data/nested.elcl --branch application.logging``

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90mTRC ␛[95mconfigured␛[90m: Candidate route: Turku → Jääjärvi → Majakka
    ␛[97mINF ␛[95mconfigured␛[97m: The explorer guild loaded its logging configuration.
    ␛[93mWRN ␛[95mconfigured␛[93m: One route marker still needs confirmation.␛[0m

.. rubric:: ``$ log/configured_logging demos/log/ConfiguredLogging/data/invalid.elcl``

.. erbsland-ansi::
    :escape-char: ␛


      ␛[1;91mValidating␛[22m ␛[1mthe␛[22m ␛[1mConfiguration␛[22m ␛[1mFailed

      ␛[22;39mThe value must be at least 1

    ␛[1mError␛[22m ␛[1mSource:
      ␛[22;90mPath:␛[39m     /var/folders/f2/rq38shx1797_b7m0vy142phw0000gn/T/erbsland-demo-doc-7r59jojt/fi
                xture-0.elcl
      ␛[90mLine:␛[39m     3
      ␛[90mColumn:␛[39m   1
      ␛[90mPosition:␛[39m 159

    ␛[1mConfiguration␛[22m ␛[1mError␛[22m ␛[1mDetails:
      ␛[22;90mcategory:␛[39m  Validation
      ␛[90mname path:␛[39m queue.maximum_entries

.. erbsland-demo-end::

Let Configuration Errors Reach the Application Boundary
=======================================================

The invalid scenario is as important as the successful ones.
No startup method catches
:cpp:class:`ConfError <erbsland::conf::ConfError>` merely to replace it with a shorter message. The exception reaches
:cpp:func:`Application::run() <erbsland::core::Application::run>`, where the standard application boundary renders the
reason, source path, line and column, and logging-specific name path.
It then returns a nonzero exit status to the shell or service manager.

Catch a configuration error inside startup only when the application can genuinely recover—for example, when an
explicitly optional file is absent and a documented built-in configuration is acceptable—or when you can add useful
context before rethrowing it.
Otherwise, allowing the original diagnostic to reach the boundary gives the operator the most precise explanation and
keeps error handling consistent with the rest of the application framework.

Once this loading path is in place, :doc:`elcl_configuration` explains how to write the logging section itself, from a
minimal console setup to production writer routes and queue limits.
