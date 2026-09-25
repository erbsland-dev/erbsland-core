..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Fuzzy Matching
    single: Text Suggestions
    single: Damerau-Levenshtein Distance

*****************************
Finding Similar Text by Edits
*****************************

A user remembers the name of a sound, but types ``kaik`` instead of ``kaiku``.
An exact lookup finds nothing.
If the application has a short list of known names, it can offer the nearest ones and let the user decide.

:cpp:class:`Matcher <erbsland::text::fuzzy::Matcher>` compares the typed text with each candidate and ranks the
results by how many edits they need.
This page shows what that distance means, how to keep the suggestions useful, and how to read each returned
:cpp:class:`Match <erbsland::text::fuzzy::Match>`.

What a Distance Means
=====================

The matcher uses a Damerau-Levenshtein edit distance.
It counts changes to decoded Unicode code points, rather than changes to the bytes of a UTF-8 string.

Adding one character costs one edit.
So does removing or replacing one character.
Swapping two adjacent characters also costs one: ``kaiku`` and ``kaiuk`` are one edit apart.
This model catches several common mistakes in short names without needing rules tailored to each name.

The distance is only a count of edits.
It is not a probability, and a low distance does not prove that the candidate is what the user meant.
Suggestions work best when you already have a known set of names, such as commands, option names, or short labels.
For a very large collection, a search index may be a better first step than comparing every candidate.

Offering Nearby Names
=====================

Construct a matcher with the text the user typed.
Pass the known names as a :cpp:type:`StringList <erbsland::text::StringList>` to ``findMatches()``.
The method returns a :cpp:type:`MatchList <erbsland::text::fuzzy::MatchList>` in rank order.

In the example, the user has typed ``kaik``.
The matcher looks through a short list of Finnish sound terms and accepts only names within one edit.

.. erbsland-demo::
    :source: text/FuzzyMatching/FuzzyMatchingDemos.cpp
    :function-blocks: suggestSoundNames
    :function-blocks-sha256: 17ce783d1f86a119e0d482ca7ee4dfaa1dad904f9102dcc262c958796ff74780
    :exec: text/fuzzy_matching --demo SuggestSoundNames
    :source-sha256: 25d8d9fbc34d3bde7ee4f33c80f081b6e3f40c50dc7ab456f65a212164cea38a

.. code-block:: cpp

    void suggestSoundNames() {
        const auto candidates = el::StringList{"kaiku"_el, "sointu"_el, "sävel"_el, "kaiut"_el};
        const auto matches = el::fuzzy::Matcher{"kaik"_el}.setMaximumDistance(el::CpLength{1U}).findMatches(candidates);
        for (const auto &match : matches) {
            el::io::printLine(match.candidate(), " (edits: "_el, match.distance().toSizeT(), ")"_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    kaiku (edits: 1)

.. erbsland-demo-end::

``setMaximumDistance()`` is what keeps the suggestion list focused.
Without it, the limit is infinite, so even distant candidates can appear in the result.
With a small limit, an empty result simply means that none of the supplied names was close enough.

You can also default-construct a matcher and call ``setPattern()`` later.
That is useful when the application reuses one configured matcher for successive inputs.

Tuning Which Suggestions Appear
===============================

The distance limit decides which candidates qualify.
``setMaximumResults()`` then limits how many of those ranked candidates are returned.
It is infinite by default.
For a prompt or a diagnostic, two or three close suggestions are usually easier to scan than a long list.
If you set the distance limit to zero, only text that compares exactly under the selected character policy qualifies.

The default character comparison is case-sensitive.
If your names follow another policy, pass a :cpp:type:`CharCompareFn <erbsland::text::CharCompareFn>` to
``setComparisonFn()``.
For ASCII command names, ``Char::compareAsciiFolded`` treats ``KAIKU`` and ``kaiku`` as equal, giving them distance
zero.
That function folds ASCII case only; it is not a general Unicode case-folding rule.
Passing an empty function restores exact character comparison.

The example sets a one-edit distance limit, asks for at most two results, and uses ASCII folding.
It also starts with a default-constructed matcher to show how its pattern can be assigned later.

.. erbsland-demo::
    :source: text/FuzzyMatching/FuzzyMatchingDemos.cpp
    :function-blocks: tuneSuggestions
    :function-blocks-sha256: fb535d34b0747b50a305a7ed4a68b589a6cc96dc2320e67010a3a786df2987dd
    :exec: text/fuzzy_matching --demo TuneSuggestions
    :source-sha256: 25d8d9fbc34d3bde7ee4f33c80f081b6e3f40c50dc7ab456f65a212164cea38a

.. code-block:: cpp

    void tuneSuggestions() {
        auto matcher = el::fuzzy::Matcher{};
        matcher.setPattern("KAIKU"_el)
            .setMaximumDistance(el::CpLength{1U})
            .setMaximumResults(el::ItemCount{2U})
            .setComparisonFn(el::Char::compareAsciiFolded);

        const auto candidates = el::StringList{"kaiku"_el, "KAIKU"_el, "kaiuk"_el, "kaikuu"_el, "sointu"_el};
        const auto matches = matcher.findMatches(candidates);
        for (const auto &match : matches) {
            el::io::printLine(match.text(), " (edits: "_el, match.distance().toSizeT(), ")"_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    kaiku (edits: 0)
    kaiuk (edits: 1)

.. erbsland-demo-end::

The comparison function has another effect: it decides when two candidates are equivalent.
With ASCII folding, ``kaiku`` and ``KAIKU`` count as the same suggestion.
Only the first is retained.
If one spelling should be shown to the user, place it first in the input list.

When a matcher is reused, ``pattern()``, ``maximumDistance()``, ``maximumResults()``, and ``comparisonFn()`` let you
inspect its current settings.

Reading Ranked Matches
======================

Each result keeps the original candidate spelling.
Read it through ``text()`` or ``candidate()``.
``distance()`` gives the number of edits from the pattern under the chosen comparison function.

Distance zero means the candidate compares equal to the pattern.
Smaller distances appear first.
When two candidates have the same distance, their order in the input list decides their order in the result.

The next example uses ``sävel``, the Finnish word for a musical note.
It lets you see an exact match, a replacement of ``ä``, an adjacent swap, and an added character side by side.

.. erbsland-demo::
    :source: text/FuzzyMatching/FuzzyMatchingDemos.cpp
    :function-blocks: inspectMatches
    :function-blocks-sha256: 61ab35b7aa2507d09eb942c9cf121b1c6b77af51e7c5c9f0e81a212e72614e2c
    :exec: text/fuzzy_matching --demo InspectMatches
    :source-sha256: 25d8d9fbc34d3bde7ee4f33c80f081b6e3f40c50dc7ab456f65a212164cea38a

.. code-block:: cpp

    void inspectMatches() {
        const auto candidates = el::StringList{u8"sävel"_el, u8"savel"_el, u8"säevl"_el, u8"säveli"_el};
        const auto matches = el::fuzzy::Matcher{u8"sävel"_el}.setMaximumDistance(el::CpLength{1U}).findMatches(candidates);
        for (const auto &match : matches) {
            el::io::printLine("Candidate: "_el, match.text(), ", distance: "_el, match.distance().toSizeT());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Candidate: sävel, distance: 0
    Candidate: savel, distance: 1
    Candidate: säevl, distance: 1

.. erbsland-demo-end::

The ``ä`` is encoded by more than one byte in UTF-8, yet replacing it counts as one edit.
That is the benefit of measuring decoded characters here.
The match retains the original candidate text, so you can display it directly or use it as a lookup key after the user
chooses it.
