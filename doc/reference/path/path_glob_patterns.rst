.. index::
    single: Path Glob Patterns
    single: PathGlobPattern
    single: Directory filtering

******************
Path Glob Patterns
******************

``PathGlobPattern::compile()`` returns an immutable ``PathGlobPatternPtr`` suitable for concurrent matching.
Compilation and matching do not access the filesystem.
Paths need not exist.
String candidates behave exactly like constructing ``Path{text}``; empty or invalid candidates never match.
Matching is case-sensitive, uses decoded characters, and includes dotfiles.

Matching Modes
==============

``fullMatch()`` consumes all non-root components.
``suffixMatch()`` starts at a component boundary and reaches the end.
``match(path, mode)`` selects the same behavior at runtime using ``PathGlobMatchMode::Full`` or ``Suffix``.
Each operation also accepts a ``String``.

.. list-table::
    :header-rows: 1

    * - Pattern
      - Candidate
      - Full
      - Suffix
    * - ``*.cpp``
      - ``src/main.cpp``
      - False
      - True
    * - ``/*.cpp``
      - ``src/main.cpp``
      - False
      - False
    * - ``foo``
      - ``a/foo/child``
      - False
      - False
    * - ``**/foo/**``
      - ``a/foo/child``
      - True
      - True

A leading slash anchors suffix matching to the first non-root component; full matching ignores this anchor.
Drive letters and UNC root identity do not participate in matching.
Literal roots in patterns are removed and anchor suffix matching.
Wildcards in roots are rejected.
A standalone slash matches a zero-component root.
To constrain filesystem locations, combine matching with explicit path or access checks.

Syntax and Compilation Flags
============================

``*`` matches zero or more characters within one component, ``?`` matches one character, and Core ``[a-z]`` or
``[^a-z]`` sets match one character.
Backslash escapes ``?``, ``*``, ``[``, ``]``, backslash, ``;``, and ``#``.
Use slash separators on every platform.
Character sets and escaped characters are never list delimiters.

Whole-component ``**`` matches zero or more components.
``a/**/b`` includes ``a/b`` and ``a/x/y/b``; ``a/**`` also matches ``a`` itself.
Multiple recursive components are allowed.
Embedded ``**``, runs of three stars, malformed sets, unsupported escapes, and trailing slashes are rejected.
Standalone ``/`` is the trailing-slash exception.

The default flags are ``RecursiveWildcard | TrimWhitespace``.
Explicit flags replace this default set.

.. list-table::
    :header-rows: 1

    * - Flag
      - Effect
    * - ``RecursiveWildcard``
      - Permit whole-component ``**``.
    * - ``TrimWhitespace``
      - Trim bare spaces and tabs from the outer edges of each alternative, preserving interior component whitespace.
    * - ``MultiplePatterns``
      - Allow LF or CRLF between OR alternatives; ignore empty alternatives.
    * - ``SemicolonSeparator``
      - Also allow semicolon separators; requires ``MultiplePatterns``.
    * - ``IgnoreComments``
      - Ignore lines starting with ``#``, after leading spaces/tabs only when trimming is enabled.

Comment lines, including their line endings, are discarded without parsing their contents.
Inline hashes, including after a semicolon on a noncomment line, stay literal.
Use ``\#`` for a literal leading hash.
Sets such as ``[ ]`` preserve boundary whitespace.
Disabled unescaped separators raise ``ParseError``.
An invalid flag combination raises ``ParameterFailure``.
Empty or comment-only input raises ``ParseError``.
Parse errors retain code-point positions in the original source, including earlier alternatives and comments.
These options do not implement ignore-rule negation, precedence, or directory-only syntax.

.. code-block:: cpp

    using namespace el::text::literals;
    using Flag = el::PathGlobPatternFlag;
    const auto sources = el::PathGlobPattern::compile(
        "# Source files\n **/*.cpp ; **/*.hpp\n"_el,
        el::PathGlobPatternFlags{
            Flag::Default, Flag::MultiplePatterns, Flag::SemicolonSeparator, Flag::IgnoreComments});
    const auto accepted = sources->fullMatch("src/main.cpp"_el);

Walking and Pruning
===================

``mayMatchDescendant(directory, mode)`` tests whether a strict descendant could match.
False guarantees that pruning is safe; true may conservatively retain a branch, without testing whether that branch
exists.
An unanchored suffix pattern can start again farther down the tree.

``PathWalkOptions::setGlobPattern()`` accepts a compiled pointer or a string plus compile flags.
A string is compiled before replacing the previous option; failure leaves that option unchanged.
``setGlobMatchMode()`` selects Full (the default) or Suffix.
Matching is relative to the starting directory.

.. code-block:: cpp

    const auto options = el::PathWalkOptions{}
        .setGlobPattern("src/**/*.cpp"_el)
        .setTypes(el::PathType::RegularFile);
    root.walker().walkOrThrow(callback, options);

The glob runs before reporting filters and additional metadata requests.
Its descendant check runs before descent filters.
Nonmatching ancestors remain traversable.
Pruned branches do not reach user callbacks.
See :doc:`file_system_paths` for symlink, callback, and error policies.

Interface
=========

.. doxygenenum:: erbsland::path::PathGlobMatchMode
.. doxygenclass:: erbsland::path::PathGlobPattern
    :members:

.. doxygentypedef:: erbsland::path::PathGlobPatternPtr
.. doxygenenum:: erbsland::path::PathGlobPatternFlag

.. doxygentypedef:: erbsland::path::PathGlobPatternFlags
