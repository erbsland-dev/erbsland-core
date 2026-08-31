.. index::
    single: Logging

*******
Logging
*******

The log domain provides bounded background logging through
:cpp:class:`erbsland::log::LogManager <erbsland::log::LogManager>` and lightweight, hierarchical
:cpp:class:`erbsland::log::LogStream <erbsland::log::LogStream>` producer endpoints.
Entries keep their UTC creation time and sanitized message, while routing, line formatting, and output happen on the
manager's private event loop.
Queued entries and formatted lines use shared immutable pointers.
Writer batches therefore move and copy only pointer values, and a custom writer can safely retain an item beyond the
batch call by copying its shared pointer.

Configuration and Routing
=========================

:cpp:class:`erbsland::log::LogConfiguration <erbsland::log::LogConfiguration>` combines line formatting, queue
limits, enabled trace sections, and writer routes.
Each route filters complete path segments and levels.
Replacing a configuration is a synchronous barrier; streams update their cached trace flag before the call returns.
:cpp:class:`erbsland::log::LogConfigurationParser <erbsland::log::LogConfigurationParser>` validates any selected
ELCL document branch with one shared compiled ruleset and creates the corresponding built-in writers.

Writers
=======

The console writer renders one styled paragraph directly through a terminal.
The manager delivers same-configuration entries to writers in bounded batches.
Custom writers can override batch delivery to amortize target checks while retaining the single-entry method as their
compatibility baseline.
The file writer uses ordinary path and stream APIs, checks file identity once per batch, detects external replacement,
and supports time- or size-based rotation.
The syslog writer emits RFC 5424 over UDP, RFC 6587 octet-counted TCP, or TLS using the ``log/syslog`` configuration
label.
The last-errors writer retains a bounded FIFO snapshot of error entries.

Interface
=========

.. doxygenclass:: erbsland::log::ConsoleLogWriter
    :members:
.. doxygenclass:: erbsland::log::ConsoleLogWriterOptions
    :members:
.. doxygenclass:: erbsland::log::FileLogWriter
    :members:
.. doxygenclass:: erbsland::log::FileLogWriterOptions
    :members:
.. doxygenclass:: erbsland::log::LastErrorsLogWriter
    :members:
.. doxygenclass:: erbsland::log::LogConfiguration
    :members:
.. doxygenclass:: erbsland::log::LogConfigurationParser
    :members:
.. doxygenclass:: erbsland::log::LogEntry
    :members:

.. doxygentypedef:: erbsland::log::LogEntryPtr

.. doxygentypedef:: erbsland::log::LogEntryConstPtr
.. doxygenclass:: erbsland::log::LogFileMode
    :members:
.. doxygenclass:: erbsland::log::LogFileRotation
    :members:
.. doxygenclass:: erbsland::log::LogLevel
    :members:

.. doxygentypedef:: erbsland::log::LogLevels
.. doxygenclass:: erbsland::log::LogLevelFormat
    :members:
.. doxygenclass:: erbsland::log::LogLine
    :members:

.. doxygentypedef:: erbsland::log::LogLinePtr

.. doxygentypedef:: erbsland::log::LogLineConstPtr
.. doxygenclass:: erbsland::log::LogLineFormat
    :members:
.. doxygenclass:: erbsland::log::LogLinePart
    :members:
.. doxygenstruct:: erbsland::log::LogLineSegment
    :members:
.. doxygenclass:: erbsland::log::LogManager
    :members:
.. doxygenclass:: erbsland::log::LogManagerOptions
    :members:
.. doxygenstruct:: erbsland::log::LogManagerStatistics
    :members:
.. doxygenclass:: erbsland::log::LogMessageTruncation
    :members:
.. doxygenclass:: erbsland::log::LogNameFormat
    :members:
.. doxygenclass:: erbsland::log::LogPath
    :members:
.. doxygenclass:: erbsland::log::LogStream
    :members:
.. doxygenclass:: erbsland::log::LogTimestampZone
    :members:
.. doxygenclass:: erbsland::log::LogTraceSection
    :members:
.. doxygenclass:: erbsland::log::LogWriter
    :members:
.. doxygenclass:: erbsland::log::LogWriterFilter
    :members:
.. doxygenclass:: erbsland::log::SyslogLogWriter
    :members:
.. doxygenclass:: erbsland::log::SyslogLogWriterOptions
    :members:
.. doxygenclass:: erbsland::log::SyslogTransport
    :members:
