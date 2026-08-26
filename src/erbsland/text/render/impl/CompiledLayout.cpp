// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CompiledLayout.hpp"

#include "CompiledBlock.hpp"
#include "CompiledExtends.hpp"

namespace erbsland::text::render::impl {

CompiledLayout::CompiledLayout(
    String name,
    LayoutSource source,
    StringList constants,
    Program setupProgram,
    Program bodyProgram,
    StringMap<FilterFn> applicationFilters,
    StringMap<ConstCompiledBlockPtr> blocks,
    std::vector<ConstCompiledIncludePtr> includes,
    ConstCompiledExtendsPtr extends) noexcept :
    _name{std::move(name)},
    _source{std::move(source)},
    _constants{std::move(constants)},
    _setupProgram{std::move(setupProgram)},
    _bodyProgram{std::move(bodyProgram)},
    _applicationFilters{std::move(applicationFilters)},
    _blocks{std::move(blocks)},
    _includes{std::move(includes)},
    _extends{std::move(extends)} {
    buildInheritanceIndexes();
}

CompiledLayout::CompiledLayout(
    String name,
    LayoutSource source,
    StringList constants,
    Program program,
    std::vector<ConstCompiledIncludePtr> includes) noexcept :
    CompiledLayout{
        std::move(name),
        std::move(source),
        std::move(constants),
        Program{},
        std::move(program),
        {},
        {},
        std::move(includes),
        {}} {
}

auto CompiledLayout::applicationFilter(const String &name) const noexcept -> const FilterFn * {
    const auto iterator = _applicationFilters.toRawValue().find(name);
    return iterator == _applicationFilters.toRawValue().end() ? nullptr : &iterator->second;
}

auto CompiledLayout::block(const String &name) const noexcept -> ConstCompiledBlockPtr {
    const auto iterator = _blocks.toRawValue().find(name);
    if (iterator == _blocks.toRawValue().end()) {
        return nullptr;
    }
    return iterator->second;
}

auto CompiledLayout::blockChain(const String &name) const noexcept -> ConstBlockChainPtr {
    const auto iterator = _blockChains.toRawValue().find(name);
    if (iterator == _blockChains.toRawValue().end()) {
        return nullptr;
    }
    return iterator->second;
}

void CompiledLayout::buildInheritanceIndexes() {
    if (_extends != nullptr && _extends->layout() != nullptr) {
        _ancestors.emplace_back(_extends->layout());
        _ancestors.insert(
            _ancestors.end(), _extends->layout()->ancestors().begin(), _extends->layout()->ancestors().end());
    }

    auto mutableChains = StringMap<std::shared_ptr<BlockChain>>{};
    for (const auto &[name, block] : _blocks) {
        mutableChains.set(name, std::make_shared<BlockChain>(BlockChain{BlockImplementation{0U, block}}));
    }
    for (auto level = std::size_t{}; level < _ancestors.size(); ++level) {
        for (const auto &[name, block] : _ancestors[level]->blocks()) {
            auto chain = mutableChains.get(name, std::shared_ptr<BlockChain>{});
            if (chain == nullptr) {
                chain = std::make_shared<BlockChain>();
                mutableChains.set(name, chain);
            }
            chain->emplace_back(BlockImplementation{level + 1U, block});
        }
    }
    for (const auto &[name, chain] : mutableChains) {
        _blockChains.set(name, chain);
    }
}

}
