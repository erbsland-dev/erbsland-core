
*************************
Reference Page Guidelines
*************************

.. note::

    The ``pre_commit`` or ``reference_doc`` utilities create and update only pages explicitly configured in
    ``utilities/conf/reference_doc.elcl``. They manage the final ``Interface`` section of those pages.

File Structure
==============

1.  The configuration `utilities/conf/reference_doc.elcl` assigns public headers to reference groups. Each group owns
    one page and can select headers by exact source-relative path or ``Header Globs``. A selected header must belong to
    exactly one group.

2.  The ``update_doc`` utility (and ``pre_commit``) automatically create and update reference documentation pages.

    -   Only the framework with "Interface" is created/updated.
    -   Pages no longer owned by a group are removed. Register prose-only pages in ``Manual Reference Pages`` before
        running the tool.
    -   Headers that are not assigned to a reference group are reported on every run and receive no fallback page.
    -   If a header is reported, either assign it to a group, explicitly exclude it, or document it manually.

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
5.  **Interface:** Mandatory *last* section ``Interface``, level ``===``, contains only generated
    ``.. doxygenXXX:: symbol`` entries. Do not edit it manually.

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
