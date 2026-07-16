*****
Match
*****

The :expression:`Match` operation marks a potential match for the pattern. The engine continues exploring the input until all
threads end. This operation is usually used for greedy patterns.

.. code-block:: text

    ; Match example

    SPLIT       %path1, %path2
    path1:      CHAR 'a'
                MATCH
    path2:      CHAR 'a'
                CHAR 'b'
                MATCH

For the text ``abc`` the program above will match ``a`` in the text, yet the engine will continue exploring ``path2``. As split prioritizes the first branch, only the match from ``path1`` will be kept.

.. rubric:: Syntax

If you combine the :expression:`Match` operation with the :expression:`Not` modifier, it will negate its meaning. As soon the engine encounters a :expression:`NOT MATCH` operation, this thread immediately ends with no match.

.. code-block:: text

    MATCH
    NOT MATCH
