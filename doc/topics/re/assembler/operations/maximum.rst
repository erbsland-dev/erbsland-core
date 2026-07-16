*******
Maximum
*******

The :expression:`MAXIMUM` operation is a flow helper that either uses a counter to limit how often a path may be taken,
or repeats a section of the program, until the counter reached the maximum.

Conceptually it checks the counter against a maximum value and updates it as part of a repetition scheme.

Assert Variant
==============

In the assert variant, the first argument is the counter index, the second argument is the maximum value.
If at the time the operation is encountered the counter is equal or greater than the given maximum value, the
current thread ends with no match.

.. code-block:: text

    ; Example: repeat 1 to 3 times.

                COUNTER 0, 0
    loop:       CHAR 'a'
                MAXIMUM 0, 3
                SPLIT %loop, %end
    end:        MATCH

.. rubric:: Syntax

.. code-block:: text

    MAXIMUM <counter index>, <max value>

Skip Variant
============

In the jump variant, the first argument is the counter index, the second argument is the maximum value and the third
argument is the jump location.
If at the time the operation is encountered the counter is *less than* the given maximum value, the counter is
increased and the program counter set to the given jump location.
If the counter is *equal or greater* then the given maximum value, *the counter is set to zero* and the program
continues with the next operation.

.. code-block:: text

    ; Example: repeat 'a' exactly 8 times.

                COUNTER 0, 0
    loop:       CHAR 'a'
                SKIP MAXIMUM 0, 8   ; skip the jump if 8 repetitions are reached.
                JUMP %loop
    end:        MATCH

.. rubric:: Syntax

.. code-block:: text

    MAXIMUM <counter index>, <max value>, <jump target>
