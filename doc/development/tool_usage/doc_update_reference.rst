..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

******************************
Update Reference Documentation
******************************

Regenerate the final Interface sections and navigation of explicitly configured reference groups.
The command preserves hand-maintained page introductions and prose before the Interface section.

.. code-block:: console

    ./erbsland-core-tool doc-update-reference

Configuration lives in ``tool/conf/tools/doc_update_reference.elcl`` and its included ``doc_reference`` files.
Review generated interfaces, indexes, and orphan removals.
Register prose-only pages in ``Manual Reference Pages`` before running the command.

See :doc:`usage` for shared invocation options and
:doc:`../tool_reference/documentation_tools_reference` for the technical contracts.
