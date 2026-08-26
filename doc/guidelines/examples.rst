
*****************************************
Guidelines for Writing Examples and Demos
*****************************************

Examples and demos are an essential part of the documentation.
They help readers understand not only *what* an API does, but also *how* to use it effectively in real-world situations.

Whenever you explain an API, feature, or workflow, include practical examples from the perspective of a library user.
Well-written examples reduce the learning curve, communicate best practices, and make the documentation more engaging.

Good examples should be:

* concise,
* realistic,
* easy to understand,
* focused on a single concept,
* immediately useful to the reader.

Avoid overly artificial examples that only demonstrate syntax without meaningful context.

Code Style
==========

All example code must follow the Erbsland code style.

Whenever possible, format code automatically using the ``pre_commit`` utility to ensure a consistent appearance across
the documentation.

In General
----------

* Keep formatting clean and readable.
* Prefer modern C++20 style.
* Avoid unnecessary boilerplate.
* Keep examples compact without hiding important details.

For Demos
---------

* Add an API documentation block (``///``) that describes the API, concept, or class being demonstrated.
* Everything **before the first** ``///`` line gets cut-off in the documentation.
  Constants, functions, declarations that only add noise can be put in front to hide them.
* Everything **after** the ``namespace demo { ... }`` block gets cut-off in the documentation.
* This documentation is reused in the generated documentation and should be useful when read independently from
  the surrounding text.
* The text in demo documentation blocks often intentionally duplicates information that may already be visible
  in the code.
  This redundancy improves search, embedding quality, and the discoverability of APIs through natural-language
  queries.
* Prefer explaining the purpose and capabilities of the API over describing the individual demo steps.
* The documentation block also serves as semantic context for documentation search and embedding-based
  knowledge retrieval.
* Do not shorten the documentation block merely to match the size of the demo.
  A concise API description is usually more valuable than a minimal description of the example itself.
* Add short comments (``//``), what is done in the next block of code,
* to semantically link it with the following API use for embeddings to be bound to the code,
* and as users reading the documentation may be unfamiliar with the API.

API Usage
=========

Always write examples from the viewpoint of a library user.
Always write read-world examples a user might find useful for their own code.

Readers should immediately understand how they would use the API in their own code.
Avoid implementation-focused examples unless the topic explicitly requires them.

Guidelines:

* Use the short namespace alias ``el`` instead of ``erbsland``.
* Collapse domain namespaces whenever possible:

  .. code-block:: cpp

      el::text::String  ->  el::String

* Prefer concise and realistic examples.

  Instead of:

  .. code-block:: cpp

      auto text = erbsland::text::String{std::string_view{"Hello"}};

  Write:

  .. code-block:: cpp

      auto text = "Hello"_el;

* Prefer the common string aliases and literals provided by the library:

  * ``String`` is an owning, read-only string for most purposes.
  * ``""_el`` - everywhere for string literals
  * ``StringFormat`` - for formatting strings (or use ``el::io::print``)
  * ``StringEditor`` - for examples that explicitly demonstrate local in-place editing or small construction tasks.

* Avoid unnecessary usage of standard library string types in examples unless they are directly relevant to the topic.

Language and Themes in Example Code
===================================

* Non-english natural language must only be used in data, values or input an example/demo processes.
  It is a good method to separate user-input from application logic.
* Error messages, diagnostic output, and any output that explains the behavior of the example/demo must be English.
* Only if you choose a theme and language manually, see :doc:`example_themes` for a comprehensive list.

Unicode and Emojis
==================

The library provides excellent Unicode support.
Do not hesitate to use Unicode characters and emojis in example code when they improve readability or make examples more
engaging.

Well-placed Unicode text often makes examples feel more realistic and memorable.
