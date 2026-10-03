.. index::
    !single: Working with Character Sets
    single: CharSet
    single: Char
    single: Character Validation
    single: String
    single: Unicode Categories
    single: Input Validation

***************************
Working with Character Sets
***************************

A configuration key may allow letters, digits, underscores, and hyphens, while a display label may accept letters from
many writing systems.
:cpp:class:`CharSet <erbsland::text::CharSet>` lets you give such a character policy a name and reuse it for validation,
searching, trimming, and removal.
You can describe a few explicit characters, Unicode categories, or a combination of smaller sets.

This page develops those policies from simple sets, applies them to incoming text, and shows how the same definitions
support cleanup and word conversion.
Each set contains decoded Unicode code points, represented by :cpp:class:`Char <erbsland::text::Char>`.
A visible character can contain several code points, so a set describes individual membership rather than complete words
or user-perceived characters.

Defining a Reusable Character Policy
====================================

A named set makes the rule visible where you apply it.
The following option-name policy accepts ASCII letters, digits, underscores, and hyphens.
:cpp:func:`containsOnly() <erbsland::text::U8String::containsOnly>` checks every decoded character against that set:

.. erbsland-demo::
    :source: text/String/CharacterSetReusable.cpp
    :exec: text/string --demo CharacterSetReusable
    :sha256: 631e4c1a27b293c902655aa64740f6486478f91fa52f6fa9d540155cff89bce9

.. code-block:: cpp

    /// Reuse named `CharSet` objects for validation policies that are applied repeatedly.
    ///
    /// Building a set once makes the policy easier to read and avoids reconstructing category or pattern based sets in hot
    /// paths.
    void characterSetReusable() {
        static const auto optionNameChars = el::CharSet::fromPattern("-_a-zA-Z0-9"_el);

        const auto optionNames = el::StringList{
            "orbite-07"_el,
            "antenne_nord"_el,
            "équipe-science"_el,
            "module solaire"_el,
        };

        const auto yesNo = el::BooleanFormat::yesNo();
        optionNames.forEach([&](const el::String &optionName) -> void {
            el::io::printLine(optionName, " -> "_el, yesNo, optionName.containsOnly(optionNameChars));
        });
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0morbite-07 -> yes
        antenne_nord -> yes
        équipe-science -> no
        module solaire -> no␛[0m

.. erbsland-demo-end::

Keeping this repeated policy in a local ``static const`` avoids rebuilding it on each call.
That is especially useful for sets assembled from Unicode categories.
``CharSet`` values are inexpensive to copy, so you can also store a policy in an object or pass it to another component.

Explicit Characters and Ranges
------------------------------

For punctuation or separators, a literal or list of ``Char`` values directly expresses the accepted characters.
The text used to construct a set contributes individual code points: ``"!?"`` means either punctuation mark, regardless
of order or repetition in the input.

.. erbsland-demo::
    :source: text/String/CharacterSetText.cpp
    :exec: text/string --demo CharacterSetText
    :sha256: 39863789acb69cc1f2c42b22b54ae8aba67e71cd018000c6ad12094fb0e16fbd

.. code-block:: cpp

    /// `CharSet` can be created from text or from a list of decoded characters.
    ///
    /// Duplicate characters are ignored, and the resulting set is normalized for efficient membership tests.
    void characterSetText() {
        auto punctuation = el::CharSet{"!?.,;"_el};
        auto separators = el::CharSet{U',', U';', U':', U'/'};

        const auto message = el::String{"statut: prêt; orbite stable."_el};
        const auto yesNo = el::BooleanFormat::yesNo();
        el::io::printLine("Message contains punctuation : "_el, yesNo, message.containsOneOf(punctuation));
        el::io::printLine("Message contains separators .: "_el, yesNo, message.containsOneOf(separators));
        el::io::printLine("Separator accepts '/' .......: "_el, yesNo, separators.contains(U'/'));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mMessage contains punctuation : yes
        Message contains separators .: yes
        Separator accepts '/' .......: yes␛[0m

.. erbsland-demo-end::

A range is useful when the accepted code points are consecutive.
:cpp:func:`CharSet::fromRange() <erbsland::text::CharSet::fromRange>` includes both endpoints, whereas
``CharSet{U'a', U'z'}`` contains only those two characters:

.. erbsland-demo::
    :source: text/String/CharacterSetCharacters.cpp
    :exec: text/string --demo CharacterSetCharacters
    :sha256: 33b8e285f698aa20923aebd72634fbc823ab9f81f01a02c0bf2f980b9b602294

.. code-block:: cpp

    /// `CharSet` can be created directly from a single character or explicitly from an inclusive range.
    ///
    /// Use `fromRange()` when two `Char` values describe bounds instead of two individual allowed characters.
    void characterSetCharacters() {
        auto questionMark = el::CharSet{U'?'};
        auto asciiLowercase = el::CharSet::fromRange(U'a', U'z');

        const auto yesNo = el::BooleanFormat::yesNo();
        el::io::printLine("Question marker accepts '?' : "_el, yesNo, questionMark.contains(U'?'));
        el::io::printLine("Lowercase accepts 'm' ......: "_el, yesNo, asciiLowercase.contains(U'm'));
        el::io::printLine("Lowercase accepts 'M' ......: "_el, yesNo, asciiLowercase.contains(U'M'));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mQuestion marker accepts '?' : yes
        Lowercase accepts 'm' ......: yes
        Lowercase accepts 'M' ......: no␛[0m

.. erbsland-demo-end::

Patterns for Compact Policies
-----------------------------

:cpp:func:`CharSet::fromPattern() <erbsland::text::CharSet::fromPattern>` combines literal characters and inclusive
ranges in a compact spelling.
A hyphen between characters describes a range; at the start or end it represents a literal hyphen.
The syntax resembles a regular-expression character class, but it describes only membership, without repetitions or
multi-character matches.

A leading ``^`` inverts the policy: ``^-_a-zA-Z0-9`` selects Unicode scalar values outside the option-name set.
The hyphen after that prefix is still a literal hyphen.
This is useful when a cleanup operation should remove every character that the policy would reject:

.. erbsland-demo::
    :source: text/String/CharacterSetPatterns.cpp
    :exec: text/string --demo CharacterSetPatterns
    :sha256: 0d5eeb8b0c4f9a3d51cc6259097cd5456df02a92b20c4c91bea425b0b7d0fb3d

.. code-block:: cpp

    /// `CharSet::fromPattern()` creates compact character sets from literal and range patterns.
    ///
    /// A hyphen between two characters defines a range.
    /// A leading or trailing hyphen is treated as a literal hyphen.
    /// A leading caret selects every Unicode scalar value outside the following set.
    void characterSetPatterns() {
        auto identifierChars = el::CharSet::fromPattern("_a-zA-Z0-9"_el);
        auto optionNameChars = el::CharSet::fromPattern("-_a-zA-Z0-9"_el);

        const auto stationId = el::String{"ORBIT_07"_el};
        const auto optionName = el::String{"orbite-07"_el};
        const auto spacedName = el::String{"orbite 07"_el};

        const auto invalidOptionChars = el::CharSet::fromPattern("^-_a-zA-Z0-9"_el);
        el::io::printLine("Filtered option ...........: "_el, spacedName.removedAll(invalidOptionChars));

        const auto yesNo = el::BooleanFormat::yesNo();
        el::io::printLine("Identifier \"", stationId, "\" ....: "_el, yesNo, stationId.containsOnly(identifierChars));
        el::io::printLine("Option \"", optionName, "\" ........: "_el, yesNo, optionName.containsOnly(optionNameChars));
        el::io::printLine("Option \"", spacedName, "\" ........: "_el, yesNo, spacedName.containsOnly(optionNameChars));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mFiltered option ...........: orbite07
        Identifier "ORBIT_07" ....: yes
        Option "orbite-07" ........: yes
        Option "orbite 07" ........: no␛[0m

.. erbsland-demo-end::

The filtered name can differ from what the user supplied.
For a configuration key whose exact spelling matters, report an invalid character instead of silently filtering it.
Malformed patterns such as the reversed range ``z-a`` raise :cpp:class:`ParseError <erbsland::err::ParseError>`.

ASCII and Unicode Character Categories
--------------------------------------

Many formats already have a named ASCII rule, such as hexadecimal digits or words containing hyphens.
An :cpp:enum:`AsciiCategory <erbsland::text::AsciiCategory>` can be passed directly to ``containsOnly()`` and to the
scanning methods of :cpp:class:`StringCharReader <erbsland::text::StringCharReader>`.
A single ``Char`` can test the same rule with
:cpp:func:`isAsciiCategory() <erbsland::text::Char::isAsciiCategory>`.
Construct a set from that category when you need to combine it with custom characters or pass it to a set-based API.

For text spanning multiple writing systems,
:cpp:func:`CharSet::from() <erbsland::text::CharSet::from>` accepts a
:cpp:enum:`UnicodeCategory <erbsland::text::UnicodeCategory>` or
:cpp:enum:`UnicodeCategoryGroup <erbsland::text::UnicodeCategoryGroup>`.
The following example contrasts an ASCII packet identifier with reusable sets of Unicode digits and letters:

.. erbsland-demo::
    :source: text/String/CharacterSetCategories.cpp
    :exec: text/string --demo CharacterSetCategories
    :sha256: ead0fc64f8c4d6500b7f90454998f7726c25d55d8fd8ba03d4efbaf08992112e

.. code-block:: cpp

    /// Use `AsciiCategory` directly when an API accepts it, and construct a `CharSet` only when the policy must be retained
    /// or combined with other characters.
    void characterSetCategories() {
        const auto packetId = el::String{"SENSOR-07"_el};
        const auto packetIdIsValid = packetId.containsOnly(el::AsciiCategory::WordWithHyphen);
        const auto firstIsUppercase =
            packetId.charAt(el::StringSide::Front).isAsciiCategory(el::AsciiCategory::UppercaseLetter);

        static const auto unicodeDigits = el::CharSet::from(el::UnicodeCategory::DecimalNumber);
        static const auto unicodeLetters = el::CharSet::from(el::UnicodeCategoryGroup::Letter);

        const auto yesNo = el::BooleanFormat::yesNo();
        el::io::printLine("Packet-id is ASCII word ..: "_el, yesNo, packetIdIsValid);
        el::io::printLine("First letter is uppercase : "_el, yesNo, firstIsUppercase);
        el::io::printLine("Unicode digit accepts '7' : "_el, yesNo, unicodeDigits.contains(U'7'));
        el::io::printLine("Unicode letter accepts 'é': "_el, yesNo, unicodeLetters.contains(U'é'));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mPacket-id is ASCII word ..: yes
        First letter is uppercase : yes
        Unicode digit accepts '7' : yes
        Unicode letter accepts 'é': yes␛[0m

.. erbsland-demo-end::

A Unicode letter category includes accented letters such as ``é``.
A decomposed spelling may also contain combining marks, which belong to different categories.
Choose the accepted categories and, if needed, a :doc:`normalization policy <normalizing_strings>` according to the
format you are implementing.

Validating Incoming Text
========================

Character membership and encoding validity answer different questions.
Decoded string operations tolerate malformed input by presenting replacement characters in many inspection operations.
When your input must be well-formed, first call
:cpp:func:`isEncodingValid() <erbsland::text::U8String::isEncodingValid>` or
:cpp:func:`encodingValidOrThrow() <erbsland::text::U8String::encodingValidOrThrow>`.
These checks are available for all three string widths.
Then apply any length limits and character policies to the validated value.

``containsOnly(allowed)`` checks that every character belongs to an accepted set.
:cpp:func:`containsOneOf() <erbsland::text::U8String::containsOneOf>` answers whether any character belongs to a selected
set, which is useful for identifying a specific validation problem:

.. erbsland-demo::
    :source: text/String/ValidateCharacterPolicy.cpp
    :exec: text/string --demo ValidateCharacterPolicy
    :sha256: 5a81c59438ed968b29f7a853757786306e09523d55e510eff7b96212ab9ab88d

.. code-block:: cpp

    /// Combine encoding checks with `containsOnly()` and `containsOneOf()` for character-level validation.
    ///
    /// Validate externally supplied text before applying tolerant decoded-character operations.
    void validateCharacterPolicy() {
        static const auto userNameChars = el::CharSet::fromPattern("-_a-zA-Z0-9"_el);
        static const auto forbiddenChars = el::CharSet{" \t\r\n"_el};

        const auto userNames = el::StringList{
            "orbite-07"_el,
            "module solaire"_el,
            "équipe-science"_el,
        };

        const auto yesNo = el::BooleanFormat::yesNo();
        userNames.forEach([&](const el::String &userName) -> void {
            const auto isAccepted = userName.isEncodingValid() && userName.containsOnly(userNameChars) &&
                !userName.containsOneOf(forbiddenChars);
            el::io::printLine(userName, " -> "_el, yesNo, isAccepted);
        });
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0morbite-07 -> yes
        module solaire -> no
        équipe-science -> no␛[0m

.. erbsland-demo-end::

The example's explicit whitespace check shows how to test a forbidden set; its allowed set already excludes those
characters.
In a validator with several diagnostics, separate checks can explain why a value failed.
For a single acceptance decision, the encoding check and allowed-set check are sufficient here.

An empty string satisfies ``containsOnly()`` because it has no character outside the set.
Check :cpp:func:`isEmpty() <erbsland::text::U8String::isEmpty>` separately when a value is required.
A default-constructed ``CharSet`` contains no characters: only empty text passes ``containsOnly()`` with it, and
``containsOneOf()`` always returns ``false``.

Membership alone does not establish a complete grammar.
An identifier may restrict its first character, an address may require delimiters, and a field may have a length limit.
Apply those structural rules alongside the sets; :doc:`character_access_and_parsing` explains how to read fields and
sequences of characters.

Combining Policies for Different Positions
==========================================

An identifier often accepts fewer characters at its start than in the remaining positions.
You can build the continuation policy from the start policy, adding digits and a hyphen without duplicating the letter
rule:

.. erbsland-demo::
    :source: text/String/CombineCharacterSets.cpp
    :exec: text/string --demo CombineCharacterSets
    :sha256: 95509a9ef32a3657065c144ec5e90bd8cf23eb7cacc54d591ced4f70cb7b85bd

.. code-block:: cpp

    /// `CharSet` objects can be combined and compared to express larger validation policies.
    ///
    /// Use set operations to build the final policy from named parts, then use subset checks when one policy must remain
    /// inside another.
    void combineCharacterSets() {
        static const auto letters = el::CharSet::from(el::UnicodeCategoryGroup::Letter);
        static const auto digits = el::CharSet::from(el::UnicodeCategory::DecimalNumber);
        static const auto identifierStart = letters | el::CharSet{U'_'};
        static const auto identifierContinue = identifierStart | digits | el::CharSet{U'-'};

        const auto identifier = el::String{"orbite-7"_el};
        const auto firstCharacterOk = identifierStart.contains(identifier.charAt(el::StringSide::Front));
        const auto fullIdentifierOk = identifier.containsOnly(identifierContinue);

        const auto invalidContinue = identifierContinue.inverted();
        el::io::printLine("Filtered identifier .......: "_el, el::String{"orbite !7"_el}.removedAll(invalidContinue));

        const auto yesNo = el::BooleanFormat::yesNo();
        el::io::printLine("Start policy is subset ....: "_el, yesNo, identifierStart.isSubsetOf(identifierContinue));
        el::io::printLine("First character accepted ..: "_el, yesNo, firstCharacterOk);
        el::io::printLine("Identifier accepted .......: "_el, yesNo, fullIdentifierOk);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mFiltered identifier .......: orbite7
        Start policy is subset ....: yes
        First character accepted ..: yes
        Identifier accepted .......: yes␛[0m

.. erbsland-demo-end::

The union operator ``|`` accepts characters from either set.
Its named equivalent is :cpp:func:`unitedWith() <erbsland::text::CharSet::unitedWith>`.
Intersection with ``&`` or :cpp:func:`intersectedWith() <erbsland::text::CharSet::intersectedWith>` keeps only
characters accepted by both policies, while subtraction with ``-`` or
:cpp:func:`subtractedBy() <erbsland::text::CharSet::subtractedBy>` removes exceptions.
Choose the form that makes the policy easiest to read.

The example checks the first character separately, then checks the whole value against the broader continuation set.
:cpp:func:`isSubsetOf() <erbsland::text::CharSet::isSubsetOf>` can also verify a relationship between policies, for
example when a configured extension must stay inside the format's accepted alphabet.

Trimming Borders and Removing Unwanted Characters
=================================================

A character policy can select text to remove as well as text to accept.
:cpp:func:`trimmed() <erbsland::text::U8String::trimmed>` removes matching characters from the edges while leaving the
interior untouched.
Without a set it trims ASCII whitespace; a custom set can also include application-specific markers:

.. erbsland-demo::
    :source: text/String/TrimCharacterSet.cpp
    :exec: text/string --demo TrimCharacterSet
    :sha256: 36a1a1b5bd7a0badbbe227ecf5b37f3d84dde78281a34ee5c42976c2476fa6f0

.. code-block:: cpp

    /// `String::trimmed()` returns a view with selected characters removed from the front, back, or both sides.
    ///
    /// With a custom `CharSet`, trimming is not limited to whitespace.
    void trimCharacterSet() {
        const auto raw = el::String{"*** signal-orbite ;; "_el};
        static const auto border = el::CharSet{" *;"_el};

        auto clean = raw.trimmed(border);
        auto frontOnly = raw.trimmed(border, el::StringSide::Front);
        auto backOnly = raw.trimmed(border, el::StringSide::Back);

        el::io::printLine("Raw .......: \"", raw, "\""_el);
        el::io::printLine("Both sides : \"", clean, "\""_el);
        el::io::printLine("Front only : \"", frontOnly, "\""_el);
        el::io::printLine("Back only .: \"", backOnly, "\""_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mRaw .......: "*** signal-orbite ;; "
        Both sides : "signal-orbite"
        Front only : "signal-orbite ;; "
        Back only .: "*** signal-orbite"␛[0m

.. erbsland-demo-end::

The result is an owning slice into the original storage, so the selected text stays alive independently of the source
variable.
The optional :cpp:enum:`StringSide <erbsland::text::StringSide>` limits trimming to the front or back.

When selected characters can appear throughout the text,
:cpp:func:`removedAll() <erbsland::text::U8String::removedAll>` omits every matching code point.
Inverting an allowed set with ``~allowed`` or :cpp:func:`inverted() <erbsland::text::CharSet::inverted>` selects all
other Unicode scalar values:

.. erbsland-demo::
    :source: text/String/RemoveCharacters.cpp
    :exec: text/string --demo RemoveCharacters
    :sha256: f00a8cf21e03830af241dfa77d052f9a4103db9ab138e506407566212372e0f4

.. code-block:: cpp

    /// `String::removedAll()` removes every decoded character from a selected `CharSet`.
    ///
    /// This is useful for simple cleanup passes where unwanted characters may occur anywhere in the text.
    /// Inverting an allowed set removes every character outside that policy.
    void removeCharacters() {
        static const auto controlChars = el::CharSet::from(el::AsciiCategory::Control);

        const auto input = el::String{"rapport\torbite\nstable"_el};
        auto logLine = input.removedAll(controlChars);

        static const auto allowedChars = el::CharSet::fromPattern(" a-z"_el);
        const auto stationName = el::String{"orbite! 07"_el};
        el::io::printLine("Allowed  : "_el, stationName.removedAll(~allowedChars));

        el::io::printLine("Original : "_el, input.toEscaped(el::EscapeFormat::Cpp));
        el::io::printLine("Cleaned  : "_el, logLine.toEscaped(el::EscapeFormat::Cpp));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mAllowed  : orbite
        Original : rapport\torbite\nstable
        Cleaned  : rapportorbitestable␛[0m

.. erbsland-demo-end::

Removing tabs and newlines also removes the boundaries they represented: ``rapport\torbite\nstable`` becomes
``rapportorbitestable``.
When those boundaries should remain as spaces,
:cpp:func:`toWhitespaceSimplified() <erbsland::text::toWhitespaceSimplified>` expresses the intended cleanup more
accurately.
Set inversion concerns Unicode scalar values; encoding validation remains a separate step.

Character Sets in Word Transformations
======================================

The common transformation helpers accept sets where the caller defines word separators.
For example, a sensor format may treat underscores and hyphens as equivalent boundaries:

.. code-block:: cpp

    static const auto separators = el::CharSet{"_-"_el};
    const auto sensorKey = el::String{"sonda_CTD-mare"_el};
    const auto joined = el::toCamelCaseFromWords(sensorKey, separators); // sondaCTDMare

Include ``<erbsland/StringTransformations.hpp>`` for the helper.
Each character in the set is a boundary, repeated boundaries are skipped, and the input remains unchanged.
The default overload uses whitespace, so an explicit set records the format's additional rule.

:doc:`transforming_strings` develops common case conversion, word conversion, and whitespace cleanup with compiled
examples.
It also explains :cpp:func:`transformed() <erbsland::text::U8String::transformed>` for custom character mappings when
replacement or removal by a set cannot express the rule.
