.. index::
    single: Logging; Choosing a Setup
    single: Application; Logging

************************
Choosing a Logging Setup
************************

A logging setup should make the program easier to understand, not introduce a second architecture beside it.
The smallest useful setup therefore follows the shape of the application: one stream for a small command, streams owned
by components once those components matter to readers, and a deliberate routing policy when the program becomes a
long-running service.

The three setups on this page are stages rather than competing recipes.
Each one is complete enough to use as shown, and each adds a responsibility only when the application has somewhere
sensible to put it.
Start where the program is today.
Moving to the next setup later does not require rewriting existing log calls.

Use One Root Stream in a Small Application
==========================================

A short command often has only one story to tell.
It starts, performs one operation, reports anything unusual, and exits.
Giving every helper a name in such a program would make the output look more elaborate without helping the person
reading it.

:cpp:class:`Application <erbsland::core::Application>` already owns a lazily created
:cpp:class:`LogManager <erbsland::log::LogManager>`. Its
:cpp:func:`logStream() <erbsland::core::Application::logStream>` method returns the manager's root stream and installs
the default console route on first use.
Keep that pointer in the main function and write the few milestones that are useful outside the implementation.

.. erbsland-demo::
    :source: log/LoggingSetups/MinimalLogging.cpp
    :exec: log/logging_setups minimal
    :source-sha256: 843202281ce553e650015c6f011471204241fa6838c351f6f23da670670925bf

.. code-block:: cpp

    /// Give a small command-line application one root log stream.
    ///
    /// The application creates the default console configuration lazily. Keeping the root stream is enough when the
    /// program has only one useful source of messages and readers do not need subsystem names.
    auto runMinimalLogging(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.setMainFn([&app]() -> el::ExitCode {
            const auto log = app.logStream();
            log->info("Explorer registry opened."_el);
            log->warn("The route notes for 'Revontuli' are incomplete."_el);
            return el::ExitCode::success();
        });
        return app.run();
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[96m2026-09-01 14:01:31Z␛[97m INF - Explorer registry opened.
    ␛[96m2026-09-01 14:01:31Z␛[93m WRN - The route notes for 'Revontuli' are incomplete.␛[0m

.. erbsland-demo-end::

This setup deliberately has no explicit configuration and no stream name in its output.
The default pattern includes a timestamp and level, which is usually enough for an interactive command.
If the code later grows into several recognizable areas, the manager is already there; the application can begin
creating named streams without changing how it starts or shuts down.

Give Components Their Own Streams in a Medium Application
=========================================================

Once a program has modules with distinct responsibilities, the source of a message becomes part of its meaning.
“Two lanterns need oil” belongs to supply accounting, while a route-catalog message belongs to navigation.
Repeating those labels in message text is tedious and, more importantly, leaves writers unable to route them as
structured names.

Let the application continue to own the manager and its configuration.
Pass the manager to each component during construction; the component creates one stream with a stable path and retains
it as ``_log``.
This keeps logging close to the component's lifetime without giving the component authority over the application-wide
logging policy.

.. erbsland-demo::
    :source: log/LoggingSetups/MediumLogging.cpp
    :exec: log/logging_setups medium
    :source-sha256: 4d659571cf0f7341c3467279e01e58c237212982159636d11f9a24ba6ace67b7

.. code-block:: cpp

    /// Let each component in a medium application own a stable, named stream.
    ///
    /// The application still owns and configures the manager. Components receive it during construction, create the
    /// stream that identifies their responsibility, and retain that stream as `_log` for their whole lifetime.
    class RouteCatalog final {
    public:
        explicit RouteCatalog(el::LogManager &manager) : _log{manager.createStream("guild/routes"_el)} {}

        void load() { _log->info("Loaded the route catalog for 'Revontuli'."_el); }

    private:
        el::LogStreamPtr _log;
    };

    class SupplyLedger final {
    public:
        explicit SupplyLedger(el::LogManager &manager) : _log{manager.createStream("guild/supplies"_el)} {}

        void verify() { _log->warn("Two lanterns still need fresh oil."_el); }

    private:
        el::LogStreamPtr _log;
    };

    class MediumLoggingApplication final : public el::Application {
    public:
        using Application::Application;

    protected:
        void initialize() override {
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{level} [{name}] {message}"_el);
            auto configuration = log().configuration();
            configuration.setLineFormat(std::move(lineFormat));
            log().setConfiguration(std::move(configuration));

            _routes = std::make_unique<RouteCatalog>(log());
            _supplies = std::make_unique<SupplyLedger>(log());
        }

        [[nodiscard]] auto main() -> el::ExitCode override {
            _routes->load();
            _supplies->verify();
            return el::ExitCode::success();
        }

    private:
        std::unique_ptr<RouteCatalog> _routes;
        std::unique_ptr<SupplyLedger> _supplies;
    };

    auto runMediumLogging(const int argc, char *argv[]) -> int {
        auto app = MediumLoggingApplication{argc, argv};
        return app.run();
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97mINF [␛[95mguild/routes␛[97m] Loaded the route catalog for 'Revontuli'.
    ␛[93mWRN [␛[95mguild/supplies␛[93m] Two lanterns still need fresh oil.␛[0m

.. erbsland-demo-end::

The line pattern now includes ``{name}``, so the two components remain recognizable when their entries are interleaved.
The names also form a hierarchy: a future route for ``guild`` can accept both streams, while a route for
``guild/supplies`` can select just the ledger.
Choose paths for responsibilities that remain meaningful over time, rather than for individual classes or source files
that may be renamed during routine refactoring.

Define One Logging Policy for a Large Application
=================================================

A service or larger application has a different audience.
Operators need a durable history after the terminal has gone away, developers may need detailed diagnostics for one
feature, and a failed startup still needs to explain itself immediately.
These needs belong in one configuration snapshot assembled during application initialization.

The simplified service below makes those choices explicit.
All ordinary output goes to a file, one trace section is enabled for route-search diagnostics, and the expedition
component receives a named stream associated with that section.
The detailed trace and normal milestones stay in the operational log instead of filling the service console.

.. erbsland-demo::
    :source: log/LoggingSetups/LargeLogging.cpp
    :exec: log/logging_setups large
    :exec-exit-code: 2
    :source-sha256: e52d61aedfddad3dcaa683d1a7e824d0f0afa197e599e3f3b9bccf615079e0c0

.. code-block:: cpp

    /// Give a large or long-running application one deliberate logging policy.
    ///
    /// This simplified service writes its operational history to a file, enables one diagnostic trace section, and
    /// retains recent errors. Because `main()` reports failure, the retained error is repeated on the console under a
    /// localized heading while the detailed trace remains in the file.
    class LargeLoggingApplication final : public el::Application {
    public:
        using Application::Application;

    protected:
        void initialize() override {
            auto temporaryOptions = el::PathTempDirectoryOptions{};
            temporaryOptions.setPrefix("explorer-guild-"_el).setRandomLength(el::CpLength{8U});
            _temporary = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);

            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{level} [{name}] {message}"_el);
            auto fileOptions = el::FileLogWriterOptions{_temporary->path() / "service.log"_el};
            fileOptions.setMode(el::LogFileMode::Overwrite);

            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .enableTraceSection(el::LogTraceSection{"route-search"_el})
                .addWriter(std::make_shared<el::FileLogWriter>(std::move(fileOptions)));
            log().setConfiguration(std::move(configuration));

            enableLastErrorDump();
            _log = log().createStream("guild/expedition"_el, el::LogTraceSection{"route-search"_el});
        }

        [[nodiscard]] auto main() -> el::ExitCode override {
            if (_log->traceEnabled()) {
                _log->trace("Candidate route: Turku → Jääjärvi → Majakka"_el);
            }
            _log->info("Expedition 'Revontuli' departed."_el);
            _log->error("The northern checkpoint did not answer."_el);
            return el::ExitCode{2};
        }

        void cleanup() noexcept override { log().shutdown(); }

    private:
        el::TempDirectoryPtr _temporary;
        el::LogStreamPtr _log;
    };

    auto runLargeLogging(const int argc, char *argv[]) -> int {
        auto app = LargeLoggingApplication{argc, argv};
        return app.run();
    }

.. erbsland-ansi::
    :escape-char: ␛


    ␛[1mRecent␛[22m ␛[1mError␛[22m ␛[1mLog␛[22m ␛[1mEntries

    ␛[22;91mERR [␛[95mguild/expedition␛[91m] The northern checkpoint did not answer.␛[0m

.. erbsland-demo-end::

The console contains the final error because
:cpp:func:`enableLastErrorDump() <erbsland::core::Application::enableLastErrorDump>` installed a persistent error
retainer before the stream was used.
It survives replacement of the ordinary configuration, but it is not a substitute for the file: it keeps only a bounded
set of recent errors and omits the trace and informational context around them.

By default, a nonempty retained snapshot is displayed only when the application finishes with a nonzero exit code.
A successful service shutdown stays quiet.
Pass :cpp:enumerator:`LastErrorDumpMode::Always <erbsland::core::LastErrorDumpMode::Always>` when an application
intentionally wants the snapshot after successful runs as well.
The heading comes from the application's
:cpp:class:`DisplayTextMap <erbsland::i18n::DisplayTextMap>` entry ``log.LastErrorDumpTitle``, so localized applications
can replace it together with their other display text.

This is the point where :doc:`configuring_logging` and :doc:`using_writers` become useful: the application has a real
policy to express.
If deployments must choose destinations or trace sections themselves, the same snapshot can later come from
:doc:`reading_configuration`.
That is a deployment decision inside the large setup, not a separate logging architecture.

Choose the Boundary the Application Already Has
===============================================

Prefer the root stream while every message genuinely comes from one operation.
Introduce component-owned streams when names help a reader locate responsibility.
Move to a complete configuration when logs must outlive the terminal, serve different audiences, or expose diagnostics
selectively.

The useful boundary is not measured in lines of code.
A tiny daemon may need the large setup because its console disappears, while a substantial one-shot conversion tool may
remain perfectly clear with named component streams and the default console writer.
Let the readers and operating environment determine the setup, then keep ownership simple: the application owns policy,
and each component owns the stream that identifies its messages.
