..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***********************
Prepare Doxygen Sources
***********************

Prepare Doxygen-compatible source headers in ``_doxygen_input``.
Sphinx invokes this command automatically, so ordinary documentation work usually uses ``doc-build`` instead.

.. code-block:: console

    ./erbsland-core-tool doc-preprocess-sources

Use ``--force`` to regenerate despite a matching cache marker.
The generated directory is disposable; review the source transformations when changing this command.

See :doc:`usage` for shared invocation options and
:doc:`../tool_reference/documentation_tools_reference` for the technical contracts.
