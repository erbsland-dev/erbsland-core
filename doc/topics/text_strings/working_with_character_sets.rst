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

Many text-processing tasks start with a simple question:

"Which characters are allowed here?"

Whether you validate user input, parse configuration files, filter protocol fields, or normalize text, you often need a
compact way to describe a set of accepted characters.

:cpp:class:`CharSet <erbsland::text::CharSet>` turns that question into a reusable value.
It can describe a few explicit characters, broad Unicode categories, or a policy assembled from several smaller sets.
The string APIs can then search, validate, trim, remove, or transform against the same definition.

This page begins with the Unicode character model behind those operations, then develops efficient ways to construct and
reuse sets before applying them to realistic text policies.
Along the way, it explains why encoding validation must happen before a character policy at an untrusted boundary and
why a set of code points is not the same as a set of user-perceived characters.

Know What One Set Element Represents
====================================

:cpp:class:`Char <erbsland::text::Char>` represents one decoded Unicode code point.
:cpp:class:`CharSet <erbsland::text::CharSet>` represents a set of those code points.

A character set does not represent grapheme clusters, locale-specific collation rules, or regular expressions.
It answers questions such as:

- Is this character a digit?
- Is this character allowed in an identifier?
- Does this string contain any control characters?

It does not answer questions that depend on multiple code points being interpreted together, such as a user-visible
character composed from a base letter and one or more combining marks.

This distinction keeps character-set operations predictable and efficient.
When your problem is about token syntax, configuration keys, command-line options, file-name filters, protocol fields,
or simple cleanup passes,
:cpp:class:`CharSet <erbsland::text::CharSet>` is usually the right tool.

Validate the Encoding Before the Character Policy
=================================================

String operations that decode text are tolerant by default.
For UTF-8 and UTF-16 input, malformed sequences are decoded as
:cpp:func:`Char::replacement() <erbsland::text::Char::replacement>` in many
inspection operations.

This behavior is useful for display, logging, and best-effort processing, but it is not the same as rejecting invalid
input.

For security-sensitive or externally supplied input, validate the encoding before you apply character-level rules:

- Check the storage encoding with
  :cpp:func:`isValidUtf8() <erbsland::text::U8String::isValidUtf8>`,
  :cpp:func:`isValidUtf16() <erbsland::text::U16String::isValidUtf16>`, or
  :cpp:func:`isValidUtf32() <erbsland::text::U32String::isValidUtf32>`.
- Check the length with
  :cpp:func:`length() <erbsland::text::U8String::length>` or
  :cpp:func:`characterLength() <erbsland::text::U8String::characterLength>`,
  depending on whether your limit applies to storage size or decoded characters.
- Apply character policies with
  :cpp:func:`containsOnly() <erbsland::text::U8String::containsOnly>` and
  :cpp:func:`containsOneOf() <erbsland::text::U8String::containsOneOf>`.

If tolerant processing is intentional, include
:cpp:func:`Char::replacement() <erbsland::text::Char::replacement>` in the tested
set or handle it explicitly.
If malformed input must be rejected, validate the encoding before any tolerant operation can hide the distinction.

Build Expensive Policies Once and Reuse Them
============================================

Create character sets close to the policy they represent, but avoid rebuilding expensive sets in hot paths.
:cpp:class:`CharSet <erbsland::text::CharSet>` uses copy-on-write storage, so
passing instances around is inexpensive.
Constructing large sets, especially those derived from Unicode categories, can be significantly more expensive than
using them.

For validators, parsers, and repeated transformations, prefer static reusable sets:

.. erbsland-demo::
    :source: text/String/CharacterSetReusable.cpp
    :exec: text/string --demo CharacterSetReusable
    :source-sha256: 1a119ccc716c038cf502c522ebeeef1d6ecf3028c3e7d53046a364e7fd05ef7e

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

.. erbsland-ansi::
    :escape-char: ␛

    orbite-07 -> yes
    antenne_nord -> yes
    équipe-science -> no
    module solaire -> no

.. erbsland-demo-end::

Giving a set a descriptive name also makes the code easier to read.
The validation rule becomes immediately visible without having to decode a pattern string.

Empty Sets
----------

The default constructor creates an empty set:

.. erbsland-demo::
    :source: text/String/CharacterSetEmpty.cpp
    :exec: text/string --demo CharacterSetEmpty
    :source-sha256: 2b6c22a2609886fe643c574d51752616452e59c7bdcac4a3b27ffd2106a55a30

.. code-block:: cpp

    /// The default `CharSet` constructor creates an empty set.
    ///
    /// Empty sets are useful for disabled filters and for policies where no character is allowed.
    void characterSetEmpty() {
        auto disabledFilter = el::CharSet{};
        const auto sample = el::String{"orbite"_el};

        const auto yesNo = el::BooleanFormat::yesNo();
        el::io::printLine("Set is empty ................: "_el, yesNo, disabledFilter.isEmpty());
        el::io::printLine("Sample contains one of set ..: "_el, yesNo, sample.containsOneOf(disabledFilter));
        el::io::printLine("Sample contains only set ....: "_el, yesNo, sample.containsOnly(disabledFilter));
        el::io::printLine("Empty text contains only set : "_el, yesNo, el::String{}.containsOnly(disabledFilter));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Set is empty ................: yes
    Sample contains one of set ..: no
    Sample contains only set ....: no
    Empty text contains only set : yes

.. erbsland-demo-end::

An empty set is useful when a configuration option disables filtering.
It also provides a clear representation for policies where no characters are allowed.

For non-empty strings,
:cpp:func:`containsOnly() <erbsland::text::U8String::containsOnly>` returns
``false`` when the allowed set is empty.
For an empty string, it returns ``true`` because no character violates the rule.

:cpp:func:`containsOneOf() <erbsland::text::U8String::containsOneOf>` always
returns ``false`` for an empty set.

Create Sets from Characters and Ranges
--------------------------------------

Use direct construction for a small set of individual characters.
Use :cpp:func:`CharSet::fromRange() <erbsland::text::CharSet::fromRange>` when two characters describe an inclusive
range and not two separate allowed characters.
:cpp:class:`Char <erbsland::text::Char>` values make that intent explicit:

.. erbsland-demo::
    :source: text/String/CharacterSetCharacters.cpp
    :exec: text/string --demo CharacterSetCharacters
    :source-sha256: 011e7fa61c3e1ddd4ade85fd876448f46ecb1e18f5b1d21f014efa5c153c4b4a

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

.. erbsland-ansi::
    :escape-char: ␛

    Question marker accepts '?' : yes
    Lowercase accepts 'm' ......: yes
    Lowercase accepts 'M' ......: no

.. erbsland-demo-end::

The range factory creates an inclusive range.
Ranges are normalized automatically, and adjacent or overlapping ranges are merged.

Create Sets from Text and Containers
------------------------------------

When you already know the exact characters, construct the set from text or from a container:

.. erbsland-demo::
    :source: text/String/CharacterSetText.cpp
    :exec: text/string --demo CharacterSetText
    :source-sha256: 87e8a4c016e0b6592a3c3a63a34257216e8d7e9ff9cd9b0d29845994483d7601

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

.. erbsland-ansi::
    :escape-char: ␛

    Message contains punctuation : yes
    Message contains separators .: yes
    Separator accepts '/' .......: yes

.. erbsland-demo-end::

Duplicate characters are ignored.
Internally, the resulting set is stored as normalized ranges, so lookup performance is independent of the input order.

Create Sets from Patterns
-------------------------

Use
:cpp:func:`CharSet::fromPattern() <erbsland::text::CharSet::fromPattern>`
for compact literal and range patterns.

The syntax resembles the contents of a regular-expression character class, but it only describes characters and
inclusive ranges.

.. erbsland-demo::
    :source: text/String/CharacterSetPatterns.cpp
    :exec: text/string --demo CharacterSetPatterns
    :source-sha256: a496bc12d33a179e11b6ee488ec382882d5ac2562776b956912a93888ef9c91d

.. code-block:: cpp

    /// `CharSet::fromPattern()` creates compact character sets from literal and range patterns.
    ///
    /// A hyphen between two characters defines a range.
    /// A leading or trailing hyphen is treated as a literal hyphen.
    void characterSetPatterns() {
        auto identifierChars = el::CharSet::fromPattern("_a-zA-Z0-9"_el);
        auto optionNameChars = el::CharSet::fromPattern("-_a-zA-Z0-9"_el);

        const auto stationId = el::String{"ORBIT_07"_el};
        const auto optionName = el::String{"orbite-07"_el};
        const auto spacedName = el::String{"orbite 07"_el};

        const auto yesNo = el::BooleanFormat::yesNo();
        el::io::printLine("Identifier \"", stationId, "\" ....: "_el, yesNo, stationId.containsOnly(identifierChars));
        el::io::printLine("Option \"", optionName, "\" ........: "_el, yesNo, optionName.containsOnly(optionNameChars));
        el::io::printLine("Option \"", spacedName, "\" ........: "_el, yesNo, spacedName.containsOnly(optionNameChars));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Identifier "ORBIT_07" ....: yes
    Option "orbite-07" ........: yes
    Option "orbite 07" ........: no

.. erbsland-demo-end::

A hyphen between two characters defines a range.
A hyphen at the beginning or end of the pattern is treated as a literal hyphen.

Invalid ranges, such as ``z-a``, throw
:cpp:class:`ParseError <erbsland::err::ParseError>`.

Skip the Set When an ASCII Category Is Enough
---------------------------------------------

An :cpp:enum:`AsciiCategory <erbsland::text::AsciiCategory>` is already a complete policy for common ASCII grammar
classes such as digits, hexadecimal digits, words, dotted names, whitespace, and HTTP tokens.
When an API accepts the category directly, constructing a ``CharSet`` first only allocates and builds a value that the
operation does not need.

Use ``Char::isAsciiCategory()`` for one decoded character and ``String::containsOnly(AsciiCategory)`` for a complete
string.
``StringCharReader`` also accepts categories directly in ``readWhile()``, ``readUntil()``, ``advanceWhile()``,
``advanceUntil()``, and the corresponding buffer operations.
These overloads can test the compact category directly and may be faster than a set lookup.

Construct ``CharSet::from(AsciiCategory)`` when the set must be retained, combined with custom characters, or passed to
an operation that only accepts ``CharSet``.

Unicode categories require a set because they expand into ranges drawn from Unicode metadata.
Use
:cpp:func:`CharSet::from(UnicodeCategory) <erbsland::text::CharSet::from>` or
:cpp:func:`CharSet::from(UnicodeCategoryGroup) <erbsland::text::CharSet::from>`
when the policy should follow those standard character classes:

.. erbsland-demo::
    :source: text/String/CharacterSetCategories.cpp
    :exec: text/string --demo CharacterSetCategories
    :source-sha256: 6d37598d5726801dd565c11b4e143c23c94453fb31cda7bd6a2c3e82539f2ed5

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

.. erbsland-ansi::
    :escape-char: ␛

    Packet-id is ASCII word ..: yes
    First letter is uppercase : yes
    Unicode digit accepts '7' : yes
    Unicode letter accepts 'é': yes

.. erbsland-demo-end::

For testing individual characters,
:cpp:class:`Char <erbsland::text::Char>` predicate methods are often the simplest
solution.

Character sets become more useful when scanning strings repeatedly or when you need to combine categories with custom
additions or exclusions.

Compose Larger Policies from Smaller Sets
=========================================

:cpp:class:`CharSet <erbsland::text::CharSet>` supports the usual set operations.

Use the named methods when they make the policy easier to understand:

- :cpp:func:`unitedWith() <erbsland::text::CharSet::unitedWith>`
- :cpp:func:`intersectedWith() <erbsland::text::CharSet::intersectedWith>`
- :cpp:func:`subtractedBy() <erbsland::text::CharSet::subtractedBy>`
- :cpp:func:`symmetricDifferenceWith() <erbsland::text::CharSet::symmetricDifferenceWith>`

Use the operators ``|``, ``&``, ``-``, and ``^`` when the resulting expression remains readable.

.. erbsland-demo::
    :source: text/String/CombineCharacterSets.cpp
    :exec: text/string --demo CombineCharacterSets
    :source-sha256: 2116abf9cf2cb75db9b5a1fd71cc5f2afbf6207b615130f57de71e0fb1295b86

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

        const auto yesNo = el::BooleanFormat::yesNo();
        el::io::printLine("Start policy is subset ....: "_el, yesNo, identifierStart.isSubsetOf(identifierContinue));
        el::io::printLine("First character accepted ..: "_el, yesNo, firstCharacterOk);
        el::io::printLine("Identifier accepted .......: "_el, yesNo, fullIdentifierOk);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Start policy is subset ....: yes
    First character accepted ..: yes
    Identifier accepted .......: yes

.. erbsland-demo-end::

Set comparison is especially useful when part of a policy comes from user configuration.
You can verify that an extension remains within a broader safe set before accepting it.

Ask Whether Text Satisfies a Character Policy
=============================================

Use
:cpp:func:`containsOnly() <erbsland::text::U8String::containsOnly>`
when every character must belong to the allowed set.

Use
:cpp:func:`containsOneOf() <erbsland::text::U8String::containsOneOf>`
when you only need to know whether a string contains at least one character from a set.

.. erbsland-demo::
    :source: text/String/ValidateCharacterPolicy.cpp
    :exec: text/string --demo ValidateCharacterPolicy
    :source-sha256: 17c74a27693bdeb11534decb67c1183531038613516f1b89dc1bb4f8fd16675b

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
            const auto isAccepted =
                userName.isValidUtf8() && userName.containsOnly(userNameChars) && !userName.containsOneOf(forbiddenChars);
            el::io::printLine(userName, " -> "_el, yesNo, isAccepted);
        });
    }

.. erbsland-ansi::
    :escape-char: ␛

    orbite-07 -> yes
    module solaire -> no
    équipe-science -> no

.. erbsland-demo-end::

The method names intentionally express the validation strategy.

``containsOnly(allowed)`` describes an allow-list.
``containsOneOf(forbidden)`` describes a block-list or diagnostic check.

.. erbsland-demo::
    :source: text/String/TestForCharacters.cpp
    :exec: text/string --demo TestForCharacters
    :source-sha256: 7e928d552f63f913ff429cd4813bbec23d3d9c1944cbe9663c31a855337ffe08

.. code-block:: cpp

    /// A validation error that can be thrown by the validation functions.
    class ValidationError : public el::Exception {
    public:
        explicit ValidationError(const el::String &message) : Exception(message) {}
    };

    /// Validate if the given email address is valid.
    void validateEmailAddress(const el::String &emailAddress) {
        static const auto requiredAt = "@"_el;
        static const auto allowedDomainChars = el::CharSet::fromPattern("-a-zA-Z0-9."_el);
        static const auto allowedLocalChars = el::CharSet::fromPattern("-a-zA-Z0-9._+!#$%&'*=?^`{|}~"_el);
        if (!emailAddress.contains(requiredAt)) {
            throw ValidationError{"Email address must contain '@' character."_el};
        }
        if (emailAddress.count(requiredAt) > el::ItemCount{1}) {
            throw ValidationError{"Email address can only contain one '@' character."_el};
        }
        const auto indexOfAt = emailAddress.find(requiredAt);
        const auto domain = emailAddress.slice(el::ByteRange{indexOfAt + requiredAt.length(), el::ByteLength::infinite()});
        if (!domain.containsOnly(allowedDomainChars)) {
            throw ValidationError{"Email domain contains invalid characters."_el};
        }
        if (domain.isEmpty()) {
            throw ValidationError{"Email domain must not be empty."_el};
        }
        const auto local = emailAddress.slice(el::ByteRange{el::ByteIndex::zero(), indexOfAt});
        if (!local.containsOnly(allowedLocalChars)) {
            throw ValidationError{"Email local part contains invalid characters."_el};
        }
        if (local.isEmpty()) {
            throw ValidationError{"Email local part must not be empty."_el};
        }
    }

    /// This demo shows how text, character-set, and accepted-character tests can be used as an efficient input filter.
    void testForCharacters() {
        auto emailAddressesToValidate = el::StringList{
            "tree🌲@forest.org"_el,
            "anna.wald@example.com"_el,
            "river@mountain!.org"_el,
            "maria.silva@green-energy.eu"_el,
            "space in@address.com"_el,
            "takashi.yama@tokyo.jp"_el,
            "wolf@nächtlich.de"_el, // fails: unicode domain
            "luca+weather@forest-mail.net"_el,
            "missing-at-symbol.example.com"_el,
            "fatma+birds@forest.example"_el,
            "alice\nbob@example.com"_el,
            "nora+rain@climate.example"_el,
            "double@@example.com"_el, // fails: repeated at sign
            "forest@domain#name.com"_el,
            "carlos.sunrise@weather.es"_el,
            "@empty-local.org"_el,
            "greta.wind@north-sea.dk"_el,
            "user@exa mple.com"_el,
            "sofia.rivera@biology.org"_el,
            "雨@example.jp"_el, // fails: unicode local part
            "mehmet_istanbul@trees.dev"_el,
            "invalid<char>@example.com"_el,
            "élise@fleurs.fr"_el, // fails: unicode local part
            "empty-domain@"_el,
            "jan.kowalski@oakforest.pl"_el,
        };

        el::io::printLine("Validating all "_el, emailAddressesToValidate.count(), " email addresses:"_el);
        emailAddressesToValidate.forEach([](const el::String &email) {
            el::io::print("- \"", email.toEscaped(el::EscapeFormat::Cpp), "\": "_el);
            try {
                validateEmailAddress(email);
                el::io::printLine("✅"_el);
            } catch (const ValidationError &error) {
                el::io::printLine("❌ "_el, error);
            }
        });
    }

.. erbsland-ansi::
    :escape-char: ␛

    Validating all 25 email addresses:
    - "tree🌲@forest.org": ❌ Email local part contains invalid characters.
    - "anna.wald@example.com": ✅
    - "river@mountain!.org": ❌ Email domain contains invalid characters.
    - "maria.silva@green-energy.eu": ✅
    - "space in@address.com": ❌ Email local part contains invalid characters.
    - "takashi.yama@tokyo.jp": ✅
    - "wolf@nächtlich.de": ❌ Email domain contains invalid characters.
    - "luca+weather@forest-mail.net": ✅
    - "missing-at-symbol.example.com": ❌ Email address must contain '@' character.
    - "fatma+birds@forest.example": ✅
    - "alice\nbob@example.com": ❌ Email local part contains invalid characters.
    - "nora+rain@climate.example": ✅
    - "double@@example.com": ❌ Email address can only contain one '@' character.
    - "forest@domain#name.com": ❌ Email domain contains invalid characters.
    - "carlos.sunrise@weather.es": ✅
    - "@empty-local.org": ❌ Email local part must not be empty.
    - "greta.wind@north-sea.dk": ✅
    - "user@exa mple.com": ❌ Email domain contains invalid characters.
    - "sofia.rivera@biology.org": ✅
    - "雨@example.jp": ❌ Email local part contains invalid characters.
    - "mehmet_istanbul@trees.dev": ✅
    - "invalid<char>@example.com": ❌ Email local part contains invalid characters.
    - "élise@fleurs.fr": ❌ Email local part contains invalid characters.
    - "empty-domain@": ❌ Email domain must not be empty.
    - "jan.kowalski@oakforest.pl": ✅

.. erbsland-demo-end::

Trim a Policy from the Edges
============================

Use :cpp:func:`trimmed() <erbsland::text::U8String::trimmed>` to remove selected characters from the front, the back, or
both sides of a string.

Without arguments, the function trims ASCII whitespace from both ends.
With a custom :cpp:class:`CharSet <erbsland::text::CharSet>`, it trims exactly the characters you specify.

.. erbsland-demo::
    :source: text/String/TrimCharacterSet.cpp
    :exec: text/string --demo TrimCharacterSet
    :source-sha256: cfe48e302d5b50c675dbea438f72e1a8e46de71dadc334cb20cdfe26456b3de2

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

.. erbsland-ansi::
    :escape-char: ␛

    Raw .......: "*** signal-orbite ;; "
    Both sides : "signal-orbite"
    Front only : "signal-orbite ;; "
    Back only .: "*** signal-orbite"

.. erbsland-demo-end::

For read-only strings, trimming returns a narrower owning slice into the original storage.
No new string allocation is required unless you later materialize the result.

Remove Unwanted Characters Throughout the Text
==============================================

Use :cpp:func:`removedAll() <erbsland::text::U8String::removedAll>` when unwanted characters may appear anywhere in the
text:

.. erbsland-demo::
    :source: text/String/RemoveCharacters.cpp
    :exec: text/string --demo RemoveCharacters
    :source-sha256: 14b50e7dea66d8109f9868e58a6465ec7938f07604f4afbdffc0a6d858a18687

.. code-block:: cpp

    /// `String::removedAll()` removes every decoded character from a selected `CharSet`.
    ///
    /// This is useful for simple cleanup passes where unwanted characters may occur anywhere in the text.
    void removeCharacters() {
        static const auto controlChars = el::CharSet::from(el::AsciiCategory::Control);

        const auto input = el::String{"rapport\torbite\nstable"_el};
        auto logLine = input.removedAll(controlChars);

        el::io::printLine("Original : "_el, input.toEscaped(el::EscapeFormat::Cpp));
        el::io::printLine("Cleaned  : "_el, logLine.toEscaped(el::EscapeFormat::Cpp));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Original : rapport\torbite\nstable
    Cleaned  : rapportorbitestable

.. erbsland-demo-end::

This is useful for log cleanup, user-facing diagnostics, and simple format-specific sanitizers.

The operation returns a string value.
If no characters need to be removed, the implementation can reuse the original storage instead of allocating a modified
copy.

Transform the Characters Selected by a Set
==========================================

Use :cpp:func:`transformed() <erbsland::text::U8String::transformed>` when every decoded character should pass through a
mapping function.

For common transformations,
:cpp:class:`Char <erbsland::text::Char>` already provides suitable function
pointers:

- :cpp:func:`Char::caseFolded() <erbsland::text::Char::caseFolded>`
- :cpp:func:`Char::toLowercase() <erbsland::text::Char::toLowercase>`
- :cpp:func:`Char::toUppercase() <erbsland::text::Char::toUppercase>`
- :cpp:func:`Char::toAsciiLowercase() <erbsland::text::Char::toAsciiLowercase>`
- :cpp:func:`Char::toAsciiUppercase() <erbsland::text::Char::toAsciiUppercase>`
- :cpp:func:`Char::toIdentifierNormalized() <erbsland::text::Char::toIdentifierNormalized>`

.. erbsland-demo::
    :source: text/String/CanonicalStationName.cpp
    :exec: text/string --demo CanonicalStationName
    :source-sha256: aff44463e8f64f80450348a4254009fd84d203290f1f3394abcc7cc5e3f51be7

.. code-block:: cpp

    /// `String::transformed()` can create a canonical text form with a single character-mapping function.
    ///
    /// ASCII-only mappings are useful for machine-readable identifiers because they leave non-ASCII characters untouched
    /// and avoid the Unicode database.
    void canonicalStationName() {
        const auto displayName = el::String{"Module ORBITE-Äther 07"_el};
        auto canonicalName = displayName.transformed(el::Char::toAsciiLowercase);

        el::io::printLine("Display name ..: "_el, displayName);
        el::io::printLine("Canonical .....: "_el, canonicalName);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Display name ..: Module ORBITE-Äther 07
    Canonical .....: module orbite-Äther 07

.. erbsland-demo-end::

ASCII-only transformations are useful for machine-readable identifiers because they leave non-ASCII characters untouched
and do not require Unicode case data.

Unicode-aware transformations use simple one-code-point mappings, which keeps the result length predictable.

You can also provide a custom mapping function.
Return the original character to keep it, another character to replace it, or
:cpp:func:`Char::noCodePoint() <erbsland::text::Char::noCodePoint>` to remove it.

.. erbsland-demo::
    :source: text/String/CaseTransformation.cpp
    :exec: text/string --demo CaseTransformation
    :source-sha256: 65419a07d212a93e9c0b368f8e76a80d2bf22dee852e7633c90444a5380b7651

.. code-block:: cpp

    /// `String::transformed()` maps decoded characters into a new string.
    /// You can use Unicode-aware operations, ASCII-only operations, or a custom mapping function.
    /// The original storage can be reused when the transformation does not change the text.
    void caseTransformation() {
        const auto title = el::String{"Forêt d'Été, Σκιερό Μονοπάτι"_el};

        // Normalize display text with Unicode-aware operations.
        el::io::printLine("Original ......: "_el, title);
        el::io::printLine("Lowercase .....: "_el, title.transformed(el::Char::toLowercase));
        el::io::printLine("Uppercase .....: "_el, title.transformed(el::Char::toUppercase));
        el::io::printLine("Case folded ...: "_el, title.transformed(el::Char::caseFolded));

        // Using only ASCII methods can be faster and can avoid linking the Unicode database into the executable.
        const auto sensorName = el::String{"TEMP-ÄSTHETIK-07"_el};
        el::io::printLine("\nSensor name ...: "_el, sensorName);
        el::io::printLine("ASCII lower ...:  "_el, sensorName.transformed(el::Char::toAsciiLowercase));

        // A custom transform can map individual decoded code points.
        const auto quietLabel = el::String{"wind: leise, regen: sanft"_el};
        const auto highlighted = quietLabel.transformed(
            [](const el::Char character) noexcept -> el::Char { return character == U':' ? U'→' : character; });
        el::io::printLine("\nCustom map ..:   "_el, highlighted);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Original ......: Forêt d'Été, Σκιερό Μονοπάτι
    Lowercase .....: forêt d'été, σκιερό μονοπάτι
    Uppercase .....: FORÊT D'ÉTÉ, ΣΚΙΕΡΌ ΜΟΝΟΠΆΤΙ
    Case folded ...: forêt d'été, σκιερό μονοπάτι

    Sensor name ...: TEMP-ÄSTHETIK-07
    ASCII lower ...:  temp-Ästhetik-07

    Custom map ..:   wind→ leise, regen→ sanft

.. erbsland-demo-end::
