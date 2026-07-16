
********
Overview
********

Flow and Character Operations
=============================

Operations are split into two different groups: Flow operations and character operations. Flow operations do not consume any characters from the input stream, they are executed immediately. On the other hand, character operations require a character from the input stream.

.. list-table::
    :width: 100%
    :widths: 50, 50
    :header-rows: 1

    *   -   Flow Operations
        -   Character Operations
    *   -   :doc:`None<none>`
        -   :doc:`Char<char>`
    *   -   :doc:`Split<split>`
        -   :doc:`Sequence<sequence>`
    *   -   :doc:`Jump<jump>`
        -   :doc:`Category<category>`
    *   -   :doc:`Match<match>`
        -   :doc:`Class<class>`
    *   -   :doc:`Success<success>`
        -   :doc:`Any<any>`
    *   -   :doc:`Failure<failure>`
        -
    *   -   :doc:`Anchor<anchor>`
        -
    *   -   :doc:`Capture<capture>`
        -
    *   -   :doc:`Counter<counter>`
        -
    *   -   :doc:`Maximum<maximum>`
        -
    *   -   :doc:`Minimum<minimum>`
        -

Modifier
========

Modifiers change the behaviour of an operation.

:expression:`Not`
    Negates the meaning of the operation.

:expression:`CI`
    Uses a case-folding comparison.

:expression:`Start`
    Selects the start behaviour (e.g. start capturing).

:expression:`Stop`
    Selects the stop behaviour (e.g. stop capturing).

:expression:`Assert`
    Marks a zero-width operation that only tests the *current* character, without consuming it.

The following table shows all operations and valid modifiers.

.. list-table::
    :class: flags-table
    :width: 100%
    :widths: 75, 5, 5, 5, 5, 5
    :header-rows: 1

    *   -   Operation
        -   Not
        -   CI
        -   Assert
        -   Start
        -   Stop
    *   -   :doc:`None<none>`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Split<split>`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Jump<jump>`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Match<match>`
        -   :fas:`check;sd-text-success`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Success<success>`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Failure<failure>`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Anchor<anchor>`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Capture<capture>`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`check;sd-text-success`
        -   :fas:`check;sd-text-success`
    *   -   :doc:`Counter<counter>`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Maximum<maximum>`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Minimum<minimum>`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Char<char>`
        -   :fas:`check;sd-text-success`
        -   :fas:`check;sd-text-success`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Sequence<sequence>`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`check;sd-text-success`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Category<category>`
        -   :fas:`check;sd-text-success`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`check;sd-text-success`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Class<class>`
        -   :fas:`check;sd-text-success`
        -   :fas:`check;sd-text-success`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
    *   -   :doc:`Any<any>`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
        -   :fas:`xmark;sd-text-danger`
