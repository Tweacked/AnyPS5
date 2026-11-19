// Derived from neonmodder123/PS5-WebKit-11.x/Source/WTF/wtf/text/TextBreakIterator.cpp@80f77e94c63ee7202eec3d4b5b1a021dd89011ba
/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004-2023 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "prx/libSceNKWebKit/include/GraphemeClusterExports.hpp"
#include <unicode/uchar.h>
#include <unicode/utf16.h>

#include <limits>
#include <stdexcept>

namespace {

using NKWebKit::WtfAbi::StringView;

char16_t Latin1Unit(const StringView& string, unsigned index) {
    return string.is8Bit ? static_cast<const unsigned char*>(string.data)[index] : static_cast<const char16_t*>(string.data)[index];
}

bool IsLatin1(const StringView& string) {
    if (!string.data) throw std::invalid_argument("null WTF character buffer");
    if (string.is8Bit) return true;
    const auto* characters = static_cast<const char16_t*>(string.data);
    for (unsigned i = 0; i < string.length; ++i) {
        if (characters[i] > 0xff) return false;
    }
    return true;
}

enum class IndicTail { None, Consonant, Linked };

struct GraphemeContext {
    int previous = U_GCB_OTHER;
    IndicTail indicTail = IndicTail::None;
    bool pictographTail = false;
    bool afterPictographJoiner = false;
    bool oddRegionalIndicators = false;
};

bool IsControl(int property) {
    return property == U_GCB_CONTROL || property == U_GCB_CR || property == U_GCB_LF;
}

bool IsBoundary(const GraphemeContext& context, int next, int indic, bool pictographic) {
    const auto previous = context.previous;
    if (previous == U_GCB_CR && next == U_GCB_LF) return false;
    if (IsControl(previous) || IsControl(next)) return true;
    if (previous == U_GCB_L && (next == U_GCB_L || next == U_GCB_V || next == U_GCB_LV || next == U_GCB_LVT)) return false;
    if ((previous == U_GCB_LV || previous == U_GCB_V) && (next == U_GCB_V || next == U_GCB_T)) return false;
    if ((previous == U_GCB_LVT || previous == U_GCB_T) && next == U_GCB_T) return false;
    if (next == U_GCB_EXTEND || next == U_GCB_ZWJ || next == U_GCB_SPACING_MARK || previous == U_GCB_PREPEND) return false;
    if (indic == U_INCB_CONSONANT && context.indicTail == IndicTail::Linked) return false;
    if (pictographic && context.afterPictographJoiner) return false;
    return next != U_GCB_REGIONAL_INDICATOR || !context.oddRegionalIndicators;
}

struct GraphemePrefix {
    unsigned clusters;
    unsigned codeUnits;
};

GraphemePrefix ScanUtf16(StringView string, unsigned clusterLimit) {
    const auto* characters = static_cast<const char16_t*>(string.data);
    GraphemeContext context;
    unsigned clusters = 0;
    for (unsigned position = 0; position < string.length;) {
        const auto start = position;
        UChar32 scalar;
        U16_NEXT(characters, position, string.length, scalar);
        const auto next = u_getIntPropertyValue(scalar, UCHAR_GRAPHEME_CLUSTER_BREAK);
        const auto indic = u_getIntPropertyValue(scalar, UCHAR_INDIC_CONJUNCT_BREAK);
        const bool pictographic = u_hasBinaryProperty(scalar, UCHAR_EXTENDED_PICTOGRAPHIC);
        if (start == 0 || IsBoundary(context, next, indic, pictographic)) {
            if (clusters == clusterLimit) return {clusters, start};
            ++clusters;
        }
        if (indic == U_INCB_CONSONANT) context.indicTail = IndicTail::Consonant;
        else if (indic == U_INCB_LINKER && context.indicTail != IndicTail::None) context.indicTail = IndicTail::Linked;
        else if (indic != U_INCB_EXTEND) context.indicTail = IndicTail::None;
        context.afterPictographJoiner = next == U_GCB_ZWJ && context.pictographTail;
        context.pictographTail = pictographic || (next == U_GCB_EXTEND && context.pictographTail);
        context.oddRegionalIndicators = next == U_GCB_REGIONAL_INDICATOR && !context.oddRegionalIndicators;
        context.previous = next;
    }
    return {clusters, string.length};
}

}

extern "C" {

unsigned APS5_VABI _ZN3WTF19numGraphemeClustersENS_10StringViewE(StringView string) {
    unsigned stringLength = string.length;
    if (!stringLength) return 0;
    if (!IsLatin1(string)) return ScanUtf16(string, std::numeric_limits<unsigned>::max()).clusters;
    unsigned numCRLF = 0;
    for (unsigned i = 1; i < stringLength; ++i) numCRLF += Latin1Unit(string, i - 1) == '\r' && Latin1Unit(string, i) == '\n';
    return stringLength - numCRLF;
}

unsigned APS5_VABI _ZN3WTF30numCodeUnitsInGraphemeClustersENS_10StringViewEj(StringView string, unsigned numGraphemeClusters) {
    unsigned stringLength = string.length;
    if (stringLength <= numGraphemeClusters) return stringLength;
    if (!IsLatin1(string)) return ScanUtf16(string, numGraphemeClusters).codeUnits;
    unsigned i, j;
    for (i = 0, j = 0; i < numGraphemeClusters && j + 1 < stringLength; ++i, ++j) j += Latin1Unit(string, j) == '\r' && Latin1Unit(string, j + 1) == '\n';
    return j + (i < numGraphemeClusters);
}

}
