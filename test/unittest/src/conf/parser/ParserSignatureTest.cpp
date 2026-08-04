// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserTestHelper.hpp"

#include <erbsland/conf/impl/lexer/Lexer.hpp>
#include <erbsland/conf/SignatureSigner.hpp>
#include <erbsland/conf/SignatureValidator.hpp>
#include <erbsland/conf/Signer.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/text/StringCharReader.hpp>
#include <erbsland/text/StringEditor.hpp>

#include <algorithm>
#include <ranges>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(Parser)
class ParserSignatureTest final : public UNITTEST_SUBCLASS(ParserTestHelper) {
public:
    void setUp() override { clearMocks(); }

    void tearDown() override {
        cleanUpTestFileDirectory();
        doc = {};
    }

    // IMPORTANT: This is no valid example how validation should work!
    class MockSignatureValidator final : public SignatureValidator {
    public:
        auto unshiftString(const el::text::String &str) -> el::text::String {
            auto reader = el::text::StringCharReader{str};
            auto result = el::text::StringEditor{};
            while (!reader.isAtEnd()) {
                result.append(el::text::Char{reader.read().toRawValue() - static_cast<char32_t>(1)});
            }
            return result;
        }

        [[nodiscard]] auto validate(const SignatureValidatorData &data) -> SignatureValidatorResult override {
            log.push_back("validate");
            if (data.signatureText.isEmpty()) {
                log.push_back("reject");
                return SignatureValidatorResult::Reject;
            }
            auto reader = el::text::StringCharReader{data.signatureText};
            auto foundSeparator = false;
            while (!reader.isAtEnd()) {
                if (reader.read() == el::text::Char{U';'}) {
                    foundSeparator = true;
                    break;
                }
            }
            if (!foundSeparator) {
                log.push_back("reject");
                return SignatureValidatorResult::Reject;
            }
            reader.startCapture();
            while (!reader.isAtEnd()) {
                reader.advance();
            }
            auto digest = unshiftString(reader.takeCapture().toU8String());
            if (digest != data.documentDigest) {
                log.push_back("reject");
                return SignatureValidatorResult::Reject;
            }
            log.push_back("accept");
            return SignatureValidatorResult::Accept;
        }

        auto require(const std::string &logEntry) noexcept -> bool {
            return std::ranges::find(log, logEntry) != log.end();
        }

        std::vector<std::string> log;
    };

    // IMPORTANT: This is no valid example how signing should work!
    class MockSignatureSigner final : public SignatureSigner {
    public:
        auto shiftString(const el::text::String &str) -> el::text::String {
            auto reader = el::text::StringCharReader{str};
            auto result = el::text::StringEditor{};
            while (!reader.isAtEnd()) {
                result.append(el::text::Char{reader.read().toRawValue() + static_cast<char32_t>(1)});
            }
            return result;
        }

        [[nodiscard]] auto sign(const SignatureSignerData &data) -> el::text::String override {
            log.push_back("sign");
            el::text::StringEditor signature;
            signature.append(data.signingPersonText);
            signature.append(el::text::String{";"_el});
            signature.append(shiftString(data.documentDigest));
            return signature;
        }

        auto require(const std::string &logEntry) noexcept -> bool {
            return std::ranges::find(log, logEntry) != log.end();
        }

        std::vector<std::string> log;
    };

    std::shared_ptr<MockSignatureValidator> validator = std::make_shared<MockSignatureValidator>();
    std::shared_ptr<MockSignatureSigner> signatureSigner = std::make_shared<MockSignatureSigner>();

    void clearMocks() {
        validator->log.clear();
        signatureSigner->log.clear();
    }

    void testSignatureCycle() {
        // Make sure the premises of this test are correct.
        const auto hashAlgorithm = el::conf::impl::Lexer::hashAlgorithm();
        REQUIRE_EQUAL(hashAlgorithm, el::cryptology::HashAlgorithm::Sha3_256);

        // Prepare the data
        auto unsignedPath = createTestFile("config/unsigned.elcl", "[main]\nvalue: 123\n"_el);
        auto expectedDigest = // SHA3_256
            bytesFromHex("d615780d1836a0189dc5c826f4ef6bfbbf9cc33b78d07b7a1f459c627cec1b81"_el);

        // Verify the document digest.
        el::cryptology::Hasher hash(el::cryptology::HashAlgorithm::Sha3_256);
        std::ifstream file(unsignedPath, std::ios::binary);
        REQUIRE(file.is_open());
        std::string fileContents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        REQUIRE_EQUAL(fileContents.size(), 18);
        hash.update(el::mem::toConstByteSpan(std::span<const char>{fileContents}));
        auto actualDigest = hash.finalize();
        REQUIRE_EQUAL(actualDigest, expectedDigest);

        // Sign the document.
        auto signedPath = unsignedPath.parent_path() / "signed.elcl";
        Signer signer{signatureSigner};
        REQUIRE_NOTHROW(signer.sign(el::path::Path{unsignedPath}, el::path::Path{signedPath}, "test@example.com"_el));
        REQUIRE(exists(signedPath));
        REQUIRE(is_regular_file(signedPath));
        REQUIRE(signatureSigner->require("sign"));

        // Parse the documents.
        Parser parser;
        // reading the unsigned file should be fine.
        REQUIRE_NOTHROW(doc = parser.parseOrThrow(Source::fromFile(el::path::Path{unsignedPath})));
        REQUIRE(doc);
        // reading the signed must fail.
        REQUIRE_THROWS(doc = parser.parseOrThrow(Source::fromFile(el::path::Path{signedPath})));
        REQUIRE(doc);
        // setting the validator must reject unsigned documents.
        parser.setSignatureValidator(validator);
        REQUIRE_THROWS(doc = parser.parseOrThrow(Source::fromFile(el::path::Path{unsignedPath})));
        // now, reading the signed document should work as expected.
        REQUIRE_NOTHROW(doc = parser.parseOrThrow(Source::fromFile(el::path::Path{signedPath})));
        REQUIRE(doc);
    }
};
