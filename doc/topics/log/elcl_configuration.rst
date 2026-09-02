.. index::
    single: Logging; ELCL Schema
    single: ELCL; Logging Schema
    single: Log Configuration; Fields

*************************************
Writing an ELCL Logging Configuration
*************************************

Moving logging choices into ELCL gives the people who run an application control over its output without turning those
choices into command-line switches or requiring a new build.
The application still receives the same complete configuration snapshot it would create in C++: the file simply provides
a readable, reviewable description of that snapshot.

A logging branch has four layers.
``Format`` shapes the rendered lines, ``Queue`` bounds the manager's resources, ``Trace Sections`` selects the
diagnostic groups that may produce trace entries, and the ordered ``Writers`` section list describes the destinations
and their routes.
Every layer is optional, but a useful configuration normally contains at least one writer; otherwise the manager has
nowhere to deliver accepted entries.

ELCL names are case-insensitive, and spaces and underscores are equivalent.
``Maximum Entries`` therefore names the same field as ``maximum_entries``.
The examples use spacious, human-readable names, while enumerated text values such as ``short_upper`` and ``daily`` use
the canonical lowercase spellings listed on this page.
:doc:`reading_configuration` shows how an application selects this branch and turns it into a
:cpp:class:`LogConfiguration <erbsland::log::LogConfiguration>` during startup.

Start With One Useful Destination
=================================

A small command-line tool often needs only one decision: send its log entries to the terminal.
The following complete configuration does exactly that.
Because it omits ``Levels`` and ``Paths``, the writer accepts every level and stream path.
Because it omits ``Format`` and ``Queue``, the well-defined library defaults supply the line pattern and manager limits.

The console writer is the only built-in writer for which the parser itself needs an application resource.
Construct
:cpp:class:`LogConfigurationParser <erbsland::log::LogConfigurationParser>` with a terminal before loading this file;
without one, the parser cannot create the requested writer and reports an error.

.. literalinclude:: ../../../demos/log/ConfiguredLogging/data/minimal.elcl
    :language: erbsland-conf

Make Operational Choices Explicit
=================================

Defaults are convenient while an application is young.
In a deployed service or a tool shared across a team, explicit choices are often easier to review: operators can see the
queue budget, recognize the line format, and understand which streams reach each destination without consulting the C++
defaults.

The following configuration makes those decisions visible.
It selects a format that includes the full stream name, bounds the queue, routes the ``guild`` hierarchy to the console,
and limits the amount of terminal wrapping.
These numbers are an example rather than a universal production recipe; choose capacities that match the application's
burst rate, message sizes, and destination speed.
The configured-logging demo parses and executes this exact file, so the example remains synchronized with the
implementation.

.. literalinclude:: ../../../demos/log/ConfiguredLogging/data/root.elcl
    :language: erbsland-conf

Logging does not have to occupy the document root.
The nested form below uses the same schema beneath ``Application.Logging`` and keeps the application's other settings
beside it.
It also enables one trace section, showing that a nested branch loses none of the available logging controls:

.. literalinclude:: ../../../demos/log/ConfiguredLogging/data/nested.elcl
    :language: erbsland-conf

Shape Every Log Line
====================

Each log entry carries a timestamp, level, stream path, and message.
The ``Format`` section decides how writers combine those values into a line; it does not change the entry itself.
That distinction allows the manager to retain timestamps in UTC and apply a local-time choice only when the line is
rendered.

Omitting the section uses :cpp:class:`LogLineFormat <erbsland::log::LogLineFormat>` defaults.
When you add it, begin with ``Pattern``: its placeholders decide which entry values readers can see.
The remaining fields refine how those placeholders appear and how exceptionally long names or messages are shortened.

.. list-table::
    :header-rows: 1
    :widths: 24 22 54

    * - Field
      - Default
      - Values and purpose
    * - ``Pattern``
      - ``{time} {level} - {message}``
      - Nonempty text with ``{time}``, ``{level}``, ``{name}``, and ``{message}``; doubled braces are literals.
    * - ``Timestamp Zone``
      - ``utc``
      - ``utc`` or ``local`` for rendering retained UTC timestamps.
    * - ``Level``
      - ``short_upper``
      - ``short_upper``, ``short_lower``, ``full_lower``, or ``full_upper``.
    * - ``Name``
      - ``full``
      - ``full``, ``leaf``, ``head_and_leaf``, or ``left_truncated``.
    * - ``Name Limit``
      - ``40``
      - A nonnegative code-point limit used by ``left_truncated``.
    * - ``Message Truncation``
      - ``none``
      - ``none``, ``first_line``, ``characters``, or ``total_line``.
    * - ``Message Limit``
      - ``0``
      - A nonnegative code-point limit used by the selected truncation mode.
    * - ``Truncation Mark``
      - ``…``
      - Text appended after a rendering-time truncation.

The formatting limits count Unicode code points and act while a writer renders a line.
They are intentionally separate from ``Maximum Message Bytes`` in the queue section, which protects producers and counts
encoded bytes.
For example, a service can accept a detailed message into the manager, keep it intact for a file writer, and use a
bounded console layout where space is scarce.
:doc:`configuring_line_format` demonstrates every representation and truncation mode in isolation.

Give the Manager a Clear Resource Budget
========================================

Logging sits between producer threads and destinations that may briefly be slower.
The ``Queue`` section defines how much memory and how many entries the manager may retain during that gap.
These are safeguards, not throughput targets: normal operation should leave enough headroom that short bursts do not
immediately cause drops.

Omitting ``Queue`` uses :cpp:class:`LogManagerOptions <erbsland::log::LogManagerOptions>` defaults.
Entry and byte reservations are portions of the corresponding total capacity, not additional capacity.
They preserve room for warning and error entries after ordinary trace and information traffic has filled the unreserved
part of the queue.
Byte fields are plain integer byte counts; ELCL digit separators make larger values much easier to review.

.. list-table::
    :header-rows: 1
    :widths: 30 18 52

    * - Field
      - Default
      - Values and purpose
    * - ``Maximum Entries``
      - ``4'096``
      - A positive total number of queued entries.
    * - ``Maximum Bytes``
      - ``8'388'608``
      - A positive total number of sanitized message bytes in the queue.
    * - ``Reserved Error Entries``
      - ``256``
      - A nonnegative part of the total entry capacity reserved for warnings and errors.
    * - ``Reserved Error Bytes``
      - ``1'048'576``
      - A nonnegative part of the total byte capacity reserved for warnings and errors.
    * - ``Maximum Message Bytes``
      - ``262'144``
      - A positive producer-side byte limit for one sanitized message.
    * - ``Shutdown Timeout Ms``
      - ``2'000``
      - A nonnegative graceful shutdown deadline in milliseconds.

When you reduce a maximum and omit its reservation, the parser clamps the default reservation to the new maximum.
This makes compact configurations safe.
An explicitly configured reservation expresses a deliberate relationship and must not exceed its corresponding total
when the configuration is installed.

``Maximum Message Bytes`` is enforced before a message joins the queue, whereas ``Maximum Bytes`` accounts for all
sanitized messages currently retained.
``Shutdown Timeout Ms`` sets how long shutdown waits for writers to drain pending work; zero is valid when the
application deliberately requires an immediate deadline.
The consequences of each limit, including drop statistics, are developed in :doc:`configuring_manager_options`.

Enable the Trace Detail You Need
================================

Trace sections let an application compile detailed diagnostic logging into normal code while keeping individual groups
quiet until they are needed.
The ``Trace Sections`` value list names the groups enabled by this startup configuration.
Omit it to disable every group, which is the usual steady-state choice for a production service.

Unlike ELCL field names, trace-section identifiers are case-sensitive application identifiers.
Each may contain ASCII letters, digits, underscores, and hyphens, and must begin with a letter or underscore.
Enabling a section permits its guarded code to produce trace entries; those entries become visible only when at least
one writer route also accepts the trace level and the stream path.
Keeping this configuration stable after startup avoids changing the meaning of cached trace guards while work is in
flight.
:doc:`trace_sections` explains the recommended grouping and guard pattern.

Connect Entries to Their Destinations
=====================================

The ``Writers`` section is a section list because an application commonly needs more than one destination.
Each entry creates one built-in writer together with a filter.
``Levels`` answers which severities the destination should receive; ``Paths`` answers which parts of the stream
hierarchy belong there.
Omitting either filter dimension accepts all values for that dimension.

Routes may overlap.
A warning from ``guild/routes`` can go to the console, a rotating file, and a last-error buffer at the same time when it
matches all three definitions.
Writer entries retain their document order, but routing is not a first-match decision: every matching writer receives
the entry.
Paths are lowercase slash-delimited roots matched on complete segments, so ``guild`` accepts ``guild/routes`` but does
not accidentally accept ``guildhall``.

.. list-table::
    :header-rows: 1
    :widths: 20 20 60

    * - Field
      - Default
      - Values and purpose
    * - ``Type``
      - Required
      - ``console``, ``file``, or ``syslog``.
    * - ``Levels``
      - All levels
      - A list containing ``trace``, ``information`` or ``info``, ``warning`` or ``warn``, and ``error``.
    * - ``Paths``
      - All paths
      - A list of lowercase log path roots matched on complete segments.

Keep Terminal Output Comfortable to Read
========================================

A console writer is the natural destination for interactive tools and foreground service runs.
It understands the terminal width and lays each formatted source line out as a paragraph.
The following fields let an application reserve space for surrounding terminal output and keep long messages visually
connected to their first line.

.. list-table::
    :header-rows: 1
    :widths: 28 16 56

    * - Field
      - Default
      - Values and purpose
    * - ``Line Indent``
      - ``0``
      - A nonnegative leading column count for every line.
    * - ``First Line Indent``
      - ``-1``
      - A nonnegative first-line indent, or ``-1`` to reuse ``Line Indent``.
    * - ``Wrapped Line Indent``
      - ``-1``
      - A nonnegative continuation indent, or ``-1`` to reuse ``Line Indent``.
    * - ``Maximum Line Wraps``
      - ``0``
      - A nonnegative wrap limit per source line; zero allows unlimited wrapping.

For a specialized indent, ``-1`` tells the writer to inherit ``Line Indent``; it is not a negative terminal column.
A zero wrap limit means unlimited wrapping rather than no wrapping.
Choose a finite limit when untrusted or exceptionally long messages could otherwise dominate an interactive screen.
:doc:`console_writers` shows the paragraph layout and styles in action.

Preserve an Operational History in Files
========================================

A file writer gives an application durable history that remains available after the terminal session has ended.
Its configuration begins with the active ``Path``, then decides how the first open treats an existing file and when the
writer moves the active content into an archive.
Rotation is useful only when it is paired with a retention policy that matches the available storage and the period
operators may need to investigate.

.. list-table::
    :header-rows: 1
    :widths: 24 20 56

    * - Field
      - Default
      - Values and purpose
    * - ``Path``
      - Required
      - The nonempty active file path.
    * - ``Mode``
      - ``append``
      - ``append`` or ``overwrite`` for the first successful open.
    * - ``Rotation``
      - ``none``
      - ``none``, ``hourly``, ``daily``, ``weekly``, or ``size``.
    * - ``Maximum Size``
      - ``10'485'760``
      - A positive byte threshold used by size rotation.
    * - ``Retention``
      - ``7``
      - A nonnegative number of rotated archives to keep.

``Mode`` governs the first successful open; it does not turn every later retry into an overwrite.
``Maximum Size`` is used only by size rotation, while hourly, daily, and weekly rotation use entry timestamps to choose
their boundaries.
``Retention`` counts rotated archives, not the active file, and zero removes archives once they are no longer active.
:doc:`file_writers` covers archive names, external file replacement, retry behavior, and failure accounting.

Send Service Logs to a Syslog Collector
=======================================

Syslog is useful when a service's logs must leave the local machine and join a central operational view.
The writer encodes entries as RFC 5424 messages and delivers them over UDP, TCP, or TLS.
The transport is therefore the first operational choice: UDP has no connection setup but cannot confirm delivery, while
TCP and TLS maintain a connection and can accumulate pending bytes during an interruption.

.. list-table::
    :header-rows: 1
    :widths: 28 22 50

    * - Field
      - Default
      - Values and purpose
    * - ``Endpoint``
      - Required
      - A host and nonzero port; UDP requires a numeric IP address.
    * - ``Transport``
      - ``udp``
      - ``udp``, ``tcp``, or ``tls``.
    * - ``Facility``
      - ``1``
      - An RFC 5424 facility number from zero through 23.
    * - ``Host Name``
      - ``-``
      - The RFC 5424 HOSTNAME field or ``-`` for NILVALUE.
    * - ``Application Name``
      - ``erbsland-core``
      - The RFC 5424 APP-NAME field or ``-`` for NILVALUE.
    * - ``Process Id``
      - ``-``
      - The RFC 5424 PROCID field or ``-`` for NILVALUE.
    * - ``Message Id``
      - ``-``
      - The RFC 5424 MSGID field or ``-`` for NILVALUE.
    * - ``Tls Label``
      - ``log/syslog``
      - A nonempty network/TLS configuration label used by TLS transport.
    * - ``Maximum Pending Bytes``
      - ``1'048'576``
      - A positive limit for encoded syslog data awaiting delivery.

The RFC identity fields are deliberately explicit.
Use meaningful stable values when the collector relies on them for search or routing, and retain ``-`` when a field has
no trustworthy value.
``Tls Label`` selects the application's network/TLS configuration rather than embedding certificates or trust choices in
the logging branch.
``Maximum Pending Bytes`` bounds local memory while a connection is unavailable; once that bound is reached, additional
encoded messages are dropped and counted rather than allowing logging to grow without limit.
:doc:`syslog_writer` discusses framing, transport behavior, and deployment guidance in detail.
