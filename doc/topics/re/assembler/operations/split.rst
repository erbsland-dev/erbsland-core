*****
Split
*****

The :expression:`Split` operation creates two execution branches (two threads). Both branches continue with different
*program counters*. This is the fundamental operation to express alternations and repetition.

The first branch has priority over the second branch. If two branches in the program merge, the state of the branch with a higher priority will overwrite the state of the branch with lower priority.

.. code-block:: text

    ; Split example: greedy loop (prefer branch A first)

    start:      CHAR 'a'
                SPLIT %start, %end
    end:        MATCH

.. rubric:: Syntax

.. code-block:: text

    SPLIT <pc a>, <pc b>
