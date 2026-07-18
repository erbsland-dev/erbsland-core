// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DiagnosticHelper.hpp"

#include "ErrorDocumentBuilder.hpp"
#include "Exception.hpp"

#include "impl/ExceptionDiagnostic.hpp"

#include "../i18n/DisplayTextMap.hpp"
#include "../text/EscapeFormat.hpp"
#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"
#include "../text/TextDocument.hpp"
#include "../text/TextNode.hpp"

#include <exception>
#include <string_view>
#include <utility>

namespace erbsland::err {

using namespace text::literals;

DiagnosticHelper::DiagnosticHelper(const std::exception &error, const i18n::DisplayTextMapConstPtr &displayText) :
    _error{error}, _displayText{displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap()} {
}

void DiagnosticHelper::appendCauseDocument(text::TextDocument &document, const DiagnosticConstPtr &diagnostic) {
    if (diagnostic == nullptr) {
        return;
    }
    const auto blockquote = document.addBlockquote();
    const auto diagnosticDocument = diagnostic->toTextDocument(_displayText);
    for (const auto &child : diagnosticDocument.root()->children()) {
        blockquote->add(child->clone());
    }
}

void DiagnosticHelper::appendCauses(text::TextDocument &document, std::exception_ptr cause, std::size_t depth) {
    auto *container = document.root().get();
    while (cause != nullptr && depth < cMaximumCauseDepth) {
        auto heading = container->add(text::TextNodeType::Heading);
        heading->setLevel(2);
        heading->setStyle("diagnostic-cause"_el);
        heading->addText(_displayText->text("CausedByHeading"_el));
        auto blockquote = container->add(text::TextNodeType::Blockquote);
        blockquote->setStyle("diagnostic-cause"_el);
        auto diagnostic = DiagnosticConstPtr{};
        try {
            std::rethrow_exception(std::move(cause));
        } catch (const Exception &error) {
            diagnostic = DiagnosticHelper{error}.toDiagnostic();
            cause = error.cause();
        } catch (const std::exception &error) {
            diagnostic = DiagnosticHelper(error).toDiagnostic();
            cause = nullptr;
        } catch (...) {
            diagnostic =
                std::make_shared<impl::ExceptionDiagnostic>(_displayText->text("UnknownException"_el), _displayText);
            cause = nullptr;
        }
        const auto diagnosticDocument = diagnostic->toTextDocument(_displayText);
        for (const auto &child : diagnosticDocument.root()->children()) {
            blockquote->add(child->clone());
        }
        container = blockquote.get();
        ++depth;
    }
    if (cause != nullptr) {
        auto blockquote = container->add(text::TextNodeType::Blockquote);
        blockquote->setStyle("diagnostic-cause"_el);
        auto fallback =
            std::make_shared<impl::ExceptionDiagnostic>(_displayText->text("CauseChainTooDeep"_el), _displayText)
                ->toTextDocument(_displayText);
        for (const auto &child : fallback.root()->children()) {
            blockquote->add(child->clone());
        }
    }
}

auto DiagnosticHelper::toDiagnostic() -> DiagnosticConstPtr {
    DiagnosticConstPtr result;
    if (const auto libError = dynamic_cast<const Exception *>(&_error); libError != nullptr) {
        result = libError->diagnostic();
        if (result == nullptr) {
            result = std::make_shared<impl::ExceptionDiagnostic>(libError->toString(), _displayText);
        }
    } else {
        result = std::make_shared<impl::ExceptionDiagnostic>(
            text::String{std::string_view{_error.what()}}.toEscaped(text::EscapeFormat::Display), _displayText);
    }
    return result;
}

auto DiagnosticHelper::toDocument() -> text::TextDocument {
    auto document = text::TextDocument{};
    document.root()->setStyle("error"_el);
    const auto diagnosticDocument = toDiagnostic()->toTextDocument(_displayText);
    for (const auto &child : diagnosticDocument.root()->children()) {
        document.root()->add(child->clone());
    }
    if (const auto libError = dynamic_cast<const Exception *>(&_error); libError != nullptr) {
        appendCauses(document, libError->cause(), 0U);
    }
    return document;
}

auto DiagnosticHelper::documentFromError(
    const std::exception_ptr &errorPtr, const i18n::DisplayTextMapConstPtr &displayText) -> text::TextDocument {
    const auto resolvedDisplayText = displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap();
    if (errorPtr == nullptr) {
        return std::make_shared<impl::ExceptionDiagnostic>(
            resolvedDisplayText->text("NoExceptionProvided"_el), resolvedDisplayText)
            ->toTextDocument(resolvedDisplayText);
    }
    try {
        std::rethrow_exception(errorPtr);
    } catch (const Exception &error) {
        return DiagnosticHelper{error, resolvedDisplayText}.toDocument();
    } catch (const std::exception &error) {
        return DiagnosticHelper{error, resolvedDisplayText}.toDocument();
    } catch (...) {
        return std::make_shared<impl::ExceptionDiagnostic>(
            resolvedDisplayText->text("UnknownException"_el), resolvedDisplayText)
            ->toTextDocument(resolvedDisplayText);
    }
}
}
