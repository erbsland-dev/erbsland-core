// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/HttpServer.hpp>
#include <erbsland/HttpServerRequest.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringTransformations.hpp>
#include <erbsland/UnmanagedEventThread.hpp>
#include <tool/frame/TextChecksum.hpp>
#include <tool/test_data/CryptologyVectorsTool.hpp>
#include <tool/test_data/impl/VectorConverter.hpp>

#include <atomic>
#include <future>

TESTED_TARGETS(CryptologyVectorsTool VectorConverter)
class CryptologyVectorsToolTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testResponseNormalizationAndCountsLight() {
        using tool::test_data::impl::VectorConverter;
        REQUIRE_EQUAL(VectorConverter::normalize("Count = 0 \t\r\nMsg = \r\n\r\n"_el), "Count = 0\nMsg =\n\n"_el);
        const auto [records, groups] =
            VectorConverter::counts("aes/test.rsp"_el, "[ENCRYPT]\r\nCOUNT = 0\r\n[DECRYPT]\rCOUNT = 1\n"_el);
        REQUIRE_EQUAL(records, 2);
        REQUIRE_EQUAL(groups, 2);
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testOfflineSelectionCheckAndPartialManifest() {
        prepare();
        // Every configured URL deliberately points to a closed local port. Offline imports must still work.
        REQUIRE(run(el::StringList{"--check"_el}) != 0);
        REQUIRE_FALSE((_root / "out"_el).info().exists());
        write("out/sha/retained.rsp"_el, "[L = 1]\r\nMD = aa\r\n"_el);
        write("out/unrelated.txt"_el, "keep"_el);
        REQUIRE_EQUAL(run(), 0);
        const auto manifest = files().readText(el::Path{"out/manifest.txt"_el});
        REQUIRE(manifest.contains("hash/BLAKE2b.rsp"_el));
        REQUIRE(manifest.contains("sha/retained.rsp"_el));
        REQUIRE_FALSE(manifest.contains("unrelated.txt"_el));
        REQUIRE_EQUAL(files().readText(el::Path{"out/unrelated.txt"_el}), "keep"_el);
        const auto response = files().readText(el::Path{"out/hash/BLAKE2b.rsp"_el}).replacedAll("\n"_el, "\r\n"_el);
        write("out/hash/BLAKE2b.rsp"_el, response);
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
        REQUIRE_EQUAL(run(), 0);
        REQUIRE_EQUAL(files().readText(el::Path{"out/hash/BLAKE2b.rsp"_el}), response);
        REQUIRE_EQUAL(files().readText(el::Path{"out/manifest.txt"_el}), manifest);
        REQUIRE_EQUAL(run(el::StringList{"--output-directory"_el, "alternate"_el}), 0);
        REQUIRE_FALSE(files().readText(el::Path{"alternate/manifest.txt"_el}).contains("retained"_el));
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testInvalidConfigurationAndSourcesBeforeWrites() {
        prepare();
        write("out/sentinel"_el, "keep"_el);
        write("offline/blake2-kat.json"_el, "[]"_el);
        REQUIRE(run() != 0);
        REQUIRE(_errorText.contains("Checksum Mismatch"_el));
        REQUIRE_FALSE((_root / "out/hash"_el).info().exists());
        prepare("file:///source"_el);
        REQUIRE(run() != 0);
        REQUIRE(_errorText.contains("Invalid Source URL"_el));
        prepare();
        REQUIRE(
            runTool<tool::test_data::CryptologyVectorsTool>(
                {}, {}, "import-cryptology-vectors"_el, el::StringList{"--family"_el, "invalid"_el}) != 0);
        REQUIRE(run(el::StringList{"--source-directory"_el, "../escape"_el}) != 0);
        REQUIRE(run(el::StringList{"--output-directory"_el, "../escape"_el}) != 0);
        REQUIRE_EQUAL(files().readText(el::Path{"out/sentinel"_el}), "keep"_el);
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMalformedVerifiedInputLeavesOutputsUntouched() {
        prepare("http://127.0.0.1:1/source"_el, "[{}]"_el);
        write("out/hash/BLAKE2b.rsp"_el, "sentinel"_el);
        REQUIRE(run() != 0);
        REQUIRE(_errorText.contains("Invalid Cryptology Vector Source"_el));
        REQUIRE_EQUAL(files().readText(el::Path{"out/hash/BLAKE2b.rsp"_el}), "sentinel"_el);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testOnlineSelectionChecksumsAndOfflineIsolation() {
        prepare();
        const auto body = files().readText(el::Path{"offline/blake2-kat.json"_el});
        const auto thread = el::UnmanagedEventThread::create();
        auto server = el::HttpServerPtr{};
        auto endpoint = std::promise<el::IpEndpoint>{};
        auto ready = endpoint.get_future();
        auto requests = std::atomic<int>{0};
        thread->events()->invoke([&] {
            server = thread->events()->get<el::Network>().createHttpServer();
            server->events().onListening([&] { endpoint.set_value(*server->localEndpoint()); });
            server->events().onRequest(
                el::HttpMethod{el::HttpMethodType::Get},
                "/source"_el,
                [&](el::HttpServerSessionPtr, el::HttpServerRequestPtr request, el::ByteBlock) {
                    ++requests;
                    request->sendText(body);
                });
            server->start(el::IpEndpoint{el::IpAddress::loopbackV4(), el::Port{}});
        });
        thread->start();
        REQUIRE(ready.wait_for(std::chrono::seconds{5}) == std::future_status::ready);
        const auto address = ready.get();
        const auto url =
            el::Url{el::UrlScheme::Http, el::HostEndpoint{address.address(), address.port()}, "/source"_el};
        prepare(url.toString());
        const auto online = [&](const el::StringList &extra = {}) {
            auto options = el::StringList{"--family"_el, "blake2b"_el};
            options.append(extra);
            return runTool<tool::test_data::CryptologyVectorsTool>({}, {}, "import-cryptology-vectors"_el, options);
        };
        REQUIRE(online(el::StringList{"--check"_el}) != 0);
        REQUIRE_FALSE((_root / "out"_el).info().exists());
        REQUIRE_EQUAL(requests.load(), 1);
        REQUIRE_EQUAL(online(), 0);
        REQUIRE_EQUAL(online(el::StringList{"--check"_el}), 0);
        REQUIRE_EQUAL(requests.load(), 3);
        const auto manifest = files().readData(el::Path{"out/manifest.txt"_el});
        prepare(url.toString(), "[{}]"_el);
        REQUIRE(online() != 0);
        REQUIRE(_errorText.contains("Checksum Mismatch"_el));
        REQUIRE_EQUAL(files().readData(el::Path{"out/manifest.txt"_el}), manifest);
        REQUIRE_EQUAL(requests.load(), 4);
        prepare(url.toString());
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
        REQUIRE_EQUAL(requests.load(), 4);
        thread->events()->invoke([&] { server->abort(); });
        thread->quit();
        thread->join();
        REQUIRE_FALSE(thread->eventLoop().hasError());
    }

private:
    void prepare(const el::String &url = "http://127.0.0.1:1/source"_el, el::String data = {}) {
        if (data.isEmpty()) {
            auto json = el::StringEditor{"[\n"_el};
            for (auto i = 0; i < 256; ++i) {
                if (i > 0) {
                    json.append(",\n"_el);
                }
                json.append("{\"hash\":\"blake2b\",\"key\":\"\",\"in\":\"00\",\"out\":\"ab\"}"_el);
            }
            json.append("\n]\n"_el);
            data = json;
        }
        const auto checksum = el::toAsciiLowercase(
            el::String::fromByteBlock(tool::frame::calculateTextChecksum(data, el::HashAlgorithm::Sha2_256)));
        auto config = el::StringEditor{"[rendering]\n[import_cryptology_vectors]\noutput_directory: \"out\"\n"_el};
        for (const auto &source : tool::test_data::impl::VectorSource::all()) {
            config.append(
                el::StringFormat{
                    "[import_cryptology_vectors.sources.{}]\nurl: \"{}\"\nsha256: \"{}\"\nrevision: \"\"\nattribution: \"Test source\"\n"_el}
                    .build(source.id, url, checksum));
        }
        write("tool/conf/erbsland-core-tool.elcl"_el, config);
        write("offline/blake2-kat.json"_el, data.replacedAll("\n"_el, "\r\n"_el));
    }
    auto run(const el::StringList &extra = {}) -> int {
        auto options = el::StringList{"--family"_el, "blake2b"_el, "--source-directory"_el, "offline"_el};
        options.append(extra);
        return runTool<tool::test_data::CryptologyVectorsTool>({}, {}, "import-cryptology-vectors"_el, options);
    }
};
