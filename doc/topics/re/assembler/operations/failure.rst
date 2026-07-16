*******
Failure
*******

The :expression:`Failure` operation marks no match *and* the immediate end of the program. When the engine processes this
operation, it will process all remaining current threads and following flow commands. After this, the engine will
stop exploring further paths.

.. code-block:: text

    ; Failure example: choose between matching 'a' or failing

    SPLIT       %noA, %other
    noA:        CHAR 'a'
                FAILURE
    other:      ANY
                ANY
                MATCH

For the text ``abc``, the program above will stop with no match, as processing stops after encountering the first
``a`` that leads to an error.

.. rubric:: Syntax

.. code-block:: text

    FAILURE
