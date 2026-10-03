*************************
Log Domain API Guidelines
*************************

Core Semantics
==============

.. code-block:: text

    entry = immutable log entry
    path root = complete slash-delimited path-segment prefix
    trace section = ASCII-case-insensitive identifier with equivalent hyphens and underscores to enable debug logs
    trace section name = 1-100 ASCII characters, first is a letter, remaining are letters/digits/hyphens/underscores
    route = writer destination with a level/path filter and an optional complete line-format override
    default line format = configuration format used by routes without an override

Primary Types
=============

.. code-block:: text

    LogManager // bounded background logging service
    LogStream // lightweight producer endpoint for one path and optional trace section
    LogEntry, LogEntryPtr, LogEntryConstPtr // producer-created entry and shared ownership aliases
    LogConfiguration // fluent user-facing writer, formatting, tracing, and limit settings
    LogWriter // custom output target interface

Secondary Types
===============

.. code-block:: text

    LogLevel, LogLevels // severity and severity set
    LogPath, LogTraceSection // validated routing identifiers
    LogWriterFilter // level and path-root route predicate
    LogLineFormat, LogLine, LogLinePtr, LogLineConstPtr // line settings, result, and shared ownership aliases
    LogConfigurationParser // compiled-rule ELCL branch parser
    LogManagerOptions, LogManagerStatistics // limits and counters

Manager Patterns
================

.. code-block:: text

    T::create([options]) -> LogManagerPtr // create standalone manager without writers
    o.rootStream()/createStream(path[, section]) -> LogStreamPtr // access producer endpoints
    o.setConfiguration(configuration) // synchronously replace the active snapshot
    o.pause()/resume()/shutdown() // control worker draining and lifecycle
    o.statistics() -> LogManagerStatistics // read accepted, written, dropped, and failure counters

Producer Patterns
=================

.. code-block:: text

    o.trace/info/warn/error(arguments) // format, sanitize, timestamp, and enqueue one entry
    o.traceEnabled() -> bool // unnamed or enabled section; independent of writer filters

Configuration Patterns
======================

.. code-block:: text

    o.setLineFormat(format) -> LogConfiguration& // default for routes without an override
    o.addWriter(writer[, filter[, lineFormat]]) -> LogConfiguration& // optional complete route format

Writer Filter Patterns
======================

.. code-block:: text

    o.acceptedLevels() -> LogLevels // read the accepted severity selection
    o.setAcceptedLevels(levels) -> LogWriterFilter& // keep exactly these levels
    o.setRejectedLevels(levels) -> LogWriterFilter& // keep all other levels; replaces the selection
    o.acceptedPaths() -> const vector<LogPath>& // read accepted roots; empty means all paths
    o.addAcceptedPath(path) -> LogWriterFilter& // add a root whose descendants are accepted

Writer Patterns
===============

.. code-block:: text

    T::createForConsole(terminal[, options]) -> LogWriterPtr // create the built-in console writer
    T::createForFile(options) -> LogWriterPtr // create the built-in file writer
    T::createForSyslog([options]) -> LogWriterPtr // create the built-in syslog writer
    o.write(entryPtr, linePtr) // accept one shared immutable entry and worker-formatted line
    o.writeBatch(batch) // accept shared immutable pointers in order; defaults to repeated write calls
    o.flush()/close() // finish writer output or release resources

Trace Section Validation Patterns
=================================

.. code-block:: text

    T::isValidName(name) -> bool // validate input before constructing a trace section
