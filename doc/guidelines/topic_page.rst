
.. index::
    single: Guidelines; Topic Page
    single: Topic Page

*********************
Topic Page Guidelines
*********************

Topic pages are the primary learning resource for this library.
While reference pages focus on the technical details of individual APIs, topic pages explain how frameworks, concepts,
or groups of classes work together.

A good topic page helps the reader understand a feature from a practical and conceptual point of view. Instead of
describing APIs class by class, it explains how you can solve a problem, structure your code, or use a feature
effectively.

Page Structure
==============

Each topic page should follow a consistent structure. This helps readers quickly understand where to find information
and improves the overall readability of the documentation.

#. **Index Entries:** Add one or more ``.. index::`` entries so readers can quickly locate the page through topic
   searches.

#. **Page Title:** Use a page title with the top-level ``***`` header style.

   The title should clearly and precisely describe the topic of the page.

#. **Introduction:** Add a short introduction directly after the title when the topic benefits from context.

   The introduction should summarize:

   * what the page explains,
   * which problems it solves,
   * and what the reader will learn.

   Example:

   .. code-block:: text

       This page gives you a compact overview of all string types in this library.
       You will learn how the different string classes relate to each other and
       when to use each type in your own code.

#. **Content:** Add as many sections and subsections as required.

   Organize the content in a logical order:

   * Start with the most important concepts first.
   * Continue with practical usage patterns.
   * Explain details and edge cases later.

   In most cases, no final summary section is required because the introduction already provides an overview.

Related Guidelines
==================

*   Read :doc:`rst_style` how to correctly format a documentation page, what header levels to use and how to
    correctly create links to code symbols.
*   Read :doc:`writing_style` about the tone, vocabulary and style how our documentation shall be written.
*   Read :doc:`demo` how to write demos and integrate them into documentation pages.
*   Read :doc:`examples` for guidelines how to write example and demos code.
