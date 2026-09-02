.. index::
    single: Logging; Writers
    single: Log Writers
    single: Log Writer Filters
    single: Console Log Writer
    single: File Log Writer
    single: Syslog Log Writer
    single: Application Last Error Dump

*****************
Using Log Writers
*****************

A log stream gives application code a convenient place to record what happened, but it does not decide where that record
should live.
That decision belongs to a *log writer*.
A writer may turn an entry into a styled terminal paragraph, append it to a file, or send it to a syslog service.
The application can additionally retain recent errors for a final failure report.

This separation is useful as an application grows.
Your modules can continue to write to the same named streams while the application chooses destinations that suit its
environment.
A command-line tool may need only the console, a background service may combine a rotating file with retained errors,
and a deployed service may send warnings and errors to a central syslog collector as well.

This page introduces that model, shows how to add writers and route entries, and helps you choose among the built-in
writers.
The following pages then explore the console, file, and syslog writers in depth.

Understand the Writer's Place in the Log System
===============================================

When you call a method such as :cpp:func:`info() <erbsland::log::LogStream::info>`, the stream creates a log entry and
hands it to its :cpp:class:`LogManager <erbsland::log::LogManager>`.
The manager later formats that entry according to the active :cpp:class:`LogLineFormat <erbsland::log::LogLineFormat>`
and offers it to every configured writer route.
Application code therefore does not need to open files, know terminal styles, or wait for a network destination while it
is composing a message.

The :cpp:class:`LogWriter <erbsland::log::LogWriter>` base class represents one such destination.
A writer receives both the original immutable entry and the formatted line.
Built-in writers use whichever representation their destination needs: the console uses semantic line parts for styling,
the file writes the completed line, and syslog combines entry metadata with the completed line.

A writer is always installed as part of a *route*.
Each route combines one writer with a :cpp:class:`LogWriterFilter <erbsland::log::LogWriterFilter>`.
The filter answers whether a particular level and stream path belong at that destination.
If several routes accept the same entry, all their writers receive it; routing does not select a single winner.
This is what lets one error appear in a durable file, on the operator's console, and in the application's final failure
report without three logging calls in the application.

Add a Writer to the Configuration
=================================

Create a built-in writer directly in the call to :cpp:func:`addWriter() <erbsland::log::LogConfiguration::addWriter>` on
the :cpp:class:`LogConfiguration <erbsland::log::LogConfiguration>` that you are preparing.
Calling ``addWriter(writer)`` without a filter creates the broadest route: it accepts all levels from all stream paths.
Named trace streams still require their trace section to be enabled; the filter only decides whether a writer route is
ready to receive that trace entry.
You can add a filter as the second argument when the destination should see only part of the log.

Writers are part of the complete configuration snapshot installed with
:cpp:func:`setConfiguration() <erbsland::log::LogManager::setConfiguration>`.
Replacing that snapshot also replaces its ordinary writer routes, so assemble all destinations before installing it.
The :cpp:class:`Application <erbsland::core::Application>` class already owns a manager and initially configures a
console route for information, warning, and error messages.
Use :cpp:func:`application().log() <erbsland::core::Application::log>` when configuring a regular application.
Create a separate manager only for a standalone component that is not governed by the application lifecycle.

The following example installs a console writer and a compact line format in the application's manager.
Notice that the stream does not know which writer receives its message; it only carries the meaningful
``guild/dispatch`` name.

.. erbsland-demo::
    :source: log/LoggingTopics/WriterSetup.cpp
    :exec: log/logging_topics --demo WriterSetup
    :source-sha256: 1a59e284ce8c40850094d4f21fbe317642c32c9c2838c5c2e9d17c6073b89749

.. code-block:: cpp

    /// A writer becomes part of the log system when it is added to a complete `LogConfiguration` snapshot.
    ///
    /// With no explicit filter, the route accepts every log level and stream path. Named trace streams still require an
    /// enabled trace section. A regular application installs the snapshot in its existing log manager and lets the
    /// application shut that manager down automatically when it exits.
    void writerSetup() {
        auto format = el::LogLineFormat{};
        format.setPattern("{level} [{name}] {message}"_el);

        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

        auto &manager = el::application().log();
        manager.setConfiguration(std::move(configuration));
        manager.createStream("guild/dispatch"_el)->info("The western trail report is ready."_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97mINF [␛[95mguild/dispatch␛[97m] The western trail report is ready.␛[0m

.. erbsland-demo-end::

Route the Right Entries to Each Writer
======================================

Most applications do not want every destination to receive exactly the same traffic.
A console used by a person may show normal progress, while a remote operations channel should see only warnings and
errors.
Likewise, one team may keep a separate file for the ``guild/archive`` subsystem without changing how that subsystem
writes messages.

A default :cpp:class:`LogWriterFilter <erbsland::log::LogWriterFilter>` accepts every level and every path.
Construct it with :cpp:type:`LogLevels <erbsland::log::LogLevels>` or call
:cpp:func:`setLevels() <erbsland::log::LogWriterFilter::setLevels>` to choose a severity set.
Call :cpp:func:`addPath() <erbsland::log::LogWriterFilter::addPath>` for each accepted path root.
An empty path list means all paths; once you add roots, an entry must be located below at least one of them.

Path matching follows complete path segments.
The root ``guild`` accepts both ``guild`` and ``guild/archive``, but it does not accidentally accept ``guildhall``.
Level and path restrictions are combined: an entry reaches a route only when both match.
Filters on different routes remain independent, and overlapping matches deliberately fan an entry out to several
writers.

This example gives the operations writer warnings and errors from the whole ``guild`` tree.
The archive writer receives all ordinary levels from the narrower ``guild/archive`` tree.
The warning from that stream reaches both writers, while the similarly named ``guildhall`` stream matches neither.

.. erbsland-demo::
    :source: log/LoggingTopics/WriterRouting.cpp
    :exec: log/logging_topics --demo WriterRouting
    :source-sha256: 9d7401a45170d13a5aa0c1a1ed8cea53504a7b1416f36950391ac5bebdeb86e9

.. code-block:: cpp

    /// Writer filters combine accepted levels with zero or more hierarchical path roots.
    ///
    /// Empty path filters accept every stream. Once roots are present, matching uses complete path segments, so a route
    /// for `guild` includes `guild/archive` but not `guildhall`. If several filters match, every matching writer receives
    /// the entry.
    void writerRouting() {
        const auto operations = std::make_shared<RouteCountingLogWriter>();
        const auto archive = std::make_shared<RouteCountingLogWriter>();

        auto operationsFilter = el::LogWriterFilter{el::LogLevels{el::LogLevel::Warning, el::LogLevel::Error}};
        operationsFilter.addPath(el::LogPath{"guild"_el});
        auto archiveFilter =
            el::LogWriterFilter{el::LogLevels{el::LogLevel::Information, el::LogLevel::Warning, el::LogLevel::Error}};
        archiveFilter.addPath(el::LogPath{"guild/archive"_el});

        auto configuration = el::LogConfiguration{};
        configuration.addWriter(operations, std::move(operationsFilter)).addWriter(archive, std::move(archiveFilter));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto archiveLog = manager->createStream("guild/archive"_el);
        const auto guildHallLog = manager->createStream("guildhall"_el);

        archiveLog->info("The autumn route ledger is ready."_el); // Archive only.
        archiveLog->warn("The archive door was left open."_el);   // Both writers.
        guildHallLog->error("The guildhall bell rope broke."_el); // Neither writer.
        manager->shutdown();

        el::io::printLine("Operational entries: "_el, operations->count());
        el::io::printLine("Archive entries    : "_el, archive->count());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Operational entries: 1
    Archive entries    : 2

.. erbsland-demo-end::

Choose a Writer for Each Audience
=================================

The built-in writers differ less by syntax than by audience and lifetime.
Choosing one becomes easier when you first ask who needs the record after the application has moved on.

.. list-table:: Built-in log writers
    :header-rows: 1
    :widths: 22 28 50

    * - Writer
      - Destination
      - A good fit when
    * - ``LogWriter::createForConsole()``
      - A terminal
      - A person is running or supervising the program and should see immediate, styled feedback.
    * - ``LogWriter::createForFile()``
      - A local text file and optional archives
      - The program needs durable local history for later diagnosis or routine service operation.
    * - ``LogWriter::createForSyslog()``
      - A syslog collector over UDP, TCP, or TLS
      - Operators collect logs centrally and need records after a host or process is unavailable.

These choices are complementary.
A foreground tool often needs only a console writer.
A long-running service commonly uses a file or syslog writer for its real history and enables the application's
retained-error report for a small, human-friendly failure summary.
The next sections show what each writer contributes and where its responsibility ends.

Show Immediate Feedback on the Console
======================================

The ``LogWriter::createForConsole()`` renders each accepted entry through an Erbsland Core
:cpp:class:`Terminal <erbsland::cterm::Terminal>`.
This makes it the natural choice for command-line applications, development tools, and services that are intentionally
supervised through a terminal.
It can style a complete line by severity and style individual semantic parts such as the level or stream name.
Paragraph options also keep long messages readable by controlling wrapping and continuation indentation.

Construct the writer with the application's terminal, normally obtained from
:cpp:func:`application().terminal() <erbsland::core::Application::terminal>`.
The application enables a terminal automatically when its default log system is first accessed; explicitly call
:cpp:func:`enableTerminal() <erbsland::core::Application::enableTerminal>` when terminal setup is part of your own
initialization sequence.
Then add the writer just like any other destination.

A console is immediate but not durable.
Once its scrollback disappears, so does its history.
For a background service, pair it with a file or syslog writer instead of treating terminal output as the only
operational record.

The example below combines stream-name styling with a warning style and paragraph wrapping.
The visible escape markers in the captured output are the terminal control sequences that produce those styles.

.. erbsland-demo::
    :source: log/LoggingTopics/ConsoleWriters.cpp
    :exec: log/logging_topics --demo ConsoleWriters
    :source-sha256: 107fb53b01808690118d2227aec2814b06a1c304eb98918900aa297b27cea9df

.. code-block:: cpp

    /// A console writer renders formatted log lines as terminal paragraphs.
    ///
    /// Create its options, pass them to the writer together with the application terminal, and add the writer to the
    /// complete log configuration. The application owns and shuts down its log manager automatically.
    void consoleWriters() {
        auto paragraph = el::cterm::ParagraphOptions{};
        paragraph.setWrappedLineIndent(4);

        auto writerOptions = el::ConsoleLogWriterOptions{};
        writerOptions.setParagraphOptions(std::move(paragraph));

        auto format = el::LogLineFormat{};
        format.setPattern("{level} [{name}] {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

        auto &manager = el::application().log();
        manager.setConfiguration(std::move(configuration));
        const auto log = manager.createStream("guild/weather"_el);
        log->info("Northern ridge observation opened."_el);
        log->warn("A snow squall is crossing the ridge."_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97mINF [␛[95mguild/weather␛[97m] Northern ridge observation opened.
    ␛[93mWRN [␛[95mguild/weather␛[93m] A snow squall is crossing the ridge.␛[0m

.. erbsland-demo-end::

For the complete style layering, paragraph defaults, and integration choices, continue with
:doc:`console_writers`.

Keep Durable Operational History in a File
==========================================

The ``LogWriter::createForFile()`` is the straightforward choice when logs must remain available after the terminal
closes or the process exits.
It suits desktop applications with a support log, local tools whose runs need an audit trail, and services where a
host-local file is part of the operating model.

Begin with :cpp:class:`FileLogWriterOptions <erbsland::log::FileLogWriterOptions>` and the active log path.
The options decide whether the first open appends to or replaces existing content and whether the writer rotates by size
or time.
Retention bounds the number of archives the writer creates, preventing an ordinary rotation policy from turning into
unbounded storage growth.
The writer also creates missing parent directories and recovers when an external rotation tool replaces the active file.

File output can fail for reasons outside the log system: a directory may become unwritable, a volume may fill, or a path
may disappear.
The writer retries recoverable failures, while the manager's statistics record delivery failures.
Applications that rely on the log for operations should therefore monitor those statistics or provide another route for
important errors.

This compiled example uses temporary storage, so it is safe to run repeatedly.
It demonstrates overwrite mode, small size rotation, and bounded retention, then verifies that the active log and its
first archive were created.

.. erbsland-demo::
    :source: log/LoggingTopics/FileWriters.cpp
    :exec: log/logging_topics --demo FileWriters
    :source-sha256: 70a0845e3e535e3c939d60bb7cef810c1baa9f64c3146033c3c1ffbd4451c523

.. code-block:: cpp

    /// File writers create parent directories, recover from external file replacement, and optionally rotate archives.
    ///
    /// Append or overwrite applies to the first successful open. Size rotation uses numbered archives, while hourly,
    /// daily, and weekly rotation derive archive names from entry timestamps.
    void fileWriters() {
        auto temporaryOptions = el::PathTempDirectoryOptions{};
        temporaryOptions.setPrefix("retkikunta-"_el).setRandomLength(el::CpLength{8U});
        const auto temporary =
            el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
        const auto path = temporary->path() / "guild.log"_el;

        auto writerOptions = el::FileLogWriterOptions{path};
        writerOptions.setMode(el::LogFileMode::Overwrite)
            .setRotation(el::LogFileRotation::Size)
            .setMaximumSize(el::ByteLength{150U})
            .setRetention(2U);

        auto format = el::LogLineFormat{};
        format.setPattern("{level} [{name}] {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format)).addWriter(el::LogWriter::createForFile(writerOptions));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->createStream("guild/journal"_el);
        log->info("Retkikunta Revontuli departed from the cedar gate."_el);
        log->info("The first camp was established beside Jääjärvi."_el);
        log->warn("Fresh snowfall covered the eastern trail markers."_el);
        log->info("The expedition returned with a complete route sketch."_el);
        manager->shutdown();

        el::io::printLine("Active log exists: "_el, el::BooleanFormat::yesNo(), path.info().exists());
        el::io::printLine(
            "First archive exists: "_el, el::BooleanFormat::yesNo(), path.withStem("guild.1"_el).info().exists());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Active log exists: yes
    First archive exists: yes

.. erbsland-demo-end::

The complete path, rotation, archive, retry, and failure-accounting behavior is described in :doc:`file_writers`.

Send Service Events to a Syslog Collector
=========================================

The ``LogWriter::createForSyslog()`` sends RFC 5424 messages to a remote collector.
It is useful when several services or hosts must feed the same operational view, or when log records should survive the
loss of the machine that produced them.
Instead of asking an operator to inspect individual files, the deployment can search and retain the records in one
place.

Configure the destination with :cpp:class:`SyslogLogWriterOptions <erbsland::log::SyslogLogWriterOptions>`.
UDP has low transport overhead but provides no delivery guarantee.
TCP preserves an ordered connection, while TLS adds transport security and uses a named network/TLS configuration.
The RFC header fields identify the host, application, process, and message category at the collector.
For most deployments, warnings and errors are a sensible first route; sending high-volume informational or trace data
should be a deliberate capacity decision.

Network delivery introduces back pressure that console and file destinations do not have in the same form.
The writer keeps a bounded amount of pending encoded data while a connection catches up or reconnects.
Once that bound is exhausted, it drops new messages and exposes the count through the manager's delivery statistics.
Choose the pending limit together with the expected message rate, and monitor drops if syslog is operationally
important.

Documentation demos must not contact an external collector.
The following example therefore constructs the exact writer route an application would install, then previews one
deterministic RFC 5424 message and its TCP/TLS frame without applying the configuration to a manager.

.. erbsland-demo::
    :source: log/LoggingTopics/SyslogWriters.cpp
    :exec: log/logging_topics --demo SyslogWriters
    :source-sha256: 7e1b0c3f3484a03895aeb133c0886290924eae412a0718dc97d6f6e94cce8186

.. code-block:: cpp

    /// A syslog writer sends RFC 5424 messages over UDP, TCP, or TLS.
    ///
    /// The options describe the endpoint, facility, RFC header fields, TLS configuration label, and bounded pending data.
    /// A route can be assembled without opening a connection.
    void syslogWriters() {
        auto options = el::SyslogLogWriterOptions{};
        options.setTransport(el::SyslogTransport::Tls)
            .setEndpoint(el::HostEndpoint::fromStringOrThrow("logs.example:6514"_el))
            .setFacility(1U)
            .setHostName("guild-hall"_el)
            .setApplicationName("explorer-guild"_el)
            .setProcessId("314"_el)
            .setMessageId("route"_el)
            .setTlsConfigurationLabel("guild/syslog"_el)
            .setMaximumPendingBytes(el::ByteLength{256U * 1024U});

        auto configuration = el::LogConfiguration{};
        configuration.addWriter(
            el::LogWriter::createForSyslog(options),
            el::LogWriterFilter{el::LogLevels{el::LogLevel::Warning, el::LogLevel::Error}});

        el::io::printLine("Syslog endpoint: "_el, options.endpoint().toString());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Syslog endpoint: logs.example:6514

.. erbsland-demo-end::

For endpoint rules, facilities, framing, TLS labels, retry behavior, and pending-byte limits, continue with
:doc:`syslog_writer`.

Recent Errors on Application Exit
=================================

Retained errors are an application-lifecycle feature.
Call :cpp:func:`enableLastErrorDump() <erbsland::core::Application::enableLastErrorDump>` before startup work can fail.
The application then keeps a bounded snapshot of recent errors and displays it during final cleanup when the run fails.
This setting is kept across later logging configuration replacements.

This report is valuable at an application boundary.
Imagine a service that normally writes to a file and exits because startup failed.
The file remains the complete record, but repeating the last few errors on the terminal gives the person who started the
service an immediate explanation without flooding them with the whole log.

By default, a nonempty report is displayed only after a failed run.
Pass :cpp:enumerator:`LastErrorDumpMode::Always <erbsland::core::LastErrorDumpMode::Always>` when an application also
needs the report after a successful run.
The following application enables the default failure-only mode, writes two errors, and returns exit code two.

.. erbsland-demo::
    :source: log/LoggingSetups/LastErrorDump.cpp
    :exec: log/logging_setups last-error-dump
    :exec-exit-code: 2
    :source-sha256: a9bec3da3a8910e5a2761bff83932159d256a12c454e5c19938d9655b5e78b0a

.. code-block:: cpp

    /// Display recent errors when an application exits with a failure.
    ///
    /// Enable the retained-error safety net before startup work can fail. The application keeps these errors across later
    /// logging configuration changes and displays them during final cleanup when `main()` returns a failure exit code.
    auto runLastErrorDump(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.enableLastErrorDump();
        app.setMainFn([&app]() -> el::ExitCode {
            const auto log = app.log().createStream("guild/expedition"_el);
            log->info("The expedition entered the northern pass."_el);
            log->error("The reserve compass failed its check."_el);
            log->error("The return route is blocked by ice."_el);
            return el::ExitCode{2};
        });
        return app.run();
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[96m2026-09-04 19:19:06Z␛[97m INF - The expedition entered the northern pass.
    ␛[96m2026-09-04 19:19:06Z␛[91m ERR - The reserve compass failed its check.
    ␛[96m2026-09-04 19:19:06Z␛[91m ERR - The return route is blocked by ice.

    ␛[39;1mRecent␛[22m ␛[1mError␛[22m ␛[1mLog␛[22m ␛[1mEntries

    ␛[22;96m2026-09-04 19:19:06Z␛[91m ERR - The reserve compass failed its check.
    ␛[96m2026-09-04 19:19:06Z␛[91m ERR - The return route is blocked by ice.␛[0m

.. erbsland-demo-end::

Introduce a Custom Destination Only When Necessary
==================================================

If a deployment has a destination that none of the built-in writers represent, derive a class from
:cpp:class:`LogWriter <erbsland::log::LogWriter>` and implement :cpp:func:`write()
<erbsland::log::LogWriter::write>`.
The manager provides the immutable entry and formatted line, and the default
:cpp:func:`writeBatch() <erbsland::log::LogWriter::writeBatch>` implementation forwards a batch one item at a time.
A custom writer may override batching, flushing, and closing when its destination benefits from those operations.

Keep this extension at the destination boundary.
Routing still belongs in :cpp:class:`LogWriterFilter <erbsland::log::LogWriterFilter>`, and formatting still belongs in
:cpp:class:`LogLineFormat <erbsland::log::LogLineFormat>`.
Writer calls do not run on the producer thread, but a slow custom writer can still delay the other writers attached to
the same manager.
Let delivery exceptions propagate so the manager can contain and count them, and do not share one writer instance
between active managers.

Custom writer design is intentionally outside this introductory topic.
In most applications, combining the built-in writers with well-chosen filters is both simpler and easier to operate.
