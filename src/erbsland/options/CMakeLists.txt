# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        Option.cpp
        Option.hpp
        Option_fwd.hpp
        OptionCallback.hpp
        OptionChoice.cpp
        OptionChoice.hpp
        OptionChoice_fwd.hpp
        OptionChoices.cpp
        OptionChoices.hpp
        OptionChoices_fwd.hpp
        OptionDisplayInfo.hpp
        OptionEditor.cpp
        OptionEditor.hpp
        OptionErrorContext.hpp
        OptionErrorContext_fwd.hpp
        OptionErrorReason.hpp
        OptionFlag.hpp
        OptionHelp.cpp
        OptionHelp.hpp
        OptionHelpVisibility.hpp
        OptionInteger.hpp
        OptionManager.cpp
        OptionManager.hpp
        OptionModule.cpp
        OptionModule.hpp
        OptionModule_fwd.hpp
        OptionRenderer.cpp
        OptionRenderer.hpp
        OptionRenderer_fwd.hpp
        OptionResult.hpp
        OptionResult_fwd.hpp
        OptionResultStatus.hpp
        Options.cpp
        Options.hpp
        Options_fwd.hpp
        OptionSet.cpp
        OptionSet.hpp
        OptionSet_fwd.hpp
        OptionSetManager.cpp
        OptionSetManager.hpp
        OptionType.cpp
        OptionType.hpp
        OptionValue.cpp
        OptionValue.hpp
        OptionValue_fwd.hpp
        OptionValueNameHash.hpp
        OptionValues.cpp
        OptionValues.hpp
        OptionValues_fwd.hpp
        OptionValueStorage.hpp
        OptionValueType.hpp
        StandardOptionRenderer.cpp
        StandardOptionRenderer.hpp
        StandardOptionRenderer_fwd.hpp
)
