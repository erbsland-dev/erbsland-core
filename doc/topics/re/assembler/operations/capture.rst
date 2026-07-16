*******
Capture
*******

The :expression:`Capture` operation starts or stops a capture group at the current position. It is a flow operation.

You must prefix the command with either the modifier :expression:`Start` or :expression:`Stop` to select if the capturing shall start or stop.

.. code-block:: text

    ; Capture example: capture one character
    .groups 1

    START CAPTURE 1
    ANY
    STOP CAPTURE 1
    MATCH

.. rubric:: Syntax

.. code-block:: text

    START CAPTURE <capture group>
    STOP CAPTURE <capture group>
