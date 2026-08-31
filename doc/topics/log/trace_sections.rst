.. index::
    single: Logging; Trace Sections
    single: Trace Sections
    single: Trace Logging; Sections

********************
Using Trace Sections
********************

Trace messages are most useful when they answer a particular debugging question.
A route planner may need to reveal every candidate it considered, while a network investigation may need raw protocol
decisions; enabling both families at once creates noise, consumes queue capacity, and makes the useful sequence harder
to follow.
Trace sections let you name these diagnostic families and select them as part of the application's startup
configuration.
This page explains what a section controls, how to organize streams around investigations, how to avoid work while a
section is inactive, and why the resulting policy should remain stable for the lifetime of the application.

Understand What a Trace Section Controls
========================================

A :cpp:class:`LogTraceSection <erbsland::log::LogTraceSection>` is a case-sensitive identifier attached to a
:cpp:class:`LogStream <erbsland::log::LogStream>` when that stream is created.
It controls only calls to :cpp:func:`LogStream::trace <erbsland::log::LogStream::trace>`.
Information, warning, and error messages on the same stream continue to be produced normally, whether the trace section
is enabled or not.

Sections complement rather than replace levels, paths, and writer filters.
The section answers *which family of detailed producer diagnostics is active*.
The stream path still answers *which component produced the entry*, and each writer's level-and-path filter still
decides *which destination receives it*.
Keeping these responsibilities separate lets one investigation span several components without giving up ordinary
routing.

A named section is inactive by default.
A stream with a named section becomes trace-enabled only when both of these conditions are true:

* its exact, case-sensitive section is enabled in the configuration, and
* at least one configured writer accepts trace entries from the stream's path.

An unnamed stream, created with the default empty section, has no named section gate.
Its trace state depends only on whether a writer route accepts trace for its path.
Attach an explicit section to every stream whose detailed output should remain selectable.

Design Sections Around Investigations
=====================================

Choose section boundaries from the reader's question rather than from the source-tree layout.
``route-search`` can include map lookup, weather constraints, and transport scheduling because those components all help
explain why a route was selected.
``radio-frames`` belongs in a separate section because it serves a different investigation, even if some of its code
lives in the same module.

This approach has two practical benefits.
An application can enable enough context to follow one story without opening every trace source, and code in several
modules can contribute to that story under one stable name.
A stream carries one section for its lifetime, so create and store the stream that represents that diagnostic role
instead of manufacturing temporary section names at each call site.

Section identifiers follow configuration-identifier syntax: the first character is an ASCII letter or underscore, and
the remaining characters may also contain digits and hyphens.
Names are case-sensitive, so ``route-search`` and ``Route-Search`` are different sections.
A consistent lower-case, hyphen-separated convention makes configuration files easier to read and avoids accidental
duplicates.
Invalid identifiers raise :cpp:class:`ParameterError <erbsland::err::ParameterError>` when
:cpp:class:`LogTraceSection <erbsland::log::LogTraceSection>` is constructed.

The demo enables one investigation across two unrelated stream paths.
The radio stream has a different, inactive section, so its trace call remains silent even though the writer accepts
trace from every path.

.. erbsland-demo::
    :source: log/LoggingTopics/TraceSectionGrouping.cpp
    :exec: log/logging_topics --demo TraceSectionGrouping
    :source-sha256: cc3976567e078bd543c112b600362edaca6e42c71d904bfe776e9d3b59c726ae

.. code-block:: cpp

    /// Group diagnostics by investigation rather than by stream path.
    ///
    /// The route-search section spans map and weather modules because both help explain route selection. Radio frames
    /// answer a different question and remain silent even though the writer route accepts trace from every path.
    void traceSectionGrouping() {
        const auto routeSearch = el::LogTraceSection{"route-search"_el};
        const auto radioFrames = el::LogTraceSection{"radio-frames"_el};
        auto format = el::LogLineFormat{};
        format.setPattern("{level} [{name}] {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .enableTraceSection(routeSearch)
            .addWriter(
                std::make_shared<el::ConsoleLogWriter>(el::application().terminal()),
                el::LogWriterFilter{el::LogLevels{el::LogLevel::Trace}});

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto mapLog = manager->createStream("guild/map"_el, routeSearch);
        const auto weatherLog = manager->createStream("guild/weather"_el, routeSearch);
        const auto radioLog = manager->createStream("guild/radio"_el, radioFrames);
        mapLog->trace("Candidate path crosses the eastern ridge."_el);
        weatherLog->trace("The eastern ridge remains below the cloud line."_el);
        radioLog->trace("Raw frame bytes are available."_el);
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90mTRC [␛[95mguild/map␛[90m] Candidate path crosses the eastern ridge.
    TRC [␛[95mguild/weather␛[90m] The eastern ridge remains below the cloud line.␛[0m

.. erbsland-demo-end::

Attach a Stable Section When Creating the Stream
================================================

The recommended pattern is to create one stable section value, use the same value while assembling the configuration,
and attach it when the owning module creates its long-lived log stream.
In a larger application the module stores that stream alongside its other dependencies, just as it stores an ordinary
named stream.
Every trace call on the stream then belongs to the same documented diagnostic family without repeating a string
identifier.

The complete example below enables ``route-search`` during startup, adds a writer route that accepts trace and
information, and creates the route-search stream with that section.
Its detailed candidate route is conditional, while the final information message is always produced.

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

Enable the Section and a Matching Destination
=============================================

Call :cpp:func:`LogConfiguration::enableTraceSection <erbsland::log::LogConfiguration::enableTraceSection>` while
building the complete startup configuration.
Enabling a section says that its producers may create trace entries; it does not by itself give those entries a
destination.
At least one writer must also have a :cpp:class:`LogWriterFilter <erbsland::log::LogWriterFilter>` that accepts the
trace level and contains the stream path.

The reverse is equally important.
A writer route that accepts trace does not activate a stream carrying an inactive named section.
This allows the same broad file route to be ready for several investigations while the enabled-section list selects
which producers actually do detailed work.
The following demo uses separate managers to isolate the three possible configurations and show the two-part decision.

.. erbsland-demo::
    :source: log/LoggingTopics/TraceSectionActivation.cpp
    :exec: log/logging_topics --demo TraceSectionActivation
    :source-sha256: 7932fd717ec1242efdf73fd7ed7c38fc064f958919db5335f00a422fa05b7798

.. code-block:: cpp

    /// Require an enabled section and a matching trace route.
    ///
    /// These three independent managers show the two-part rule without changing configuration at runtime. A stream becomes
    /// trace-enabled only when the section is listed and a writer accepts trace entries from its path.
    void traceSectionActivation() {
        const auto showState = [](const el::String &label, const bool enableSection, const bool acceptTrace) -> void {
            const auto section = el::LogTraceSection{"route-search"_el};
            auto configuration = el::LogConfiguration{};
            if (enableSection) {
                configuration.enableTraceSection(section);
            }
            const auto levels = acceptTrace ? el::LogLevels{el::LogLevel::Trace} : el::LogLevels{el::LogLevel::Information};
            configuration.addWriter(std::make_shared<el::LastErrorsLogWriter>(), el::LogWriterFilter{levels});

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            const auto log = manager->createStream("guild/route-search"_el, section);
            el::io::printLine(label, log->traceEnabled() ? "enabled"_el : "disabled"_el);
            manager->shutdown();
        };

        showState("Section only : "_el, true, false);
        showState("Route only   : "_el, false, true);
        showState("Both together: "_el, true, true);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Section only : disabled
    Route only   : disabled
    Both together: enabled

.. erbsland-demo-end::

Calling ``enableTraceSection()`` more than once with the same value has no additional effect, and an empty section is
not added to the named list.
When logging configuration comes from ELCL, the ``trace_sections`` field supplies the same enabled list; see
:doc:`elcl_configuration` for its syntax and :doc:`reading_configuration` for the startup sequence.

Guard Preparation That Exists Only for Tracing
==============================================

:cpp:func:`LogStream::trace <erbsland::log::LogStream::trace>` checks the stream's trace state before it formats its own
arguments.
For a direct call with values the application already has, that built-in check is enough: a disabled call does not build
a message or enqueue an entry.

Some diagnostics require work before the trace call can be made.
They may collect a route, serialize a protocol structure, walk a cache, or render a detailed state report.
Wrap that preparation and the resulting call in
:cpp:func:`LogStream::traceEnabled <erbsland::log::LogStream::traceEnabled>` so none of the diagnostic-only work happens
while the section is inactive.
The check reads the stream's cached flag with one atomic operation and is suitable for frequently executed paths.

The demo runs the same guarded search under two independent startup configurations.
With the section disabled the preparation count stays at zero; with it enabled the route is prepared once and its trace
entry is written.

.. erbsland-demo::
    :source: log/LoggingTopics/TraceSectionGuard.cpp
    :exec: log/logging_topics --demo TraceSectionGuard
    :source-sha256: 171983db1ac2a3db4b118bd77676e5593cc150e3ef18b4a2f5647284e6389160

.. code-block:: cpp

    /// Guard preparation that exists only to produce a trace message.
    ///
    /// `trace()` already suppresses its own formatting while disabled. An explicit `traceEnabled()` guard also avoids the
    /// route search performed before the call, so the disabled manager performs no diagnostic preparation at all.
    void traceSectionGuard() {
        const auto runSearch = [](const bool enableSection) -> std::size_t {
            const auto section = el::LogTraceSection{"route-search"_el};
            auto format = el::LogLineFormat{};
            format.setPattern("{level}: {message}"_el);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(format))
                .addWriter(
                    std::make_shared<el::ConsoleLogWriter>(el::application().terminal()),
                    el::LogWriterFilter{el::LogLevels{el::LogLevel::Trace}});
            if (enableSection) {
                configuration.enableTraceSection(section);
            }

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            const auto log = manager->createStream("guild/route-search"_el, section);
            auto preparationCount = std::size_t{};
            if (log->traceEnabled()) {
                ++preparationCount;
                const auto route = el::String::fromJoined({"Kallio"_el, " → "_el, "Jääjärvi"_el, " → "_el, "Majakka"_el});
                log->trace("Candidate route: "_el, route);
            }
            manager->shutdown();
            return preparationCount;
        };

        el::io::printLine("Preparations while disabled: "_el, runSearch(false));
        const auto enabledCount = runSearch(true);
        el::io::printLine("Preparations while enabled : "_el, enabledCount);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Preparations while disabled: 0
    ␛[90mTRC: Candidate route: Kallio → Jääjärvi → Majakka
    ␛[39mPreparations while enabled : 1

.. erbsland-demo-end::

Keep the Trace Policy Stable After Startup
==========================================

Treat trace sections as startup policy rather than as live switches.
Read command-line and application configuration, decide which investigations are active, assemble the writer routes and
enabled sections in one :cpp:class:`LogConfiguration <erbsland::log::LogConfiguration>`, and install that snapshot
before ordinary application work begins.
If logging was paused during early initialization, resume it after this complete policy is ready.

A stable policy makes a diagnostic run understandable.
Every producer observes the same intended investigation, related entries are not split across several policy
generations, and a configuration change cannot accidentally replace unrelated routes, queue limits, or formatting
settings.
When another set of traces is needed, change the startup configuration and begin a new run with a clear diagnostic
scope.

Understand What Runtime Replacement Would Do
============================================

The API does permit runtime replacement because
:cpp:func:`LogManager::setConfiguration <erbsland::log::LogManager::setConfiguration>` accepts a new complete snapshot.
Installing one refreshes the cached trace flag of every existing stream synchronously.
When the call returns, :cpp:func:`LogStream::traceEnabled <erbsland::log::LogStream::traceEnabled>` on those streams
reflects the new enabled-section list and writer routes; streams created later start with the same active decision.
There is no separate disable method—omitting a section from the replacement snapshot disables its named gate.

That mechanical possibility does not make runtime switching a recommended operating model.
The replacement changes the complete logging policy, not just one section, and producer threads can be on different
sides of the transition while the call is in progress.
A thread that passed an earlier guard may finish expensive preparation after the policy changes, while one that observed
the previous disabled state skips an event that occurs near the boundary.
Entries already accepted by the manager remain queued entries and follow the normal configuration-replacement behavior.
The result is not an application-wide, atomic “trace session,” which makes the captured diagnostic sequence harder to
interpret.

The final demo demonstrates the synchronous flag refresh so the behavior is explicit.
Use it to understand configuration replacement, not as a pattern for repeatedly toggling sections in a running service.

.. erbsland-demo::
    :source: log/LoggingTopics/TraceSectionReplacement.cpp
    :exec: log/logging_topics --demo TraceSectionReplacement
    :source-sha256: 5519f8328f1a68ec92a31bb0aaee2d1aeb89479a5ff332a9b7944184543bfad6

.. code-block:: cpp

    /// Show the synchronous flag refresh caused by configuration replacement.
    ///
    /// Runtime replacement is possible, but each call installs a complete manager policy. Immediately after
    /// `setConfiguration()` returns, existing streams expose the new section-and-route decision through `traceEnabled()`.
    void traceSectionReplacement() {
        const auto section = el::LogTraceSection{"route-search"_el};
        const auto writer = std::make_shared<el::LastErrorsLogWriter>();
        const auto traceFilter = el::LogWriterFilter{el::LogLevels{el::LogLevel::Trace}};
        const auto manager = el::LogManager::create();

        auto disabled = el::LogConfiguration{};
        disabled.addWriter(writer, traceFilter);
        manager->setConfiguration(std::move(disabled));
        const auto log = manager->createStream("guild/route-search"_el, section);
        el::io::printLine("Initial state          : "_el, log->traceEnabled() ? "enabled"_el : "disabled"_el);

        auto enabled = el::LogConfiguration{};
        enabled.enableTraceSection(section).addWriter(writer, traceFilter);
        manager->setConfiguration(std::move(enabled));
        el::io::printLine("After enabling section : "_el, log->traceEnabled() ? "enabled"_el : "disabled"_el);

        disabled = el::LogConfiguration{};
        disabled.addWriter(writer, traceFilter);
        manager->setConfiguration(std::move(disabled));
        el::io::printLine("After replacing policy : "_el, log->traceEnabled() ? "enabled"_el : "disabled"_el);
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Initial state          : disabled
    After enabling section : enabled
    After replacing policy : disabled

.. erbsland-demo-end::
