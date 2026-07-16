// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

// Functions and macros for low-level engine debugging
// ---------------------------------------------------
// Important:
// 1. Enabling this debugging code will significantly increase memory usage and runtime overhead.
// 2. We will remove this debugging code without further notice.

#undef ERBSLAND_RE_ENGINE_DEBUG_ENABLED
// #define ERBSLAND_RE_ENGINE_DEBUG_ENABLED

#ifdef ERBSLAND_RE_ENGINE_DEBUG_ENABLED
#include "../diagnostics/OperationData.hpp"

#include "../../../stream/StandardStreams.hpp"
#include "../../../text/StringFormat.hpp"

#include <list>

namespace erbsland::re::impl {

enum class EDMType : uint8_t {
    Run,       ///< State at `run()`
    Match,     ///< State at `MATCH`
    Operation, ///< An executed operation
    Advance,   ///< Record cursor advance

    All,       ///< All states
};

using EngineDebugMessages = std::list<std::pair<EDMType, text::String>>;

[[nodiscard]] inline auto engineDebugMessages() -> EngineDebugMessages & {
    static EngineDebugMessages messages;
    return messages;
}

template <typename Fwd>
void addEngineDebugMessage(EDMType type, Fwd &&message) {
    engineDebugMessages().emplace_back(type, std::forward<Fwd>(message));
    if (engineDebugMessages().size() > 10'000) {
        engineDebugMessages().pop_front();
    }
}

inline void printEngineDebugMessages(EDMType type = EDMType::All) {
    for (const auto &[msgType, message] : engineDebugMessages()) {
        if (type == EDMType::All || msgType == type) {
            stream::stdOut()->writeLine(message);
        }
    }
}

}

#define ERBSLAND_RE_ENGINE_DEBUG(type, ...) ::erbsland::re::impl::addEngineDebugMessage(type, __VA_ARGS__);

#define ERBSLAND_RE_ENGINE_DEBUG_REC_STATE(type, fnName, stateVar)                                                     \
    ERBSLAND_RE_ENGINE_DEBUG(type, ::erbsland::text::StringFormat{"{}: {}"}.build(fnName, stateVar.toDebugString()))

#define ERBSLAND_RE_ENGINE_DEBUG_OP_VAR()                                                                              \
    const auto debugOperation = _programDecoder.peekOperation(thread.programCounter);
#define ERBSLAND_RE_ENGINE_DEBUG_MOP0(matchesExpr)                                                                     \
    ERBSLAND_RE_ENGINE_DEBUG(                                                                                          \
        EDMType::Operation,                                                                                            \
        ::erbsland::text::StringFormat{                                                                                \
            "${:04X}: {} -> matches: {}",                                                                              \
        }                                                                                                              \
            .build(thread.programCounter, toString(debugOperation), matchesExpr))
#define ERBSLAND_RE_ENGINE_DEBUG_MOP1(arg1, matchesExpr)                                                               \
    ERBSLAND_RE_ENGINE_DEBUG(                                                                                          \
        EDMType::Operation,                                                                                            \
        ::erbsland::text::StringFormat{                                                                                \
            "${:04X}: {} {} -> matches: {}",                                                                           \
        }                                                                                                              \
            .build(thread.programCounter, toString(debugOperation), arg1, matchesExpr))
#define ERBSLAND_RE_ENGINE_DEBUG_MOP2(arg1, arg2, matchesExpr)                                                         \
    ERBSLAND_RE_ENGINE_DEBUG(                                                                                          \
        EDMType::Operation,                                                                                            \
        ::erbsland::text::StringFormat{                                                                                \
            "${:04X}: {} {}, {} -> matches: {}",                                                                       \
        }                                                                                                              \
            .build(thread.programCounter, toString(debugOperation), arg1, arg2, matchesExpr))
#define ERBSLAND_RE_ENGINE_DEBUG_OP1(arg1)                                                                             \
    ERBSLAND_RE_ENGINE_DEBUG(                                                                                          \
        EDMType::Operation,                                                                                            \
        ::erbsland::text::StringFormat{                                                                                \
            "${:04X}: {} {}",                                                                                          \
        }                                                                                                              \
            .build(thread.programCounter, toString(debugOperation), arg1))
#define ERBSLAND_RE_ENGINE_DEBUG_OP2(arg1, arg2)                                                                       \
    ERBSLAND_RE_ENGINE_DEBUG(                                                                                          \
        EDMType::Operation,                                                                                            \
        ::erbsland::text::StringFormat{                                                                                \
            "${:04X}: {} {}",                                                                                          \
        }                                                                                                              \
            .build(thread.programCounter, toString(debugOperation), arg1, arg2))
#define ERBSLAND_RE_ENGINE_DEBUG_OP1PC(pc)                                                                             \
    ERBSLAND_RE_ENGINE_DEBUG(                                                                                          \
        EDMType::Operation,                                                                                            \
        ::erbsland::text::StringFormat{                                                                                \
            "${:04}: {} ${:04X}",                                                                                      \
        }                                                                                                              \
            .build(thread.programCounter, toString(debugOperation), pc))
#define ERBSLAND_RE_ENGINE_DEBUG_OP2PC(pc1, pc2)                                                                       \
    ERBSLAND_RE_ENGINE_DEBUG(                                                                                          \
        EDMType::Operation,                                                                                            \
        ::erbsland::text::StringFormat{                                                                                \
            "${:04}: {} ${:04X}, ${:04X}",                                                                             \
        }                                                                                                              \
            .build(thread.programCounter, toString(debugOperation), pc1, pc2))
#else
#define ERBSLAND_RE_ENGINE_DEBUG
#define ERBSLAND_RE_ENGINE_DEBUG_REC_STATE(type, fnName, stateVar)
#define ERBSLAND_RE_ENGINE_DEBUG_OP_VAR()
#define ERBSLAND_RE_ENGINE_DEBUG_MOP0(matchesVar)
#define ERBSLAND_RE_ENGINE_DEBUG_MOP1(arg1, matchesVar)
#define ERBSLAND_RE_ENGINE_DEBUG_MOP2(arg1, arg2, matchesVar)
#define ERBSLAND_RE_ENGINE_DEBUG_OP1(arg1)
#define ERBSLAND_RE_ENGINE_DEBUG_OP2(arg1, arg2)
#define ERBSLAND_RE_ENGINE_DEBUG_OP1PC(pc)
#define ERBSLAND_RE_ENGINE_DEBUG_OP2PC(pc1, pc2)
#endif
