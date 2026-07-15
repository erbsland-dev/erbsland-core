
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
* Yet, avoid too much noise, explain blocks not lines and do it in a brief way.

API Usage
=========

Always write examples from the viewpoint of a library user.

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

  * ``StringView`` is an owning, read-only string for most purposes.
  * ``""_el`` - everywhere for string literals
  * ``StringFormat`` - for formatting strings (or use ``el::io::print``)
  * ``String`` - when building strings from scratch.

* Avoid unnecessary usage of standard library string types in examples unless they are directly relevant to the topic.

Theme Selection
===============

When creating demonstration content, prefer themes that reflect nature, science, learning, discovery, creativity, and
exploration.

Suitable themes include:

* Nature and the environment
* Weather and climate
* Landscapes, mountains, rivers, lakes, and oceans
* Plants, forests, trees, flowers, and ecosystems
* Animals, birds, fish, insects, and wildlife
* Astronomy and space
* Physics, chemistry, biology, and mathematics
* Puzzles, games, logic problems, and educational challenges
* Fantasy worlds, creatures, and adventures
* Music, instruments, sound, and composition

Thematic examples should be positive, educational, timeless, and broadly accessible.

Avoid themes centered around:

* Money, finance, banking, investing, or cryptocurrencies
* Businesses, companies, brands, marketing, or sales
* Social media, influencers, celebrities, or advertising
* Human resources, employees, customers, or organizational structures
* Food, restaurants, recipes, or shopping
* Cars, traffic, logistics, or transportation systems
* Politics, religion, or controversial social topics

When selecting a theme, choose from the available theme categories in a balanced and varied manner.
Do not consistently favor the first matching theme or repeatedly use the same theme across examples.
Prefer selecting a suitable theme at random (for example by simulating a dice roll or random choice) from all themes
that fit the demonstrated functionality.

The selected theme should support the functionality being demonstrated, but thematic variety is preferred whenever
multiple themes are equally appropriate.

Demo Content Inspirations
=========================

The following scenarios provide suitable sources for generating realistic demo content, sample data, stories,
calculations, visualizations, and examples.

Nature & Environment
--------------------

* A forest ecosystem
* A mountain landscape
* A river system
* A lake and its wildlife
* Seasonal changes throughout a year
* A nature reserve
* Weather observations over time
* Plant growth and development
* Water cycles and rainfall measurements

Animals & Wildlife
------------------

* Animal classifications
* Bird species and migration patterns
* Insect collections
* Marine life ecosystems
* Wildlife observations
* Habitat comparisons
* Predator-prey relationships
* Endangered species monitoring

Earth & Space
-------------

* The solar system
* Planetary characteristics
* Moon phases
* Star constellations
* Deep-space objects
* Space exploration missions
* Astronomical observations
* Exoplanet discoveries

Science & Mathematics
---------------------

* Physics experiments
* Forces, motion, and energy
* Light and optics
* Sound and acoustics
* Chemical elements and compounds
* Biological systems
* Cell structures and genetics
* Mathematical formulas and functions
* Statistical measurements and datasets

Games & Puzzles
---------------

* Logic puzzles
* Maze generation
* Board game components
* Card game mechanics
* Scoring systems
* Strategy game worlds
* Procedurally generated maps
* Educational quiz systems

Fantasy & Adventure
-------------------

* Fantasy kingdoms
* Magical creatures
* Adventurer journals
* Quest descriptions
* Fictional maps
* Mythical ecosystems
* Magical artifacts
* World-building data

Music & Sound
-------------

* Musical instruments
* Orchestra structures
* Song metadata
* Sound wave analysis
* Rhythm and timing patterns
* Chord and scale collections
* Composition structures
* Audio processing examples

Natural Language in Examples
============================

When using natural language in example code, use a balanced mix of different languages.
Natural language must only be used in data, values or input a demo processes.
Error messages, diagnostic output, and any output that explains the behavior of the demo must be English.

Preferred languages include:

* German
* French
* Spanish
* Italian
* Turkish
* Japanese
* Chinese
* Polish
* Dutch
* Portuguese
* Greek
* Czech
* Swedish
* Danish
* Finnish

This keeps examples visually interesting and also demonstrates the Unicode capabilities of the library.

Unicode and Emojis
==================

The library provides excellent Unicode support.
Do not hesitate to use Unicode characters and emojis in example code when they improve readability or make examples more
engaging.

Well-placed Unicode text often makes examples feel more realistic and memorable.

