..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*******************
Framework Reference
*******************

This page describes the shared runtime behavior in ``tool::frame``.
Use it when changing the framework, writing boundary tests, or investigating a startup or file access problem.
For everyday command invocation, see :doc:`Use the maintenance tool <../tool_usage/usage>`.

.. _development-tool-framework_reference-startup-sequence:

Startup Sequence
================

``ToolApplication`` (``tool/lib/frame/src/tool/frame/ToolApplication.cpp``) uses Core's application lifecycle for
parsing options and displaying help, version information, and the command overview.
Those displays do not require project discovery or configuration.

For command execution, the application:

1. Initializes console logging and any requested log file.
2. Discovers or validates the project root and creates ``ProjectFiles``.
3. Reads the configuration, including its included files.
4. Validates expanded global rendering headers, selects the command, and validates its configuration section.
5. Resolves the command's ``usedDataSources()`` identifiers, deduplicating them in declaration order.
6. Validates every required source section before loading any source.
7. Assigns the command's log stream and calls ``configure()`` to validate its options and settings before loading sources.
8. Assigns every selected source's log stream and calls every source's ``configure()`` before any source initializes.
9. Initializes sources in dependency declaration order, marking each ready only after successful initialization.
10. Calls the command's ``initialize()`` and ``execute()``. Normal completion returns the command's ``exitCode()``, which defaults to success.

Command and source ``configure()`` callbacks must not access initialized source data.
Move source-dependent work to ``initialize()`` or ``execute()``.

Project file and rendering services are available through ``frame::app()`` during source and command ``configure()`` and
``initialize()``, and command ``execute()``.
Helpers that use them also depend on an active application lifecycle.
Log files are opened before project validation and are not mediated by ``ProjectFiles``.

.. _development-tool-framework_reference-project-discovery:

Project Discovery
=================

``ProjectRoot`` (``tool/lib/frame/src/tool/frame/ProjectRoot.cpp``) searches from the working directory upward unless
``--project`` selects a directory explicitly.
An explicit directory is validated directly; it does not start an upward search.

The first 1,024 characters of the root ``CMakeLists.txt`` must declare the CMake project supplied by the executable, for
example ``project(erbsland-core ...)`` for Core or ``project(erbsland-core-ldap ...)`` for LDAP.
Matching ignores case and permits whitespace around the command and project name.
The name must end with whitespace or ``)``, so a longer name beginning with ``erbsland-core`` does not match.
Discovery stops at the filesystem root or an access failure.

These marker probes occur before a project root is known and may inspect parent directories outside the eventual
project.
Once a root is selected, project data access goes through ``ProjectFiles``.

.. _development-tool-framework_reference-project-file-access:

Project File Access
===================

``ProjectFiles`` (``tool/lib/frame/src/tool/frame/ProjectFiles.hpp``) interprets relative paths from the validated
project root.
It normalizes paths lexically, verifies containment before querying path components, and rejects symlinks and reparse
points before visiting children.
General text and binary reads enforce the link policy at the native file-open boundary, avoiding a redundant metadata
walk.
Absolute paths are allowed if they remain inside the project.
Commands must use this service for project inputs and outputs to receive these checks.

.. list-table::
    :header-rows: 1
    :widths: auto

    * - Method
      - Behavior
    * - ``resolve()``
      - Resolves and confines a path lexically; does not check filesystem entries
    * - ``checked()``
      - Returns a checked absolute path; permits a missing final file and optionally missing output parents
    * - ``createDirectories()``
      - Creates confined output directories and missing parents, preserving existing directories and their contents
    * - ``collectFilenamesOnly()``
      - Returns sorted matching filenames for inventories and name checks, with optional recursive descent
    * - ``readTextFile()``
      - Reads tolerant UTF-8 with file identity, size and timestamps for later comparison
    * - ``readWalkedTextFile()``
      - Reads a retained regular-file entry from a link-skipping walk below a validated project directory
    * - ``isUnchanged()``
      - Reloads snapshot metadata and compares equivalent text without rereading an unchanged file
    * - ``readText()``
      - Reads a regular file as tolerant UTF-8
    * - ``readData()``
      - Reads a regular file as bytes
    * - ``textChecksum()``
      - Calculates a confined file's line-ending-independent checksum
    * - ``readConfiguration()``
      - Parses a checked configuration file with the include rules below
    * - ``updateText()``
      - Compares text and optionally replaces a changed output file

Text reads use ``EncodingMode::Tolerant`` with automatic BOM handling.
For UTF-8 input, the reader removes an optional initial BOM and retains the remaining bytes without a validation pass.
String character operations interpret malformed sequences as replacement characters when needed.
Use ``readData()`` when exact file bytes matter, including the BOM, or ``textChecksum()`` for text snapshot hashes.
Maintenance inputs do not require strict text decoding; commands validate the records and syntax their formats need.

The maintenance tool prevents accidental access outside the project; it does not treat a concurrently manipulated
checkout as a security boundary.
``readWalkedTextFile()`` trusts the parents inspected by the walk and uses an ordinary file open.
It still checks lexical containment and the regular-file snapshot.
A worker may retain the reported path and read it after the callback returns, within the same inventory run.
Configured inputs, output destinations and mutations retain their independent checks.

Use ``collectFilenamesOnly()`` when only names are needed.
For content processing, use ``PathWalker`` directly, skip links, and filter matching regular files.
Read each selected file with ``readWalkedTextFile()`` inside the callback to reuse the walk's directory context and
avoid repeating parent-directory checks.

.. _development-tool-framework-text-checksums:

Text Checksums
--------------

``frame::calculateTextChecksum(text, algorithm)`` returns digest bytes after representing CRLF and lone CR as LF.
Its byte-block overload applies the same contract without decoding text.
``ProjectFiles::textChecksum(path, algorithm)`` performs a confined read and retains file diagnostics.
All other content remains significant, including whitespace, BOM bytes, and the presence of a final line terminator.

Cryptology text sources and fixture manifests use SHA-256. ZIP source archives use binary SHA-256. Unicode,
public-suffix, time-zone, and terminal-frame snapshot manifests use SHA3-256 with the same text normalization.
The library's cryptology manifest test implements this contract independently of the maintenance tool.
No Git line-ending attributes are required.

Parallel Work
-------------

The tool uses Core's ``util::WorkerPool`` for independent, joinable parallel execution.
``PublicHeaders`` creates one pool with the configured ``--jobs`` worker count, independently of the stream I/O service
that its jobs may wait for.
Each reading, forward-header parsing, or rendering stage snapshots its inputs and submits one bulk ``map()`` operation.
The pool schedules items incrementally, so a stage needs one aggregate future and no per-header queue entries.
The calling thread consumes the ordered results and updates its indexes after the stage completes.

Operational errors propagate through the aggregate future after running items finish, and a failed stage starts no
further items.
Pool destruction drains accepted work before releasing borrowed inputs.
Jobs must not synchronously wait on work from the same pool.
Callers must consume each stage's future and finish validation before writing outputs.

Output Comparison and Replacement
---------------------------------

``readTextFile()`` returns an immutable ``ProjectTextFile`` snapshot.
``isUnchanged()`` reuses its contents only when fresh metadata confirms the same regular file, physical path, identity,
size, modification time and metadata-change time.
Snapshots live within a generation run; there is no persistent file-content cache.
A changed snapshot falls back to the normal confined read and replacement path.

``updateText()`` compares physical lines using two LF :cpp:type:`StringSplitter <erbsland::text::StringSplitter>`
instances with ``KeepSeparator``.
It strips only trailing CR/LF characters, preserving spaces, tabs, interior characters, and blank lines.
A missing final terminator is equivalent; additional blank lines are meaningful.
Empty text differs from one blank line.

On the first physical line only, recognized ``//`` or ``#`` copyright comments using ``Copyright (c)`` or
``Copyright ©`` ignore a four-digit year or year range.
Marker, owner, and surrounding text still compare normally.
Later copyright lines compare exactly.

Equivalent content returns ``false`` and retains existing bytes and modification time.
For different or missing output, it returns ``true``.
With ``checkOnly`` enabled, it does not write.
Otherwise, it writes the complete text through a temporary file in the existing destination directory and moves that
file over the output.
Existing output is preserved if preparation or writing fails.

Configuration Placeholders
--------------------------

``readConfiguration()`` accepts an optional placeholder-variable map.
Supplying it enables Core's variable and environment sources and text filters, including in included configuration
files.
Omitting it leaves manifests unexpanded.
See :doc:`rendering_reference` for rendering settings and placeholder syntax.

.. _development-tool-framework_reference-configuration-loading-and-includes:

Configuration Loading and Includes
==================================

The executable supplies a config path relative to the selected root.
Core uses ``tool/conf/erbsland-core-tool.elcl``.
An explicit relative ``--config`` path is instead resolved from the working directory.
The main configuration file is checked by ``ProjectFiles`` before parsing.

Includes use Core's :cpp:class:`FileSourceResolver <erbsland::conf::FileSourceResolver>`.
The default configuration includes the ``.elcl`` files in its ``misc``, ``source``, and ``tools`` directories.
Filename wildcards are supported; recursive wildcards are disabled.

Include Access Checks
---------------------

Included sources take a different route from ordinary project input files.
``ProjectAccessCheck`` (``tool/lib/frame/src/tool/frame/ProjectAccessCheck.cpp``) checks each source's absolute path
before Core opens it.
Core resolves included symlinks to their physical targets, and only targets inside the project are accepted.
Directory resolution and wildcard scanning happen before this access check.
Thus an include symlink with an internal target can be accepted even though an ordinary ``ProjectFiles`` input path
cannot traverse a symlink.

Section Validation
------------------

The entire configuration and its includes must be syntactically valid.
The framework validates rendering, the selected command, and its declared sources.
The following semantics apply to both command and source ``configurationRules()``:

.. list-table::
    :header-rows: 1
    :widths: auto

    * - ``configurationRules()`` returns
      - Required section
      - Argument to ``configure()``
    * - Rules
      - A regular section named after the command or source
      - Validated section, with defaults applied using rule version 1
    * - ``nullptr``
      - No section with that name
      - ``nullptr``

Unused sources remain unloaded and their sections remain unvalidated.
Invalid values in another command's section do not affect the selected command's validation.
Any required source failure prevents all command callbacks and execution.
Parse errors and include access errors still prevent execution.

.. _development-tool-framework_reference-registration-checks:

Registration Checks
===================

Command and source identifiers must match ``[a-z]+(?:_[a-z]+)*``.
Identifiers must be unique across both registries and cannot use the reserved ``rendering`` section name.
Sources and commands can be registered in either order; registration closes before command-line processing.
Command registration also rejects empty descriptions and titles that are empty or longer than 60 characters.
During option registration, the application also requires a non-null option module whose name matches the command's
canonical hyphen spelling and whose help title is nonempty.

.. _development-tool-reference-template-rendering:

Template Rendering
==================

The maintenance library embeds ``.cpp`` and ``.tpl`` templates from ``tool/lib/<part>/resources`` and its
subdirectories.
``createRenderEnvironment()`` loads those templates by resource path under the ``erbsland-core-tool`` identifier.
The display-text compiler embeds shared ``.hpp`` and ``.cpp`` templates from ``cmake/tools/display-text-compiler/data``
under a separate identifier and adds its own loader.
See :doc:`display_text_generator_reference` for that generator's contract and :doc:`../tool_dev/write_command` for
adding a generator.

The renderer supplies ``tool_name``, ``current_year``, ``copyright_line``, and ``license_line`` globals.
Local context values take precedence.
Templates supply comment prefixes; output comparison recognizes copyright years only on the first physical line.
See :doc:`rendering_reference` for header configuration.

``TemplateLanguage::Text`` uses the normal ``{{`` / ``}}`` expression delimiters and ``{%`` / ``%}`` statements.
It shares the copyright and license globals with C++ rendering.

``TemplateLanguage::Cpp`` uses these delimiters so that C++ braces do not conflict with template expressions:

.. list-table::
    :header-rows: 1
    :widths: auto

    * - Template element
      - Delimiters
    * - Expression
      - ``/*{`` and ``}*/``
    * - Statement
      - ``/*%`` and ``%*/``

A ``-`` immediately after a statement opener (``/*%-``) removes preceding whitespace.
The IDNA template uses this on control lines to keep generated arrays free of blank lines.

Automatic escaping is disabled.
Templates are loaded from compiled resources; there is no filesystem template loader.
The default resource collection comes from the active application, though ``createRenderEnvironment()`` also accepts an
explicit collection.

