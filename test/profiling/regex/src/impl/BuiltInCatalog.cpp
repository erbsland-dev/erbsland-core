// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "BuiltInCatalog.hpp"

#include "CorpusDirectory.hpp"

namespace app::regex::impl::built_in_catalog {

using namespace el::text::literals;

auto generatedCorpus(const el::String &name) -> std::optional<el::String> {
    if (name == "empty"_el) {
        return el::String{};
    }
    if (name == "micro-match"_el) {
        return el::String{"alpha beta aaaaab"_el};
    }
    if (name == "micro-miss"_el) {
        return el::String{"alpha beta aaaaac"_el};
    }
    if (name == "dense-4k"_el) {
        return repeatCharacter(U'a', 4096U);
    }
    if (name == "dense-64k"_el) {
        return repeatCharacter(U'a', 65536U);
    }
    if (name == "middle-64k"_el) {
        auto result = el::StringEditor{repeatCharacter(U'a', 32768U)};
        result.append("b"_el);
        result.append(repeatCharacter(U'a', 32767U));
        return el::String{result};
    }
    if (name == "sparse-1m"_el) {
        auto result = el::StringEditor{};
        result.reserve(el::ByteLength{1024U * 1024U});
        const auto block = repeatCharacter(U'a', 1023U);
        for (auto index = 0U; index < 1024U; ++index) {
            result.append(block);
            result.append("b"_el);
        }
        return el::String{result};
    }
    if (name == "adversarial-64k"_el) {
        auto result = el::StringEditor{repeatCharacter(U'a', 65535U)};
        result.append("c"_el);
        return el::String{result};
    }
    if (name == "late-64k"_el) {
        auto result = el::StringEditor{repeatCharacter(U'a', 65535U)};
        result.append("b"_el);
        return el::String{result};
    }
    if (name == "overlap-success"_el) {
        return "aaaaab"_el;
    }
    if (name == "overlap-miss"_el) {
        return "aaaaac"_el;
    }
    if (name == "ambiguous-miss"_el) {
        auto result = el::StringEditor{repeatCharacter(U'a', 256U)};
        result.append("c"_el);
        return el::String{result};
    }
    if (name == "unicode-4k"_el) {
        auto result = el::StringEditor{};
        for (auto index = 0U; index < 256U; ++index) {
            result.append("Grüezi Καλημέρα 東京 😀 123\n"_el);
        }
        return el::String{result};
    }
    if (name == "crlf-4k"_el) {
        auto result = el::StringEditor{};
        for (auto index = 0U; index < 256U; ++index) {
            result.append("alpha beta\r\ngamma delta\r\n"_el);
        }
        return el::String{result};
    }
    return {};
}

auto file(const el::String &name) -> std::optional<el::String> {
    if (name == "shakespeare-text"_el) {
        return (el::Path{el::String{cCorpusDirectory}} / "shakespeare.txt"_el).toString();
    }
    if (name == "shakespeare-html"_el) {
        return (el::Path{el::String{cCorpusDirectory}} / "shakespeare.html"_el).toString();
    }
    return {};
}

auto pattern(const el::String &name) -> std::optional<el::String> {
    if (name == "literal-a"_el) {
        return "a"_el;
    }
    if (name == "word"_el) {
        return R"(\b\w+\b)"_el;
    }
    if (name == "capitalized-word"_el) {
        return R"(\b[A-Z][a-z]*\b)"_el;
    }
    if (name == "email"_el) {
        return R"(([a-zA-Z0-9\._%\+\-]+)@([a-zA-Z0-9\.\-]+\.[a-zA-Z]{2,}))"_el;
    }
    if (name == "url"_el) {
        return R"(https?://([a-zA-Z0-9\.]+))"_el;
    }
    if (name == "html-tag"_el) {
        return R"(<[a-z1-6]+[^>]*>)"_el;
    }
    if (name == "toc-capture"_el) {
        return R"re(<a href="#(chap([0-9]{2}))" class="pginternal">([^<]+)</a>)re"_el;
    }
    if (name == "toc-possessive"_el) {
        return R"re(<a href="#(chap([0-9]{2}))" class="pginternal">([^<]++)</a>)re"_el;
    }
    if (name == "dot-plus"_el) {
        return ".+"_el;
    }
    if (name == "markdown-link"_el) {
        return R"(\[([^\]]+)\]\(([^\)]+)\))"_el;
    }
    if (name == "zero-width"_el) {
        return ""_el;
    }
    if (name == "overlap"_el) {
        return "(?:a|aa|aaa|aaaa)+b"_el;
    }
    return {};
}

auto repeatCharacter(const el::Char character, const std::size_t count) -> el::String {
    return el::String::fromCharacter(character, el::CpLength::fromSizeTOrThrow(count));
}

}
