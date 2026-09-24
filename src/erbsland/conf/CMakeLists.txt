# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)
add_subdirectory(vr)

target_sources(erbsland-core PRIVATE
        AccessCheck.hpp
        AccessCheck_fwd.hpp
        AccessCheckResult.hpp
        AccessSources.hpp
        ConfError.cpp
        ConfError.hpp
        ConfErrorCategory.cpp
        ConfErrorCategory.hpp
        ConfErrorContext.cpp
        ConfErrorContext.hpp
        Document.hpp
        Document_fwd.hpp
        DocumentBuilder.hpp
        FileAccessCheck.cpp
        FileAccessCheck.hpp
        FileAccessCheck_fwd.hpp
        FileSourceResolver.cpp
        FileSourceResolver.hpp
        FileSourceResolver_fwd.hpp
        Float.hpp
        Integer.hpp
        Location.cpp
        Location.hpp
        Matrix.hpp
        Name.cpp
        Name.hpp
        NamePath.cpp
        NamePath.hpp
        NameType.hpp
        Parser.cpp
        Parser.hpp
        PlaceholderFilter.hpp
        PlaceholderFilter_fwd.hpp
        PlaceholderSource.hpp
        PlaceholderSource_fwd.hpp
        SignatureSigner.hpp
        SignatureSigner_fwd.hpp
        SignatureSignerData.hpp
        SignatureValidator.hpp
        SignatureValidator_fwd.hpp
        SignatureValidatorData.hpp
        SignatureValidatorResult.hpp
        Signer.cpp
        Signer.hpp
        Source.cpp
        Source.hpp
        Source_fwd.hpp
        SourceIdentifier.cpp
        SourceIdentifier.hpp
        SourceIdentifier_fwd.hpp
        SourceResolver.hpp
        SourceResolver_fwd.hpp
        SourceResolverContext.hpp
        StdFormat.hpp
        TestFormat.hpp
        Value.cpp
        Value.hpp
        Value_asType.tpp
        Value_fwd.hpp
        Value_get.tpp
        Value_list.tpp
        Value_matrix.tpp
        ValueIterator.cpp
        ValueIterator.hpp
        ValueList.hpp
        ValueMatrix.hpp
        ValueType.cpp
        ValueType.hpp
)
