.. index::
    single: Logging; File Writer
    single: Log Writers; File
    single: Log Files; Append and Overwrite
    single: Log Files; Rotation
    single: Log Files; Retention

*********************
Writing Logs to Files
*********************

The ``LogWriter::createForFile()`` gives an application durable local history.
A terminal tells the person who is watching what happens now; a log file lets someone return hours or days later and
reconstruct what the process saw.
That makes file logging useful for background services, desktop support logs, long-running tools, and any application
where the interesting failure may be reported after the original process has already exited.

A useful file writer has to do more than append text.
It must decide what an existing file means at startup, keep growth bounded, preserve recognizable archive names, and
recover when an external log collector moves or replaces the active file.
Erbsland Core keeps those policies in :cpp:class:`FileLogWriterOptions <erbsland::log::FileLogWriterOptions>` so the
writer can be configured as one coherent destination.

This page first builds a normal file route, then explains the required path and every file-writer option in detail.
The line content itself comes from :cpp:class:`LogLineFormat <erbsland::log::LogLineFormat>`, while levels and paths are
selected by the route described in :doc:`using_writers`.

Add a File Writer to a Log Configuration
========================================

Begin with the active :cpp:class:`Path <erbsland::path::Path>` and use it to construct
:cpp:class:`FileLogWriterOptions <erbsland::log::FileLogWriterOptions>`.
Configure the policies needed by the application, pass the finished options to ``LogWriter::createForFile()``, and add
the shared writer through
:cpp:func:`addWriter() <erbsland::log::LogConfiguration::addWriter>`.
The route filter can then restrict the file to selected levels or stream trees without changing the file options.

The following example uses temporary storage because a documentation demo must be safe to run repeatedly.
A real application would normally choose a stable path from its application-data or service-state directory.
The example uses a standalone manager so it can call :cpp:func:`shutdown() <erbsland::log::LogManager::shutdown>` and
inspect the files immediately afterward.
An :cpp:class:`Application <erbsland::core::Application>` shuts its own manager down automatically during cleanup.

.. erbsland-demo::
    :source: log/LoggingTopics/FileWriters.cpp
    :exec: log/logging_topics --demo FileWriters
    :sha256: 2d672727f35130221cdef1ce60c11c0973815868f9748b196b72ebd7d5b9b018

.. code-block:: cpp

    /// File writers create parent directories, recover from external file replacement, and optionally rotate archives.
    ///
    /// Append or overwrite applies to the first successful open. Size rotation uses numbered archives, while hourly,
    /// daily, and weekly rotation derive archive names from entry timestamps.
    void fileWriters() {
        auto temporaryOptions = el::PathTempDirectoryOptions{};
        temporaryOptions.setPrefix("retkikunta-"_el).setRandomLength(el::CpCount{8U});
        const auto temporary =
            el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
        const auto path = temporary->path() / "guild.log"_el;

        auto writerOptions = el::FileLogWriterOptions{path};
        writerOptions.setMode(el::LogFileMode::Overwrite)
            .setRotation(el::LogFileRotation::Size)
            .setMaximumSize(el::ByteCount{150U})
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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mActive log exists: yes
        First archive exists: yes␛[0m

.. erbsland-demo-end::

File Writer Path and Options
============================

Unlike the other file settings, the active path is required by the
:cpp:class:`FileLogWriterOptions <erbsland::log::FileLogWriterOptions>` constructor and has no later setter.
This keeps an options object from ever representing a file policy without a destination.
The path must be nonempty; ``LogWriter::createForFile()`` validates it when constructed.

The writer uses the path exactly as supplied.
Resolve relative paths according to your application's path policy before building the options when the working
directory is not a stable location.
On the first write, the writer creates missing parent directories and opens the active file.
Archive paths are derived from this same base path, retaining its parent directory and extension.
For ``journal/guild.log``, size rotation produces names such as ``journal/guild.1.log`` rather than changing the
extension to an archive-specific one.

Choose Append or Overwrite for the First Open
=============================================

:cpp:func:`setMode() <erbsland::log::FileLogWriterOptions::setMode>` selects the
:cpp:class:`LogFileMode <erbsland::log::LogFileMode>` used for the first successful open.
:cpp:enumerator:`Append <erbsland::log::LogFileMode::Append>` is the default and preserves existing content before
writing the new run beneath it.
This is usually the right choice for a service log whose active file spans process restarts.

:cpp:enumerator:`Overwrite <erbsland::log::LogFileMode::Overwrite>` replaces existing content on that first open.
It suits per-run reports and temporary diagnostics where mixing two runs would be misleading.
The distinction is intentionally limited to the first successful open of one writer.
If the file later disappears or is replaced, recovery reopens it in append mode so an operational interruption cannot
erase content produced since the current application started.

The demo prepares two files with text from an earlier run, then writes the same current entry with each mode.
It checks the resulting content rather than relying on file sizes, making the behavioral difference explicit.

.. erbsland-demo::
    :source: log/LoggingTopics/FileWriterOptions.cpp
    :show: f:fileWriterModes
    :exec: log/logging_topics --demo FileWriterModes
    :sha256: 81c5370792bfd16262c438ba6df65d341a909abd144d8d88b035ff07543a6a9a

.. code-block:: cpp

    void fileWriterModes() {
        const auto temporary = createFileWriterDemoDirectory();
        const auto appendPath = temporary->path() / "append.log"_el;
        const auto overwritePath = temporary->path() / "overwrite.log"_el;
        appendPath.content().writeTextOrThrow("Earlier run\n"_el);
        overwritePath.content().writeTextOrThrow("Earlier run\n"_el);

        const auto appendWriter = el::LogWriter::createForFile(el::FileLogWriterOptions{appendPath});
        writeFileDemoLine(appendWriter, el::Timestamp::now(), "Current run"_el);
        appendWriter->close();

        auto overwriteOptions = el::FileLogWriterOptions{overwritePath};
        overwriteOptions.setMode(el::LogFileMode::Overwrite);
        const auto overwriteWriter = el::LogWriter::createForFile(overwriteOptions);
        writeFileDemoLine(overwriteWriter, el::Timestamp::now(), "Current run"_el);
        overwriteWriter->close();

        const auto yesNo = el::BooleanFormat::yesNo();
        el::io::printLine(
            "Append preserved earlier content   : "_el,
            yesNo,
            appendPath.content().readTextOrThrow().contains("Earlier run"_el));
        el::io::printLine(
            "Overwrite preserved earlier content: "_el,
            yesNo,
            overwritePath.content().readTextOrThrow().contains("Earlier run"_el));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mAppend preserved earlier content   : yes
        Overwrite preserved earlier content: no␛[0m

.. erbsland-demo-end::

Time-Based and Size-Based File Rotation
=======================================

:cpp:func:`setRotation() <erbsland::log::FileLogWriterOptions::setRotation>` chooses a
:cpp:class:`LogFileRotation <erbsland::log::LogFileRotation>` policy.
The default :cpp:enumerator:`None <erbsland::log::LogFileRotation::None>` leaves one active file in place indefinitely.
This is simple, but it also leaves growth entirely to an external collector or storage policy.

Three policies rotate on time boundaries.
:cpp:enumerator:`Hourly <erbsland::log::LogFileRotation::Hourly>` uses the key ``YYYY-MM-DD-HH``,
:cpp:enumerator:`Daily <erbsland::log::LogFileRotation::Daily>` uses ``YYYY-MM-DD``, and
:cpp:enumerator:`Weekly <erbsland::log::LogFileRotation::Weekly>` uses the date of the week's starting day.
The writer compares UTC timestamps stored in log entries, so its boundaries do not move with the host's local time or
daylight-saving changes.
When the first entry with a new key arrives, the old active file becomes the archive for its previous key and the new
entry is written to a fresh active file.

:cpp:enumerator:`Size <erbsland::log::LogFileRotation::Size>` instead compares the active byte count with the configured
maximum before each write.
It uses numbered archives: the previous active file becomes ``.1``, the previous ``.1`` becomes ``.2``, and so on up to
the retention limit.
The next section explains the threshold that drives this mode.

This deterministic example sends entries from two UTC dates directly to a daily writer.
The second entry rotates the first day's active file into ``guild.2026-09-01.log``.

.. erbsland-demo::
    :source: log/LoggingTopics/FileWriterOptions.cpp
    :show: f:fileWriterRotation
    :exec: log/logging_topics --demo FileWriterRotation
    :sha256: 245f46da17bc0250a38e1b0355c0cedfdefbb5f8f053661a4220fb700a74f6ac

.. code-block:: cpp

    void fileWriterRotation() {
        const auto temporary = createFileWriterDemoDirectory();
        const auto path = temporary->path() / "guild.log"_el;
        auto options = el::FileLogWriterOptions{path};
        options.setMode(el::LogFileMode::Overwrite).setRotation(el::LogFileRotation::Daily);
        const auto writer = el::LogWriter::createForFile(options);

        writeFileDemoLine(
            writer,
            el::Timestamp{el::Date::fromYearMonthDay(2026, 9, 1), el::Time{el::Hour{18}, el::Minute{0}}},
            "First day"_el,
            1U);
        writeFileDemoLine(
            writer,
            el::Timestamp{el::Date::fromYearMonthDay(2026, 9, 2), el::Time{el::Hour{7}, el::Minute{0}}},
            "Second day"_el,
            2U);
        writer->close();

        const auto archive = path.withStem("guild.2026-09-01"_el);
        el::io::printLine("Daily archive: "_el, archive.name());
        el::io::printLine("Archive exists: "_el, el::BooleanFormat::yesNo(), archive.info().exists());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mDaily archive: guild.2026-09-01.log
        Archive exists: yes␛[0m

.. erbsland-demo-end::

Set the Byte Threshold for Size Rotation
========================================

:cpp:func:`setMaximumSize() <erbsland::log::FileLogWriterOptions::setMaximumSize>` supplies the
:cpp:type:`ByteCount <erbsland::unit::ByteCount>` threshold used only when rotation is
:cpp:enumerator:`Size <erbsland::log::LogFileRotation::Size>`.
The default is 10 MiB, or 10,485,760 bytes.
A zero threshold is invalid for size rotation and is rejected when the writer is constructed.

The writer tracks the bytes already present after opening the file and adds the encoded length of every formatted line
plus its line break.
Before writing a new line, it rotates a nonempty active file if the complete new line would cross the threshold.
It never splits a log line across files.
Consequently, a single line larger than the configured maximum is still written intact to an empty active file; the
maximum is a rotation boundary, not a producer-side message limit.
Use :cpp:func:`setMaximumMessageBytes() <erbsland::log::LogManagerOptions::setMaximumMessageBytes>` when individual
producer messages themselves must be bounded.

Here, the first observation fits in a small active file and the second complete line would cross the limit, creating the
first numbered archive.

.. erbsland-demo::
    :source: log/LoggingTopics/FileWriterOptions.cpp
    :show: f:fileWriterMaximumSize
    :exec: log/logging_topics --demo FileWriterMaximumSize
    :sha256: fb54184b3526b2377d5ba47fe688e72743c1e8aaf7b4c84ebaee875c9d473bff

.. code-block:: cpp

    void fileWriterMaximumSize() {
        const auto temporary = createFileWriterDemoDirectory();
        const auto path = temporary->path() / "guild.log"_el;
        auto options = el::FileLogWriterOptions{path};
        options.setMode(el::LogFileMode::Overwrite)
            .setRotation(el::LogFileRotation::Size)
            .setMaximumSize(el::ByteCount{24U});
        const auto writer = el::LogWriter::createForFile(options);

        writeFileDemoLine(writer, el::Timestamp::now(), "First observation"_el, 1U);
        writeFileDemoLine(writer, el::Timestamp::now(), "Second observation"_el, 2U);
        writer->close();

        el::io::printLine(
            "Rotation created guild.1.log: "_el, el::BooleanFormat::yesNo(), path.withStem("guild.1"_el).info().exists());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mRotation created guild.1.log: yes␛[0m

.. erbsland-demo-end::

Rotated Archive Retention
=========================

:cpp:func:`setRetention() <erbsland::log::FileLogWriterOptions::setRetention>` sets how many rotated archives the
writer retains.
The default is seven.
The active file is not counted in this number, so a retention of two permits the active file plus two archives.

For size rotation, the writer manages numbered archives already present beside the active file.
Each rotation shifts them toward larger numbers and removes anything beyond the limit.
For hourly, daily, and weekly rotation, it tracks archives created during the current writer lifetime and removes the
oldest tracked archive when the configured count is exceeded.
Time-keyed archives that predate the current writer are not discovered and pruned automatically; use an external storage
policy when retention must span process restarts.

Zero is a valid retention count.
With size rotation it removes the rotated active file instead of preserving an archive.
With time rotation, the newly created time-keyed archive is removed as soon as the retention queue is enforced.
Choose zero when rotation should bound the active file but historical files have no value.

The following size-rotation demo performs enough writes to require three generations.
With retention set to two, the first two numbered archives remain and ``guild.3.log`` does not.

.. erbsland-demo::
    :source: log/LoggingTopics/FileWriterOptions.cpp
    :show: f:fileWriterRetention
    :exec: log/logging_topics --demo FileWriterRetention
    :sha256: 4d1124f76828a22e5989f924509160959ad146864f4720707401630ec1486517

.. code-block:: cpp

    void fileWriterRetention() {
        const auto temporary = createFileWriterDemoDirectory();
        const auto path = temporary->path() / "guild.log"_el;
        auto options = el::FileLogWriterOptions{path};
        options.setMode(el::LogFileMode::Overwrite)
            .setRotation(el::LogFileRotation::Size)
            .setMaximumSize(el::ByteCount{12U})
            .setRetention(2U);
        const auto writer = el::LogWriter::createForFile(options);

        for (auto sequence = uint64_t{1U}; sequence <= 4U; ++sequence) {
            writeFileDemoLine(writer, el::Timestamp::now(), "Entry 0001"_el, sequence);
        }
        writer->close();

        const auto yesNo = el::BooleanFormat::yesNo();
        el::io::printLine("guild.1.log exists: "_el, yesNo, path.withStem("guild.1"_el).info().exists());
        el::io::printLine("guild.2.log exists: "_el, yesNo, path.withStem("guild.2"_el).info().exists());
        el::io::printLine("guild.3.log exists: "_el, yesNo, path.withStem("guild.3"_el).info().exists());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mguild.1.log exists: yes
        guild.2.log exists: yes
        guild.3.log exists: no␛[0m

.. erbsland-demo-end::

External File Replacement and Automatic Reopening
=================================================

Some deployments rotate logs outside the application.
A collector may rename the active file and create a replacement at the configured path, or an administrator may remove
and reconstruct its directory.
Keeping the original open handle forever would send new entries into the moved file and leave the expected active path
quiet.

The file writer periodically compares the identity of its open stream with the identity currently found at the active
path.
If the path is missing or points to a different file, the writer closes the stale stream and reopens the configured path
in append mode.
No signal or explicit reopen call is required from application code.
This check complements built-in rotation: choose either policy according to the deployment, but both use the same stable
active path.

File Writer Failures and Recovery
=================================

Opening, writing, rotating, and flushing all depend on the host filesystem.
A directory may become unwritable, a volume may be full, or an external collector may temporarily leave an invalid path.
When an operation fails, the writer closes its stream and schedules another attempt with an increasing delay, capped at
30 seconds.
The failed delivery is not silently replayed later; retrying repairs the destination for future entries.

The manager contains concrete :cpp:class:`RuntimeError <erbsland::err::RuntimeError>` subclasses raised by writers,
and increments
:cpp:member:`writerFailures <erbsland::log::LogManagerStatistics::writerFailures>` in its
:cpp:struct:`LogManagerStatistics <erbsland::log::LogManagerStatistics>` snapshot.
Monitor that counter when the file is operationally important, and consider a second destination for high-severity
events.
A retained-error writer can improve the final console report, but it is memory only and does not replace durable
history.

During orderly shutdown, the manager drains accepted entries, flushes writers, and closes their resources.
:cpp:class:`Application <erbsland::core::Application>` performs this automatically.
Only code that owns a standalone :cpp:class:`LogManager <erbsland::log::LogManager>` must call
:cpp:func:`shutdown() <erbsland::log::LogManager::shutdown>` itself before it reads, moves, or otherwise depends on the
completed file.
