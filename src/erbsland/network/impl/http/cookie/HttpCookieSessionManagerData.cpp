// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpCookieSessionManagerData.hpp"

#include "../../../../core/Application.hpp"
#include "../../../../err/ParameterError.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../random/Random.hpp"
#include "../../../../text/AnyString.hpp"
#include "../../../../text/AsciiCategory.hpp"
#include "../../../../text/base_n/BaseNEncoder.hpp"
#include "../../../../text/base_n/BaseNFormat.hpp"
#include "../../../../text/base_n/BaseNFormatFlag.hpp"
#include "../../../../text/CharSet.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringCharReader.hpp"
#include "../../../../text/StringEditor.hpp"
#include "../../../http/HttpFieldType.hpp"
#include "../../../http_server/HttpServerRequest.hpp"
#include "../../../http_server/HttpServerSession.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

HttpCookieSessionManagerData::HttpCookieSessionManagerData(HttpCookieSessionManagerOptions options) :
    _options{std::move(options)} {
    validate();
}

auto HttpCookieSessionManagerData::select(const HttpServerSessionContext &context) -> HttpServerSessionSelection {
    captureOwner(context.request()->ownerEvents());
    const auto now = time::TimePoint::now();
    removeExpired(now);
    const auto identifier = requestIdentifier(context.request()->head().headers());
    if (identifier.has_value()) {
        const auto iterator = _entries.find(*identifier);
        if (iterator != _entries.end() && iterator->second->session->isValid()) {
            touch(iterator->second, now);
            return HttpServerSessionSelection{iterator->second->session};
        }
    }
    while (_entries.size() >= _options.maximumSessions().toSizeT() && !_idle.empty()) {
        erase(_idle.front());
    }
    auto token = createIdentifier();
    while (_entries.contains(token)) {
        token = createIdentifier();
    }
    auto session = context.createSession(token);
    if (session == nullptr) {
        return {};
    }
    auto entry = std::make_shared<Entry>();
    entry->identifier = token;
    entry->session = session;
    entry->created = now;
    entry->lastUsed = now;
    entry->secure = usesSecureCookie(context.isSecure());
    _creation.emplace_back(entry);
    entry->creationPosition = std::prev(_creation.end());
    _idle.emplace_back(entry);
    entry->idlePosition = std::prev(_idle.end());
    _entries.emplace(token, entry);
    scheduleCleanup();
    auto fields = HttpHeaders{};
    fields.addField(HttpFieldType::SetCookie, cookieValue(token, entry->secure, false));
    return HttpServerSessionSelection{std::move(session), std::move(fields), _options.cookieName()};
}

auto HttpCookieSessionManagerData::invalidate(const network::HttpServerSessionPtr &session) -> HttpHeaders {
    auto secure = usesSecureCookie(false);
    if (session != nullptr && session->identifier().has_value()) {
        const auto iterator = _entries.find(*session->identifier());
        if (iterator != _entries.end() && iterator->second->session == session) {
            secure = iterator->second->secure;
            erase(iterator->second);
        }
    }
    auto fields = HttpHeaders{};
    fields.addField(HttpFieldType::SetCookie, cookieValue({}, secure, true));
    return fields;
}

void HttpCookieSessionManagerData::validate() const {
    if (_options.cookieName().isEmpty() || !_options.cookieName().containsOnly(AsciiCategory::HttpToken) ||
        !_options.cookiePath().startsWith("/"_el) || _options.cookiePath().contains(";"_el) ||
        !_options.idleTimeout().isPositive() || !_options.absoluteTimeout().isPositive() ||
        _options.maximumSessions().isZero() || _options.maximumSessions().isInfinite() ||
        (_options.sameSite() == HttpCookieSameSite::None && _options.securePolicy() == HttpCookieSecurePolicy::Never)) {
        throw err::ParameterError{"The HTTP cookie session manager options are invalid."_el, "options"_el};
    }
}

auto HttpCookieSessionManagerData::requestIdentifier(const HttpHeaders &headers) const -> std::optional<String> {
    auto found = std::optional<String>{};
    for (const auto &fieldValue : headers.getAll(HttpFieldType::Cookie)) {
        auto reader = StringCharReader{fieldValue};
        while (!reader.isAtEnd()) {
            reader.advanceWhile(AsciiCategory::Whitespace);
            reader.startCapture();
            reader.advanceUntil(CharSet{U'=', U';'});
            const auto name = reader.takeCapture().toString();
            if (!reader.advanceIf(U'=')) {
                reader.advanceIf(U';');
                continue;
            }
            reader.startCapture();
            reader.advanceUntil(CharSet{U';'});
            auto value = reader.takeCapture().toString();
            while (!value.isEmpty() && value.charAt(StringSide::Back).isAsciiWhitespace()) {
                value = value.slice(StringSide::Front, value.length() - unit::ByteLength::one());
            }
            reader.advanceIf(U';');
            if (name != _options.cookieName()) {
                continue;
            }
            if (found.has_value() || value.length() != unit::ByteLength{43U} ||
                !value.containsOnly(AsciiCategory::WordWithHyphen)) {
                return std::nullopt;
            }
            found = std::move(value);
        }
    }
    return found;
}

auto HttpCookieSessionManagerData::createIdentifier() -> String {
    auto format = base_n::BaseNFormat::base64Url();
    format.clearFlags(base_n::BaseNFormatFlag::EmitPadding | base_n::BaseNFormatFlag::RequirePadding);
    return base_n::BaseNEncoder{
        core::application().secureRandom().buildByteBlock(unit::ByteLength{32U}), std::move(format)}
        .toString();
}

auto HttpCookieSessionManagerData::usesSecureCookie(const bool secureRequest) const noexcept -> bool {
    return _options.securePolicy() == HttpCookieSecurePolicy::Always ||
        (_options.securePolicy() == HttpCookieSecurePolicy::Automatic &&
            (secureRequest || _options.sameSite() == HttpCookieSameSite::None));
}

auto HttpCookieSessionManagerData::cookieValue(const String &identifier, const bool secure, const bool deletion) const
    -> String {
    auto result = StringEditor{};
    result.append(_options.cookieName());
    result.append(U'=');
    result.append(identifier);
    result.append("; Path="_el);
    result.append(_options.cookiePath());
    result.append("; HttpOnly; SameSite="_el);
    switch (_options.sameSite()) {
    case HttpCookieSameSite::Strict:
        result.append("Strict"_el);
        break;
    case HttpCookieSameSite::Lax:
        result.append("Lax"_el);
        break;
    case HttpCookieSameSite::None:
        result.append("None"_el);
        break;
    }
    if (deletion) {
        result.append("; Max-Age=0"_el);
    } else {
        result.append("; Max-Age="_el);
        result.append(String::fromInteger(_options.absoluteTimeout().toSeconds().toRawValue()));
    }
    if (secure) {
        result.append("; Secure"_el);
    }
    return String{result};
}

void HttpCookieSessionManagerData::removeExpired(const time::TimePoint now) {
    while (!_creation.empty() && _creation.front()->created + _options.absoluteTimeout() <= now) {
        erase(_creation.front());
    }
    while (!_idle.empty() && _idle.front()->lastUsed + _options.idleTimeout() <= now) {
        erase(_idle.front());
    }
}

void HttpCookieSessionManagerData::captureOwner(const event::EventsPtr &owner) {
    if (_ownerEvents == nullptr) {
        _ownerEvents = owner;
        return;
    }
    if (_ownerEvents != owner) {
        throw err::ParameterError{
            "An HTTP cookie session manager can only be used by one owner event loop."_el, "context"_el};
    }
}

void HttpCookieSessionManagerData::scheduleCleanup() {
    if (_cleanupScheduled || _ownerEvents == nullptr || _entries.empty()) {
        return;
    }
    auto deadline = _creation.front()->created + _options.absoluteTimeout();
    const auto idleDeadline = _idle.front()->lastUsed + _options.idleTimeout();
    if (idleDeadline < deadline) {
        deadline = idleDeadline;
    }
    const auto delay = deadline - time::TimePoint::now();
    const auto weakSelf = std::weak_ptr<HttpCookieSessionManagerData>{shared_from_this()};
    _cleanupScheduled = true;
    _ownerEvents->invokeAfter(delay, [weakSelf]() -> void {
        if (const auto self = weakSelf.lock()) {
            self->_cleanupScheduled = false;
            self->removeExpired(time::TimePoint::now());
            self->scheduleCleanup();
        }
    });
}

void HttpCookieSessionManagerData::touch(const std::shared_ptr<Entry> &entry, const time::TimePoint now) {
    _idle.erase(entry->idlePosition);
    entry->lastUsed = now;
    _idle.emplace_back(entry);
    entry->idlePosition = std::prev(_idle.end());
}

void HttpCookieSessionManagerData::erase(const std::shared_ptr<Entry> &entry) {
    _entries.erase(entry->identifier);
    _creation.erase(entry->creationPosition);
    _idle.erase(entry->idlePosition);
}

}
