// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineCaptureTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testCaptureOneChar() {
        WITH_CONTEXT(assembleProgram({
            "          .groups 1",
            "          START CAPTURE 0",
            "          ANY",
            "          STOP CAPTURE 0",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("abc"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0001 'a'",
            "01: 0000-0001 'a'",
        }));
        WITH_CONTEXT(requireNoMatch(""_el));
    }

    void testNestedCaptures() {
        WITH_CONTEXT(assembleProgram({
            "          .groups 15",
            "          START CAPTURE 0",
            "          START CAPTURE 1",
            "          START CAPTURE 2",
            "          START CAPTURE 3",
            "          START CAPTURE 4",
            "          START CAPTURE 5",
            "          START CAPTURE 6",
            "          START CAPTURE 7",
            "          START CAPTURE 8",
            "          START CAPTURE 9",
            "          START CAPTURE 10",
            "          START CAPTURE 11",
            "          START CAPTURE 12",
            "          START CAPTURE 13",
            "          START CAPTURE 14",
            "          ANY",
            "          STOP CAPTURE 14",
            "          ANY",
            "          STOP CAPTURE 13",
            "          ANY",
            "          STOP CAPTURE 12",
            "          ANY",
            "          STOP CAPTURE 11",
            "          ANY",
            "          STOP CAPTURE 10",
            "          ANY",
            "          STOP CAPTURE 9",
            "          ANY",
            "          STOP CAPTURE 8",
            "          ANY",
            "          STOP CAPTURE 7",
            "          ANY",
            "          STOP CAPTURE 6",
            "          ANY",
            "          STOP CAPTURE 5",
            "          ANY",
            "          STOP CAPTURE 4",
            "          ANY",
            "          STOP CAPTURE 3",
            "          ANY",
            "          STOP CAPTURE 2",
            "          ANY",
            "          STOP CAPTURE 1",
            "          ANY",
            "          STOP CAPTURE 0",
            "          ANY",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("abcdefghijklmnopqrestuvwxyz"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0016 'abcdefghijklmnop'",
            "01: 0000-0015 'abcdefghijklmno'",
            "02: 0000-0014 'abcdefghijklmn'",
            "03: 0000-0013 'abcdefghijklm'",
            "04: 0000-0012 'abcdefghijkl'",
            "05: 0000-0011 'abcdefghijk'",
            "06: 0000-0010 'abcdefghij'",
            "07: 0000-0009 'abcdefghi'",
            "08: 0000-0008 'abcdefgh'",
            "09: 0000-0007 'abcdefg'",
            "10: 0000-0006 'abcdef'",
            "11: 0000-0005 'abcde'",
            "12: 0000-0004 'abcd'",
            "13: 0000-0003 'abc'",
            "14: 0000-0002 'ab'",
            "15: 0000-0001 'a'",
        }));
        WITH_CONTEXT(requireMatch("😀😃😄😁→⇒⇐←ÀƊËḦ012345"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0041 '😀😃😄😁→⇒⇐←ÀƊËḦ0123'",
            "01: 0000-0040 '😀😃😄😁→⇒⇐←ÀƊËḦ012'",
            "02: 0000-0039 '😀😃😄😁→⇒⇐←ÀƊËḦ01'",
            "03: 0000-0038 '😀😃😄😁→⇒⇐←ÀƊËḦ0'",
            "04: 0000-0037 '😀😃😄😁→⇒⇐←ÀƊËḦ'",
            "05: 0000-0034 '😀😃😄😁→⇒⇐←ÀƊË'",
            "06: 0000-0032 '😀😃😄😁→⇒⇐←ÀƊ'",
            "07: 0000-0030 '😀😃😄😁→⇒⇐←À'",
            "08: 0000-0028 '😀😃😄😁→⇒⇐←'",
            "09: 0000-0025 '😀😃😄😁→⇒⇐'",
            "10: 0000-0022 '😀😃😄😁→⇒'",
            "11: 0000-0019 '😀😃😄😁→'",
            "12: 0000-0016 '😀😃😄😁'",
            "13: 0000-0012 '😀😃😄'",
            "14: 0000-0008 '😀😃'",
            "15: 0000-0004 '😀'",
        }));
        WITH_CONTEXT(requireNoMatch(""_el));
        WITH_CONTEXT(requireNoMatch("a"_el));
        WITH_CONTEXT(requireNoMatch("abc"_el));
        WITH_CONTEXT(requireNoMatch("abcdefghijklmno"_el));
    }

    void testTwoPaths() {
        // The first path has priority.
        WITH_CONTEXT(assembleProgram({
            "          .groups 2",
            "          SPLIT %path1, %path2",
            "path1:    START CAPTURE 0",
            "          ANY",
            "          STOP CAPTURE 0",
            "          CHAR 'a'",
            "          JUMP %end",
            "path2:    START CAPTURE 1",
            "          ANY",
            "          ANY",
            "          STOP CAPTURE 1",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("aaa"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0002 'aa'",
            "01: 0000-0001 'a'",
            "02: 0000-0000 ''",
        }));
        WITH_CONTEXT(requireMatch("bbb"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0002 'bb'",
            "01: 0000-0000 ''",
            "02: 0000-0002 'bb'",
        }));
    }

    void testNonGreedy() {
        // Test for regex pattern: .*?/(.*?)/'
        // This matches: any chars (lazy), /, capture group (lazy), /
        WITH_CONTEXT(assembleProgram({
            "           .groups 1",
            "loop:      SPLIT %quote, %any",
            "any:       ANY",
            "           JUMP %loop",
            "quote:     CHAR '/'",
            "           START CAPTURE 0",
            "inner:     SPLIT %end_cap, %any_inner",
            "any_inner: ANY",
            "           JUMP %inner",
            "end_cap:   STOP CAPTURE 0",
            "           CHAR '/'",
            "end:       MATCH",
        }));
        WITH_CONTEXT(requireMatch("test /hello/ world"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0012 'test /hello/'",
            "01: 0006-0011 'hello'",
        }));
        WITH_CONTEXT(requireMatch("/foo/"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0005 '/foo/'",
            "01: 0001-0004 'foo'",
        }));
        WITH_CONTEXT(requireMatch("abc/x/def/y/ghi"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0006 'abc/x/'",
            "01: 0004-0005 'x'",
        }));
        WITH_CONTEXT(requireMatch("before/😀😃/after"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0016 'before/😀😃/'",
            "01: 0007-0015 '😀😃'",
        }));
        WITH_CONTEXT(requireNoMatch(""_el));
        WITH_CONTEXT(requireNoMatch("no quotes here"_el));
        WITH_CONTEXT(requireNoMatch("only/one"_el));
    }

    void testGreedy() {
        // Test for regex pattern: .*/(.*)/'
        // This matches: any chars (greedy), /, capture group (greedy), /
        WITH_CONTEXT(assembleProgram({
            "           .groups 1",
            "loop:      SPLIT %any, %quote",
            "any:       ANY",
            "           JUMP %loop",
            "quote:     CHAR '/'",
            "           START CAPTURE 0",
            "inner:     SPLIT %any_inner, %end_cap",
            "any_inner: ANY",
            "           JUMP %inner",
            "end_cap:   STOP CAPTURE 0",
            "           CHAR '/'",
            "end:       MATCH",
        }));
        WITH_CONTEXT(requireMatch("test /hello/ world"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0012 'test /hello/'",
            "01: 0006-0011 'hello'",
        }));
        WITH_CONTEXT(requireMatch("/foo/"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0005 '/foo/'",
            "01: 0001-0004 'foo'",
        }));
        WITH_CONTEXT(requireMatch("abc/x/def/y/ghi"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0012 'abc/x/def/y/'",
            "01: 0010-0011 'y'",
        }));
        WITH_CONTEXT(requireMatch("before/😀😃/after"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0016 'before/😀😃/'",
            "01: 0007-0015 '😀😃'",
        }));
        WITH_CONTEXT(requireNoMatch(""_el));
        WITH_CONTEXT(requireNoMatch("no quotes here"_el));
        WITH_CONTEXT(requireNoMatch("only/one"_el));
    }

    void testLazyAndGreedy() {
        // Test for regex pattern: .*?/(.*)/'
        // This matches: any chars (lazy), /, capture group (greedy), /
        WITH_CONTEXT(assembleProgram({
            "           .groups 1",
            "loop:      SPLIT %quote, %any",
            "any:       ANY",
            "           JUMP %loop",
            "quote:     CHAR '/'",
            "           START CAPTURE 0",
            "inner:     SPLIT %any_inner, %end_cap",
            "any_inner: ANY",
            "           JUMP %inner",
            "end_cap:   STOP CAPTURE 0",
            "           CHAR '/'",
            "end:       MATCH",
        }));
        WITH_CONTEXT(requireMatch("test /hello/ world"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0012 'test /hello/'",
            "01: 0006-0011 'hello'",
        }));
        WITH_CONTEXT(requireMatch("/foo/"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0005 '/foo/'",
            "01: 0001-0004 'foo'",
        }));
        WITH_CONTEXT(requireMatch("abc/x/def/y/ghi"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0012 'abc/x/def/y/'",
            "01: 0004-0011 'x/def/y'",
        }));
        WITH_CONTEXT(requireMatch("before/😀😃/after"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0016 'before/😀😃/'",
            "01: 0007-0015 '😀😃'",
        }));
        WITH_CONTEXT(requireNoMatch(""_el));
        WITH_CONTEXT(requireNoMatch("no quotes here"_el));
        WITH_CONTEXT(requireNoMatch("only/one"_el));
    }

    void testGreedyAndLazy() {
        // Test for regex pattern: .*/(.*?)/'
        // This matches: any chars (greedy), /, capture group (lazy), /
        WITH_CONTEXT(assembleProgram({
            "           .groups 1",
            "loop:      SPLIT %any, %quote",
            "any:       ANY",
            "           JUMP %loop",
            "quote:     CHAR '/'",
            "           START CAPTURE 0",
            "inner:     SPLIT %end_cap, %any_inner",
            "any_inner: ANY",
            "           JUMP %inner",
            "end_cap:   STOP CAPTURE 0",
            "           CHAR '/'",
            "end:       MATCH",
        }));
        WITH_CONTEXT(requireMatch("test /hello/ world"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0012 'test /hello/'",
            "01: 0006-0011 'hello'",
        }));
        WITH_CONTEXT(requireMatch("/foo/"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0005 '/foo/'",
            "01: 0001-0004 'foo'",
        }));
        WITH_CONTEXT(requireMatch("abc/x/def/y/ghi"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0012 'abc/x/def/y/'",
            "01: 0010-0011 'y'",
        }));
        WITH_CONTEXT(requireMatch("before/😀😃/after"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0016 'before/😀😃/'",
            "01: 0007-0015 '😀😃'",
        }));
        WITH_CONTEXT(requireNoMatch(""_el));
        WITH_CONTEXT(requireNoMatch("no quotes here"_el));
        WITH_CONTEXT(requireNoMatch("only/one"_el));
    }
};
