*******
Minimum
*******

The :expression:`MINIMUM` operation is a flow helper that enforces a minimum count for a counter-based repetition.
It is typically used together with :expression:`MAXIMUM` and branching logic.

The first argument is the counter index, the second argument is the minimum value.

.. code-block:: text

    ; Minimum example (schematic)

                COUNTER 0, 0
    loop:       CHAR 'a'
                MAXIMUM 0, 10
                SPLIT %loop, %end
    end:        MINIMUM 0, 3
                SUCCESS

.. rubric:: Syntax

.. code-block:: text

    MINIMUM <counter index>, <min value>
