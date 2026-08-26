// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("BasicTests"_el, basicTests);
    app.registerDemo("ByteRangeSlicing"_el, byteRangeSlicing);
    app.registerDemo("CanonicalStationName"_el, canonicalStationName);
    app.registerDemo("CaseTransformation"_el, caseTransformation);
    app.registerDemo("CharacterAccess"_el, characterAccess);
    app.registerDemo("CharacterReplacement"_el, characterReplacement);
    app.registerDemo("CharacterSetCategories"_el, characterSetCategories);
    app.registerDemo("CharacterSetCharacters"_el, characterSetCharacters);
    app.registerDemo("CharacterSetEmpty"_el, characterSetEmpty);
    app.registerDemo("CharacterSetPatterns"_el, characterSetPatterns);
    app.registerDemo("CharacterSetReusable"_el, characterSetReusable);
    app.registerDemo("CharacterSetText"_el, characterSetText);
    app.registerDemo("CombineCharacterSets"_el, combineCharacterSets);
    app.registerDemo("Comparison"_el, comparison);
    app.registerDemo("ComparisonFunctions"_el, comparisonFunctions);
    app.registerDemo("ConstructionAndStorage"_el, constructionAndStorage);
    app.registerDemo("CodePointRangeSlicing"_el, codePointRangeSlicing);
    app.registerDemo("EfficientLiteralUsage"_el, efficientLiteralUsage);
    app.registerDemo("FirstAndLastCharacter"_el, firstAndLastCharacter);
    app.registerDemo("FindingAllOccurrences"_el, findingAllOccurrences);
    app.registerDemo("FindingCharacterSets"_el, findingCharacterSets);
    app.registerDemo("FrontBackSlicing"_el, frontBackSlicing);
    app.registerDemo("Hashing"_el, hashing);
    app.registerDemo("IdealFunctionParameter"_el, idealFunctionParameter);
    app.registerDemo("Indexing"_el, indexing);
    app.registerDemo("IteratingCharacters"_el, iteratingCharacters);
    app.registerDemo("JoiningText"_el, joiningText);
    app.registerDemo("LengthAttributes"_el, lengthAttributes);
    app.registerDemo("NormalizationForms"_el, normalizationForms);
    app.registerDemo("NormalizeJoinedText"_el, normalizeJoinedText);
    app.registerDemo("PartialStringComparison"_el, partialStringComparison);
    app.registerDemo("RemoveCharacters"_el, removeCharacters);
    app.registerDemo("SliceAndKept"_el, sliceAndKept);
    app.registerDemo("SliceBoundaries"_el, sliceBoundaries);
    app.registerDemo("SequentialCharacterReading"_el, sequentialCharacterReading);
    app.registerDemo("StorageIdentifier"_el, storageIdentifier);
    app.registerDemo("SplittingText"_el, splittingText);
    app.registerDemo("StdVsCoreCaseFold"_el, stdVsCoreCaseFold);
    app.registerDemo("StdVsCoreEscape"_el, stdVsCoreEscape);
    app.registerDemo("StdVsCoreSplitAndJoin"_el, stdVsCoreSplitAndJoin);
    app.registerDemo("TestForCharacters"_el, testForCharacters);
    app.registerDemo("TestStartsEndsAndContains"_el, testStartsEndsAndContains);
    app.registerDemo("TokenRuns"_el, tokenRuns);
    app.registerDemo("TrimCharacterSet"_el, trimCharacterSet);
    app.registerDemo("ValidateCharacterPolicy"_el, validateCharacterPolicy);
    app.registerDemo("WholeStringComparison"_el, wholeStringComparison);
    return app.run();
}
