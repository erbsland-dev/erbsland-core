// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathInfo.hpp"

#include "PathError.hpp"

#include "impl/BackendFactory.hpp"
#include "impl/PathBackend.hpp"

#include "../core/Application.hpp"
#include "../system/PlatformError.hpp"
#include "../system/UserLookup.hpp"
#include "../text/Literals.hpp"
#include "../time/TimeDelta.hpp"

#include <mutex>
#include <utility>

namespace erbsland::path {

using namespace text::literals;
using namespace system;
using time::DateTime;
using time::TimeDelta;
using time::TimePoint;

PathInfo::PathInfo(const Path &path, const PathInfoParts parts) noexcept : PathInfo{path, parts, {}} {
}

PathInfo::PathInfo(const Path &path, const PathInfoParts parts, impl::PathInfoCacheTrustWeakPtr cacheTrust) noexcept :
    _cacheTrust{std::move(cacheTrust)} {
    if (path.isEmpty()) {
        return;
    }
    _path = path;
    _cache = path._data->infoCache();
    ensureParts(parts);
}

PathInfo::PathInfo(PathInfo &&other) noexcept :
    _path{std::move(other._path)},
    _cache{std::exchange(other._cache, nullptr)},
    _resolvedPath{std::move(other._resolvedPath)},
    _cacheTrust{std::move(other._cacheTrust)} {
}

auto PathInfo::operator=(PathInfo &&other) noexcept -> PathInfo & {
    if (this != &other) {
        _path = std::move(other._path);
        _cache = std::exchange(other._cache, nullptr);
        _resolvedPath = std::move(other._resolvedPath);
        _cacheTrust = std::move(other._cacheTrust);
    }
    return *this;
}

auto PathInfo::fromDirectoryScan(
    const Path &path, const PathInfoParts parts, const impl::PathInfoCacheTrustPtr &cacheTrust) noexcept -> PathInfo {
    return PathInfo{path, parts, cacheTrust};
}

auto PathInfo::isEmpty() const noexcept -> bool {
    return _cache == nullptr;
}

auto PathInfo::path() const noexcept -> const Path & {
    return isEmpty() ? Path::empty() : _path;
}

auto PathInfo::resolvedPath() const noexcept -> const Path & {
    ensureParts(PathInfoPart::Type);
    if (isEmpty()) {
        return Path::empty();
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    _resolvedPath = _cache->data.resolvedPath;
    return _resolvedPath;
}

auto PathInfo::exists() const noexcept -> bool {
    ensureParts(PathInfoPart::Type);
    if (isEmpty()) {
        return false;
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.exists;
}

auto PathInfo::type() const noexcept -> PathType {
    ensureParts(PathInfoPart::Type);
    if (isEmpty()) {
        return PathType::Unknown;
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.type;
}

auto PathInfo::fileSize() const noexcept -> unit::ByteLength {
    ensureParts(PathInfoPart::Size);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.fileSize;
}

auto PathInfo::fileIdentity() const noexcept -> system::FileIdentity {
    ensureParts(PathInfoPart::FileIdentity);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.fileIdentity;
}

auto PathInfo::lastModified() const noexcept -> DateTime {
    ensureParts(PathInfoPart::Times);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.lastModified;
}

auto PathInfo::lastAccessed() const noexcept -> DateTime {
    ensureParts(PathInfoPart::Times);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.lastAccessed;
}

auto PathInfo::birthTime() const noexcept -> DateTime {
    ensureParts(PathInfoPart::Times);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.birthTime;
}

auto PathInfo::lastMetadataChange() const noexcept -> DateTime {
    ensureParts(PathInfoPart::Times);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.lastMetadataChange;
}

auto PathInfo::creationTime() const noexcept -> DateTime {
    ensureParts(PathInfoPart::Times);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    const auto &data = _cache->data;
    if (data.birthTime.isValid()) {
        return data.birthTime;
    }
    if (data.lastMetadataChange.isValid()) {
        return data.lastMetadataChange;
    }
    return data.lastModified;
}

auto PathInfo::isReadable() const noexcept -> bool {
    return accessInfo().currentProcessRights().isSet(PathAccessRight::Read);
}

auto PathInfo::isWritable() const noexcept -> bool {
    return accessInfo().currentProcessRights().isSet(PathAccessRight::Write);
}

auto PathInfo::isExecutable() const noexcept -> bool {
    return accessInfo().currentProcessRights().isSet(PathAccessRight::Execute);
}

auto PathInfo::ownerName() const noexcept -> UserName {
    ensureParts(PathInfoPart::OwnerName);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.ownerName;
}

auto PathInfo::ownerNameOrThrow() const -> UserName {
    ensurePartsOrThrow(PathInfoPart::OwnerName);
    if (isEmpty()) {
        throw PathError{"File owner is unavailable"_el};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    if (_cache->data.ownerName.isEmpty()) {
        throw PathError{
            PathErrorContext{"File owner is unavailable"_el, "The owner name could not be determined for the path."_el}
                .setSourcePath(path().toString())};
    }
    return _cache->data.ownerName;
}

auto PathInfo::ownerId() const noexcept -> UserId {
    ensureParts(PathInfoPart::OwnerId);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.ownerId;
}

auto PathInfo::ownerIdOrThrow() const -> UserId {
    ensurePartsOrThrow(PathInfoPart::OwnerId);
    if (isEmpty()) {
        throw PathError{"File owner identifier is unavailable"_el};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    if (_cache->data.ownerId.isEmpty()) {
        throw PathError{PathErrorContext{
            "File owner identifier is unavailable"_el, "The owner identifier could not be determined for the path."_el}
                .setSourcePath(path().toString())};
    }
    return _cache->data.ownerId;
}

auto PathInfo::groupName() const noexcept -> GroupName {
    ensureParts(PathInfoPart::GroupName);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.groupName;
}

auto PathInfo::groupNameOrThrow() const -> GroupName {
    ensurePartsOrThrow(PathInfoPart::GroupName);
    if (isEmpty()) {
        throw PathError{"File group is unavailable"_el};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    if (_cache->data.groupName.isEmpty()) {
        throw PathError{
            PathErrorContext{"File group is unavailable"_el, "The group name could not be determined for the path."_el}
                .setSourcePath(path().toString())};
    }
    return _cache->data.groupName;
}

auto PathInfo::groupId() const noexcept -> GroupId {
    ensureParts(PathInfoPart::GroupId);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.groupId;
}

auto PathInfo::groupIdOrThrow() const -> GroupId {
    ensurePartsOrThrow(PathInfoPart::GroupId);
    if (isEmpty()) {
        throw PathError{"File group identifier is unavailable"_el};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    if (_cache->data.groupId.isEmpty()) {
        throw PathError{PathErrorContext{
            "File group identifier is unavailable"_el, "The group identifier could not be determined for the path."_el}
                .setSourcePath(path().toString())};
    }
    return _cache->data.groupId;
}

auto PathInfo::accessInfo() const noexcept -> PathAccessInfo {
    ensureParts(PathInfoPart::AccessRights);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.accessInfo;
}

auto PathInfo::accessInfoOrThrow() const -> PathAccessInfo {
    ensurePartsOrThrow(PathInfoPart::AccessRights);
    if (isEmpty()) {
        throw PathError{PathErrorContext{
            "File permissions are unavailable"_el, "The access permissions could not be determined for the path."_el}
                .setSourcePath(path().toString())};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.accessInfo;
}

auto PathInfo::attributes() const noexcept -> PathAttributes {
    ensureParts(PathInfoPart::Attributes);
    if (isEmpty()) {
        return {};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.attributes;
}

auto PathInfo::attributesOrThrow() const -> PathAttributes {
    ensurePartsOrThrow(PathInfoPart::Attributes);
    if (isEmpty()) {
        throw PathError{PathErrorContext{
            "File attributes are unavailable"_el, "The attributes could not be determined for the path."_el}
                .setSourcePath(path().toString())};
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    return _cache->data.attributes;
}

auto PathInfo::hasAttribute(const PathAttribute attribute) const noexcept -> bool {
    return attributes().isSet(attribute);
}

void PathInfo::reload() {
    if (isEmpty()) {
        return;
    }
    auto parts = PathInfoParts{};
    {
        const auto lock = std::scoped_lock{_cache->mutex};
        parts = _cache->data.requestedParts;
    }
    if (parts.isEmpty()) {
        parts = PathInfoPart::Default;
    }
    try {
        reloadPartsOrThrow(parts);
    } catch (const PathError &) {}
}

void PathInfo::reload(const PathInfoParts parts) {
    if (isEmpty()) {
        return;
    }
    auto requestedParts = parts;
    if (requestedParts.isEmpty()) {
        requestedParts = PathInfoPart::Default;
    }
    try {
        reloadPartsOrThrow(requestedParts);
    } catch (const PathError &) {}
}

void PathInfo::ensureParts(const PathInfoParts parts) const noexcept {
    try {
        ensurePartsOrThrow(parts);
    } catch (const PathError &) {}
}

void PathInfo::ensurePartsOrThrow(const PathInfoParts parts) const {
    if (isEmpty() || parts.isEmpty()) {
        return;
    }
    const auto lock = std::scoped_lock{_cache->mutex};
    auto &data = _cache->data;
    data.requestedParts.set(parts);
    if (data.refreshFailed) {
        throw PathError{PathErrorContext{
            "Path information is unavailable"_el,
            "A previous refresh failed and the cached information is no longer valid."_el}
                .setSourcePath(_path.toString())
                .setHelp("Reload the path information and try again."_el)};
    }

    const auto missingParts = !data.loadedParts.contains(parts);
    const auto hasCache = data.lastRefresh != TimePoint{};
    const auto cacheExpired =
        _cacheTrust.expired() && hasCache && data.lastRefresh.timeDeltaToNow() > TimeDelta::seconds(1);
    if (missingParts || cacheExpired) {
        const auto trustResolvedPath = !cacheExpired && !data.resolvedPath.isEmpty();
        refreshDataOrThrow(data, trustResolvedPath);
    }
    resolveNames(parts, data);
}

void PathInfo::reloadPartsOrThrow(const PathInfoParts parts) const {
    const auto lock = std::scoped_lock{_cache->mutex};
    _cache->data.requestedParts = parts;
    refreshDataOrThrow(_cache->data, false);
    resolveNames(parts, _cache->data);
}

void PathInfo::refreshDataOrThrow(impl::PathInfoData &data, const bool trustResolvedPath) const {
    auto requestedParts = data.requestedParts | PathInfoPart::Type;
    if (requestedParts.isSet(PathInfoPart::OwnerName)) {
        requestedParts.set(PathInfoPart::OwnerId);
    }
    if (requestedParts.isSet(PathInfoPart::GroupName)) {
        requestedParts.set(PathInfoPart::GroupId);
    }
    try {
        auto loadedData = trustResolvedPath
            ? impl::pathBackend().loadResolvedInfoOrThrow(_path, data.resolvedPath, requestedParts)
            : impl::pathBackend().loadInfoOrThrow(_path, requestedParts);
        loadedData.requestedParts = data.requestedParts;
        loadedData.refreshFailed = false;
        data = std::move(loadedData);
    } catch (const PathError &) {
        const auto requested = data.requestedParts;
        data = {};
        data.requestedParts = requested;
        data.refreshFailed = true;
        data.lastRefresh = TimePoint::now();
        throw;
    }
}

void PathInfo::resolveNames(const PathInfoParts parts, impl::PathInfoData &data) const {
    const auto needsOwner = parts.isSet(PathInfoPart::OwnerName) && !data.ownerId.isEmpty() && data.ownerName.isEmpty();
    const auto needsGroup = parts.isSet(PathInfoPart::GroupName) && !data.groupId.isEmpty() && data.groupName.isEmpty();
    if (!needsOwner && !needsGroup) {
        return;
    }
    auto &lookup = core::application().userLookup();
    if (needsOwner) {
        try {
            data.ownerName = lookup.userNameForId(data.ownerId);
        } catch (const PlatformError &error) {
            throw PathError{PathErrorContext{
                "File owner is unavailable"_el,
                "The operating system could not resolve the owner name for the path."_el}
                    .setSourcePath(_path.toString())
                    .setPlatformContext(error.context())};
        }
        data.loadedParts.set(PathInfoPart::OwnerName);
    }
    if (needsGroup) {
        try {
            data.groupName = lookup.groupNameForId(data.groupId);
        } catch (const PlatformError &error) {
            throw PathError{PathErrorContext{
                "File group is unavailable"_el,
                "The operating system could not resolve the group name for the path."_el}
                    .setSourcePath(_path.toString())
                    .setPlatformContext(error.context())};
        }
        data.loadedParts.set(PathInfoPart::GroupName);
    }
}

}
