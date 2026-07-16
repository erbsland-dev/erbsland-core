********
Category
********

The :expression:`Category` operation consumes one character and tests it against a Unicode (or RegExp-specific) category.
Depending on the first argument the result is used normally or negated.

.. code-block:: text

    ; Category example: match an uppercase letter

    CATEGORY &Lu
    MATCH

.. rubric:: Syntax

If you prefix the operation with the :expression:`Not` modifier, it negates it's meaning. If you prefix the operation with the :expression:`Assert` modifier, the operation is testing the *current* character *without* consuming it.

.. code-block:: text

    CATEGORY &<category>
    NOT CATEGORY &<category>
    ASSERT CATEGORY &<category>
    NOT ASSERT CATEGORY &<category>


