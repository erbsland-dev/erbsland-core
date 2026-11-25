..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****************
Format ELCL Files
*****************

``elcl-format`` reformats ELCL sources using :doc:`the syntax formatting API </reference/conf/syntax_formatting>`.
It works from any working directory, including outside an Erbsland Core checkout.

.. code-block:: console

    erbsland-core-tool elcl-format settings.elcl
    erbsland-core-tool elcl-format --recursive config/ another.elcl
    erbsland-core-tool elcl-format --output-only settings.elcl > formatted.elcl
    erbsland-core-tool elcl-format --config styles/elcl_format.elcl config/

Inputs and Output
=================

Supply one or more files or directories.
Explicit files are accepted regardless of extension; directories select ``.elcl`` files, including descendants only with
``--recursive`` (``-r``).
Overlapping selections are deduplicated and processed in stable path order.
Traversal skips symbolic links; explicitly supplied symbolic links are rejected.
Hidden directories are included in recursive scans, and repository ignore files are not consulted.

Normal operation replaces only files whose exact bytes change.
Unchanged files retain their modification time.
Replacement uses a sibling temporary file, retains permission bits, and checks that the original file has not changed
before committing.
Input must be valid UTF-8; a UTF-8 BOM is retained.

``--output-only`` requires exactly one discovered file.
It writes raw formatted source to stdout and never changes the input.
Diagnostics use stderr.
Syntax errors preserve damaged constructs while allowing valid surrounding statements to be formatted.
Processing continues after individual file errors.
Errors produce a nonzero exit status; formatting changes and signed-content warnings alone do not.

Configuration Discovery
=======================

For each input file, search its containing directory and ancestors for the nearest ``elcl_format.elcl``.
If absent, use built-in defaults.
Ancestor configurations are not merged.
``--config`` overrides discovery for the whole command.
All selected configurations are loaded and validated before writing begins, including when a configuration file itself
is selected for formatting.
``--project`` is not applicable.

The example ``tool/conf/elcl_format.elcl`` documents a complete base format and selector overrides.
Copy it into an input directory or select it explicitly with ``--config``.
Configuration includes use the normal local file resolver and default access policy; placeholders are not expanded.

Configuration Structure
=======================

``[Elcl Format . Base]`` supplies ``Maximum Line Length`` and ``Value Wrapping``.
Optional child sections group the other settings:

* ``Names``: ``Section Case``, ``Section Separator``, ``Value Case``, ``Value Separator``, ``Text Name Case``,
  ``Text Name Separator``, ``Text Identifier Case``, ``Text Identifier Separator``, and ``Abbreviations``.
* ``Boolean``: ``Case`` and ``Form``.
* ``Section``: ``Indent``, ``Suffix``, ``Star``, ``Space After Open``, ``Space Before Close``, and ``Space Around Dot``.
* ``Vector``: ``Style`` and ``Spaces After Star``; ``Matrix``: ``Style``.
* ``Separator``: ``Character``, ``Position Mode``, ``Position``, ``Value Position Mode``, and ``Value Position``.
* ``Indentation``: ``Style`` and ``Count``.

Case settings accept ``keep``, ``lower``, ``title`` or ``upper``.
``Names.Abbreviations`` accepts one text value or a text list of single identifier words.
The defaults are HTTP, HTTPS, FTP, FTPS, SSH, TLS, URL, and VR; configured lists replace the inherited list completely.
Matching ignores case, and duplicates are removed.
Use ``Abbreviations: ""`` to clear the list.
Abbreviations affect title casing only, including explicitly enabled quoted-name and text-identifier formatting.

``Boolean.Case`` defaults to ``title``.
``Boolean.Form`` accepts ``keep`` (the default), ``true_false``, ``yes_no``, ``enabled_disabled``, or ``on_off``.
Keep case retains exact spelling when the pair is unchanged and uses lowercase when changing pairs.
For example, Yes/No with Keep case preserves ``YES`` but changes ``ON`` to ``yes``.
These settings can be overridden independently by selector rules.

Word separators accept ``space`` or ``underscore``.
Vector styles accept ``keep``, ``compact`` or ``multiline``; matrix styles accept ``keep`` or ``align``.
Value wrapping accepts ``keep``, ``same_line``, ``next_line``, ``next_line_if_fits`` (the default), or
``when_too_long``.
It moves complete non-multiline values; it does not split strings or other literals.
``next_line_if_fits`` moves an overlong value only when the indented value and trailing comment fit on the next line.
``when_too_long`` moves it whenever its assignment line exceeds the limit.
Standalone comments, blank lines, and multiline literal payloads retain their structure.

Position modes accept ``spaces`` or ``column``.
Indentation styles accept ``spaces`` or ``tabs``.
Section indent and suffix accept nonnegative counts or ``"infinite"``.
Line length, indentation count and bullet spacing must be positive.

Ordered ``*[Elcl Format . Rules]`` entries contain an optional ``Selector`` section and a required ``Style`` section.
List sections use the same default bracket alignment as regular sections; centering remains available through an
explicit rule.
Style uses the same property groups as Base, but omitted properties inherit.
Selector accepts ``Kind`` (``section`` or ``assignment`` ), ``List Section``, ``Relative Section``,
``Section Names More Than``, ``Meta Value``, ``Paths`` and ``Value Types``.
Paths and value types accept text lists.
Types are ``integer``, ``boolean``, ``float``, ``text``, ``date``, ``time``, ``date_time``, ``bytes``, ``time_delta``,
``regex`` and ``value_list``.
Unknown settings and invalid values fail configuration validation.
