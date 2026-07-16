********
Sequence
********

The :expression:`SEQUENCE` operation consumes a sequence of characters from the input stream and compares them against a stored
sequence from the sequence data section.

The first argument is the sequence offset/index in the sequence data section, the second argument is the length.

.. code-block:: text

    ; Sequence example: match the sequence at offset 10 with length 3

    SEQUENCE 10, 3
    MATCH

.. rubric:: Syntax

If you combine the operation with the :expression:`CI` modifier, the characters in the sequence are compared using case-folding. For a case-folded comparison, the characters in the sequence must be case-folded.

.. code-block:: text

    SEQUENCE <sequence index>, <length>
    CI SEQUENCE <sequence index>, <length>
