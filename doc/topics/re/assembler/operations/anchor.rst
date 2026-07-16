******
Anchor
******

The :expression:`Anchor` operation performs a zero-width assertion at the current position. It does not consume a character.
If the anchor condition fails, the current thread is stopped.

Anchors are selected by an identifier (prefixed with ``&``). Anchor identifiers are case-insensitive.

.. code-block:: text

    ; Anchor example: match at start of input

    ANCHOR &Start
    CHAR 'a'
    SUCCESS

.. rubric:: Syntax

.. code-block:: text

    ANCHOR &<anchor>
