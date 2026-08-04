# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Anchor.hpp
        CharacterCategory.hpp
        CharacterClass.hpp
        CharacterSequence.hpp
        Group.hpp
        GroupFlag.hpp
        GroupFlags.hpp
        NodeData.hpp
        Quantifier.hpp
        Sequence.hpp
)
