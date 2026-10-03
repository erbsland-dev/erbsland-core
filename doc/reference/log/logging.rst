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
Each route filters complete path segments and levels and may carry a complete line-format override.
Pass the override as the third argument to ``LogConfiguration::addWriter()``; omitting it selects the configuration's
default format.
The manager retains the fluent configuration value for ``configuration()`` and separately creates internal runtime data
with active writer routes, limits, and trace policy.
Each active route points directly to its compiled formatter; the runtime data does not own the user-facing
configuration.
The manager prepares immutable formatters before activating this runtime data, parsing each pattern once.
Routes using the default share one formatter and reuse the same rendered line for each entry within a batch.
Replacing a configuration is a synchronous barrier; streams update their cached trace flag before the call returns.
An active batch finishes with its original formatters, while queued entries use the replacement configuration.
Preparation or writer-binding failure leaves the active configuration intact.
:cpp:class:`erbsland::log::LogConfigurationParser <erbsland::log::LogConfigurationParser>` validates any selected
ELCL document branch with one shared compiled ruleset and creates the corresponding built-in writers.

A default writer filter accepts all levels and paths.
``setAcceptedLevels()`` keeps exactly the listed levels; ``setRejectedLevels()`` keeps all supported levels except those
listed.
Each setter replaces the previous level selection.
``addAcceptedPath()`` limits acceptance to the given roots using complete path segments.

Trace Sections
==============

A ``LogTraceSection`` accepts an empty name or an ASCII identifier beginning with a letter.
Names contain at most 100 characters; subsequent characters may also include digits, underscores, and hyphens.
Comparison ignores ASCII case and treats hyphens and underscores as equivalent: ``Foo-Bar`` equals ``foo_bar``.
Application-created sections retain their original spelling, including when converted with ``toString()``.
Repeated programmatic enabling retains the first spelling.

Literal construction validates at compile time when used in a constant expression.
Invalid runtime construction raises ``ParameterFailure``.

.. code-block:: cpp

    class RoutePlanner {
        static constexpr auto cVerboseTraceSection = el::LogTraceSection{"verbose"_el};
    };

``LogConfigurationParser`` converts ELCL trace-section names to lowercase with underscores and rejects equivalent or
exact duplicates with a source-aware ``ConfError``.
Empty names remain ignored.
See :doc:`../../topics/log/trace_sections` for grouping and guard examples.

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

Writers report expected delivery and flush failures with concrete exceptions derived from
:cpp:class:`RuntimeError <erbsland::err::RuntimeError>`, such as
:cpp:class:`StreamError <erbsland::stream::StreamError>` for output-stream failures.
The manager contains and counts these failures so other writers can continue.
Other exception types escape this boundary and terminate the worker.
Programming errors, allocation failures, and thread-resource exhaustion are fatal; they are not discarded by the logging
worker or its submission paths.

Interface
=========

.. doxygenclass:: erbsland::log::ConsoleLogWriterOptions
    :members:
.. doxygenclass:: erbsland::log::FileLogWriterOptions
    :members:
.. doxygenclass:: erbsland::log::LogLine
    :members:

.. doxygentypedef:: erbsland::log::LogLinePtr

.. doxygentypedef:: erbsland::log::LogLineConstPtr
.. doxygenclass:: erbsland::log::LogLinePart
    :members:
.. doxygenstruct:: erbsland::log::LogLineSegment
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
.. doxygenclass:: erbsland::log::LogLineFormat
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
.. doxygenclass:: erbsland::log::SyslogLogWriterOptions
    :members:
.. doxygenclass:: erbsland::log::SyslogTransport
    :members:
