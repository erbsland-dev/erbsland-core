
******
Groups
******

Capture groups in the engine are indexed, and (optionally) can be assigned a name.

The assembler provides two commands to configure capture groups:

- ``.groups`` sets how many capture groups exist.
- ``.group`` assigns a name to a specific capture group.

Groups are one-based, i.e. the first group has index ``1``.

The ``.groups`` Command
=======================

Syntax
------

:expression:`.groups 3`
    Declare that the program uses three capture groups.

Notes
-----

- ``.groups`` can only be used once.
- It must appear before any program code is emitted.
- The maximum number of capture groups is ``100``.

The ``.group`` Command
======================

Syntax
------

:expression:`.group 1 "name"`
    Assign the name ``name`` to capture group 1.

Arguments
---------

``index``
    Group index in the range ``1`` to the value passed to ``.groups``.

``"text"``
    The group name.

Notes
-----

- ``.groups`` must be used before ``.group``.
- Each group name can only be set once.
- Any additional token after the name causes an error.

Examples
========

.. code-block:: text

    .groups 2
    .group 1 "first"
    .group 2 "second"
