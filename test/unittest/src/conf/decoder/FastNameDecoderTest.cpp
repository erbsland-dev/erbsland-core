// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/impl/char/NamedChars.hpp>
#include <erbsland/conf/impl/decoder/FastNameDecoder.hpp>
#include <erbsland/conf/Source.hpp>
#include <erbsland/conf/StdFormatForConf.hpp>

using namespace el::conf;
using el::conf::impl::FastNameDecoder;
using el::conf::impl::internalView;
using namespace el::text::literals;
namespace nc = el::conf::impl::nc;

TESTED_TARGETS(FastNameDecoder)
class FastNameDecoderTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    void requireAndNext(FastNameDecoder &decoder, const char32_t expectedUnicode) {
        REQUIRE(decoder.character() == expectedUnicode);
        decoder.next();
    }

    void requireEndOfData(FastNameDecoder &decoder) { REQUIRE(decoder.character().isEndOfData()); }

    void testSimpleSequence() {
        FastNameDecoder decoder("abc\n😀\nxyz"_el);
        decoder.initialize();
        WITH_CONTEXT(requireAndNext(decoder, U'a'));
        WITH_CONTEXT(requireAndNext(decoder, U'b'));
        WITH_CONTEXT(requireAndNext(decoder, U'c'));
        WITH_CONTEXT(requireAndNext(decoder, U'\n'));
        WITH_CONTEXT(requireAndNext(decoder, U'😀'));
        WITH_CONTEXT(requireAndNext(decoder, U'\n'));
        WITH_CONTEXT(requireAndNext(decoder, U'x'));
        WITH_CONTEXT(requireAndNext(decoder, U'y'));
        WITH_CONTEXT(requireAndNext(decoder, U'z'));
        WITH_CONTEXT(requireEndOfData(decoder));
    }

    void testTransactionFromStart() {
        FastNameDecoder decoder("abcdef"_el);
        decoder.initialize();
        {
            auto transaction = el::conf::impl::Transaction{decoder};
            WITH_CONTEXT(requireAndNext(decoder, U'a'));
            WITH_CONTEXT(requireAndNext(decoder, U'b'));
            WITH_CONTEXT(requireAndNext(decoder, U'c'));
            REQUIRE_EQUAL(transaction.capturedString(), "abc"_el);
            REQUIRE_EQUAL(transaction.capturedSize(), 3);
        } // rollback
        {
            auto transaction = el::conf::impl::Transaction{decoder};
            WITH_CONTEXT(requireAndNext(decoder, U'a'));
            WITH_CONTEXT(requireAndNext(decoder, U'b'));
            REQUIRE_EQUAL(transaction.capturedString(), "ab"_el);
            REQUIRE_EQUAL(transaction.capturedSize(), 2);
        } // rollback
        {
            auto transaction = el::conf::impl::Transaction{decoder};
            WITH_CONTEXT(requireAndNext(decoder, U'a'));
            WITH_CONTEXT(requireAndNext(decoder, U'b'));
            WITH_CONTEXT(requireAndNext(decoder, U'c'));
            WITH_CONTEXT(requireAndNext(decoder, U'd'));
            REQUIRE_EQUAL(transaction.capturedString(), "abcd"_el);
            REQUIRE_EQUAL(transaction.capturedSize(), 4);
        } // rollback
        WITH_CONTEXT(requireAndNext(decoder, U'a'));
        WITH_CONTEXT(requireAndNext(decoder, U'b'));
        {
            auto transaction = el::conf::impl::Transaction{decoder};
            WITH_CONTEXT(requireAndNext(decoder, U'c'));
            WITH_CONTEXT(requireAndNext(decoder, U'd'));
            WITH_CONTEXT(requireAndNext(decoder, U'e'));
            WITH_CONTEXT(requireAndNext(decoder, U'f'));
            REQUIRE_EQUAL(transaction.capturedString(), "cdef"_el);
            REQUIRE_EQUAL(transaction.capturedSize(), 4);
        } // rollback
        {
            auto transaction = el::conf::impl::Transaction{decoder};
            WITH_CONTEXT(requireAndNext(decoder, U'c'));
            WITH_CONTEXT(requireAndNext(decoder, U'd'));
            REQUIRE_EQUAL(transaction.capturedString(), "cd"_el);
            REQUIRE_EQUAL(transaction.capturedSize(), 2);
            transaction.commit();
        } // no rollback
        WITH_CONTEXT(requireAndNext(decoder, U'e'));
        WITH_CONTEXT(requireAndNext(decoder, U'f'));
        WITH_CONTEXT(requireEndOfData(decoder));
    }

    void testMultibyteCaptureAndEndRollback() {
        FastNameDecoder decoder(u8"A😀β"_el);
        decoder.initialize();
        {
            auto transaction = el::conf::impl::Transaction{decoder};
            WITH_CONTEXT(requireAndNext(decoder, U'A'));
            WITH_CONTEXT(requireAndNext(decoder, U'😀'));
            REQUIRE_EQUAL(transaction.capturedString(), u8"A😀"_el);
            REQUIRE_EQUAL(transaction.capturedSize(), std::size_t{2U});
            REQUIRE_EQUAL(decoder.location().codeLocation().column(), el::unit::ColumnIndex{2U});
            WITH_CONTEXT(requireAndNext(decoder, U'β'));
            WITH_CONTEXT(requireEndOfData(decoder));
            REQUIRE_EQUAL(transaction.capturedString(), u8"A😀β"_el);
            REQUIRE_EQUAL(transaction.capturedSize(), std::size_t{3U});
        }
        REQUIRE_EQUAL(decoder.location().codeLocation().column(), el::unit::ColumnIndex::zero());
        WITH_CONTEXT(requireAndNext(decoder, U'A'));
        WITH_CONTEXT(requireAndNext(decoder, U'😀'));
        WITH_CONTEXT(requireAndNext(decoder, U'β'));
        WITH_CONTEXT(requireEndOfData(decoder));
    }

    void testNestedTransactions1() {
        FastNameDecoder decoder("abcdef"_el);
        decoder.initialize();
        {
            auto transaction1 = el::conf::impl::Transaction{decoder};
            WITH_CONTEXT(requireAndNext(decoder, U'a'));
            WITH_CONTEXT(requireAndNext(decoder, U'b'));
            REQUIRE_EQUAL(transaction1.capturedString(), "ab"_el);
            REQUIRE_EQUAL(transaction1.capturedSize(), 2);
            {
                auto transaction2 = el::conf::impl::Transaction{decoder};
                WITH_CONTEXT(requireAndNext(decoder, U'c'));
                WITH_CONTEXT(requireAndNext(decoder, U'd'));
                REQUIRE_EQUAL(transaction2.capturedString(), "cd"_el);
                REQUIRE_EQUAL(transaction2.capturedSize(), 2);
                {
                    auto transaction3 = el::conf::impl::Transaction{decoder};
                    WITH_CONTEXT(requireAndNext(decoder, U'e'));
                    WITH_CONTEXT(requireAndNext(decoder, U'f'));
                    REQUIRE_EQUAL(transaction3.capturedString(), "ef"_el);
                    REQUIRE_EQUAL(transaction3.capturedSize(), 2);
                    transaction3.commit();
                }
                REQUIRE_EQUAL(transaction2.capturedString(), "cdef"_el);
                REQUIRE_EQUAL(transaction2.capturedSize(), 4);
                transaction2.commit();
            }
            REQUIRE_EQUAL(transaction1.capturedString(), "abcdef"_el);
            REQUIRE_EQUAL(transaction1.capturedSize(), 6);
            transaction1.commit();
        }
        WITH_CONTEXT(requireEndOfData(decoder));
    }

    void testNestedTransactions2() {
        FastNameDecoder decoder("abcdef"_el);
        decoder.initialize();
        {
            auto transaction1 = el::conf::impl::Transaction{decoder};
            WITH_CONTEXT(requireAndNext(decoder, U'a'));
            WITH_CONTEXT(requireAndNext(decoder, U'b'));
            REQUIRE_EQUAL(transaction1.capturedString(), "ab"_el);
            REQUIRE_EQUAL(transaction1.capturedSize(), 2);
            {
                auto transaction2 = el::conf::impl::Transaction{decoder};
                WITH_CONTEXT(requireAndNext(decoder, U'c'));
                WITH_CONTEXT(requireAndNext(decoder, U'd'));
                REQUIRE_EQUAL(transaction2.capturedString(), "cd"_el);
                REQUIRE_EQUAL(transaction2.capturedSize(), 2);
                {
                    auto transaction3 = el::conf::impl::Transaction{decoder};
                    WITH_CONTEXT(requireAndNext(decoder, U'e'));
                    WITH_CONTEXT(requireAndNext(decoder, U'f'));
                    REQUIRE_EQUAL(transaction3.capturedString(), "ef"_el);
                    REQUIRE_EQUAL(transaction3.capturedSize(), 2);
                    transaction3.commit();
                }
                REQUIRE_EQUAL(transaction2.capturedString(), "cdef"_el);
                REQUIRE_EQUAL(transaction2.capturedSize(), 4);
                // ROLLBACK
            }
            WITH_CONTEXT(requireAndNext(decoder, U'c'));
            WITH_CONTEXT(requireAndNext(decoder, U'd'));
            {
                auto transaction2 = el::conf::impl::Transaction{decoder};
                WITH_CONTEXT(requireAndNext(decoder, U'e'));
                WITH_CONTEXT(requireAndNext(decoder, U'f'));
                REQUIRE_EQUAL(transaction2.capturedString(), "ef"_el);
                REQUIRE_EQUAL(transaction2.capturedSize(), 2);
                // ROLLBACK
            }
            WITH_CONTEXT(requireAndNext(decoder, U'e'));
            WITH_CONTEXT(requireAndNext(decoder, U'f'));
            REQUIRE_EQUAL(transaction1.capturedString(), "abcdef"_el);
            REQUIRE_EQUAL(transaction1.capturedSize(), 6);
            transaction1.commit();
        }
        WITH_CONTEXT(requireEndOfData(decoder));
    }
};
