#pragma once

#include <memory>

namespace erbsland::cryptology::impl {

class TlsSigner;
using TlsSignerPtr = std::shared_ptr<const TlsSigner>;

}
