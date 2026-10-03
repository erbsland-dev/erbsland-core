#include "SecureStorage.hpp"

#include "SecureStorageError.hpp"

#include "impl/secure_storage/SecureStorageBackend.hpp"

#include "../text/Literals.hpp"
#include "../text/u8/U8StringConstIterator.hpp"
#include "../unit/ByteCount.hpp"

namespace erbsland::cryptology {

using namespace text;
using namespace text::literals;

SecureStorage::SecureStorage(String applicationNamespace, const SecureStorageInteraction interaction) :
    _namespace{std::move(applicationNamespace)},
    _interaction{interaction},
    _backend{impl::SecureStorageBackend::create()} {
    if (!isValidName(_namespace)) {
        throw SecureStorageError{SecureStorageErrorCategory::InvalidName, "Invalid secure-storage namespace."_el};
    }
}

auto SecureStorage::isValidName(const String &name) noexcept -> bool {
    if (name.isEmpty() || name.length() > unit::ByteCount{256U} || !name.isEncodingValid()) {
        return false;
    }
    for (const auto character : name) {
        if (character.isControl()) {
            return false;
        }
    }
    return true;
}

auto SecureStorage::capabilities() const noexcept -> SecureStorageCapabilities {
    return _backend->capabilities();
}

auto SecureStorage::read(const String &item) const -> std::optional<ProtectedByteBlock> {
    if (!isValidName(item)) {
        throw SecureStorageError{SecureStorageErrorCategory::InvalidName, "Invalid secure-storage item name."_el};
    }
    auto result = _backend->read(_namespace, item, _interaction);
    if (result.has_value() && result->byteLength() > maximumPayloadLength()) {
        throw SecureStorageError{
            SecureStorageErrorCategory::SizeLimit, "Stored secret exceeds the 2,560-byte limit."_el};
    }
    return result;
}

void SecureStorage::write(const String &item, const mem::ConstByteSpan data) const {
    if (!isValidName(item)) {
        throw SecureStorageError{SecureStorageErrorCategory::InvalidName, "Invalid secure-storage item name."_el};
    }
    if (data.size() > maximumPayloadLength().toSizeT()) {
        throw SecureStorageError{SecureStorageErrorCategory::SizeLimit, "Secret exceeds the 2,560-byte limit."_el};
    }
    _backend->write(_namespace, item, data, _interaction);
}

void SecureStorage::write(const String &item, const ProtectedByteBlock &data) const {
    if (data.byteLength() > maximumPayloadLength()) {
        throw SecureStorageError{SecureStorageErrorCategory::SizeLimit, "Secret exceeds the 2,560-byte limit."_el};
    }
    data.withUnprotectedData([&](const mem::ConstByteSpan plaintext) -> void { write(item, plaintext); });
}

auto SecureStorage::remove(const String &item) const -> bool {
    if (!isValidName(item)) {
        throw SecureStorageError{SecureStorageErrorCategory::InvalidName, "Invalid secure-storage item name."_el};
    }
    return _backend->remove(_namespace, item, _interaction);
}

}
