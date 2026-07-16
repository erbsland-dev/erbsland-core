*****
Class
*****

The :expression:`Class` operation consumes one character and tests it against a character class from the class data section.
The argument is the index (decimal integer or hexadecimal offset) at which position the class definition is stored.

.. code-block:: text

    ; Class example: match character class at index 0

    CLASS 0
    MATCH

.. rubric:: Syntax

If you prefix the operation with the :expression:`Not` modifier, it negates it's meaning.
If you combine the operation with the :expression:`CI` modifier, the characters in the sequence are compared using case-folding. For a case-folded comparison, the characters in the class must be case-folded.

.. code-block:: text

    CLASS <class index>
    NOT CLASS <class index>
    CI CLASS <class index>
    NOT CI CLASS <class index>
