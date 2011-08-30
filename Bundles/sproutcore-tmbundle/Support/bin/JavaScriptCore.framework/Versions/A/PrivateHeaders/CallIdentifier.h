/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef CallIdentifier_h
#define CallIdentifier_h

#include <kjs/ustring.h>

namespace JSC {

    struct CallIdentifier {
        UString m_name;
        UString m_url;
        unsigned m_lineNumber;
        
        CallIdentifier(UString name, UString url, int lineNumber)
            : m_name(name)
            , m_url(url)
            , m_lineNumber(lineNumber)
        {
        }

        CallIdentifier(const CallIdentifier& ci)
            : m_name(ci.m_name)
            , m_url(ci.m_url)
            , m_lineNumber(ci.m_lineNumber)
        {
        }

        inline bool operator==(const CallIdentifier& ci) const { return ci.m_lineNumber == m_lineNumber && ci.m_name == m_name && ci.m_url == m_url; }
        inline bool operator!=(const CallIdentifier& ci) const { return !(*this == ci); }

#ifndef NDEBUG
        operator const char*() const { return c_str(); }
        const char* c_str() const { return m_name.UTF8String().c_str(); }
#endif
    };

} // namespace JSC

namespace WTF {
    template<> struct IntHash<JSC::CallIdentifier> {
        static unsigned hash(const JSC::CallIdentifier& key)
        {
            unsigned hashCodes[3] = {
                key.m_name.rep()->hash(),
                key.m_url.rep()->hash(),
                key.m_lineNumber
            };
            return JSC::UString::Rep::computeHash(reinterpret_cast<char*>(hashCodes), sizeof(hashCodes));
        }

        static bool equal(const JSC::CallIdentifier& a, const JSC::CallIdentifier& b) { return a == b; }
        static const bool safeToCompareToEmptyOrDeleted = true;
    };
    template<> struct DefaultHash<JSC::CallIdentifier> { typedef IntHash<JSC::CallIdentifier> Hash; };

    template<> struct HashTraits<JSC::CallIdentifier> : GenericHashTraits<JSC::CallIdentifier> {
        static const bool emptyValueIsZero = false;
        static JSC::CallIdentifier emptyValue() { return JSC::CallIdentifier("", "", 0); }
        static const bool needsDestruction = false;
        static void constructDeletedValue(JSC::CallIdentifier& slot) { new (&slot) JSC::CallIdentifier(JSC::UString(), JSC::UString(), 0); }
        static bool isDeletedValue(const JSC::CallIdentifier& value) { return value.m_name.isNull() && value.m_url.isNull() && value.m_lineNumber == 0; }
    };
} // namespace WTF

#endif  // CallIdentifier_h

