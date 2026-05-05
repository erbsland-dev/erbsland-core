
*************************
Reference Page Guidelines
*************************

.. note::

    The ``pre_commit`` or ``reference_doc`` utilities automatically create missing reference pages from the
    file structure in ``src``. This scripts also automatically manage the "Interface" section.
    After writing new API, run one of these utilities to create the documentation skeleton

File Structure
==============

1.  The default file structure follows the structure in the ``src`` directory:

    -   Each module in ``src`` is documented in a reference page.
    -   The configuration `utilities/conf/reference_doc.elcl` controls how the header files are
        grouped in reference pages.

2.  The ``update_doc`` utility (and ``pre_commit``) automatically create and update reference documentation pages.

    -   Only the framework with "Interface" is created/updated.
    -   Obsolete pages are marked in the "Interface" section.
    -   Headers that are not yet assigned to a reference group keep their default page and are reported as
        uncategorized.
    -   If a uncategorized page is reported, edit `utilities/conf/reference_doc.elcl` and assign it to either
        an existing or new reference page.

Page Structure
==============

Each reference page follows this structure.

1.  **Index entries:** One or more index entries ``.. index::`` to quickly find the right type via search.
2.  **Page Title:** The page title using level double-asterisk ``***\nAbc\n***``.
    The page title describes the type naturally and can derive from a class name.
3.  **Introduction:** If the topic needs an introduction ``Introduction`` using level ``===```.
4.  **Usage:** As many sections (level ``===``) necessary to describe the individual types, methods, enums on this
    reference page as a short technical overview.
    For a class, its enough to explain what this class is for and give a summary of its API.
    On the reference page we don't go into details of individual methods - these details are on topic pages.
    The goal of each section is to give the user a quick summary, so they can find a type or method.
    If a topic exists for a given type, enum or method, the reference page shall reference it.
5.  **Interface:** Mandatory *last* section ``Interface``, level ``===``, contains only ``.. doxygenXXX:: symbol``
    entries for the types of the documented file.

    Example:

    .. code-block:: text

       .. doxygenclass:: erbsland::math::SaturatingInteger
           :members:

       .. doxygentypedef:: erbsland::math::SatInt8
       .. doxygentypedef:: erbsland::math::SatInt16

Related Guidelines
==================

*   Read :doc:`rst_style` how to correctly format a documentation page, what header levels to use and how to
    correctly create links to code symbols.
*   Read :doc:`writing_style` about the tone, vocabulary and style how our documentation shall be written.
*   Read :doc:`examples` for guidelines how to write example and demos code.
*   Read :doc:`demo` how to write demos and integrate them into documentation pages.
