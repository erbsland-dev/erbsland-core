
*******
Section
*******

The assembler uses different *sections* for different kinds of data:

- The *program* section contains operations (instructions).
- The *sequence* section contains character sequences referenced by ``SEQUENCE`` operations.
- The *class* section contains character classes referenced by ``CLASS`` operations.

Use the ``.section`` command to switch between these sections.

Syntax
======

:expression:`.section &program`
    Switch to the program section.

:expression:`.section &sequence`
    Switch to the sequence section.

:expression:`.section &class`
    Switch to the class section.

Arguments
=========

The ``.section`` command expects exactly one argument:

``&identifier``
    The section name. Valid values are ``&program``, ``&sequence`` and ``&class``.

Notes
=====

- Section identifiers are case-insensitive.
- If a character class definition is currently open, switching sections will close it.
- Any additional token after the section identifier causes an error.

Examples
========

.. code-block:: text

    .section &program
    start:  CHAR 'a'
            MATCH

    .section &sequence
    myText: .data "hello"

