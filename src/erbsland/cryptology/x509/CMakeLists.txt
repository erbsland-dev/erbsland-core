# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        X509AlgorithmIdentifier.cpp
        X509AlgorithmIdentifier.hpp
        X509AlgorithmIdentifier_fwd.hpp
        X509BasicConstraints.hpp
        X509Certificate.cpp
        X509Certificate.hpp
        X509Certificate_fwd.hpp
        X509CertificateBuilder.cpp
        X509CertificateBuilder.hpp
        X509CertificateBuilder_fwd.hpp
        X509CertificateBundle.cpp
        X509CertificateBundle.hpp
        X509CertificateBundle_fwd.hpp
        X509CertificateProfile.hpp
        X509CertificateProfileIssue.hpp
        X509CertificateProfileIssueCategory.hpp
        X509CertificateProfileMode.hpp
        X509CertificateSigningRequest.cpp
        X509CertificateSigningRequest.hpp
        X509CertificateSigningRequest_fwd.hpp
        X509CertificateValidation.hpp
        X509CertificateValidationFailure.hpp
        X509CertificateValidationFailureCategory.hpp
        X509Extension.hpp
        X509Extension_fwd.hpp
        X509GeneralName.hpp
        X509KeyUsage.hpp
        X509Name.cpp
        X509Name.hpp
        X509Name_fwd.hpp
        X509NameAttribute.hpp
        X509RelativeDistinguishedName.hpp
        X509ServerCertificatePolicy.cpp
        X509ServerCertificatePolicy.hpp
        X509Version.hpp
)
