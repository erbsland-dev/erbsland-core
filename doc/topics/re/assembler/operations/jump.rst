****
Jump
****

The :expression:`Jump` operation sets the *program counter* to a new value. This changes the execution flow without consuming
any characters from the input stream.

.. code-block::

    ; Jump example

           JUMP %end
    end:   MATCH

.. rubric:: Syntax

.. code-block:: text

    JUMP <pc>
