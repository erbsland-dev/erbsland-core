
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

.. _topic-page-practical-separation:

Separate Topic Text from Practical Discussion
=============================================

Regular topic text must explain concepts, API behavior, requirements, and generally applicable design choices
independently of any demo.
It must remain coherent and sufficient to understand the topic when readers skip every practical block.
Neutral text can still address the reader directly, explain practical consequences, and use short generic expressions or
fragments that require no knowledge of a particular example, such as ``app.loop()``.

Every demo walkthrough or discussion of a concrete example application must appear inside an
``erbsland-in-practice-begin/end`` block.
Include the example's introduction, source excerpts, commands, output, interpretation, and discussion of its design
choices within that boundary.
This also applies to practical walkthroughs without a compiled demo block.
See :doc:`demo_integration` for the directive syntax and placement rules.

Explain the general concept in the regular text before showing its application in a practical block.
Keep demo names, helper identifiers, and references such as "our monitor" inside the practical discussion.
After the block, continue the conceptual explanation without relying on the demo's state or execution history.
General requirements, including ownership and execution rules, must be explained in the regular text; repeat them inside
a practical block when needed to explain the example safely.

Section headings must describe concepts, API operations, or reader tasks independently of the demo.
Use the practical block's label to identify the concrete example or the part of it being discussed.
A section needs a practical block only when it contains such a discussion.

Complete demo applications have their own walkthroughs under ``doc/demo_apps``; see :doc:`demo_app_page`.
A topic's practical block can link to that walkthrough and keep its commentary focused on how the selected code
illustrates the current concept.
Do not repeat the application's entire architecture in each topic.
The regular topic text must still teach the complete concept without following the walkthrough link.

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
* Support API usage with compiled demos.
  A practical block can explain an earlier demo or show a focused excerpt instead of introducing another complete
  program.
  Creating these demos before writing the content can help to find more questions a developer will have.
* Next, write the sections as described in "Page Structure".
* After writing the topic page, review it, unbiased and don't be shy to remove/rewrite content if you
  think it does not match our guidelines.

Teach Concepts Through Working Examples
=======================================

Before drafting, write down the practical question this page answers and what the reader should be able to do after
reading it.
Choose an example that demonstrates that outcome, and decide which related questions belong on other pages.
The example supports the explanation; its execution order does not have to determine the page's structure.
A topic page does not need to cover every API involved in its example.

Introduce the underlying concept before walking through the demo
----------------------------------------------------------------

Explain in the regular text what the relevant API does, why the reader would choose it, and which ownership or execution
rules apply.
Then use focused excerpts inside a practical block to make those rules concrete.
Connect the example back to the reader's own code: distinguish API requirements from choices made for that demo, and
explain how common alternatives change the design.
This is a teaching progression, not a mandatory set of headings for every section.

Introduce the purpose of each operation before its architectural classification
-------------------------------------------------------------------------------

For example, explain that a method queues work on a loop before introducing the target interface and its type aliases.
Within a practical block, use concrete object names consistently and relate them to the concepts introduced in the
regular text.

Explain the normal execution path before discussing variations
--------------------------------------------------------------

When work is deferred, show what runs now, what runs later, and what keeps the required state alive between them.
Place prerequisites beside the first code that depends on them: a required execution context or stored owner should
never be a surprise several sections later.
Keep rules necessary for safe use of the shown example there, even when more advanced lifetime details belong elsewhere.

Give each example one teaching purpose
--------------------------------------

Do not add unrelated accessors, scheduling, output, or framework features merely because they are available.
Use a consistent scenario across practical blocks where it helps readers connect the concepts.
Do not turn the page into a commentary on every function in the demo.
If a variation requires a second framework and several new concepts, consider a separate page or a link to an existing
example instead.

Keep declarations that explain ownership and execution visible
--------------------------------------------------------------

Includes and unrelated scaffolding may be omitted, but hiding the stored pointer or state that makes an example work
prevents readers from understanding it.
Make clear when an example is an excerpt; only call it complete when the displayed code supports that claim.
A demo block already lets readers expand its referenced source file, so avoid separate full listings or directions whose
only purpose is to show the rest of that file.
There is no need to repeatedly describe omitted boilerplate; see :doc:`demo_integration` for choosing and presenting
excerpts.

Explain details in proportion to their role in the task
-------------------------------------------------------

After the working pattern is established, cover common variations and the relevant error paths.
Put exhaustive contracts, uncommon exceptions, and implementation details in the appropriate reference or advanced page.
Keep the learning path within topic pages: link to the relevant advanced topic when readers need a deeper explanation.
API symbol links still provide access to individual reference entries.
Avoid qualifying every sentence with exceptions from other APIs.

Section Titles That Help Readers Navigate
=========================================

A section title should identify its subject even when read without the surrounding text.
Readers scan headings to find a concept, API operation, or problem; name that subject directly.
For example, "Callback Execution and Responsiveness" is more useful than "Give Each Callback Time to Finish", and
"Result Ownership and Synchronization" is clearer than "Decide Who Owns the Work and Results".

Both descriptive titles and titles with verbs can work.
"Cancelling a Timer" names a concrete operation; "Choose Whether You Need a Handle" leaves the reader guessing which
handle and why it matters.
Avoid metaphors, generic advice, and a repeated imperative formula.

Read the headings alone and check that they describe the content and fulfill the promise of the page title.
The tree of headings, with the page title at its root should allow the reader to navigate to the section of a page their
are interested in.

Review the Reader's Path
========================

Review the displayed page, including the code selected by demo directives, rather than only the source demo.
Before considering the page finished, check:

* Can the reader identify the task and the expected result before learning several unfamiliar type names?
* Can they find a particular subject by scanning the headings alone?
* Can they explain how to use the concepts and APIs in their own code, beyond reproducing the demo?
* Does the regular text remain coherent and explain the necessary concepts and requirements when all practical blocks
  are skipped?
* Are demo identifiers, scenario references, and the complete discussion of each example inside practical blocks?
* Are general ownership, execution, and safety requirements explained in the regular text rather than only in examples?
* Can they follow the first example's execution and locate the state that must survive between calls?
* Is each non-obvious prerequisite explained when it first matters?
* Does every substantial part of an example help answer its section's question?
* Do later sections build on established concepts, with practical blocks reusing a familiar example where useful?
* Can a detail be moved to an existing page without making the demonstrated pattern unsafe or misleading?

When the answer exposes a structural problem, revise the example or section order first.
Adding more explanatory paragraphs to an overloaded example usually increases the reader's work.

When adapting an existing page, review the prose as well as the code blocks.
Move the complete demo discussion inside the practical boundary and ensure that general explanations remain in the
regular text.
Wrapping only the code is not sufficient.

Recommended Writing Style
=========================

Additionally to the writing style guidelines, topic pages shall follow these writing style rules:

* Write in the style of a high-quality technical introduction aimed at intelligent readers who may be new to the topic.
* Use clear, direct, and approachable language that is also well understood by non-native English speakers.
* Introduce concepts from the reader's perspective by explaining why they matter before discussing how they work.
* Build ideas incrementally, using concrete examples and realistic scenarios rather than abstract definitions.
* Maintain a conversational but professional tone, avoiding marketing language, unnecessary enthusiasm,
  and excessive jargon.
* Prefer explanation over formal specification, and favour readability over brevity.
* Assume the reader is curious and technically capable, but not yet familiar with the subject.

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

See :doc:`demo_integration` for synchronized source excerpts, selectors, captured output, and full-source views.
