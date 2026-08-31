.. index::
    single: Logging; Overview

****************
Logging Overview
****************

Erbsland Core logging moves formatting and destination work away from application threads while bounding the memory used
in between.
Producers write immutable entries through named streams; the manager applies one configuration snapshot on its worker
and delivers each accepted entry to matching writers.

Choose a Setup
==============

:doc:`using_logging` starts with the application-wide defaults and shows when a program benefits from file history,
retained service errors, or configuration outside the executable.
It is the best entry point when you are deciding how much logging infrastructure an application actually needs.

Write Messages
==============

:doc:`using_log_streams` explains how streams are named and shared, how severity communicates intent, and how printable
values and formatting controls become safe log messages.
It also introduces the inexpensive trace guard for diagnostics that are costly to prepare.

Configure the Manager in Code
=============================

:doc:`configuring_logging` places configuration replacement, pause and resume, and shutdown in the application
lifecycle.
It treats a configuration as a complete snapshot so routes and operational settings remain predictable.

:doc:`configuring_line_format` covers patterns, timestamps, level and stream-name representations, and every
rendering-time truncation mode.
:doc:`configuring_manager_options` then explains the independent producer and queue limits, reserved warning and error
capacity, shutdown deadlines, and statistics.

:doc:`trace_sections` shows how to group detailed diagnostics by investigation, enable them with routes, and guard
producer work while a section is inactive.

Route Entries to Destinations
=============================

:doc:`using_writers` introduces the writer-and-filter model. It explains complete-segment path routing, overlapping
routes, the built-in destinations, and the small extension point for a custom writer.

:doc:`console_writers` focuses on terminal integration, paragraph wrapping and indentation, and layered semantic
styles.
:doc:`file_writers` covers initial file handling, time and size rotation, archive retention, external file replacement,
and retry accounting.
:doc:`syslog_writer` completes the built-in destination sequence with UDP, TCP, and TLS transport, RFC 5424 fields,
framing, and pending-data limits.

Load Configuration From ELCL
============================

:doc:`reading_configuration` demonstrates the complete application startup path: parse a file, optionally select a
subsection, validate it, and install the result while preserving useful configuration diagnostics.

:doc:`elcl_configuration` is the field guide for writing that configuration. It provides parser-exercised minimal,
root-level, and nested examples, followed by every format, queue, trace-section, writer, and route field with its
default and valid values.
