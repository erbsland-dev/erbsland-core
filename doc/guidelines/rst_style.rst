
**********************
ReStructuredText Style
**********************

* Keep lines within 120 columns and hard-wrap text at this width.
* Add a new-line after each sentence to improve diffs.
* Prefer explicit and clean structure over compact formatting.
* Use ``code`` formatting consistently for all inline code, symbols, types, and filenames.

Header Style
============

Use the following hierarchy for page headers.

.. code-block:: rst

    **********
    Page Title
    **********

    Section
    =======

    Subsection
    ----------

    Sub-Subsection
    ~~~~~~~~~~~~~~

Avoid skipping header levels because this makes pages harder to navigate.

Linking Symbols
===============

Always link symbols that are described in the text.

Show the short symbol name to keep the text readable, but always link using the fully qualified namespace.

.. code-block:: rst

    :cpp:any:`Class <erbsland::abc::Class>`

This approach keeps the text compact while ensuring links remain unambiguous and stable.
