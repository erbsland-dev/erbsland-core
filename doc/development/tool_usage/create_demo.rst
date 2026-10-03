..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***************************
Create a Documentation Demo
***************************

Create a demo collection or add a part to an existing multi-part collection.
Choose the domain, collection name, and part name before running the command.

.. code-block:: console

    ./erbsland-core-tool create-demo text/Sample/BasicUsage

The command prints the planned files and naming warnings before asking for confirmation.
Use ``--yes`` when running noninteractively.
Review generated CMake lists, declarations, registrations, and the printed documentation directive.
Implement the new source part, build the demo, then synchronize its documentation using ``doc-process-rst``.

See :doc:`usage` for shared invocation options and
:doc:`../tool_reference/documentation_tools_reference` for the technical contracts.
