.. index::
    single: Logging; Writing Messages
    single: Log Stream
    single: Trace Logging

********************
Writing Log Messages
********************

A useful log entry preserves a small piece of the application's story: what happened, where it happened, and how much
attention it deserves.
:cpp:class:`LogStream <erbsland::log::LogStream>` supplies the stable “where”; the method used to write the entry
supplies its level; and the values passed to that method form one coherent message.

Streams are lightweight, thread-safe producer handles.
They are meant to live with application components and to be shared with work performed on other threads.
The manager performs queueing, formatting, and writer delivery in the background, leaving producers with a small and
predictable interface.

Create a Stable Stream for a Component
======================================

Create a stream once when a component is constructed and retain its
:cpp:type:`LogStreamPtr <erbsland::log::LogStreamPtr>` for the component's lifetime. Recreating the same stream for each
message adds ceremony but creates no additional isolation.
Passing the manager into the constructor also gives the component exactly the logging capability it needs, without
coupling it to the global application object.

.. erbsland-demo::
    :source: log/LoggingTopics/StoredLogStream.cpp
    :exec: log/logging_topics --demo StoredLogStream
    :source-sha256: a9558cb67c277c8484c8a460d18d173d67972fbb2159ed65d8700a53c5adb16e

.. code-block:: cpp

    /// Store one log stream with the component that gives its path meaning.
    ///
    /// The component receives the shared manager rather than the whole application. It creates `_log` once, keeps the
    /// lightweight pointer, and can safely use that same stream from work performed on different threads.
    class ExpeditionJournal final {
    public:
        explicit ExpeditionJournal(el::LogManager &manager) : _log{manager.createStream("guild/journal"_el)} {}

        void open() { _log->info("Opened the expedition journal."_el); }
        void addWeatherWarning() { _log->warn("Fresh snow covered the eastern trail markers."_el); }

    private:
        el::LogStreamPtr _log;
    };

    void storedLogStream() {
        auto format = el::LogLineFormat{};
        format.setPattern("{level} [{name}] {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .addWriter(std::make_shared<el::ConsoleLogWriter>(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        auto journal = ExpeditionJournal{*manager};
        journal.open();
        journal.addWeatherWarning();
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97mINF [␛[95mguild/journal␛[97m] Opened the expedition journal.
    ␛[93mWRN [␛[95mguild/journal␛[93m] Fresh snow covered the eastern trail markers.␛[0m

.. erbsland-demo-end::

The stored pointer may be copied and shared.
Calls from several threads are safe, and each call becomes one entry in the manager queue.
Ordering between concurrent producers naturally follows the order in which the manager accepts their entries, so a
message should contain enough context to stand on its own rather than depending on the previous line.

Give the stream a path that describes a durable responsibility.
Paths use lowercase ASCII letters, digits, underscores, hyphens, and slash separators; they contain at most 16 nonempty
segments and 255 bytes.
Names such as ``network/http`` and ``network/dns`` form a useful hierarchy.
Writer path filters compare complete segments, so a route for ``network`` accepts both names, while a route for ``net``
accepts neither.

Choose a Level for the Person Reading It
========================================

Levels are not a measure of how interesting the code was to write.
They describe the consequence of the event for the reader.
The four calls below show the complete vocabulary and how the default short level names are rendered by a console
writer.

.. erbsland-demo::
    :source: log/LoggingTopics/LogLevels.cpp
    :exec: log/logging_topics --demo LogLevels
    :source-sha256: e1f8563a5609a3d29ec33bc735b74bd85f30e9a73b5875882e6d99777fe036af

.. code-block:: cpp

    /// Choose a level by the consequence an entry has for its reader.
    ///
    /// Trace explains an enabled diagnostic path, information records ordinary progress, a warning describes work that
    /// can continue in a degraded state, and an error marks an operation that failed.
    void logLevels() {
        auto format = el::LogLineFormat{};
        format.setPattern("{level}: {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .enableTraceSection(el::LogTraceSection{"route-search"_el})
            .addWriter(std::make_shared<el::ConsoleLogWriter>(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->createStream("guild/routes"_el, el::LogTraceSection{"route-search"_el});
        log->trace("Compared the lake and ridge routes."_el);
        log->info("Selected the ridge route."_el);
        log->warn("The last marker is weathered, but still readable."_el);
        log->error("The northern checkpoint did not answer."_el);
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90mTRC: Compared the lake and ridge routes.
    ␛[97mINF: Selected the ridge route.
    ␛[93mWRN: The last marker is weathered, but still readable.
    ␛[91mERR: The northern checkpoint did not answer.␛[0m

.. erbsland-demo-end::

Use :cpp:func:`trace() <erbsland::log::LogStream::trace>` for detailed diagnostic steps that are useful only while a
specific investigation is enabled.
Use :cpp:func:`info() <erbsland::log::LogStream::info>` for normal milestones and meaningful state changes.
A :cpp:func:`warn() <erbsland::log::LogStream::warn>` entry says that something unexpected happened but the current
operation can continue, perhaps with reduced quality.
An
:cpp:func:`error() <erbsland::log::LogStream::error>` entry says that an operation failed and deserves attention.

The same event should normally keep the same level across installations.
Routes decide where a level is delivered; changing an error into information merely to make one console quieter hides
its meaning from every other writer as well.

Build One Complete Message from Printable Values
================================================

Each log method accepts the same printable-value model as Erbsland Core text output.
Strings, numbers, paths, byte blocks, and other printable values can remain in their natural types until the stream
prepares the message.
Formatting controls such as :cpp:class:`IntegerFormat <erbsland::text::IntegerFormat>`,
:cpp:class:`BooleanFormat <erbsland::text::BooleanFormat>`, and
:cpp:class:`ByteFormat <erbsland::text::ByteFormat>` affect the values that follow them within that call.

.. erbsland-demo::
    :source: log/LoggingTopics/LogStreams.cpp
    :exec: log/logging_topics --demo LogStreams
    :source-sha256: 15ea28250248b3bd012e2d69466fd159b28342f2c9ac0c3348ead810ba2fd90a

.. code-block:: cpp

    /// Build one coherent entry from printable values and formatting controls.
    ///
    /// Log methods accept the same printable values as Erbsland Core text streams. Inline controls affect the values that
    /// follow them, so structured values can stay typed until the stream prepares one complete, safely escaped message.
    void logStreams() {
        auto format = el::LogLineFormat{};
        format.setPattern("{level} [{name}] {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .addWriter(std::make_shared<el::ConsoleLogWriter>(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->createStream("guild/equipment"_el);

        const auto seal = el::ByteBlock::fromVector(std::vector<std::uint8_t>{0x2aU, 0x7cU, 0x91U, 0xe0U});
        log->info(
            "Crates="_el,
            el::IntegerFormat::hexadecimal(),
            42,
            ", checked="_el,
            el::BooleanFormat::yesNo(),
            true,
            ", seal="_el,
            el::ByteFormat::separated(),
            seal);
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97mINF [␛[95mguild/equipment␛[97m] Crates=2a, checked=yes, seal=2a 7c 91 e0␛[0m

.. erbsland-demo-end::

Prefer one call for one event.
Splitting a sentence over several entries allows unrelated threads to appear between the fragments and asks every writer
to make several routing decisions for what was conceptually one fact.
Newlines inside a message are retained when a genuinely multiline value is useful; other control and format characters
are escaped so untrusted data cannot quietly manipulate the rendered log.

Use Multiple Streams When the Names Add Meaning
===============================================

Most components need only one ``_log``.
A component with two genuinely distinct responsibilities can own more than one, especially when operators may route
those responsibilities differently.
The example service separates route selection from supply accounting and lets the line format show the result.

.. erbsland-demo::
    :source: log/LoggingTopics/MultipleLogStreams.cpp
    :exec: log/logging_topics --demo MultipleLogStreams
    :source-sha256: a069ceba66751b9cd174322fd51519f8b191901ba0aa937a01ffe8612a895308

.. code-block:: cpp

    /// Use several streams when one component carries several distinct responsibilities.
    ///
    /// The expedition service separates route decisions from supply accounting. Names in the rendered lines tell the
    /// reader where an event originated, and the same paths can later become precise writer routes.
    class ExpeditionService final {
    public:
        explicit ExpeditionService(el::LogManager &manager) :
            _routeLog{manager.createStream("guild/expedition/routes"_el)},
            _supplyLog{manager.createStream("guild/expedition/supplies"_el)} {}

        void prepare() {
            _routeLog->info("The ridge route has three checkpoints."_el);
            _supplyLog->warn("Two lanterns still need fresh oil."_el);
        }

    private:
        el::LogStreamPtr _routeLog;
        el::LogStreamPtr _supplyLog;
    };

    void multipleLogStreams() {
        auto format = el::LogLineFormat{};
        format.setPattern("{level} [{name}] {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .addWriter(std::make_shared<el::ConsoleLogWriter>(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        auto expedition = ExpeditionService{*manager};
        expedition.prepare();
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97mINF [␛[95mguild/expedition/routes␛[97m] The ridge route has three checkpoints.
    ␛[93mWRN [␛[95mguild/expedition/supplies␛[93m] Two lanterns still need fresh oil.␛[0m

.. erbsland-demo-end::

This is more useful than prefixing message text with ``Routes:`` or ``Supplies:``.
The name is a structured field, so a file writer can later receive the whole ``guild/expedition`` hierarchy while a
focused console route selects only ``guild/expedition/supplies``.
Do not create a new stream for every object instance or request identifier; put changing identifiers in the message and
reserve stream paths for stable routing boundaries.

Guard Expensive Trace Preparation
=================================

A trace call that receives a few already available values needs no special handling: when tracing is disabled, the
stream discards it cheaply.
Preparing a route graph, serializing a large object, or collecting a diagnostic snapshot can be far more expensive than
the call itself.
Associate that stream with a
:cpp:class:`LogTraceSection <erbsland::log::LogTraceSection>` and ask
:cpp:func:`traceEnabled() <erbsland::log::LogStream::traceEnabled>` before doing the preparation.

.. erbsland-demo::
    :source: log/LoggingTopics/TraceSections.cpp
    :exec: log/logging_topics --demo TraceSections
    :source-sha256: a235b23519c037d4e99b40c445dd9996968eaa0e25200c7ffd776d1201f4e076

.. code-block:: cpp

    /// Use one trace section as a stable startup choice for related diagnostics.
    ///
    /// Create the section once, enable it in the complete configuration, and attach it when creating the stream. The same
    /// stream continues to carry information messages whether or not its trace section is enabled.
    void traceSections() {
        const auto routeSearchTrace = el::LogTraceSection{"route-search"_el};
        auto format = el::LogLineFormat{};
        format.setPattern("{level} [{name}] {message}"_el);

        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .enableTraceSection(routeSearchTrace)
            .addWriter(
                std::make_shared<el::ConsoleLogWriter>(el::application().terminal()),
                el::LogWriterFilter{el::LogLevels{el::LogLevel::Trace, el::LogLevel::Information}});

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto routeLog = manager->createStream("guild/route-search"_el, routeSearchTrace);

        // Only build a detailed route description when tracing is active.
        if (routeLog->traceEnabled()) {
            const auto route = el::String::fromJoined({"Kallio"_el, " → "_el, "Jääjärvi"_el, " → "_el, "Majakka"_el});
            routeLog->trace("Candidate route: "_el, route);
        }
        routeLog->info("The route search selected three waypoints."_el);
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90mTRC [␛[95mguild/route-search␛[90m] Candidate route: Kallio → Jääjärvi → Majakka
    ␛[97mINF [␛[95mguild/route-search␛[97m] The route search selected three waypoints.␛[0m

.. erbsland-demo-end::

The check becomes true only when the stream's section is enabled and at least one trace route accepts the stream path.
Configuration replacement refreshes that cached flag synchronously, so producers immediately observe the new startup
policy.
Keeping configuration stable after startup makes this behavior easy to reason about and avoids turning normal logging
into a runtime control protocol.

See :doc:`trace_sections` for choosing section boundaries and configuring them.
For the routing rules that connect levels and paths to writers, continue with :doc:`using_writers`.
