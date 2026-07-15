// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/path/impl/CommonPathBackend.hpp>
#include <erbsland/path/impl/PathInfoData.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/text/Literals.hpp>

namespace erbsland::test::pathtest {

/// Test-only backend with explicit failing defaults for all native path primitives.
class PathBackendTestBase : public path::impl::CommonPathBackend {
public: // implement PathBackend
    [[nodiscard]] auto currentDirectoryOrThrow() const -> path::Path override { return unsupportedPath(); }
    [[nodiscard]] auto systemTempDirectoryOrThrow() const -> path::Path override { return unsupportedPath(); }
    [[nodiscard]] auto resolveOrThrow(const path::Path &, path::PathResolveOptions) const -> path::Path override {
        return unsupportedPath();
    }
    [[nodiscard]] auto loadInfoOrThrow(const path::Path &, path::PathInfoParts) const
        -> path::impl::PathInfoData override {
        throwUnsupported();
    }
    [[nodiscard]] auto directoryEntriesOrThrow(const path::Path &) const -> std::vector<path::Path> override {
        throwUnsupported();
    }
    void createDirectoryEntryOrThrow(const path::Path &, path::PathAccessProfile) const override { throwUnsupported(); }
    void removeEntryOrThrow(const path::Path &) const override { throwUnsupported(); }
    void copyFileEntryOrThrow(const path::Path &, const path::Path &) const override { throwUnsupported(); }
    void moveEntryOrThrow(const path::Path &, const path::Path &) const override { throwUnsupported(); }
    [[nodiscard]] auto readSymlinkOrThrow(const path::Path &) const -> path::Path override { return unsupportedPath(); }
    void createSymlinkOrThrow(const path::Path &, const path::Path &, bool) const override { throwUnsupported(); }
    [[nodiscard]] auto openByteInputStreamOrThrow(const path::Path &, path::PathReadDataOptions) const
        -> stream::ByteInputStreamPtr override {
        throwUnsupported();
    }
    void setAccessProfileOrThrow(const path::Path &, path::PathAccessProfile, path::PathChangeOptions) const override {
        throwUnsupported();
    }
    void addAttributesOrThrow(const path::Path &, path::PathAttributes, path::PathChangeOptions) const override {
        throwUnsupported();
    }
    void clearAttributesOrThrow(const path::Path &, path::PathAttributes, path::PathChangeOptions) const override {
        throwUnsupported();
    }

protected:
    [[nodiscard]] auto openByteOutputStreamWithExistingContentOrThrow(
        const path::Path &, path::PathWriteDataOptions) const -> PathByteOutputStreamOpenResult override {
        throwUnsupported();
    }

private:
    [[noreturn]] static void throwUnsupported() {
        using namespace text::literals;
        throw path::PathError{"The test backend operation is not implemented."_el};
    }
    [[nodiscard]] static auto unsupportedPath() -> path::Path { throwUnsupported(); }
};

}
