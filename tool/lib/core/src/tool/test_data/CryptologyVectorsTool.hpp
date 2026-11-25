// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FixtureTool.hpp"

#include "impl/VectorSource.hpp"

#include <erbsland/ByteBlock.hpp>
#include <erbsland/TimeDelta.hpp>
#include <erbsland/Url.hpp>

#include <optional>

namespace tool::test_data {

using namespace el::text::literals;

/// Import pinned cryptology vectors from authenticated downloads or an explicit offline directory.
class CryptologyVectorsTool final : public FixtureTool {
    /// Configuration setting limiting each input source's byte count.
    constexpr static auto cMaximumSourceBytesValue = "maximum_source_bytes"_el;
    /// Configuration setting for the overall per-source HTTP deadline.
    constexpr static auto cDownloadTimeoutSecondsValue = "download_timeout_seconds"_el;
    /// Render field containing a lowercase hexadecimal source or fixture digest.
    constexpr static auto cChecksumKey = "checksum"_el;
    /// Source attribution shared by configuration and provenance rendering.
    constexpr static auto cAttributionKey = "attribution"_el;

public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto optionModule() const -> el::OptionModulePtr override;
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

protected:
    [[nodiscard]] auto defaultDirectory() const -> el::String override;

private:
    /// One fully validated source, retaining the configuration context for diagnostics.
    struct Source {
        impl::VectorSource descriptor;
        el::Url url;
        el::ByteBlock checksum;
        el::String revision;
        el::conf::ValuePtr configuration;
    };
    /// Validate and bind every source before performing any network or output operation.
    void configureSources(const el::conf::ValuePtr &configuration);
    /// Render provenance and the manifest from proposed and retained response files.
    void prepareMetadata();
    /// Test whether the selected family includes this descriptor.
    [[nodiscard]] auto selected(const impl::VectorSource &source) const -> bool;

private:
    el::List<Source> _sources;                ///< Validated pinned sources in import order.
    std::optional<el::Path> _sourceDirectory; ///< Explicit offline directory; otherwise online.
    el::String _family;                       ///< Selected family, or all.
    el::ByteCount _maximumBytes;              ///< Limit for each input before parsing.
    el::TimeDelta _timeout;                   ///< Overall deadline per download.
};

}
