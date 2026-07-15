// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathInfo.hpp"

#include "PathError.hpp"

#include "impl/BackendFactory.hpp"

#include "../core/Application.hpp"
#include "../system/PlatformError.hpp"
#include "../system/UserLookup.hpp"
#include "../text/Literals.hpp"
#include "../time/TimeDelta.hpp"

#include <utility>

namespace erbsland::path {

using namespace text::literals;

PathInfo::PathInfo(const Path &path, const PathInfoParts parts) noexcept {
    if (path.isEmpty()) {
        return;
    }
    _data = impl::PathInfoDataPtr{new impl::PathInfoData{path}};
    _data->requestedParts = parts;
    ensureParts(parts);
}

auto PathInfo::isEmpty() const noexcept -> bool {
    return _data.isNull();
}

auto PathInfo::path() const noexcept -> const Path & {
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? Path::empty() : pathInfoData->originalPath;
}

auto PathInfo::resolvedPath() const noexcept -> const Path & {
    ensureParts(PathInfoPart::Type);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? Path::empty() : pathInfoData->resolvedPath;
}

auto PathInfo::exists() const noexcept -> bool {
    ensureParts(PathInfoPart::Type);
    const auto *pathInfoData = data();
    return pathInfoData != nullptr && pathInfoData->exists;
}

auto PathInfo::type() const noexcept -> PathType {
    ensureParts(PathInfoPart::Type);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? PathType::Unknown : pathInfoData->type;
}

auto PathInfo::fileSize() const noexcept -> unit::ByteLength {
    ensureParts(PathInfoPart::Size);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? unit::ByteLength{} : pathInfoData->fileSize;
}

auto PathInfo::lastModified() const noexcept -> time::DateTime {
    ensureParts(PathInfoPart::Times);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? time::DateTime{} : pathInfoData->lastModified;
}

auto PathInfo::lastAccessed() const noexcept -> time::DateTime {
    ensureParts(PathInfoPart::Times);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? time::DateTime{} : pathInfoData->lastAccessed;
}

auto PathInfo::birthTime() const noexcept -> time::DateTime {
    ensureParts(PathInfoPart::Times);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? time::DateTime{} : pathInfoData->birthTime;
}

auto PathInfo::lastMetadataChange() const noexcept -> time::DateTime {
    ensureParts(PathInfoPart::Times);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? time::DateTime{} : pathInfoData->lastMetadataChange;
}

auto PathInfo::creationTime() const noexcept -> time::DateTime {
    ensureParts(PathInfoPart::Times);
    const auto *pathInfoData = data();
    if (pathInfoData == nullptr) {
        return {};
    }
    if (pathInfoData->birthTime.isValid()) {
        return pathInfoData->birthTime;
    }
    if (pathInfoData->lastMetadataChange.isValid()) {
        return pathInfoData->lastMetadataChange;
    }
    return pathInfoData->lastModified;
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

auto PathInfo::ownerName() const noexcept -> system::UserName {
    ensureParts(PathInfoPart::OwnerName);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? system::UserName{} : pathInfoData->ownerName;
}

auto PathInfo::ownerNameOrThrow() const -> system::UserName {
    ensurePartsOrThrow(PathInfoPart::OwnerName);
    const auto *pathInfoData = data();
    if (pathInfoData == nullptr || pathInfoData->ownerName.isEmpty()) {
        throw PathError{
            PathErrorContext{"File owner is unavailable"_el, "The owner name could not be determined for the path."_el}
                .setSourcePath(path().toString())};
    }
    return pathInfoData->ownerName;
}

auto PathInfo::ownerId() const noexcept -> system::UserId {
    ensureParts(PathInfoPart::OwnerId);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? system::UserId{} : pathInfoData->ownerId;
}

auto PathInfo::ownerIdOrThrow() const -> system::UserId {
    ensurePartsOrThrow(PathInfoPart::OwnerId);
    const auto *pathInfoData = data();
    if (pathInfoData == nullptr || pathInfoData->ownerId.isEmpty()) {
        throw PathError{PathErrorContext{
            "File owner identifier is unavailable"_el, "The owner identifier could not be determined for the path."_el}
                .setSourcePath(path().toString())};
    }
    return pathInfoData->ownerId;
}

auto PathInfo::groupName() const noexcept -> system::GroupName {
    ensureParts(PathInfoPart::GroupName);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? system::GroupName{} : pathInfoData->groupName;
}

auto PathInfo::groupNameOrThrow() const -> system::GroupName {
    ensurePartsOrThrow(PathInfoPart::GroupName);
    const auto *pathInfoData = data();
    if (pathInfoData == nullptr || pathInfoData->groupName.isEmpty()) {
        throw PathError{
            PathErrorContext{"File group is unavailable"_el, "The group name could not be determined for the path."_el}
                .setSourcePath(path().toString())};
    }
    return pathInfoData->groupName;
}

auto PathInfo::groupId() const noexcept -> system::GroupId {
    ensureParts(PathInfoPart::GroupId);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? system::GroupId{} : pathInfoData->groupId;
}

auto PathInfo::groupIdOrThrow() const -> system::GroupId {
    ensurePartsOrThrow(PathInfoPart::GroupId);
    const auto *pathInfoData = data();
    if (pathInfoData == nullptr || pathInfoData->groupId.isEmpty()) {
        throw PathError{PathErrorContext{
            "File group identifier is unavailable"_el, "The group identifier could not be determined for the path."_el}
                .setSourcePath(path().toString())};
    }
    return pathInfoData->groupId;
}

auto PathInfo::accessInfo() const noexcept -> PathAccessInfo {
    ensureParts(PathInfoPart::AccessRights);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? PathAccessInfo{} : pathInfoData->accessInfo;
}

auto PathInfo::accessInfoOrThrow() const -> PathAccessInfo {
    ensurePartsOrThrow(PathInfoPart::AccessRights);
    const auto *pathInfoData = data();
    if (pathInfoData == nullptr) {
        throw PathError{PathErrorContext{
            "File permissions are unavailable"_el, "The access permissions could not be determined for the path."_el}
                .setSourcePath(path().toString())};
    }
    return pathInfoData->accessInfo;
}

auto PathInfo::attributes() const noexcept -> PathAttributes {
    ensureParts(PathInfoPart::Attributes);
    const auto *pathInfoData = data();
    return pathInfoData == nullptr ? PathAttributes{} : pathInfoData->attributes;
}

auto PathInfo::attributesOrThrow() const -> PathAttributes {
    ensurePartsOrThrow(PathInfoPart::Attributes);
    const auto *pathInfoData = data();
    if (pathInfoData == nullptr) {
        throw PathError{PathErrorContext{
            "File attributes are unavailable"_el, "The attributes could not be determined for the path."_el}
                .setSourcePath(path().toString())};
    }
    return pathInfoData->attributes;
}

auto PathInfo::hasAttribute(const PathAttribute attribute) const noexcept -> bool {
    return attributes().isSet(attribute);
}

void PathInfo::reload() {
    const auto *pathInfoData = data();
    if (pathInfoData == nullptr) {
        return;
    }
    auto parts = pathInfoData->requestedParts;
    if (parts.isEmpty()) {
        parts = PathInfoPart::Default;
    }
    try {
        ensurePartsOrThrow(parts, true);
    } catch (const PathError &) {}
}

void PathInfo::reload(const PathInfoParts parts) {
    auto *pathInfoData = mutableData();
    if (pathInfoData == nullptr) {
        return;
    }
    auto requestedParts = parts;
    if (requestedParts.isEmpty()) {
        requestedParts = PathInfoPart::Default;
    }
    pathInfoData->requestedParts = requestedParts;
    try {
        ensurePartsOrThrow(requestedParts, true);
    } catch (const PathError &) {}
}

auto PathInfo::data() const noexcept -> const impl::PathInfoData * {
    return _data.isNull() ? nullptr : _data.constGet();
}

auto PathInfo::mutableData() const noexcept -> impl::PathInfoData * {
    return _data.isNull() ? nullptr : _data.get();
}

void PathInfo::ensureParts(const PathInfoParts parts) const noexcept {
    try {
        ensurePartsOrThrow(parts);
    } catch (const PathError &) {}
}

void PathInfo::ensurePartsOrThrow(const PathInfoParts parts, const bool forceReload) const {
    auto *pathInfoData = mutableData();
    if (pathInfoData == nullptr || parts.isEmpty()) {
        return;
    }
    pathInfoData->requestedParts.set(parts);
    if (pathInfoData->refreshFailed && !forceReload) {
        throw PathError{PathErrorContext{
            "Path information is unavailable"_el,
            "A previous refresh failed and the cached information is no longer valid."_el}
                .setSourcePath(pathInfoData->originalPath.toString())
                .setHelp("Reload the path information and try again."_el)};
    }

    const auto missingParts = !pathInfoData->loadedParts.contains(parts);
    const auto hasCache = pathInfoData->lastRefresh != time::TimePoint{};
    const auto cacheExpired = hasCache && pathInfoData->lastRefresh.timeDeltaToNow() > time::TimeDelta::seconds(1);
    if (forceReload || missingParts || cacheExpired) {
        const auto originalPath = pathInfoData->originalPath;
        auto requestedParts = pathInfoData->requestedParts | PathInfoPart::Type;
        if (requestedParts.isSet(PathInfoPart::OwnerName)) {
            requestedParts.set(PathInfoPart::OwnerId);
        }
        if (requestedParts.isSet(PathInfoPart::GroupName)) {
            requestedParts.set(PathInfoPart::GroupId);
        }
        try {
            auto loadedData = impl::pathBackend().loadInfoOrThrow(originalPath, requestedParts);
            loadedData.requestedParts = pathInfoData->requestedParts;
            loadedData.refreshFailed = false;
            *pathInfoData = std::move(loadedData);
        } catch (const PathError &) {
            pathInfoData->resolvedPath = {};
            pathInfoData->loadedParts.clear();
            pathInfoData->exists = false;
            pathInfoData->type = PathType::Unknown;
            pathInfoData->fileSize = {};
            pathInfoData->lastModified = {};
            pathInfoData->lastAccessed = {};
            pathInfoData->birthTime = {};
            pathInfoData->lastMetadataChange = {};
            pathInfoData->ownerName = {};
            pathInfoData->ownerId = {};
            pathInfoData->groupName = {};
            pathInfoData->groupId = {};
            pathInfoData->accessInfo = {};
            pathInfoData->attributes = {};
            pathInfoData->refreshFailed = true;
            pathInfoData->lastRefresh = time::TimePoint::now();
            throw;
        }
    }
    resolveNames(parts);
}

void PathInfo::resolveNames(const PathInfoParts parts) const {
    auto *pathInfoData = mutableData();
    if (pathInfoData == nullptr) {
        return;
    }
    const auto needsOwner =
        parts.isSet(PathInfoPart::OwnerName) && !pathInfoData->ownerId.isEmpty() && pathInfoData->ownerName.isEmpty();
    const auto needsGroup =
        parts.isSet(PathInfoPart::GroupName) && !pathInfoData->groupId.isEmpty() && pathInfoData->groupName.isEmpty();
    if (!needsOwner && !needsGroup) {
        return;
    }
    auto &lookup = core::application().userLookup();
    if (needsOwner) {
        try {
            pathInfoData->ownerName = lookup.userNameForId(pathInfoData->ownerId);
        } catch (const system::PlatformError &error) {
            throw PathError{PathErrorContext{
                "File owner is unavailable"_el,
                "The operating system could not resolve the owner name for the path."_el}
                    .setSourcePath(pathInfoData->originalPath.toString())
                    .setPlatformContext(error.context())};
        }
        pathInfoData->loadedParts.set(PathInfoPart::OwnerName);
    }
    if (needsGroup) {
        try {
            pathInfoData->groupName = lookup.groupNameForId(pathInfoData->groupId);
        } catch (const system::PlatformError &error) {
            throw PathError{PathErrorContext{
                "File group is unavailable"_el,
                "The operating system could not resolve the group name for the path."_el}
                    .setSourcePath(pathInfoData->originalPath.toString())
                    .setPlatformContext(error.context())};
        }
        pathInfoData->loadedParts.set(PathInfoPart::GroupName);
    }
}

}
