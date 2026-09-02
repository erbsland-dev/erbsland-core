*************************
Log Domain API Guidelines
*************************

Core Semantics
==============

.. code-block:: text

    entry = immutable log entry
    path root = complete slash-delimited path-segment prefix
    trace section = case-sensitive configuration identifier to enable additional debug logs
    route = log destination, usually a writer

Primary Types
=============

.. code-block:: text

    LogManager // bounded background logging service
    LogStream // lightweight producer endpoint for one path and optional trace section
    LogEntry, LogEntryPtr, LogEntryConstPtr // producer-created entry and shared ownership aliases
    LogConfiguration // complete writer, formatting, tracing, and limit snapshot
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
    o.traceEnabled() -> bool // read the cached trace decision

Writer Patterns
===============

.. code-block:: text

    T::createForConsole(terminal[, options]) -> LogWriterPtr // create the built-in console writer
    T::createForFile(options) -> LogWriterPtr // create the built-in file writer
    T::createForSyslog([options]) -> LogWriterPtr // create the built-in syslog writer
    o.write(entryPtr, linePtr) // accept one shared immutable entry and worker-formatted line
    o.writeBatch(batch) // accept shared immutable pointers in order; defaults to repeated write calls
    o.flush()/close() // finish writer output or release resources
