****
Char
****

The :expression:`CHAR` operation consumes one character from the input stream and compares it with the given character.
If it matches, the thread continues. Otherwise the thread stops.

The argument can be given as a decimal integer code point or as a quoted character.

.. code-block:: text

    ; Char example

    CHAR 'a'
    MATCH

.. rubric:: Syntax

If you prefix the operation with the :expression:`Not` modifier, it negates it's meaning. For example ``NOT CHAR 'a'`` does match all characters, *except* the character ``a``. You can also combine the operation with the :expression:`CI` modifier to use a case-folding comparison. For a case-folded comparison, the character must be case-folded.

.. code-block:: text

    CHAR <unicode code point | 'c'>
    NOT CHAR <unicode code point | 'c'>
    CI CHAR <unicode code point | 'c'>
    NOT CI CHAR <unicode code point | 'c'>
