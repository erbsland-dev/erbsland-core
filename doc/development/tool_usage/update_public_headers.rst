..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*********************
Update Public Headers
*********************

``update-public-headers`` maintains the public include tree, domain umbrellas, namespace declarations and consumer
exports.
Run it after adding, moving, or removing public headers or source directories.
Library builds consume the checked-in results and do not run the generator automatically.
See :doc:`usage` for shared options and :doc:`../setup` for installation.

Configure and Generate
======================

Edit ``tool/conf/tools/update_public_headers.elcl``.
Every source directory outside an excluded subtree needs a directory section.
Header patterns match filenames in that directory only.
An organizational subdirectory inherits its parent's namespace and publication settings; set ``Nested Namespace: Yes``
when its name is also a C++ namespace.
Each entry computes its own output location from ``Flatten``: the include root for ``Yes``, or its full source-relative
directory for ``No``.
Use ``Target Directory`` only for a local override such as the shortened ``zip`` destination.

.. code-block:: console

    ./erbsland-core-tool update-public-headers

Reading, parsing and rendering use the worker-count hint from Core's system service by default.
Use ``--jobs 2`` to limit concurrency while other builds are running, or ``--jobs 1`` to use a single maintenance
worker.
The accepted range is 1 to 64; worker count does not change the generated output.

Review the public wrappers, source umbrellas, ``core/Namespaces.hpp``, ``core/impl/MakeOneNamespace.hpp``, and
``tool/data/public_headers/header_map.txt`` together with the source changes.
Do not edit these generated files.
The generator retains compatible include aliases and removes obsolete files carrying its generated-file marker.
The repository's ``pre-commit`` utility runs this command before normalizing consumer includes.

Use Focused Includes
====================

Prefer the canonical public header for each API unit, for example ``<erbsland/TlsClientConnection.hpp>`` or
``<erbsland/RegEx.hpp>``.
Real public namespaces keep their own include directories, such as ``<erbsland/cterm/Terminal.hpp>``.
Disabling consumer namespace flattening does not change these include paths.

Named umbrellas such as ``<erbsland/all_network.hpp>`` include only the public headers directly in that directory.
Include a subgroup explicitly, for example ``<erbsland/all_network_http.hpp>`` or ``<erbsland/network/http/all.hpp>``.
Nested aliases such as ``<erbsland/network/all.hpp>`` remain available.
There is no universal ``<erbsland/all.hpp>``.
Global ``Excluded From All Headers`` patterns apply to every umbrella; directory patterns add local exclusions.
Optional formatter adapters and resource-storage registration require explicit includes.

Check and Diagnose
==================

.. code-block:: console

    ./erbsland-core-tool update-public-headers --check

Check mode fails for missing, changed, or obsolete outputs without writing or removing files.
Run without ``--check`` to refresh them.
Unchanged files retain their timestamps.

An uncovered-directory error requires a directory section or an explicit exclusion.
A namespace mismatch requires correcting the declaration or the directory's ``Nested Namespace`` setting.
Wrapper collisions require distinct targets or an intentional entry in ``Mergeable Header Names``.
Unsupported selective declarations need a canonical forward header.
These errors are reported before any outputs change.

If the installed tool does not recognize ``pre-commit``, rerun ``setup.py`` to refresh it.
See :doc:`../tool_reference/public_headers_reference` for configuration inheritance, exports, and the mapping format.
