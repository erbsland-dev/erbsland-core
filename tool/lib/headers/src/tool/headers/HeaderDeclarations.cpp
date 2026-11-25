// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HeaderDeclarations.hpp"

#include <erbsland/StringEditor.hpp>

namespace tool::headers {

using namespace el::text::literals;

HeaderDeclarations::HeaderDeclarations(const el::String &text) : _tokenizer{text} {
    advance();
    scan();
}

void HeaderDeclarations::advance() {
    while (true) {
        const auto token = _tokenizer.next();
        if (token.kind() == el::cpp::CppToken::Kind::End || token.kind() == el::cpp::CppToken::Kind::Identifier ||
            token.kind() == el::cpp::CppToken::Kind::Number || token.kind() == el::cpp::CppToken::Kind::Punctuation) {
            _token = token.text();
            return;
        }
    }
}

void HeaderDeclarations::scan() {
    auto scopes = std::vector<std::pair<el::String, std::size_t>>{};
    auto sourceNamespace = el::String{};
    std::size_t depth = 0;
    bool isTemplate = false;
    while (!_token.isEmpty()) {
        if (_token == "namespace"_el) {
            auto anonymous = false;
            const auto name = readNamespaceName(sourceNamespace, anonymous);
            if (_token == "{"_el) {
                ++depth;
                scopes.emplace_back(sourceNamespace, depth);
                if (!anonymous) {
                    sourceNamespace = name;
                    _namespaces.insert(sourceNamespace);
                }
                advance();
                continue;
            }
        }
        if (_token == "{"_el) {
            ++depth;
            isTemplate = false;
        } else if (_token == "}"_el && depth != 0) {
            if (!scopes.empty() && scopes.back().second == depth) {
                sourceNamespace = scopes.back().first;
                scopes.pop_back();
            }
            --depth;
        } else if (depth == 0 || (!scopes.empty() && depth == scopes.back().second)) {
            if (_token == "template"_el) {
                isTemplate = true;
                advance();
                skipTemplateParameters();
                continue;
            }
            if (_token == "class"_el || _token == "struct"_el || _token == "enum"_el || _token == "using"_el) {
                record(sourceNamespace, isTemplate);
                continue;
            }
            if (_token == ";"_el) {
                isTemplate = false;
            }
        }
        advance();
    }
}

auto HeaderDeclarations::readNamespaceName(const el::String &sourceNamespace, bool &anonymous) -> el::String {
    auto name = el::StringEditor{};
    name.append(sourceNamespace);
    if (!sourceNamespace.isEmpty()) {
        name.append("::"_el);
    }
    advance();
    anonymous = _token == "{"_el;
    while (!_token.isEmpty() && _token != "{"_el && _token != ";"_el && _token != "="_el) {
        name.append(_token);
        advance();
    }
    return name;
}

void HeaderDeclarations::skipTemplateParameters() {
    if (_token == "<"_el) {
        std::size_t brackets = 0;
        do {
            if (_token == "<"_el) {
                ++brackets;
            } else if (_token == ">"_el) {
                --brackets;
            } else if (_token == ">>"_el) {
                brackets -= std::min(brackets, std::size_t{2});
            }
            advance();
        } while (!_token.isEmpty() && brackets != 0);
    }
}

void HeaderDeclarations::record(const el::String &sourceNamespace, const bool isTemplate) {
    const auto kind = _token;
    auto prefix = el::StringEditor{kind};
    advance();
    if (kind == "using"_el && _token == "namespace"_el) {
        while (!_token.isEmpty() && _token != ";"_el) {
            advance();
        }
        return;
    }
    if (kind == "enum"_el && (_token == "class"_el || _token == "struct"_el)) {
        prefix.append(" "_el).append(_token);
        advance();
    }
    if (_token.isEmpty() || !_token.charAt(el::StringSide::Front).isAsciiWord()) {
        return;
    }
    auto name = _token;
    advance();
    auto namespaceBuffer = el::StringEditor{sourceNamespace};
    auto qualified = false;
    while (_token == "::"_el) {
        qualified = true;
        if (!namespaceBuffer.isEmpty()) {
            namespaceBuffer.append("::"_el);
        }
        namespaceBuffer.append(name);
        advance();
        name = _token;
        advance();
    }
    if (kind == "using"_el && _token != "="_el) {
        return;
    }
    auto forward = el::StringEditor{};
    if (!isTemplate && !qualified && (kind == "class"_el || kind == "struct"_el)) {
        forward.append(kind).append(" "_el).append(name).append(";"_el);
    } else if (!isTemplate && kind == "enum"_el && _token == ":"_el) {
        auto underlying = el::StringEditor{};
        auto previous = el::String{};
        advance();
        while (!_token.isEmpty() && _token != "{"_el && _token != ";"_el) {
            if (!underlying.isEmpty() && _token != "::"_el && previous != "::"_el) {
                underlying.append(" "_el);
            }
            underlying.append(_token);
            previous = _token;
            advance();
        }
        const auto type = el::String{underlying};
        if (type == "std::uint8_t"_el || type == "uint8_t"_el || type == "int"_el || type == "unsigned int"_el ||
            type == "std::uint16_t"_el || type == "std::uint32_t"_el || type == "std::uint64_t"_el) {
            forward.append(prefix).append(" "_el).append(name).append(" : "_el).append(type).append(";"_el);
        }
    }
    _declarations.push_back({namespaceBuffer, name, forward});
}

}
