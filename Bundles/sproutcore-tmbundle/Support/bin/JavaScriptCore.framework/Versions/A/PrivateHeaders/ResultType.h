/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef ResultType_h
#define ResultType_h

namespace JSC {

    struct ResultType {
        friend struct OperandTypes;

        typedef char Type;
        static const Type TypeReusable = 1;
        
        static const Type TypeMaybeNumber = 2;
        static const Type TypeMaybeString = 4;
        static const Type TypeMaybeNull = 8;
        static const Type TypeMaybeBool = 16;
        static const Type TypeMaybeOther = 32;

        static const Type TypeReusableNumber = 3;
        static const Type TypeStringOrReusableNumber = 4;
    
        explicit ResultType(Type type)
            : m_type(type)
        {
        }
        
        bool isReusable()
        {
            return (m_type & TypeReusable);
        }
        
        bool definitelyIsNumber()
        {
            return ((m_type & ~TypeReusable) == TypeMaybeNumber);
        }
        
        bool isNotNumber()
        {
            return ((m_type & TypeMaybeNumber) == 0);
        }
        
        bool mightBeNumber()
        {
            return !isNotNumber();
        }
        
        static ResultType nullType()
        {
            return ResultType(TypeMaybeNull);
        }
        
        static ResultType boolean()
        {
            return ResultType(TypeMaybeBool);
        }
        
        static ResultType constNumber()
        {
            return ResultType(TypeMaybeNumber);
        }
        
        static ResultType reusableNumber()
        {
            return ResultType(TypeReusable | TypeMaybeNumber);
        }
        
        static ResultType reusableNumberOrString()
        {
            return ResultType(TypeReusable | TypeMaybeNumber | TypeMaybeString);
        }
        
        static ResultType string()
        {
            return ResultType(TypeMaybeString);
        }
        
        static ResultType unknown()
        {
            return ResultType(TypeMaybeNumber | TypeMaybeString | TypeMaybeNull | TypeMaybeBool | TypeMaybeOther);
        }
        
        static ResultType forAdd(ResultType op1, ResultType op2)
        {
            if (op1.definitelyIsNumber() && op2.definitelyIsNumber())
                return reusableNumber();
            if (op1.isNotNumber() || op2.isNotNumber())
                return string();
            return reusableNumberOrString();
        }

    private:
        Type m_type;
    };
    
    struct OperandTypes
    {
        OperandTypes(ResultType first = ResultType::unknown(), ResultType second = ResultType::unknown())
        {
            m_u.rds.first = first.m_type;
            m_u.rds.second = second.m_type;
        }
        
        union {
            struct {
                ResultType::Type first;
                ResultType::Type second;
            } rds;
            int i;
        } m_u;

        ResultType first()
        {
            return ResultType(m_u.rds.first);
        }

        ResultType second()
        {
            return ResultType(m_u.rds.second);
        }

        int toInt()
        {
            return m_u.i;
        }
        static OperandTypes fromInt(int value)
        {
            OperandTypes types;
            types.m_u.i = value;
            return types;
        }
    };

} // namespace JSC

#endif // ResultType_h
