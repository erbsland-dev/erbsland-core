*********
Registers
*********

The engine and each thread in the engine has a set of registers.

.. rubric:: Engine Registers

.. list-table::
    :width: 100%

    *   -   *Current Character*
        -   This is the current character from the input stream that is processed by the engine.
    *   -   *Next Character*
        -   This is the next character in the input stream.

.. rubric:: Per-Thread Registers

.. list-table::
    :width: 100%

    *   -   *Program Counter*
        -   A program counter pointing to the currently executed
    *   -   *Sequence Index*
        -   The sequence index is exclusively used by the :expression:`Sequence` operation.
    *   -   *Counter Array*
        -   A set of counters that are used by the :expression:`Counter`, :expression:`Maximum` and :expression:`Minimum` operations.
    *   -   *Capture Groups*
        -   A set of capture groups to mark the start and end of captured text.
