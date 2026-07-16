******************
Fundamental Syntax
******************

The assembler language is line based. Additional spacing around expressions is ignored. Also, empty lines and lines that only contain a comment are ignored as well.

Example Listing
===============

Here a short example, illustrating the syntax.

.. code-block:: text

    ;
    ; Example Program
    ;

    start:       CHAR 'a'             ; match the char 'a'
                 SPLIT %start, %end   ; greedy
    end:         SUCCESS              ; match

Language Elements
=================

.. list-table::
    :class: expressions
    :header-rows: 1
    :width: 100%
    :widths: 25, 25, 50

    *   -   Syntax
        -   Name
        -   Description
    *   -   :expression:`; comment`
        -   Comment
        -   A comment starts with a semicolon character (``;``). Any text after the semicolon until the end of the line is ignored.
    *   -   :expression:`label:`
        -   Label Target
        -   A label target must be the first element in a line. Line labels are case-insensitive, and must only consist of letters, digits and underscores. The maximum length is 16 characters. Line labels must be unique for the whole assembly code. Targets can be set inside program, character sequence or character class blocks and referenced from the corresponding operations.
    *   -   :expression:`%label`
        -   Label Reference
        -   A label reference always starts with the percentage character (``%``). It references a label target with the same name.
    *   -   :expression:`123`
        -   Integer
        -   Any sequence of digits is interpreted as integer value. It can be used anywhere an integer value is accepted.
    *   -   :expression:`$12af`
        -   Offset
        -   An offset starts with a dollar sign (``$``) and is followed by up to 8 hexadecimal digits. Technically there is no difference between an integer and a hexadecimal offset. They can be used interchangeably.
    *   -   :expression:`"text"`
        -   Text
        -   Text is enclosed in two double quotes (``"``). All safe Unicode characters can be used in a text. Also, text supports the escape sequences ``\\"``, ``\\'``, ``\\\\``, ``\\n``, ``\\t`` and ``\\r``. Text is primarily used in data segments.
    *   -   :expression:`'c'`
        -   Character
        -   A single character is enclosed in single quotes (``'``). All safe Unicode characters can be used for the character. The following escape sequences are supported: ``\\"``, ``\\'``, ``\\\\``, ``\\n``, ``\\t`` and ``\\r``. Characters can be used in data segments, or as argument for the ``CHAR`` operation.
    *   -   :expression:`&identifier`
        -   Identifier
        -   An identifier starts with the ``&`` character. It can have up to 16 letters, digits and underscores. Identifiers are used to select character categories, anchor types and data segments. Identifiers are always case-insensitive.
    *   -   :expression:`.command`
        -   Assembler Command
        -   Assembler commands are used to switch data section or write data. About about them in a later section.
    *   -   |   :expression:`Not`
            |   :expression:`CI`
            |   :expression:`Start`
            |   :expression:`Stop`
            |   :expression:`Assert`
        -   Modifiers
        -   Modifiers change the following operation. All these modifiers are case-insensitive.
    *   -   |   :expression:`None`
            |   :expression:`Jump`
            |   :expression:`Match`
            |   :expression:`NoMatch`
            |   :expression:`Split`
            |   :expression:`Anchor`
            |   :expression:`Capture`
            |   :expression:`Counter`
            |   :expression:`Maximum`
            |   :expression:`Minimum`
            |   :expression:`Success`
            |   :expression:`Failure`
            |   :expression:`Char`
            |   :expression:`Sequence`
            |   :expression:`Category`
            |   :expression:`Class`
            |   :expression:`Any`
        -   Operation
        -   Each operation writes an instruction into the program. One or two arguments may follow a operation. Operations are case-insensitive and usually written with uppercase letter to stand out.
