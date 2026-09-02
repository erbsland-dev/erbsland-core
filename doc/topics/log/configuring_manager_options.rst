.. index::
    single: Logging; Queue Limits
    single: Log Manager; Limits
    single: Log Manager; Statistics
    single: Log Manager; Shutdown Timeout

******************************
Configuring Log Manager Limits
******************************

Logging moves destination work away from application threads, but that separation needs clear boundaries.
If a file system or network destination slows down, the manager must know how much work it may retain, which messages
deserve protected space, and how long shutdown may wait for the worker to catch up.
This page introduces :cpp:class:`LogManagerOptions <erbsland::log::LogManagerOptions>` and develops each boundary
independently, so you can keep the defaults where they fit and change only the limits your application has reason to
change.

Install the Options With the Complete Configuration
===================================================

:cpp:class:`LogManagerOptions <erbsland::log::LogManagerOptions>` is a value object containing the queue, producer-message,
and shutdown limits for one manager configuration.
Construct it locally, call the setters for the boundaries you want to change, and pass the completed value to
:cpp:func:`LogConfiguration::setManagerOptions <erbsland::log::LogConfiguration::setManagerOptions>`.
The options become active when :cpp:func:`LogManager::setConfiguration <erbsland::log::LogManager::setConfiguration>`
installs that complete configuration snapshot.

An application-owned manager already exists when application initialization begins; configure that manager rather than
constructing another one.
The demos on this page use standalone managers only to make every example independent.
A standalone manager can also receive options in :cpp:func:`LogManager::create <erbsland::log::LogManager::create>` when
its limits must apply before the first complete writer configuration is ready.
Remember that a later configuration is a replacement snapshot: include the intended manager options in it instead of
expecting values from an earlier snapshot to survive.

The defaults are 4,096 total queue entries, 8 MiB of total queue storage, 256 protected warning/error entries, 1 MiB of
protected warning/error storage, 256 KiB for one producer message, and two seconds for graceful shutdown.
They are a sensible starting point for ordinary tools and services.
Tune them from observed traffic and destination latency rather than increasing every value pre-emptively.

.. erbsland-demo::
    :source: log/LoggingTopics/ManagerOptions.cpp
    :exec: log/logging_topics --demo ManagerOptions
    :source-sha256: 441b4d0429b3362d92419dc9025f89afea80aaca2e23671b547e99d2e5ec16c5

.. code-block:: cpp

    /// Install manager limits as part of a complete logging configuration.
    ///
    /// Construct `LogManagerOptions` as a value, customize it, and move it into `LogConfiguration`. Passing the completed
    /// configuration to `setConfiguration()` makes these limits active together with the writer routes.
    void managerOptions() {
        auto options = el::LogManagerOptions{};
        options.setMaximumEntries(2048U);

        auto configuration = el::LogConfiguration{};
        configuration.setManagerOptions(std::move(options))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto activeConfiguration = manager->configuration();
        el::io::printLine("Active queue-entry limit: "_el, activeConfiguration.managerOptions().maximumEntries());
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Active queue-entry limit: 2048

.. erbsland-demo-end::

Bound the Number of Waiting Entries
===================================

:cpp:func:`LogManagerOptions::setMaximumEntries <erbsland::log::LogManagerOptions::setMaximumEntries>` sets the total
number of entries that may wait in the manager queue.
The default is 4,096. The limit includes the warning/error reservation discussed below, but it does not include an entry
that the worker has already handed to a writer.
This distinction matters when diagnosing a slow destination: one entry may be inside the writer while the complete queue
capacity waits behind it.

Entry count protects the manager from a burst of many small messages.
When the applicable capacity is full, a producer is not made to wait for the worker; its new entry is dropped and
``droppedEntries`` increases.
That non-blocking behavior keeps logging from turning a slow diagnostic destination into application-wide backpressure.
The setter requires a positive value and raises :cpp:class:`ParameterError <erbsland::err::ParameterError>` for zero.

The demo holds one writer call open, fills all 260 queue slots with warnings, and then submits one more warning.
Warnings are used so the example exercises the complete entry capacity rather than stopping at the ordinary-entry
boundary.

.. erbsland-demo::
    :source: log/LoggingTopics/ManagerEntryCapacity.cpp
    :exec: log/logging_topics --demo ManagerEntryCapacity
    :source-sha256: 9dbc61c41d67862480d84d807d84678d3fe545581ce60d60a1d5a93d083946c1

.. code-block:: cpp

    /// Bound the number of entries waiting behind a slow writer.
    ///
    /// `setMaximumEntries()` limits queued entries, including warning and error reservations. The entry already inside the
    /// writer is no longer in that queue, so 260 further warnings fit and the next one is dropped without blocking.
    void managerEntryCapacity() {
        auto options = el::LogManagerOptions{};
        options.setMaximumEntries(260U);
        const auto writer = std::make_shared<BlockingLogWriter>();
        auto configuration = el::LogConfiguration{};
        configuration.setManagerOptions(options).addWriter(writer);

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->rootStream();
        log->warn("The journal destination is occupied."_el);
        writer->waitUntilWriting();

        for (auto index = 0U; index < 260U; ++index) {
            log->warn("A route report is waiting."_el);
        }
        log->warn("This report exceeds the queue-entry limit."_el);
        const auto full = manager->statistics();
        el::io::printLine("Queued entries: "_el, full.queuedEntries);
        el::io::printLine("Dropped entries: "_el, full.droppedEntries);

        writer->release();
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Queued entries: 260
    Dropped entries: 1

.. erbsland-demo-end::

Bound the Storage Retained by the Queue
=======================================

An entry count alone cannot protect memory when messages vary greatly in size.
:cpp:func:`LogManagerOptions::setMaximumBytes <erbsland::log::LogManagerOptions::setMaximumBytes>` therefore sets an
independent total byte capacity, which defaults to 8 MiB.
Every new entry must fit both the applicable entry limit and the applicable byte limit; reaching either boundary is
enough to drop it.

Treat this setting as a budget for the queued entries as a whole, not as the sum of source-message lengths you happen to
see in a test.
The manager's statistics provide the authoritative current queue usage.
As with the entry setter, the byte limit must be positive and zero raises
:cpp:class:`ParameterError <erbsland::err::ParameterError>`.

In the demo the queue still has thousands of unused entry slots, but four large warnings consume almost all of a 1 MiB
byte budget.
The fifth is dropped because its retained data no longer fits.

.. erbsland-demo::
    :source: log/LoggingTopics/ManagerByteCapacity.cpp
    :exec: log/logging_topics --demo ManagerByteCapacity
    :source-sha256: 203d0901924ed8a4dde457a4e54bab20452cffe084f2550b038fe7d9772465cf

.. code-block:: cpp

    /// Bound the memory retained by entries waiting in the queue.
    ///
    /// `setMaximumBytes()` applies even when many entry slots remain. Four large warnings fit below this one-mebibyte byte
    /// budget; another large warning is dropped while the producer continues immediately.
    void managerByteCapacity() {
        auto options = el::LogManagerOptions{};
        options.setMaximumBytes(el::ByteLength{1024U * 1024U});
        const auto writer = std::make_shared<BlockingLogWriter>();
        auto configuration = el::LogConfiguration{};
        configuration.setManagerOptions(options).addWriter(writer);

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->rootStream();
        log->warn("The journal destination is occupied."_el);
        writer->waitUntilWriting();

        const auto largeReport = el::String::fromCharacter(el::text::Char{U'x'}, el::CpLength{250U * 1024U});
        for (auto index = 0U; index < 4U; ++index) {
            log->warn(largeReport);
        }
        log->warn(largeReport);
        const auto full = manager->statistics();
        el::io::printLine("Queued large entries: "_el, full.queuedEntries);
        el::io::printLine("Dropped entries: "_el, full.droppedEntries);

        writer->release();
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Queued large entries: 4
    Dropped entries: 1

.. erbsland-demo-end::

Protect Entry Slots for Warnings and Errors
===========================================

A flood of routine information should not crowd out the warning that explains why a service is struggling.
:cpp:func:`LogManagerOptions::setReservedErrorEntries <erbsland::log::LogManagerOptions::setReservedErrorEntries>` keeps
part of the total entry capacity available to warnings and errors.
The default reservation is 256 entries.
Trace and information entries can use only the unreserved portion; warnings and errors can use both the ordinary and
protected portions.

The reservation is not an additional queue and does not increase ``maximumEntries``.
For example, a total of four entries with two reserved entries gives trace and information traffic two available slots,
while warning and error traffic can fill all four.
A reservation of zero lets every level compete for the same capacity.
:cpp:func:`LogConfiguration::setManagerOptions <erbsland::log::LogConfiguration::setManagerOptions>` validates the
completed pair and raises :cpp:class:`ParameterError <erbsland::err::ParameterError>` if the reservation exceeds the
total.

The demo fills both ordinary slots, shows that another information entry is dropped, and then uses the two protected
slots for a warning and an error.
Only after the complete four-entry queue is full is another error dropped.

.. erbsland-demo::
    :source: log/LoggingTopics/ManagerErrorEntryReserve.cpp
    :exec: log/logging_topics --demo ManagerErrorEntryReserve
    :source-sha256: 6fc3a90b539a486c07b81eb2211a7f02da711abe4aaf1c6bc8e82a6df0d8bdc3

.. code-block:: cpp

    /// Keep queue slots available for warning and error entries.
    ///
    /// The total capacity is four entries and `setReservedErrorEntries()` protects two of them. Information fills the two
    /// ordinary slots, another information entry is dropped, and warning plus error can still use the protected slots.
    void managerErrorEntryReserve() {
        auto options = el::LogManagerOptions{};
        options.setMaximumEntries(4U).setReservedErrorEntries(2U);
        const auto writer = std::make_shared<BlockingLogWriter>();
        auto configuration = el::LogConfiguration{};
        configuration.setManagerOptions(options).addWriter(writer);

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->rootStream();
        log->info("The journal destination is occupied."_el);
        writer->waitUntilWriting();

        log->info("Ordinary slot one."_el);
        log->info("Ordinary slot two."_el);
        log->info("No ordinary slot remains."_el);
        log->warn("The warning uses a protected slot."_el);
        log->error("The error uses the final protected slot."_el);
        log->error("The complete queue is now full."_el);
        const auto full = manager->statistics();
        el::io::printLine("Queued entries: "_el, full.queuedEntries);
        el::io::printLine("Dropped entries: "_el, full.droppedEntries);

        writer->release();
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Queued entries: 4
    Dropped entries: 2

.. erbsland-demo-end::

Protect Queue Storage for Warnings and Errors
=============================================

The entry reservation is only half of the protection because a few large information messages could otherwise consume
the byte budget first.
:cpp:func:`LogManagerOptions::setReservedErrorBytes <erbsland::log::LogManagerOptions::setReservedErrorBytes>` reserves
storage within ``maximumBytes`` using the same level policy.
Its default is 1 MiB.
Trace and information entries stay within the unreserved byte budget, while warnings and errors may use the complete
byte capacity.

This reservation also remains part of, rather than additional to, the total.
Set it to zero when no byte protection is wanted.
The completed configuration is rejected with :cpp:class:`ParameterError <erbsland::err::ParameterError>` if the reserved
byte count exceeds ``maximumBytes``.
Entry and byte reservations are evaluated independently, so an entry must have both a slot and enough storage in the
capacity available to its level.

The demo gives ordinary traffic 1.5 KiB of a 2 KiB total.
After a large information entry uses most of that ordinary budget, another information entry is dropped; a warning of
the same size still fits by using the protected 512 bytes.

.. erbsland-demo::
    :source: log/LoggingTopics/ManagerErrorByteReserve.cpp
    :exec: log/logging_topics --demo ManagerErrorByteReserve
    :source-sha256: 31e6e5c0380c1c8fcd2b7607b85ecdbc135748301683097999b0bbc83c0644d2

.. code-block:: cpp

    /// Keep part of the queue's byte budget available for warning and error entries.
    ///
    /// `setReservedErrorBytes()` protects 512 bytes within a two-kibibyte total. A large information entry consumes most of
    /// the ordinary budget, so another is dropped while a warning of the same size can use the reservation.
    void managerErrorByteReserve() {
        auto options = el::LogManagerOptions{};
        options.setMaximumBytes(el::ByteLength{2048U}).setReservedErrorBytes(el::ByteLength{512U});
        const auto writer = std::make_shared<BlockingLogWriter>();
        auto configuration = el::LogConfiguration{};
        configuration.setManagerOptions(options).addWriter(writer);

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->rootStream();
        log->info("The journal destination is occupied."_el);
        writer->waitUntilWriting();

        log->info(el::String::fromCharacter(el::text::Char{U'i'}, el::CpLength{1200U}));
        const auto shortReport = el::String::fromCharacter(el::text::Char{U'w'}, el::CpLength{300U});
        log->info(shortReport);
        log->warn(shortReport);
        const auto full = manager->statistics();
        el::io::printLine("Queued entries: "_el, full.queuedEntries);
        el::io::printLine("Dropped entries: "_el, full.droppedEntries);

        writer->release();
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Queued entries: 2
    Dropped entries: 1

.. erbsland-demo-end::

Bound Each Message Before It Reaches the Queue
==============================================

Queue capacity controls the collection of waiting entries.
:cpp:func:`LogManagerOptions::setMaximumMessageBytes <erbsland::log::LogManagerOptions::setMaximumMessageBytes>` controls
one message before its entry joins that collection.
The default is 256 KiB of sanitized, encoded message text.
This producer-side boundary prevents an unexpectedly large diagnostic value from claiming an excessive share of the
queue even when plenty of total capacity remains.

When a message exceeds the limit, the manager preserves a valid character boundary, appends an ellipsis when the budget
allows it, and marks the immutable :cpp:class:`LogEntry <erbsland::log::LogEntry>` as truncated.
The entry can inspect that fact with :cpp:func:`LogEntry::isTruncated <erbsland::log::LogEntry::isTruncated>`.
The setter requires a positive byte count; zero is invalid rather than meaning unlimited.

This option is deliberately independent of the code-point limits in :doc:`configuring_line_format`.
The producer limit bounds retained data for memory safety, while line formatting decides how much of that retained
message a writer presents to its reader.

.. erbsland-demo::
    :source: log/LoggingTopics/ManagerMessageSize.cpp
    :exec: log/logging_topics --demo ManagerMessageSize
    :source-sha256: 8972e0da2dd499d38524cc1a6277be62030c5fbb9774c7b3b4fbac6c9a271be2

.. code-block:: cpp

    /// Bound one producer message before its entry enters the queue.
    ///
    /// `setMaximumMessageBytes()` counts encoded bytes, preserves a valid character boundary, and adds an ellipsis when the
    /// message is shortened. The immutable entry records that producer-side truncation occurred.
    void managerMessageSize() {
        auto options = el::LogManagerOptions{};
        options.setMaximumMessageBytes(el::ByteLength{32U});
        const auto captured = std::make_shared<CapturingLogWriter>();
        auto configuration = el::LogConfiguration{};
        configuration.setManagerOptions(options).addWriter(captured);

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        manager->rootStream()->error("Aurora observation continues beyond midnight."_el);
        manager->shutdown();

        const auto entry = captured->lastEntry();
        el::io::printLine("Retained message: "_el, entry->message());
        el::io::printLine("Marked as truncated: "_el, entry->isTruncated() ? "yes"_el : "no"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Retained message: Aurora observation continues …
    Marked as truncated: yes

.. erbsland-demo-end::

Give the Worker Time to Finish During Shutdown
==============================================

Shutdown is the moment when bounded background work must become a bounded application delay.
:cpp:func:`LogManagerOptions::setShutdownTimeout <erbsland::log::LogManagerOptions::setShutdownTimeout>` sets the positive
time available for queued entries to drain; the default is two seconds.
Choose a duration that gives an ordinarily healthy destination time to finish without allowing a failed destination to
hold application cleanup indefinitely.

When :cpp:func:`LogManager::shutdown <erbsland::log::LogManager::shutdown>` begins, the manager accepts no new entries
and lets its worker process the queue.
If the deadline passes, entries still waiting in the queue are discarded and included in ``droppedEntries``.
The timeout cannot forcibly interrupt a writer method that is already executing.
Shutdown still waits for that active call to return so the worker and writer can be closed safely; the deadline
determines how long the remaining queue waits behind it.

:cpp:class:`Application <erbsland::core::Application>` calls shutdown automatically when the application exits.
Call it explicitly only when you own a standalone :cpp:class:`LogManager <erbsland::log::LogManager>` and need to choose
the exact end of its lifetime.
The demo uses such a standalone manager and a deliberately occupied writer to make the deadline behavior visible.

.. erbsland-demo::
    :source: log/LoggingTopics/ManagerShutdownTimeout.cpp
    :exec: log/logging_topics --demo ManagerShutdownTimeout
    :source-sha256: 3a330f0ab2a391d90070a8b7714c280388f001569634434f44f48d5c2e9f523b

.. code-block:: cpp

    /// Limit how long shutdown keeps entries waiting behind an active writer call.
    ///
    /// `setShutdownTimeout()` gives the queue 20 milliseconds to drain. The simulated destination remains occupied for
    /// longer, so the two entries still waiting at the deadline are dropped and recorded in the statistics.
    void managerShutdownTimeout() {
        auto options = el::LogManagerOptions{};
        options.setShutdownTimeout(el::Milliseconds{20});
        const auto writer = std::make_shared<BlockingLogWriter>();
        auto configuration = el::LogConfiguration{};
        configuration.setManagerOptions(options).addWriter(writer);

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->rootStream();
        log->info("The destination is writing this entry."_el);
        writer->waitUntilWriting();
        log->info("This entry is waiting in the queue."_el);
        log->info("This entry is also waiting in the queue."_el);

        auto releaseThread = std::thread{[writer]() -> void {
            std::this_thread::sleep_for(std::chrono::milliseconds{40});
            writer->release();
        }};
        manager->shutdown();
        releaseThread.join();

        const auto finished = manager->statistics();
        el::io::printLine("Written entries: "_el, finished.writtenEntries);
        el::io::printLine("Dropped entries: "_el, finished.droppedEntries);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Written entries: 1
    Dropped entries: 2

.. erbsland-demo-end::

Read Statistics as a Point-in-Time Snapshot
===========================================

Limits are useful only when you can see whether they fit the real workload.
:cpp:func:`LogManager::statistics <erbsland::log::LogManager::statistics>` returns a
:cpp:struct:`LogManagerStatistics <erbsland::log::LogManagerStatistics>` snapshot without pausing producers or resetting
any counters.
The cumulative counters describe everything observed since this manager was created, while the two queue values describe
the instant at which the snapshot was taken.

.. list-table:: Log manager statistics
    :header-rows: 1

    * - Field
      - Meaning
    * - ``acceptedEntries``
      - Entries admitted to the queue after producer and queue limits were applied.
    * - ``writtenEntries``
      - Accepted entries delivered successfully to at least one matching writer.
    * - ``droppedEntries``
      - Entries rejected by a capacity boundary or discarded from the queue during shutdown.
    * - ``writerFailures``
      - Writer operations whose exceptions were contained by the manager so other destinations could continue.
    * - ``queuedEntries``
      - Entries currently waiting for the worker.
    * - ``queuedBytes``
      - Storage currently retained by those waiting entries.

Do not expect ``acceptedEntries`` to always equal ``writtenEntries + droppedEntries``.
An accepted entry that matches no writer route is processed without being counted as written or dropped.
Use ``queuedEntries`` and ``queuedBytes`` to distinguish an active backlog from cumulative history, and investigate a
rising ``writerFailures`` count before compensating with larger queues.

The statistics demo changes no manager option.
It pauses the worker briefly to expose two queued entries, then resumes with one successful destination and one
deliberately failing destination.
Both entries are still written successfully, while the contained failure is visible as a separate operational signal.

.. erbsland-demo::
    :source: log/LoggingTopics/ManagerStatistics.cpp
    :exec: log/logging_topics --demo ManagerStatistics
    :source-sha256: 72e910789da8c6e3877e6ff0ac34286a0e548cdb6354eaabc70eda34986f2687

.. code-block:: cpp

    /// Read point-in-time counters without changing manager options.
    ///
    /// While paused, accepted entries remain visible as queued work. After resume and shutdown, the capturing writer has
    /// delivered both entries and the deliberately failing destination contributes one contained writer failure.
    void managerStatistics() {
        auto configuration = el::LogConfiguration{};
        configuration.addWriter(std::make_shared<FailingLogWriter>()).addWriter(std::make_shared<CapturingLogWriter>());
        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));

        manager->pause();
        manager->rootStream()->error("The northern gate could not be opened."_el);
        manager->rootStream()->error("The ridge team returned to the lodge."_el);
        const auto paused = manager->statistics();
        el::io::printLine("While paused:"_el);
        el::io::printLine("  Accepted entries: "_el, paused.acceptedEntries);
        el::io::printLine("  Written entries : "_el, paused.writtenEntries);
        el::io::printLine("  Queued entries  : "_el, paused.queuedEntries);
        el::io::printLine("  Queue has bytes : "_el, paused.queuedBytes.isZero() ? "no"_el : "yes"_el);

        manager->resume();
        manager->shutdown();
        const auto finished = manager->statistics();
        el::io::printLine("After shutdown:"_el);
        el::io::printLine("  Written entries : "_el, finished.writtenEntries);
        el::io::printLine("  Dropped entries : "_el, finished.droppedEntries);
        el::io::printLine("  Writer failures : "_el, finished.writerFailures);
        el::io::printLine("  Queued entries  : "_el, finished.queuedEntries);
    }

.. erbsland-ansi::
    :escape-char: ␛

    While paused:
      Accepted entries: 2
      Written entries : 0
      Queued entries  : 2
      Queue has bytes : yes
    After shutdown:
      Written entries : 2
      Dropped entries : 0
      Writer failures : 1
      Queued entries  : 0

.. erbsland-demo-end::
