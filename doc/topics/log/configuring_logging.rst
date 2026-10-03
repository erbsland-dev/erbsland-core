.. index::
    single: Logging; Configuration in Code
    single: Log Manager; Configuration
    single: Log Manager; Shutdown

***************************
Configuring Logging in Code
***************************

A logging configuration is the application's policy for turning produced entries into useful output.
It brings together the line format, resource limits, trace sections, and writer routes that should operate as one
coherent setup.
Applications with a fixed policy can assemble this snapshot directly in C++ during startup.

This page follows that policy through the application lifecycle.
It begins with the common case, where configuration is known immediately, then covers replacement and the important
startup pattern where command-line options or an application file determine the final destinations.
The last section draws a clear boundary between the manager owned by
:cpp:class:`Application <erbsland::core::Application>` and a standalone manager whose lifetime you control yourself.

Logging Configuration during Application Initialization
=======================================================

An :cpp:class:`Application <erbsland::core::Application>` already owns its
:cpp:class:`LogManager <erbsland::log::LogManager>`. You do not construct another manager for normal application
logging.
Calling :cpp:func:`log() <erbsland::core::Application::log>` gives you the application manager, creating it lazily on
first access together with its default console setup.

When the final logging policy is compiled into the program, install it in ``initialize()`` before creating components or
starting background work.
At that point there are no producer threads to coordinate and no earlier application messages whose destination is
ambiguous.
Components created afterwards can receive the configured manager and retain their named streams for the rest of the run.

.. erbsland-demo::
    :source: log/LoggingSetups/ApplicationConfiguration.cpp
    :exec: log/logging_setups application-configuration
    :sha256: eb3dfcb7efde4cd8084a543e240330af0b504ee0686d2a2e3d343f73d89db1d0

.. code-block:: cpp

    /// Install a fixed logging policy while the application is being initialized.
    ///
    /// `Application` already owns the log manager and shuts it down at the end of its lifecycle. The application only
    /// assembles the line format, enabled trace sections, and writer routes that form its complete logging policy.
    class ApplicationConfiguration final : public el::Application {
    public:
        using Application::Application;

    protected:
        void initialize() override {
            auto &manager = log();
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{level} [{name}] {message}"_el);

            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .enableTraceSection(el::LogTraceSection{"route-search"_el})
                .addWriter(el::LogWriter::createForConsole(terminal()));
            manager.setConfiguration(std::move(configuration));

            _log = manager.createStream("guild/routes"_el, el::LogTraceSection{"route-search"_el});
        }

        [[nodiscard]] auto main() -> el::ExitCode override {
            if (_log->traceEnabled()) {
                _log->trace("Compared the ridge and lake routes."_el);
            }
            _log->info("Selected the ridge route for 'Revontuli'."_el);
            return el::ExitCode::success();
        }

    private:
        el::LogStreamPtr _log;
    };

    auto runApplicationConfiguration(const int argc, char *argv[]) -> int {
        auto app = ApplicationConfiguration{argc, argv};
        return app.run();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mTRC [guild/routes] Compared the ridge and lake routes.
        INF [guild/routes] Selected the ridge route for 'Revontuli'.␛[0m

.. erbsland-demo-end::

The :cpp:class:`LogConfiguration <erbsland::log::LogConfiguration>` in this example is the whole policy, not just a
collection of optional changes.
Its line format defines the default representation for routes without a custom format.
The enabled trace section makes route-search diagnostics available, and the console writer route gives accepted entries
an actual destination.
Without a writer route the manager can accept entries, but there is nowhere to deliver them.

The example does not call ``shutdown()``.
The application owns the manager and closes it automatically when the application exits, including paths where the
lifecycle handles an Erbsland Core exception.
Application code is responsible for stopping its own producer work through the normal lifecycle; manager shutdown
remains an application service.

Replacing the Complete Logging Configuration
============================================

Sometimes the final policy is not the one installed initially.
An application may load an operator-selected profile, enable a diagnostic mode for a controlled run, or replace a
temporary startup setup after configuration has been validated.
:cpp:func:`setConfiguration() <erbsland::log::LogManager::setConfiguration>` supports that boundary without invalidating
streams already held by components.

Replacement means replacement.
It does not append writers or preserve individual fields from the previous snapshot.
Every format setting, queue option, trace section, and ordinary writer route that should remain active must be present
in the new :cpp:class:`LogConfiguration <erbsland::log::LogConfiguration>`.
The retained-error writer installed by
:cpp:func:`enableLastErrorDump() <erbsland::core::Application::enableLastErrorDump>` is the intentional exception: it is
persistent application support and survives ordinary configuration replacement.

.. erbsland-demo::
    :source: log/LoggingTopics/ConfigurationReplacement.cpp
    :exec: log/logging_topics --demo ConfigurationReplacement
    :sha256: 524201f0ff4361b1e72c3cdb1a5578286454501324b7e5228630c1e262e14b69

.. code-block:: cpp

    /// Replace the active logging policy with one complete configuration snapshot.
    ///
    /// Existing streams remain valid. `setConfiguration()` returns only after their cached trace flags reflect the new
    /// enabled trace sections, so guarded diagnostic preparation can use the new policy immediately.
    void configurationReplacement() {
        auto &manager = el::application().log();
        const auto log = manager.createStream("guild/routes"_el, el::LogTraceSection{"route-search"_el});
        el::io::printLine("Trace before replacement: "_el, el::BooleanFormat::yesNo(), log->traceEnabled());

        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{level} [{name}] {message}"_el);
        auto replacement = el::LogConfiguration{};
        replacement.setLineFormat(std::move(lineFormat))
            .enableTraceSection(el::LogTraceSection{"route-search"_el})
            .addWriter(el::LogWriter::createForConsole(el::application().terminal()));
        manager.setConfiguration(std::move(replacement));

        el::io::printLine("Trace after replacement: "_el, el::BooleanFormat::yesNo(), log->traceEnabled());
        log->trace("Prepared the detailed ridge-route comparison."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mTrace before replacement: no
        Trace after replacement: yes
        ␛[90;49mTRC [␛[95;49mguild/routes␛[90;49m] Prepared the detailed ridge-route comparison.␛[0m

.. erbsland-demo-end::

The stream is created before replacement and remains the same object afterwards.
The first check reports that its trace section is inactive.
The replacement enables that section and supplies a route for delivery; when ``setConfiguration()`` returns, the
stream's cached :cpp:func:`traceEnabled() <erbsland::log::LogStream::traceEnabled>` flag has already been refreshed.
Expensive diagnostic preparation can therefore consult the new policy immediately.

An entry already being written finishes with the old snapshot, while entries still waiting in the manager queue use the
replacement.
This well-defined handover makes startup and explicit reconfiguration boundaries predictable, but it is not intended as
a per-message switch.
Frequent replacement creates coordination work and makes an operational log much harder to interpret.

Buffering Startup Messages with pause() and resume()
====================================================

Many applications cannot build their final logging policy in ``initialize()``.
The configuration path may come from a command-line option, and the selected application file may contain writer
destinations, formats, or trace sections.
Logging those early steps through the temporary default console route loses exactly the startup history that becomes
most useful when configuration fails.

For this lifecycle, call :cpp:func:`pause() <erbsland::log::LogManager::pause>` as the first action in ``initialize()``,
before writing the first startup entry.
Producers can continue to submit entries, but the manager retains them instead of immediately sending them through the
temporary setup.
After command-line parsing and configuration loading have succeeded, install the complete snapshot and call
:cpp:func:`resume() <erbsland::log::LogManager::resume>`.

.. erbsland-demo::
    :source: log/LoggingSetups/DeferredConfiguration.cpp
    :exec: log/logging_setups deferred-configuration
    :sha256: 50090e8402d1fb503c84e6a54802a35c95343908db4ce388c3bd4e4a7c289ad0

.. code-block:: cpp

    /// Retain startup entries until the final logging policy is installed after command-line parsing.
    ///
    /// Pausing is the first initialization action, before any startup message is written. After options and configuration
    /// are available, the application installs the complete snapshot and resumes delivery through the new writer route.
    class DeferredConfiguration final : public el::Application {
    public:
        using Application::Application;

    protected: // implement Application
        void initialize() override {
            log().pause();
            logStream()->info("Application initialization started."_el);
        }

        void handleCommandLineOptions([[maybe_unused]] const el::OptionValuesPtr &values) override {
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("configured: {level} - {message}"_el);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat)).addWriter(el::LogWriter::createForConsole(terminal()));
            log().setConfiguration(std::move(configuration));

            logStream()->info("Logging configuration loaded."_el);
            log().resume();
        }

        [[nodiscard]] auto main() -> el::ExitCode override {
            logStream()->info("Explorer registry opened."_el);
            return el::ExitCode::success();
        }
    };

    auto runDeferredConfiguration(const int argc, char *argv[]) -> int {
        auto app = DeferredConfiguration{argc, argv};
        return app.run();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mconfigured: INF - Application initialization started.
        configured: INF - Logging configuration loaded.
        configured: INF - Explorer registry opened.␛[0m

.. erbsland-demo-end::

All three messages use the ``configured:`` pattern, including the one produced before command-line parsing.
The queued startup entries adopt the replacement configuration, and ``resume()`` allows the worker to deliver them to
its final writer route.
This preserves a continuous startup account without briefly leaking entries to the wrong console or file.

Pause is deliberately bounded rather than transactional.
Normal and warning/error queue limits still apply, and the manager may drain entries if reserved capacity is reached.
Keep the paused startup phase short, avoid starting noisy background producers during it, and resume as soon as the
final configuration is active.
The mechanism protects a modest sequence of important startup messages; it is not an unbounded memory buffer or a way to
freeze writer activity indefinitely.

Application-Owned and Standalone Log Manager Shutdown
=====================================================

Code using :cpp:class:`Application <erbsland::core::Application>` normally never calls
:cpp:func:`shutdown() <erbsland::log::LogManager::shutdown>`. The application performs that step automatically during
final cleanup, after its managed lifecycle has stopped producing work.
Calling it manually inside ordinary application code would end a service the application still expects to own.

An explicitly created standalone manager has a different boundary.
It may belong to a library tool, an isolated processing pipeline, or a test harness that intentionally does not use the
application manager.
Once every producer using that instance has stopped, an explicit ``shutdown()`` makes the end of delivery deterministic
and releases writer resources before their surrounding context disappears.

.. erbsland-demo::
    :source: log/LoggingTopics/ManualConfiguration.cpp
    :exec: log/logging_topics --demo ManualConfiguration
    :sha256: 6cb4b6eb61fada894ec27aaf82a5b703b99223b6dd5f946fd183d289b4083556

.. code-block:: cpp

    /// Shut down a standalone log manager after its producers have finished.
    ///
    /// Unlike the manager owned by `Application`, a manager created with `LogManager::create()` has no application
    /// lifecycle to close it. An explicit shutdown drains its accepted entries and releases its writer resources.
    void manualConfiguration() {
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{level} [{name}] {message}"_el);

        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat));
        configuration.addWriter(
            el::LogWriter::createForConsole(el::application().terminal()),
            el::LogWriterFilter{}.setAcceptedLevels(
                el::LogLevels{
                    el::LogLevel::Information,
                    el::LogLevel::Warning,
                    el::LogLevel::Error,
                }));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->createStream("guild/archive"_el);
        log->info("Map collection 'Järvien maa' was archived."_el);
        log->warn("One map sheet still needs a waterproof cover."_el);

        // Stop the standalone manager after the last producer call.
        manager->shutdown();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m␛[97;49mINF [␛[95;49mguild/archive␛[97;49m] Map collection 'Järvien maa' was archived.
        ␛[93;49mWRN [␛[95;49mguild/archive␛[93;49m] One map sheet still needs a waterproof cover.␛[0m

.. erbsland-demo-end::

Shutdown accepts no new producer entries.
It drains entries already accepted by the manager until the configured shutdown deadline, then discards any remainder so
process termination cannot be delayed indefinitely.
The manager's destructor provides the same safety for a forgotten standalone shutdown, but an explicit call documents
the producer boundary and lets the surrounding code decide exactly when writer resources are released.

When loss accounting matters, read
:cpp:func:`statistics() <erbsland::log::LogManager::statistics>` after producers have stopped. Queue drops, shutdown
drops, and writer failures describe different operational problems; :doc:`configuring_manager_options` explains their
limits and counters in detail.
