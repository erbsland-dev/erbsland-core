// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509CertificateBundle.hpp"

#include "../impl/PemCodec.hpp"
#include "../impl/X509FileTools.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../path/Path.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

X509CertificateBundle::X509CertificateBundle(util::List<X509Certificate> certificates) :
    _certificates{std::move(certificates)} {
    if (_certificates.count().toSizeT() > impl::PemCodec::cMaximumCertificates) {
        throw err::ParameterError{
            "Certificate bundle exceeds the fixed certificate-count limit."_el, "certificates"_el};
    }
    for (const auto &certificate : _certificates) {
        if (certificate.isEmpty()) {
            throw err::ParameterError{"Certificate bundle contains an empty certificate."_el, "certificates"_el};
        }
    }
}

auto X509CertificateBundle::toPem() const -> text::String {
    auto data = util::List<mem::ByteBlock>{};
    for (const auto &certificate : _certificates) {
        data.append(certificate.toDer());
    }
    return impl::PemCodec{std::move(data)}.encode();
}

auto X509CertificateBundle::toDer() const -> mem::ByteBlock {
    if (_certificates.count() != unit::ItemCount{1U}) {
        throw err::LogicError{"DER output requires a certificate bundle containing exactly one certificate."_el};
    }
    return _certificates.first().toDer();
}

void X509CertificateBundle::writeToFile(const path::Path &path, const X509CertificateFormat format) const {
    const auto file = impl::X509FileTools{path};
    const auto selected = file.outputFormat(format);
    if (selected == X509CertificateFormat::Pem) {
        file.writePem(toPem());
    } else {
        file.writeDer(toDer());
    }
}

auto X509CertificateBundle::fromPem(const text::String &text, const X509CertificateProfileMode mode) noexcept
    -> X509CertificateBundle {
    try {
        return fromPemOrThrow(text, mode);
    } catch (...) {
        return {};
    }
}

auto X509CertificateBundle::fromPemOrThrow(const text::String &text, const X509CertificateProfileMode mode)
    -> X509CertificateBundle {
    const auto blocks = impl::PemCodec{text}.decode();
    auto certificates = util::List<X509Certificate>{};
    for (const auto &block : blocks) {
        certificates.append(X509Certificate::fromDerOrThrow(block, mode));
    }
    return X509CertificateBundle{std::move(certificates)};
}

auto X509CertificateBundle::fromDer(const mem::ByteBlock &data, const X509CertificateProfileMode mode) noexcept
    -> X509CertificateBundle {
    try {
        return fromDerOrThrow(data, mode);
    } catch (...) {
        return {};
    }
}

auto X509CertificateBundle::fromDerOrThrow(const mem::ByteBlock &data, const X509CertificateProfileMode mode)
    -> X509CertificateBundle {
    return X509CertificateBundle{util::List<X509Certificate>{X509Certificate::fromDerOrThrow(data, mode)}};
}

auto X509CertificateBundle::fromFile(
    const path::Path &path, const X509CertificateFormat format, const X509CertificateProfileMode mode) noexcept
    -> X509CertificateBundle {
    try {
        return fromFileOrThrow(path, format, mode);
    } catch (...) {
        return {};
    }
}

auto X509CertificateBundle::fromFileOrThrow(
    const path::Path &path, const X509CertificateFormat format, const X509CertificateProfileMode mode)
    -> X509CertificateBundle {
    const auto file = impl::X509FileTools{path};
    const auto data = file.read();
    const auto selected = file.inputFormat(data, format);
    return selected == X509CertificateFormat::Pem ? fromPemOrThrow(impl::X509FileTools::toPemText(data), mode)
                                                  : fromDerOrThrow(data, mode);
}

}
