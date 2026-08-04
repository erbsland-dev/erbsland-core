
.. index::
    single: Guidelines; Topic Page
    single: Topic Page

*********************
Topic Page Guidelines
*********************

Topic pages are the primary learning resource for this library.
While reference pages focus on the technical details of individual APIs, topic pages explain how frameworks, concepts,
or groups of classes work together.

What are Good Topic Pages?
==========================

A topic page always **answers a user question** that starts with "How do I ...?".
This is not meant literally, but conceptually how to think about topic pages.
Example: A question could be "How do I access characters in a string?" and "How do I write a parser for strings?".
It leads to a topic page called "Character Access and Parsing of Strings".
This page answers both questions in detail.

A good topic page helps the reader understand a feature from a practical and conceptual point of view.
It does not describe APIs class by class, function by function.
It answers user questions and explains **how you can solve a problem**.

A topic page goes even further, it demonstrates **how the API should be used** and it also teaches developers best
practices and how to use a feature effectively.

File Structure
==============

.. code-block::

    +-- doc
        +-- topics
            +-- <domain>  // directory with the domain name, usually equals the namespace
                +-- index.rst  // entry page, short domain intro + TOC
                +-- overview.rst  // overview page; one section per topic; brief summary linking to the topic
                +-- <topic>.rst  // one topic from this domain

Page Structure
==============

Each topic page should follow a consistent structure.
This helps readers quickly understand where to find information and improves the overall readability of the
documentation.

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
   * Continue with practical usage patterns, but **avoid** a list like "Use x ..., use y ...".
   * Explain details and edge cases later.
   * Write and explain naturally, address the reader personally und try your best to show what problems an API
     (class, enum, or method) solves, and how to use it best. It shall inspire the reader, so they would like
     to use that API in their own application they write.

   In most cases, no final summary section is required because the introduction already provides an overview.

Recommended Workflow to Create the Content
==========================================

* Start by creating a list of questions a developer will have about the topic and/or API.
  Here, just a few examples, but remember, each topic will have different questions.

  * How do I use the API for <use-case>?
  * What is <function> for?
  * How do I handle errors?
  * ...

* Next, create a logical sequence of sections that answer these questions.
  These sections don't need to have the questions in its titles, they just need to answer them and
  provide a title a reader immediately understands that this section will answer their question.
* Each section that describes how to use an API must have a demo that demonstrates what it describes.
  Creating these demos before writing the content can help to find more questions a developer will have.
* Next, write the sections as described in "Page Structure".
* After writing the topic page, review it, unbiased and don't be shy to remove/rewrite content if you
  think it does not match our guidelines.

Anti-Patterns
=============

* **Don't** write lists of usage instructions. If you have a list of e.g. "Use x ...", "Use y ..." in a
  section, this no longer a topic page but usage instructions.
* **Don't** write about internals and implementation. This is just noise, for several reasons:
  A developer should not need to know it to use the API - otherwise it is bad API design.
  Internals and implementation may change.
* **Don't** write about what's missing. This is just distracting.
* **Don't** write what was changed or what is new. A topic page always covers the present situation and
  is by no means a changelog. A developer that reads the page for the first time may be confused about
  references to old functionality they never knew.
* **Don't** just insert/change a sentence because of an API change without reviewing the whole document.
  Even a small change may require rewriting a section/demo etc. to keep the topic page consistent.

Related Guidelines
==================

*   Read :doc:`rst_style` how to correctly format a documentation page, what header levels to use and how to
    correctly create links to code symbols.
*   Read :doc:`writing_style` about the tone, vocabulary and style how our documentation shall be written.
*   Read :doc:`demo` how to write demos and integrate them into documentation pages.
*   Read :doc:`examples` for guidelines how to write example and demos code.
