..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***************************
Customize Generated Notices
***************************

Generators share copyright and license settings in ``tool/conf/misc/rendering.elcl``.
The main configuration includes this file automatically.
Leave its ``[Rendering]`` section empty to use the repository defaults.

Change a Notice
===============

Set the text without a C++ comment prefix; templates supply the prefix:

.. code-block:: elcl

    [Rendering]
    Copyright Line: "Copyright (c) ${var:year} Example Project"
    License Line: "SPDX-License-Identifier: Apache-2.0"

``${var:year}`` uses the year captured when the tool starts.
Each notice must expand to one nonempty line.
You can also use an environment variable, for example ``${env:OWNER,required|trim}`` for an owner supplied by your
shell.

Run the affected generator after changing these settings and review its output.
A change only to the copyright year leaves an otherwise equivalent file untouched.
See :doc:`../tool_reference/rendering_reference` for defaults, placeholder syntax, validation, and reproducibility.
