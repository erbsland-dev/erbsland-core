..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

************************
Public Headers Reference
************************

``update-public-headers`` resolves an explicit directory policy into a public include tree and consumer namespace
exports.
See :doc:`../tool_usage/update_public_headers` for the generation workflow.

Global Configuration
====================

The required ``[Update Public Headers]`` section lives in ``tool/conf/tools/update_public_headers.elcl``.
There are no command-line path overrides.
``--check`` suppresses all writes and deletions and reports outdated output as a failure.
``--jobs`` (``-j``) selects 1 to 64 read, parsing and rendering workers.
Without an override, the default uses ``system::info::logicalCpuCount()``, Core's usable logical CPU count hint.

.. list-table::
    :header-rows: 1

    * - Text setting
      - Default
    * - ``Source Base Directory``
      - ``src/erbsland``
    * - ``Include Base Directory``
      - ``include/erbsland``
    * - ``Base Namespace``
      - ``erbsland``
    * - ``Namespaces Header``
      - ``core/Namespaces.hpp``
    * - ``Make One Namespace Header``
      - ``core/impl/MakeOneNamespace.hpp``
    * - ``Mapping File``
      - ``tool/data/public_headers/header_map.txt``

Domain-scoped extensions can set ``Generate Namespace Headers`` to ``No`` to generate only their wrappers, umbrellas,
and mapping.

Base directories and the mapping file are relative to the selected project.
Namespace-header paths are relative to the source base.
Source and include trees must not overlap; the mapping file must live outside both trees.
Paths use ASCII letters, digits, underscores, dots, hyphens and slash separators, without empty or dot components.
Project access rejects paths outside the selected project.
Inventory skips links and queues accepted paths for reading during the same generation run, trusting that parent
directories are not concurrently replaced.
Output access retains separate link and containment checks.
Missing output directories are created only during generation.

The following optional settings accept one text value or a list, defaulting to empty:

* ``Excluded Directories`` lists directory basenames whose subtrees are excluded from publication and coverage checks.
* ``Excluded Headers`` contains filename-only publication exclusion globs.
* ``Excluded From All Headers`` contains filename-only globs excluding headers from every umbrella, while retaining their
  individual public wrappers.
* ``Mergeable Header Names`` lists filenames intentionally allowed to forward to multiple source headers.
* ``Declared Namespaces`` supplies additional namespace names relative to ``Base Namespace``.

``Generated Markers`` accepts one text value or a list of line-comment prefixes.
It defaults to ``"// !!! THIS IS A GENERATED FILE"``, covering both current and legacy generated headers.
The first marker is emitted in new output; later markers recognize older output during migrations.
Markers must start with ``//`` and contain no line breaks.
Recognition examines at most the first ten physical lines of the initial comment banner, allowing a preceding
``#pragma once``.
Blank lines count toward this limit.
A later marker or text inside a string literal does not establish ownership.

The optional ``Namespace Aliases`` section maps base-namespace alias names to qualified names relative to the base.
For example, ``Sys Info: "system::info"`` declares the target namespace and exports ``sys_info``.
The production configuration retains the existing aliases for ``cpp``, ``io``, ``html``, ``punycode``, and ``sys_info``.

The optional ``Header Aliases`` section contains named entries with ``Source Header`` and ``Include Header`` paths,
relative to the source and include bases respectively.
Explicit entries also restore missing compatibility wrappers.
The production entry for ``system/SystemInfo.hpp`` preserves ``SystemInfo.hpp`` independently of the selective system
namespace exports.

Directory Sections
==================

``Directory`` is a required section containing the source-directory tree.
Section names are normalized ELCL names; their lowercase underscore spelling names the directory.
Every non-excluded source directory must be represented, including organizational and structural parent directories.
Configured directories must exist.

Top-level sections establish a domain namespace, default to ``Flatten: No``, use their directory name as the public
target directory, and select ``*.hpp``.
Nested sections inherit publication and namespace settings unless overridden.
Each entry computes its own target directory from its effective ``Flatten`` value.
Target-directory overrides are not inherited.

.. list-table::
    :header-rows: 1

    * - Setting
      - Behavior
    * - ``Nested Namespace``
      - Boolean, default ``No`` at each level. ``Yes`` appends the directory name to the parent's C++ namespace.
    * - ``Flatten``
      - Boolean, inherited. ``Yes`` defaults the target directory to the include root and exports the namespace.
    * - ``Target Directory``
      - Text overriding this entry's canonical wrapper directory. When omitted, ``Flatten: Yes`` selects the include root and
        ``Flatten: No`` selects the full source-relative directory. Explicit empty text selects the include root.
    * - ``Target Namespace``
      - Text naming a base-namespace alias. Overrides the namespace-wide import.
    * - ``Included Headers``
      - Filename-only Core globs selecting published headers.
    * - ``Excluded Headers``
      - Filename-only globs removing headers from publication and umbrellas.
    * - ``Excluded From Flattening``
      - Filename-only globs retaining source-shaped canonical paths, without removing publication.
    * - ``Excluded From All Headers``
      - Filename-only globs requiring explicit includes instead of umbrella inclusion.
    * - ``Included Symbols``
      - Explicit names replacing namespace-wide imports; only their owning headers gain the flattened target path.

``Included Symbols`` requires ``Flatten: Yes`` and cannot be combined with ``Target Namespace``.
Directory lists are inherited and replaced by an explicit value.
Global umbrella exclusions apply independently: a header is omitted if either its global or effective directory patterns
match.
A local list cannot cancel a global exclusion.
The production configuration excludes ``StdFormat.hpp`` globally and ``ResourceStorageRegistration.hpp`` only in the
``resource`` entry.
The latter is an implementation interface published for generated resource code.
Glob settings accept a single text value or a list and use ``StringPattern::fromGlob`` against filenames only.
A slash in a pattern is an error.
Generated ``all.hpp`` and ``fwd.hpp`` files are never discovered as ordinary public headers.
The production exclusions also omit underscore part headers and canonical forward headers.

A new nested namespace clears inherited selective exports and target aliases.
Select ``Flatten: No`` when the nested namespace must not inherit flattening.
Specify ``Target Directory`` only to override the computed destination, for example ``"zip"`` for ``compression/zip``.
Organizational sections need no settings:

.. code-block:: erbsland-conf

    [Update Public Headers.Directory.Network]
    Flatten: Yes
    [Update Public Headers.Directory.Network.Tls]
    [Update Public Headers.Directory.Conf]
    Flatten: No
    [Update Public Headers.Directory.Conf.Vr]
    Nested Namespace: Yes

Namespace Verification and Exports
==================================

The scanner ignores comments, string and character literals, preprocessor directives, and nested type bodies.
Library namespace declarations must match the configured namespace or an auxiliary child namespace.
Specializations in ``std`` and collection headers without declarations are allowed.
The generated namespace-list header is exempt.
Corresponding forward headers are inspected as well.

Selective exports reuse canonical forward headers, but export only the explicitly listed names.
Without such a declaration, extraction supports non-template classes and structs and enums with supported built-in or
fixed-width integer underlying types.
Unsupported aliases, templates, and enum types require a canonical forward header.
The generator never includes a full definition header merely to make a selective export available.

Consumer flattening remains controlled by ``Definitions.hpp``.
The generated helper retains the ``ERBSLAND_CORE_BUILDING_LIBRARY`` guard; consumers can disable flattening with
``ERBSLAND_CORE_DO_NOT_FLATTEN_NS``.
The production policy exports only ``RegEx`` from ``re`` and preserves the selective ``system`` exports.

Wrappers, Umbrellas and Updates
===============================

Each published header has a canonical wrapper and its source-shaped alias.
Existing recognized generated wrappers are retained as aliases while their source remains public.
Wrapper paths differing only in ASCII case are rejected.
Multiple sources may share a wrapper only when its filename is explicitly mergeable.

Each configured directory receives a source ``all.hpp`` containing only its directly selected public headers, excluding
headers matched by global or local umbrella exclusions.
Descendant headers and umbrellas are not collected automatically, regardless of whether the child introduces a C++
namespace or organizes declarations in the same namespace.
Directories without eligible direct headers still receive an empty umbrella.
Ordinary dependencies included by individual headers remain unchanged.
Its preferred public wrapper is ``all_<source_path>.hpp`` with slashes replaced by underscores; the nested ``all.hpp``
wrapper remains an alias.
Real namespace destinations also provide an ``all.hpp`` alias, such as ``render/all.hpp``.
The universal root ``all.hpp`` is not generated.

Each source and include tree is walked once.
The walk records inputs for one bulk ``WorkerPool::map()`` operation that reads headers and parses selected source
declarations with bounded worker concurrency.
A second bulk operation parses the corresponding forward headers needed for declaration validation.
The inventory retains shared header text and path metadata for declaration checks, compatibility aliases, ownership
checks and cleanup.
Workers return independent results; the calling thread merges them into ordered indexes and resolves publication
relationships.
Rendering maps independent outputs through a shared, fully configured rendering environment.
Each stage has one future, and its ordered results make generated bytes independent of worker completion order.
Queued and running work drains before preparation returns, including when a worker reports a read, parsing or rendering
error.
Output planning validates each distinct parent directory once and resolves the mapping destination once.
Unchanged outputs reuse the inventory text after reloading their file identity, size and timestamps.
Changed files use the normal confined comparison and atomic replacement path.
Parent creation is deferred until an output needs updating.
Excluded subtrees remain visible to obsolete-umbrella cleanup; other excluded files are not read.
Discovery, namespace checks, collisions and output validation finish before writes start.
Updates use the framework's atomic per-file replacement and equivalent-text comparison, which ignores line endings and
first-line copyright years.
This includes the mapping file: an existing CRLF file equivalent to the generated LF text retains its contents and
timestamp.
This is not a transaction across every file if an I/O error occurs during writing.
Obsolete public wrappers and source ``all.hpp`` umbrellas are removed only when their initial banner matches a
configured marker.
Other source headers, including generated implementation data, are outside this cleanup scope.
Unmanaged output conflicts are rejected, except for the explicitly configured namespace headers adopted during
migration.

Mapping File
============

New mappings are UTF-8 without a BOM and use LF endings.
Readers also accept CRLF endings, blank lines and comment lines beginning with ``#``.
Every data row contains ``source;include``, without a header row, quoting or metadata.
Both paths are relative to their configured bases and use slash separators.
The restricted path alphabet excludes semicolons and line breaks.

Rows are grouped by source path in ASCII order.
The first row of each group is canonical; remaining distinct aliases are sorted.
Generated directory umbrellas are included.
A merged wrapper has one relationship for each source it forwards to:

.. code-block:: text

    network/tls/TlsClientConnection.hpp;TlsClientConnection.hpp
    network/tls/TlsClientConnection.hpp;network/tls/TlsClientConnection.hpp

The ``code-cleanup`` command uses these canonical paths without narrowing merged wrappers.
Dependency analysis uses source membership from the same file, excluding generated umbrellas from its
regular-public-header counts.

Reusable C++ Tokenizer
======================

``erbsland::text::cpp::CppTokenizer`` streams tokens using ``StringCharReader`` captures that share source storage.
``next()`` supplies one token at a time; ``tokens(source)`` exposes a ``CoGenerator`` owning its source string.
Tokens retain their lexical kind, original spelling and character position.
Whitespace is skipped, while comments, directives, ordinary literals and raw literals remain available to analysis
tools.
Unterminated comments and literals raise ``ParseError`` with a code-point position.
See :doc:`/reference/text/cpp_tokenizer` for the public interface.
The tokenizer does not expand macros or select conditional preprocessing branches.

``HeaderDeclarations`` consumes significant tokens directly, tracking namespace scopes without retaining a token list.
Its reusable ``StringEditor`` buffer assembles qualified namespace names.
Output templates under ``tool/lib/headers/resources/headers`` own include statements, namespace declarations, guards and
exports; the planner supplies structured lists and maps.
