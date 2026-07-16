*******
Success
*******

The :expression:`Success` operation marks a successful match *and* the immediate end of the program. When the engine processes
this operation, it will process all current threads and following flow commands. After that the engine will stop
exploring further paths.

This operation exists to optimize non-greedy matches at the very end of a pattern.

.. code-block:: text

    ; Success example

    SPLIT       %path1, %path2
    path1:      CHAR 'a'
                SUCCESS
    path2:      CHAR 'a'
                CHAR 'b'
                MATCH

In the program above, for the text ``abc``, ``path2`` is not explored to it's end, as the program stops after matching the first ``a`` in the text.

.. rubric:: Syntax

.. code-block:: text

    SUCCESS
