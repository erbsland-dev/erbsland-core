..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

****************************
Inspect Include Dependencies
****************************

``include-dependencies`` inspects explicit includes under ``src/erbsland``.
Use it to find dependency chains, cycles, and published headers missing a required dependency.
See :doc:`usage` for shared invocation options.

.. code-block:: console

    ./erbsland-core-tool include-dependencies
    ./erbsland-core-tool include-dependencies tree text/String.hpp --max-depth 3
    ./erbsland-core-tool include-dependencies tree core/Definitions.hpp --reverse
    ./erbsland-core-tool include-dependencies path text/String.hpp core/Definitions.hpp
    ./erbsland-core-tool include-dependencies cycles
    ./erbsland-core-tool include-dependencies check definitions --all
    ./erbsland-core-tool include-dependencies export --format json
    ./erbsland-core-tool include-dependencies export --format dot --output includes.dot

The default mode prints a summary.
Source arguments accept complete source-relative paths or unambiguous suffixes.
Checks suggest a minimal set of public headers in which to add a direct include; they do not edit source files.
Run ``update-public-headers`` first if the generated publication mapping is missing or stale.

Exports go to stdout unless ``--output`` supplies a filename.
Named exports stay under ``temporary_data/include_dependencies`` and require Git to confirm that the destination is
ignored and untracked.
An export never overwrites a tracked report.
Settings and required rules live in ``tool/conf/tools/include_dependencies.elcl``.

See :doc:`../tool_reference/include_dependencies_reference` for the graph model, options, schema, and configuration.
