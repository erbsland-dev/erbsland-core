*******
Counter
*******

The :expression:`Counter` operation writes a value into a counter register. Counters are used by repetition helpers and by
operations that need a small amount of per-thread state.

The first argument is the counter index, the second argument is the value.

.. code-block:: text

    ; Counter example

    COUNTER 0, 3
    ; ... code using counter 0

.. rubric:: Syntax

.. code-block:: text

    COUNTER <counter index>, <value>
