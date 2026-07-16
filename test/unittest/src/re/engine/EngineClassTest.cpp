// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineClassTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testOneCharClass() {
        WITH_CONTEXT(assembleProgram({
            "           .section &class",
            "class1:    .class           ; starts a new class",
            "               .data 'a'    ; adds the range a-a to the class",
            "           .section &program",
            "           CLASS %class1",
            "end:       MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireMatch("abc"_el, 1));
        WITH_CONTEXT(requireNoMatch(""_el));
        WITH_CONTEXT(requireNoMatch("A"_el));
        WITH_CONTEXT(requireNoMatch("ä"_el));
        WITH_CONTEXT(requireNoMatch("Ä"_el));
        WITH_CONTEXT(requireNoMatch("z"_el));
        WITH_CONTEXT(requireNoMatch("😀"_el));
    }

    void testOneCiCharClass() {
        WITH_CONTEXT(assembleProgram({
            "           .section &class",
            "class1:    .class           ; starts a new class",
            "               .data 'a'    ; adds the range a-a to the class",
            "           .section &program",
            "           CI CLASS %class1 ; CI = Case Insensitive",
            "end:       MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireMatch("A"_el, 1));
        WITH_CONTEXT(requireMatch("abc"_el, 1));
        WITH_CONTEXT(requireMatch("ABC"_el, 1));
        WITH_CONTEXT(requireNoMatch(""_el));
        WITH_CONTEXT(requireNoMatch("z"_el));
        WITH_CONTEXT(requireNoMatch("Ä"_el));
        WITH_CONTEXT(requireNoMatch("😀"_el));
    }

    void testOneNotCharClass() {
        WITH_CONTEXT(assembleProgram({
            "           .section &class",
            "class1:    .class           ; starts a new class",
            "               .data 'a'    ; adds the range a-a to the class",
            "           .section &program",
            "           NOT CLASS %class1 ; NOT = Negation",
            "end:       MATCH",
        }));
        WITH_CONTEXT(requireMatch("z"_el, 1));
        WITH_CONTEXT(requireMatch("A"_el, 1));
        WITH_CONTEXT(requireMatch("Ä"_el, 2));
        WITH_CONTEXT(requireMatch("😀"_el, 4));
        WITH_CONTEXT(requireMatch("ABC"_el, 1));
        WITH_CONTEXT(requireNoMatch("a"_el));
        WITH_CONTEXT(requireNoMatch("abc"_el));
        WITH_CONTEXT(requireNoMatch(""_el));
    }

    void testOneNotCiCharClass() {
        WITH_CONTEXT(assembleProgram({
            "           .section &class",
            "class1:    .class           ; starts a new class",
            "               .data 'a'    ; adds the range a-a to the class",
            "           .section &program",
            "           NOT CI CLASS %class1 ; NOT = Negation, CI = Case Insensitive",
            "end:       MATCH",
        }));
        WITH_CONTEXT(requireMatch("z"_el, 1));
        WITH_CONTEXT(requireMatch("Ä"_el, 2));
        WITH_CONTEXT(requireMatch("😀"_el, 4));
        WITH_CONTEXT(requireNoMatch("a"_el));
        WITH_CONTEXT(requireNoMatch("A"_el));
        WITH_CONTEXT(requireNoMatch("abc"_el));
        WITH_CONTEXT(requireNoMatch("ABC"_el));
        WITH_CONTEXT(requireNoMatch(""_el));
    }

    void testManyClasses() {
        WITH_CONTEXT(assembleProgram({
            "           .section &class",
            "class1:    .class           ; starts a new class",
            "               .data 'a'    ; adds the range a-a to the class",
            "               .data 'x'    ; adds the range a-a to the class",
            "               .data '_'    ; adds the range a-a to the class",
            "class2:    .class",
            "               .data 128515 ; 😃",
            "class3:    .class",
            "               .data $2190  ; ←",
            "               .data $2191  ; ↑",
            "               .data $2192  ; →",
            "               .data $2193  ; ↓",
            "class4:    .class",
            "               .data 65-90  ; A-Z",
            "class5:    .class",
            "               .data 'a'-'z'",
            "class6:    .class",
            "               .data \"'123456789\"",
            "           .section &program",
            "           CLASS %class6",
            "           CLASS %class5",
            "           CLASS %class4",
            "           CLASS %class3",
            "           CLASS %class2",
            "           CLASS %class1",
            "end:       MATCH",
        }));
        WITH_CONTEXT(requireMatch("8fX→😃a"_el, 11));
        WITH_CONTEXT(requireMatch("'zQ↓😃_"_el, 11));
        WITH_CONTEXT(requireMatch("1aA←😃x"_el, 11));

        WITH_CONTEXT(requireNoMatch(""_el));
        WITH_CONTEXT(requireNoMatch("8fX→😃"_el));  // missing last class
        WITH_CONTEXT(requireNoMatch("0fX→😃a"_el)); // class6 doesn't include '0'
        WITH_CONTEXT(requireNoMatch("8FX→😃a"_el)); // class5 is lowercase only
        WITH_CONTEXT(requireNoMatch("8f0→😃a"_el)); // class4 is A-Z only
        WITH_CONTEXT(requireNoMatch("8fX↔😃a"_el)); // arrow not in class3
        WITH_CONTEXT(requireNoMatch("8fX→😀a"_el)); // emoji not in class2
        WITH_CONTEXT(requireNoMatch("8fX→😃b"_el)); // class1 is a/x/_ only
    }

    void testCaseFoldedClassDefinition_WithCiAndNot() {
        // Note: For CI, the class *definition* must already be case-folded.
        // Here we define the class using only lowercase characters.
        WITH_CONTEXT(assembleProgram({
            "           .section &class",
            "folded:    .class",
            "               .data 'a'-'z'",
            "               .data $00E4       ; ä",
            "           .section &program",
            "           CLASS %folded",
            "           CI CLASS %folded",
            "           NOT CLASS %folded",
            "           NOT CI CLASS %folded",
            "end:       MATCH",
        }));

        // 1) CLASS: must match only lowercase (case-sensitive).
        // 2) CI CLASS: must match uppercase too.
        // 3) NOT CLASS: must treat uppercase as different.
        // 4) NOT CI CLASS: must treat uppercase as equal after case-folding.
        WITH_CONTEXT(requireMatch("aAA0"_el, 4));
        WITH_CONTEXT(requireNoMatch("aAAA"_el));
        WITH_CONTEXT(requireNoMatch("AAAA"_el));

        // Same idea with a non-ASCII character that folds to lowercase.
        WITH_CONTEXT(requireMatch("äÄÄ0"_el, 7));
        WITH_CONTEXT(requireNoMatch("äÄÄÄ"_el));
    }

    void testSameClassDefinition_CanBeUsedWithAnyModifier_NoInference() {
        // This is a regression-style test: the engine must not "infer" or cache the `CI`/`NOT` flags into
        // the class definition itself.
        //
        // If `CI CLASS` would permanently mark the class as case-insensitive, the subsequent `CLASS` or
        // `NOT CI CLASS` instructions could behave incorrectly.
        WITH_CONTEXT(assembleProgram({
            "           .section &class",
            "c:         .class",
            "               .data 'a'",
            "           .section &program",
            "           CLASS %c",        // must *not* match 'A'
            "           CI CLASS %c",     // must match 'A'
            "           NOT CLASS %c",    // must match 'A'
            "           NOT CI CLASS %c", // must *not* match 'A'
            "end:       MATCH",
        }));
        WITH_CONTEXT(requireMatch("aAAz"_el, 4));
        WITH_CONTEXT(requireNoMatch("aAAA"_el));
    }
};
