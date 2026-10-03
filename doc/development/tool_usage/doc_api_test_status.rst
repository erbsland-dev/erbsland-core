..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

************************
Validate API Test Status
************************

Validate coverage markers in library source headers under ``src/erbsland``.
Tools, tests, and demo comments do not require coverage markers.

.. code-block:: console

    ./erbsland-core-tool doc-api-test-status

Use ``--normalize`` to normalize legacy marker syntax and remove member-level markers before validation.
Review changes before committing; validation alone does not write files.

See :doc:`usage` for shared invocation options and
:doc:`../tool_reference/documentation_tools_reference` for the technical contracts.
