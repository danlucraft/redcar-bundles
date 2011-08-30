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

#ifndef X86Assembler_h
#define X86Assembler_h

#if ENABLE(MASM) && PLATFORM(X86)

#include <wtf/Assertions.h>
#include <wtf/AlwaysInline.h>
#if HAVE(MMAN)
#include <sys/mman.h>
#endif

#include <string.h>

namespace JSC {

class JITCodeBuffer {
public:
    JITCodeBuffer(int size)
        : m_buffer(static_cast<char*>(fastMalloc(size)))
        , m_size(size)
        , m_index(0)
    {
    }

    ~JITCodeBuffer()
    {
        fastFree(m_buffer);
    }

    void ensureSpace(int space)
    {
        if (m_index > m_size - space)
            growBuffer();
    }

    void putByteUnchecked(int value)
    {
        m_buffer[m_index] = value;
        m_index++;
    }

    void putByte(int value)
    {
        if (m_index > m_size - 4)
            growBuffer();
        putByteUnchecked(value);
    }
    
    void putShortUnchecked(int value)
    {
        *(short*)(&m_buffer[m_index]) = value;
        m_index += 2;
    }

    void putShort(int value)
    {
        if (m_index > m_size - 4)
            growBuffer();
        putShortUnchecked(value);
    }
    
    void putIntUnchecked(int value)
    {
        *(int*)(&m_buffer[m_index]) = value;
        m_index += 4;
    }

    void putInt(int value)
    {
        if (m_index > m_size - 4)
            growBuffer();
        putIntUnchecked(value);
    }

    void* getEIP()
    {
        return m_buffer + m_index;
    }
    
    void* start()
    {
        return m_buffer;
    }
    
    int getOffset()
    {
        return m_index;
    }
    
    JITCodeBuffer* reset()
    {
        m_index = 0;
        return this;
    }
    
    void* copy()
    {
        if (!m_index)
            return 0;

        void* result = fastMalloc(m_index);

        if (!result)
            return 0;

        return memcpy(result, m_buffer, m_index);
    }

private:
    void growBuffer()
    {
        m_size += m_size / 2;
        m_buffer = static_cast<char*>(fastRealloc(m_buffer, m_size));
    }

    char* m_buffer;
    int m_size;
    int m_index;
};

#define MODRM(type, reg, rm) ((type << 6) | (reg << 3) | (rm))
#define SIB(type, reg, rm) MODRM(type, reg, rm)
#define CAN_SIGN_EXTEND_8_32(value) (value == ((int)(signed char)value))

namespace X86 {
    typedef enum {
        eax,
        ecx,
        edx,
        ebx,
        esp,
        ebp,
        esi,
        edi,

        noBase = ebp,
        hasSib = esp,
        noScale = esp,
    } RegisterID;

    typedef enum {
        xmm0,
        xmm1,
        xmm2,
        xmm3,
        xmm4,
        xmm5,
        xmm6,
        xmm7,
    } XMMRegisterID;
}

class X86Assembler {
public:
    typedef X86::RegisterID RegisterID;
    typedef X86::XMMRegisterID XMMRegisterID;
    typedef enum {
        OP_ADD_EvGv                     = 0x01,
        OP_ADD_GvEv                     = 0x03,
        OP_OR_EvGv                      = 0x09,
        OP_OR_GvEv                      = 0x0B,
        OP_2BYTE_ESCAPE                 = 0x0F,
        OP_AND_EvGv                     = 0x21,
        OP_SUB_EvGv                     = 0x29,
        OP_SUB_GvEv                     = 0x2B,
        PRE_PREDICT_BRANCH_NOT_TAKEN    = 0x2E,
        OP_XOR_EvGv                     = 0x31,
        OP_CMP_EvGv                     = 0x39,
        OP_PUSH_EAX                     = 0x50,
        OP_POP_EAX                      = 0x58,
        PRE_OPERAND_SIZE                = 0x66,
        PRE_SSE_66                      = 0x66,
        OP_IMUL_GvEvIz                  = 0x69,
        OP_GROUP1_EvIz                  = 0x81,
        OP_GROUP1_EvIb                  = 0x83,
        OP_TEST_EvGv                    = 0x85,
        OP_MOV_EvGv                     = 0x89,
        OP_MOV_GvEv                     = 0x8B,
        OP_LEA                          = 0x8D,
        OP_GROUP1A_Ev                   = 0x8F,
        OP_CDQ                          = 0x99,
        OP_SETE                         = 0x94,
        OP_SETNE                        = 0x95,
        OP_GROUP2_EvIb                  = 0xC1,
        OP_RET                          = 0xC3,
        OP_GROUP11_EvIz                 = 0xC7,
        OP_INT3                         = 0xCC,
        OP_GROUP2_Ev1                   = 0xD1,
        OP_GROUP2_EvCL                  = 0xD3,
        OP_CALL_rel32                   = 0xE8,
        OP_JMP_rel32                    = 0xE9,
        PRE_SSE_F2                      = 0xF2,
        OP_GROUP3_Ev                    = 0xF7,
        OP_GROUP3_EvIz                  = 0xF7, // OP_GROUP3_Ev has an immediate, when instruction is a test. 
        OP_GROUP5_Ev                    = 0xFF,

        OP2_MOVSD_VsdWsd    = 0x10,
        OP2_MOVSD_WsdVsd    = 0x11,
        OP2_CVTSI2SD_VsdEd  = 0x2A,
        OP2_CVTTSD2SI_GdWsd = 0x2C,
        OP2_UCOMISD_VsdWsd  = 0x2E,
        OP2_ADDSD_VsdWsd    = 0x58,
        OP2_MULSD_VsdWsd    = 0x59,
        OP2_SUBSD_VsdWsd    = 0x5C,
        OP2_MOVD_EdVd       = 0x7E,
        OP2_JO_rel32        = 0x80,
        OP2_JB_rel32        = 0x82,
        OP2_JAE_rel32       = 0x83,
        OP2_JE_rel32        = 0x84,
        OP2_JNE_rel32       = 0x85,
        OP2_JBE_rel32       = 0x86,
        OP2_JA_rel32        = 0x87,
        OP2_JP_rel32        = 0x8A,
        OP2_JL_rel32        = 0x8C,
        OP2_JGE_rel32       = 0x8D,
        OP2_JLE_rel32       = 0x8E,
        OP2_IMUL_GvEv       = 0xAF,
        OP2_MOVZX_GvEb      = 0xB6,
        OP2_MOVZX_GvEw      = 0xB7,
        OP2_PEXTRW_GdUdIb   = 0xC5,

        GROUP1_OP_ADD = 0,
        GROUP1_OP_OR  = 1,
        GROUP1_OP_AND = 4,
        GROUP1_OP_SUB = 5,
        GROUP1_OP_XOR = 6,
        GROUP1_OP_CMP = 7,

        GROUP1A_OP_POP = 0,

        GROUP2_OP_SHL = 4,
        GROUP2_OP_SAR = 7,

        GROUP3_OP_TEST = 0,
        GROUP3_OP_IDIV = 7,

        GROUP5_OP_CALLN = 2,
        GROUP5_OP_JMPN  = 4,
        GROUP5_OP_PUSH  = 6,

        GROUP11_MOV = 0,
    } OpcodeID;
    
    static const int MAX_INSTRUCTION_SIZE = 16;

    X86Assembler(JITCodeBuffer* m_buffer)
        : m_buffer(m_buffer)
    {
        m_buffer->reset();
    }

    void emitInt3()
    {
        m_buffer->putByte(OP_INT3);
    }
    
    void pushl_r(RegisterID reg)
    {
        m_buffer->putByte(OP_PUSH_EAX + reg);
    }
    
    void pushl_m(int offset, RegisterID base)
    {
        m_buffer->putByte(OP_GROUP5_Ev);
        emitModRm_opm(GROUP5_OP_PUSH, base, offset);
    }
    
    void popl_r(RegisterID reg)
    {
        m_buffer->putByte(OP_POP_EAX + reg);
    }

    void popl_m(int offset, RegisterID base)
    {
        m_buffer->putByte(OP_GROUP1A_Ev);
        emitModRm_opm(GROUP1A_OP_POP, base, offset);
    }
    
    void movl_rr(RegisterID src, RegisterID dst)
    {
        m_buffer->putByte(OP_MOV_EvGv);
        emitModRm_rr(src, dst);
    }
    
    void addl_rr(RegisterID src, RegisterID dst)
    {
        m_buffer->putByte(OP_ADD_EvGv);
        emitModRm_rr(src, dst);
    }

    void addl_i8r(int imm, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP1_EvIb);
        emitModRm_opr(GROUP1_OP_ADD, dst);
        m_buffer->putByte(imm);
    }

    void addl_i8m(int imm, void* addr)
    {
        m_buffer->putByte(OP_GROUP1_EvIb);
        emitModRm_opm(GROUP1_OP_ADD, addr);
        m_buffer->putByte(imm);
    }

    void addl_i32r(int imm, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP1_EvIz);
        emitModRm_opr(GROUP1_OP_ADD, dst);
        m_buffer->putInt(imm);
    }

    void addl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_buffer->putByte(OP_ADD_GvEv);
        emitModRm_rm(dst, base, offset);
    }

    void andl_rr(RegisterID src, RegisterID dst)
    {
        m_buffer->putByte(OP_AND_EvGv);
        emitModRm_rr(src, dst);
    }

    void andl_i32r(int imm, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP1_EvIz);
        emitModRm_opr(GROUP1_OP_AND, dst);
        m_buffer->putInt(imm);
    }

    void cmpl_i8r(int imm, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP1_EvIb);
        emitModRm_opr(GROUP1_OP_CMP, dst);
        m_buffer->putByte(imm);
    }

    void cmpl_rr(RegisterID src, RegisterID dst)
    {
        m_buffer->putByte(OP_CMP_EvGv);
        emitModRm_rr(src, dst);
    }

    void cmpl_rm(RegisterID src, int offset, RegisterID base)
    {
        m_buffer->putByte(OP_CMP_EvGv);
        emitModRm_rm(src, base, offset);
    }

    void cmpl_i32r(int imm, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP1_EvIz);
        emitModRm_opr(GROUP1_OP_CMP, dst);
        m_buffer->putInt(imm);
    }

    void cmpl_i32m(int imm, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP1_EvIz);
        emitModRm_opm(GROUP1_OP_CMP, dst);
        m_buffer->putInt(imm);
    }

    void cmpl_i32m(int imm, int offset, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP1_EvIz);
        emitModRm_opm(GROUP1_OP_CMP, dst, offset);
        m_buffer->putInt(imm);
    }

    void cmpl_i32m(int imm, void* addr)
    {
        m_buffer->putByte(OP_GROUP1_EvIz);
        emitModRm_opm(GROUP1_OP_CMP, addr);
        m_buffer->putInt(imm);
    }

    void cmpl_i8m(int imm, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_buffer->putByte(OP_GROUP1_EvIb);
        emitModRm_opmsib(GROUP1_OP_CMP, base, index, scale, offset);
        m_buffer->putByte(imm);
    }

    void cmpw_rm(RegisterID src, RegisterID base, RegisterID index, int scale)
    {
        m_buffer->putByte(PRE_OPERAND_SIZE);
        m_buffer->putByte(OP_CMP_EvGv);
        emitModRm_rmsib(src, base, index, scale);
    }

    void sete_r(RegisterID dst)
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP_SETE);
        m_buffer->putByte(MODRM(3, 0, dst));
    }

    void setz_r(RegisterID dst)
    {
        sete_r(dst);
    }

    void setne_r(RegisterID dst)
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP_SETNE);
        m_buffer->putByte(MODRM(3, 0, dst));
    }

    void setnz_r(RegisterID dst)
    {
        setne_r(dst);
    }

    void orl_rr(RegisterID src, RegisterID dst)
    {
        m_buffer->putByte(OP_OR_EvGv);
        emitModRm_rr(src, dst);
    }

    void orl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_buffer->putByte(OP_OR_GvEv);
        emitModRm_rm(dst, base, offset);
    }

    void orl_i32r(int imm, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP1_EvIb);
        emitModRm_opr(GROUP1_OP_OR, dst);
        m_buffer->putByte(imm);
    }

    void subl_rr(RegisterID src, RegisterID dst)
    {
        m_buffer->putByte(OP_SUB_EvGv);
        emitModRm_rr(src, dst);
    }

    void subl_i8r(int imm, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP1_EvIb);
        emitModRm_opr(GROUP1_OP_SUB, dst);
        m_buffer->putByte(imm);
    }
    
    void subl_i8m(int imm, void* addr)
    {
        m_buffer->putByte(OP_GROUP1_EvIb);
        emitModRm_opm(GROUP1_OP_SUB, addr);
        m_buffer->putByte(imm);
    }

    void subl_i32r(int imm, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP1_EvIz);
        emitModRm_opr(GROUP1_OP_SUB, dst);
        m_buffer->putInt(imm);
    }

    void subl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_buffer->putByte(OP_SUB_GvEv);
        emitModRm_rm(dst, base, offset);
    }

    void testl_i32r(int imm, RegisterID dst)
    {
        m_buffer->ensureSpace(MAX_INSTRUCTION_SIZE);
        m_buffer->putByteUnchecked(OP_GROUP3_EvIz);
        emitModRm_opr_Unchecked(GROUP3_OP_TEST, dst);
        m_buffer->putIntUnchecked(imm);
    }

    void testl_i32m(int imm, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP3_EvIz);
        emitModRm_opm(GROUP3_OP_TEST, dst);
        m_buffer->putInt(imm);
    }

    void testl_i32m(int imm, int offset, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP3_EvIz);
        emitModRm_opm(GROUP3_OP_TEST, dst, offset);
        m_buffer->putInt(imm);
    }

    void testl_rr(RegisterID src, RegisterID dst)
    {
        m_buffer->putByte(OP_TEST_EvGv);
        emitModRm_rr(src, dst);
    }
    
    void xorl_i8r(int imm, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP1_EvIb);
        emitModRm_opr(GROUP1_OP_XOR, dst);
        m_buffer->putByte(imm);
    }

    void xorl_rr(RegisterID src, RegisterID dst)
    {
        m_buffer->putByte(OP_XOR_EvGv);
        emitModRm_rr(src, dst);
    }

    void sarl_i8r(int imm, RegisterID dst)
    {
        if (imm == 1) {
            m_buffer->putByte(OP_GROUP2_Ev1);
            emitModRm_opr(GROUP2_OP_SAR, dst);
        } else {
            m_buffer->putByte(OP_GROUP2_EvIb);
            emitModRm_opr(GROUP2_OP_SAR, dst);
            m_buffer->putByte(imm);
        }
    }

    void sarl_CLr(RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP2_EvCL);
        emitModRm_opr(GROUP2_OP_SAR, dst);
    }

    void shl_i8r(int imm, RegisterID dst)
    {
        if (imm == 1) {
            m_buffer->putByte(OP_GROUP2_Ev1);
            emitModRm_opr(GROUP2_OP_SHL, dst);
        } else {
            m_buffer->putByte(OP_GROUP2_EvIb);
            emitModRm_opr(GROUP2_OP_SHL, dst);
            m_buffer->putByte(imm);
        }
    }

    void shll_CLr(RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP2_EvCL);
        emitModRm_opr(GROUP2_OP_SHL, dst);
    }

    void imull_rr(RegisterID src, RegisterID dst)
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_IMUL_GvEv);
        emitModRm_rr(dst, src);
    }
    
    void imull_i32r(RegisterID src, int32_t value, RegisterID dst)
    {
        m_buffer->putByte(OP_IMUL_GvEvIz);
        emitModRm_rr(dst, src);
        m_buffer->putInt(value);
    }

    void idivl_r(RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP3_Ev);
        emitModRm_opr(GROUP3_OP_IDIV, dst);
    }

    void cdq()
    {
        m_buffer->putByte(OP_CDQ);
    }

    void movl_mr(RegisterID base, RegisterID dst)
    {
        m_buffer->putByte(OP_MOV_GvEv);
        emitModRm_rm(dst, base);
    }

    void movl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_buffer->ensureSpace(MAX_INSTRUCTION_SIZE);
        m_buffer->putByteUnchecked(OP_MOV_GvEv);
        emitModRm_rm_Unchecked(dst, base, offset);
    }

    void movl_mr(void* addr, RegisterID dst)
    {
        m_buffer->putByte(OP_MOV_GvEv);
        emitModRm_rm(dst, addr);
    }

    void movl_mr(int offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        m_buffer->putByte(OP_MOV_GvEv);
        emitModRm_rmsib(dst, base, index, scale, offset);
    }

    void movzbl_rr(RegisterID src, RegisterID dst)
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_MOVZX_GvEb);
        emitModRm_rr(dst, src);
    }

    void movzwl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_MOVZX_GvEw);
        emitModRm_rm(dst, base, offset);
    }

    void movzwl_mr(RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_MOVZX_GvEw);
        emitModRm_rmsib(dst, base, index, scale);
    }

    void movzwl_mr(int offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_MOVZX_GvEw);
        emitModRm_rmsib(dst, base, index, scale, offset);
    }

    void movl_rm(RegisterID src, RegisterID base)
    {
        m_buffer->putByte(OP_MOV_EvGv);
        emitModRm_rm(src, base);
    }

    void movl_rm(RegisterID src, int offset, RegisterID base)
    {
        m_buffer->ensureSpace(MAX_INSTRUCTION_SIZE);
        m_buffer->putByteUnchecked(OP_MOV_EvGv);
        emitModRm_rm_Unchecked(src, base, offset);
    }
    
    void movl_rm(RegisterID src, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_buffer->putByte(OP_MOV_EvGv);
        emitModRm_rmsib(src, base, index, scale, offset);
    }
    
    void movl_i32r(int imm, RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP11_EvIz);
        emitModRm_opr(GROUP11_MOV, dst);
        m_buffer->putInt(imm);
    }

    void movl_i32m(int imm, int offset, RegisterID base)
    {
        m_buffer->ensureSpace(MAX_INSTRUCTION_SIZE);
        m_buffer->putByteUnchecked(OP_GROUP11_EvIz);
        emitModRm_opm_Unchecked(GROUP11_MOV, base, offset);
        m_buffer->putIntUnchecked(imm);
    }

    void movl_i32m(int imm, void* addr)
    {
        m_buffer->putByte(OP_GROUP11_EvIz);
        emitModRm_opm(GROUP11_MOV, addr);
        m_buffer->putInt(imm);
    }

    void leal_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_buffer->putByte(OP_LEA);
        emitModRm_rm(dst, base, offset);
    }

    void leal_mr(int offset, RegisterID index, int scale, RegisterID dst)
    {
        m_buffer->putByte(OP_LEA);
        emitModRm_rmsib(dst, X86::noBase, index, scale, offset);
    }

    void ret()
    {
        m_buffer->putByte(OP_RET);
    }
    
    void jmp_r(RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP5_Ev);
        emitModRm_opr(GROUP5_OP_JMPN, dst);
    }
    
    void jmp_m(int offset, RegisterID base)
    {
        m_buffer->putByte(OP_GROUP5_Ev);
        emitModRm_opm(GROUP5_OP_JMPN, base, offset);
    }
    
    void movsd_mr(int offset, RegisterID base, XMMRegisterID dst)
    {
        m_buffer->putByte(PRE_SSE_F2);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_MOVSD_VsdWsd);
        emitModRm_rm((RegisterID)dst, base, offset);
    }

    void movsd_rm(XMMRegisterID src, int offset, RegisterID base)
    {
        m_buffer->putByte(PRE_SSE_F2);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_MOVSD_WsdVsd);
        emitModRm_rm((RegisterID)src, base, offset);
    }

    void movd_rr(XMMRegisterID src, RegisterID dst)
    {
        m_buffer->putByte(PRE_SSE_66);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_MOVD_EdVd);
        emitModRm_rr((RegisterID)src, dst);
    }

    void cvtsi2sd_rr(RegisterID src, XMMRegisterID dst)
    {
        m_buffer->putByte(PRE_SSE_F2);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_CVTSI2SD_VsdEd);
        emitModRm_rr((RegisterID)dst, src);
    }

    void cvttsd2si_rr(XMMRegisterID src, RegisterID dst)
    {
        m_buffer->putByte(PRE_SSE_F2);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_CVTTSD2SI_GdWsd);
        emitModRm_rr(dst, (RegisterID)src);
    }

    void addsd_mr(int offset, RegisterID base, XMMRegisterID dst)
    {
        m_buffer->putByte(PRE_SSE_F2);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_ADDSD_VsdWsd);
        emitModRm_rm((RegisterID)dst, base, offset);
    }

    void subsd_mr(int offset, RegisterID base, XMMRegisterID dst)
    {
        m_buffer->putByte(PRE_SSE_F2);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_SUBSD_VsdWsd);
        emitModRm_rm((RegisterID)dst, base, offset);
    }

    void mulsd_mr(int offset, RegisterID base, XMMRegisterID dst)
    {
        m_buffer->putByte(PRE_SSE_F2);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_MULSD_VsdWsd);
        emitModRm_rm((RegisterID)dst, base, offset);
    }

    void addsd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_buffer->putByte(PRE_SSE_F2);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_ADDSD_VsdWsd);
        emitModRm_rr((RegisterID)dst, (RegisterID)src);
    }

    void subsd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_buffer->putByte(PRE_SSE_F2);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_SUBSD_VsdWsd);
        emitModRm_rr((RegisterID)dst, (RegisterID)src);
    }

    void mulsd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_buffer->putByte(PRE_SSE_F2);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_MULSD_VsdWsd);
        emitModRm_rr((RegisterID)dst, (RegisterID)src);
    }

    void ucomis_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_buffer->putByte(PRE_SSE_66);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_UCOMISD_VsdWsd);
        emitModRm_rr((RegisterID)dst, (RegisterID)src);
    }

    void pextrw_irr(int whichWord, XMMRegisterID src, RegisterID dst)
    {
        m_buffer->putByte(PRE_SSE_66);
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_PEXTRW_GdUdIb);
        emitModRm_rr(dst, (RegisterID)src);
        m_buffer->putByte(whichWord);
    }

    // Opaque label types
    
    class JmpSrc {
        friend class X86Assembler;
    public:
        JmpSrc()
            : m_offset(-1)
        {
        }

    private:
        JmpSrc(int offset)
            : m_offset(offset)
        {
        }

        int m_offset;
    };
    
    class JmpDst {
        friend class X86Assembler;
    public:
        JmpDst()
            : m_offset(-1)
        {
        }

    private:
        JmpDst(int offset)
            : m_offset(offset)
        {
        }

        int m_offset;
    };

    // FIXME: make this point to a global label, linked later.
    JmpSrc emitCall()
    {
        m_buffer->putByte(OP_CALL_rel32);
        m_buffer->putInt(0);
        return JmpSrc(m_buffer->getOffset());
    }
    
    JmpSrc emitCall(RegisterID dst)
    {
        m_buffer->putByte(OP_GROUP5_Ev);
        emitModRm_opr(GROUP5_OP_CALLN, dst);
        return JmpSrc(m_buffer->getOffset());
    }

    JmpDst label()
    {
        return JmpDst(m_buffer->getOffset());
    }
    
    JmpSrc emitUnlinkedJmp()
    {
        m_buffer->putByte(OP_JMP_rel32);
        m_buffer->putInt(0);
        return JmpSrc(m_buffer->getOffset());
    }
    
    JmpSrc emitUnlinkedJne()
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_JNE_rel32);
        m_buffer->putInt(0);
        return JmpSrc(m_buffer->getOffset());
    }
    
    JmpSrc emitUnlinkedJnz()
    {
        return emitUnlinkedJne();
    }

    JmpSrc emitUnlinkedJe()
    {
        m_buffer->ensureSpace(MAX_INSTRUCTION_SIZE);
        m_buffer->putByteUnchecked(OP_2BYTE_ESCAPE);
        m_buffer->putByteUnchecked(OP2_JE_rel32);
        m_buffer->putIntUnchecked(0);
        return JmpSrc(m_buffer->getOffset());
    }
    
    JmpSrc emitUnlinkedJl()
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_JL_rel32);
        m_buffer->putInt(0);
        return JmpSrc(m_buffer->getOffset());
    }
    
    JmpSrc emitUnlinkedJb()
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_JB_rel32);
        m_buffer->putInt(0);
        return JmpSrc(m_buffer->getOffset());
    }
    
    JmpSrc emitUnlinkedJle()
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_JLE_rel32);
        m_buffer->putInt(0);
        return JmpSrc(m_buffer->getOffset());
    }
    
    JmpSrc emitUnlinkedJbe()
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_JBE_rel32);
        m_buffer->putInt(0);
        return JmpSrc(m_buffer->getOffset());
    }
    
    JmpSrc emitUnlinkedJge()
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_JGE_rel32);
        m_buffer->putInt(0);
        return JmpSrc(m_buffer->getOffset());
    }
    
    JmpSrc emitUnlinkedJa()
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_JA_rel32);
        m_buffer->putInt(0);
        return JmpSrc(m_buffer->getOffset());
    }
    
    JmpSrc emitUnlinkedJae()
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_JAE_rel32);
        m_buffer->putInt(0);
        return JmpSrc(m_buffer->getOffset());
    }
    
    JmpSrc emitUnlinkedJo()
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_JO_rel32);
        m_buffer->putInt(0);
        return JmpSrc(m_buffer->getOffset());
    }

    JmpSrc emitUnlinkedJp()
    {
        m_buffer->putByte(OP_2BYTE_ESCAPE);
        m_buffer->putByte(OP2_JP_rel32);
        m_buffer->putInt(0);
        return JmpSrc(m_buffer->getOffset());
    }
    
    void emitPredictionNotTaken()
    {
        m_buffer->putByte(PRE_PREDICT_BRANCH_NOT_TAKEN);
    }
    
    void link(JmpSrc from, JmpDst to)
    {
        ASSERT(to.m_offset != -1);
        ASSERT(from.m_offset != -1);
        
        ((int*)(((ptrdiff_t)(m_buffer->start())) + from.m_offset))[-1] = to.m_offset - from.m_offset;
    }
    
    static void linkAbsoluteAddress(void* code, JmpDst useOffset, JmpDst address)
    {
        ASSERT(useOffset.m_offset != -1);
        ASSERT(address.m_offset != -1);
        
        ((int*)(((ptrdiff_t)code) + useOffset.m_offset))[-1] = ((ptrdiff_t)code) + address.m_offset;
    }
    
    static void link(void* code, JmpSrc from, void* to)
    {
        ASSERT(from.m_offset != -1);
        
        ((int*)((ptrdiff_t)code + from.m_offset))[-1] = (ptrdiff_t)to - ((ptrdiff_t)code + from.m_offset);
    }
    
    static void* getRelocatedAddress(void* code, JmpSrc jump)
    {
        return reinterpret_cast<void*>((ptrdiff_t)code + jump.m_offset);
    }
    
    static void* getRelocatedAddress(void* code, JmpDst jump)
    {
        return reinterpret_cast<void*>((ptrdiff_t)code + jump.m_offset);
    }
    
    static int getDifferenceBetweenLabels(JmpDst src, JmpDst dst)
    {
        return dst.m_offset - src.m_offset;
    }
    
    static int getDifferenceBetweenLabels(JmpDst src, JmpSrc dst)
    {
        return dst.m_offset - src.m_offset;
    }
    
    static void repatchImmediate(intptr_t where, int32_t value)
    {
        reinterpret_cast<int32_t*>(where)[-1] = value;
    }
    
    static void repatchDisplacement(intptr_t where, intptr_t value)
    {
        reinterpret_cast<intptr_t*>(where)[-1] = value;
    }
    
    static void repatchBranchOffset(intptr_t where, void* destination)
    {
        reinterpret_cast<intptr_t*>(where)[-1] = (reinterpret_cast<intptr_t>(destination) - where);
    }
    
    void* copy() 
    {
        return m_buffer->copy();
    }

#if USE(CTI_ARGUMENT)
    void emitConvertToFastCall()
    {
        movl_mr(4, X86::esp, X86::eax);
        movl_mr(8, X86::esp, X86::edx);
        movl_mr(12, X86::esp, X86::ecx);
    }

    void emitRestoreArgumentReference()
    {
        movl_rm(X86::esp, 0, X86::esp);
    }
#else
    void emitConvertToFastCall() {};
    void emitRestoreArgumentReference() {};
#endif

private:
    void emitModRm_rr(RegisterID reg, RegisterID rm)
    {
        m_buffer->ensureSpace(MAX_INSTRUCTION_SIZE);
        emitModRm_rr_Unchecked(reg, rm);
    }

    void emitModRm_rr_Unchecked(RegisterID reg, RegisterID rm)
    {
        m_buffer->putByteUnchecked(MODRM(3, reg, rm));
    }

    void emitModRm_rm(RegisterID reg, void* addr)
    {
        m_buffer->putByte(MODRM(0, reg, X86::noBase));
        m_buffer->putInt((int)addr);
    }

    void emitModRm_rm(RegisterID reg, RegisterID base)
    {
        if (base == X86::esp) {
            m_buffer->putByte(MODRM(0, reg, X86::hasSib));
            m_buffer->putByte(SIB(0, X86::noScale, X86::esp));
        } else
            m_buffer->putByte(MODRM(0, reg, base));
    }

    void emitModRm_rm_Unchecked(RegisterID reg, RegisterID base, int offset)
    {
        if (base == X86::esp) {
            if (CAN_SIGN_EXTEND_8_32(offset)) {
                m_buffer->putByteUnchecked(MODRM(1, reg, X86::hasSib));
                m_buffer->putByteUnchecked(SIB(0, X86::noScale, X86::esp));
                m_buffer->putByteUnchecked(offset);
            } else {
                m_buffer->putByteUnchecked(MODRM(2, reg, X86::hasSib));
                m_buffer->putByteUnchecked(SIB(0, X86::noScale, X86::esp));
                m_buffer->putIntUnchecked(offset);
            }
        } else {
            if (CAN_SIGN_EXTEND_8_32(offset)) {
                m_buffer->putByteUnchecked(MODRM(1, reg, base));
                m_buffer->putByteUnchecked(offset);
            } else {
                m_buffer->putByteUnchecked(MODRM(2, reg, base));
                m_buffer->putIntUnchecked(offset);
            }
        }
    }

    void emitModRm_rm(RegisterID reg, RegisterID base, int offset)
    {
        m_buffer->ensureSpace(MAX_INSTRUCTION_SIZE);
        emitModRm_rm_Unchecked(reg, base, offset);
    }

    void emitModRm_rmsib(RegisterID reg, RegisterID base, RegisterID index, int scale)
    {
        int shift = 0;
        while (scale >>= 1)
            shift++;
    
        m_buffer->putByte(MODRM(0, reg, X86::hasSib));
        m_buffer->putByte(SIB(shift, index, base));
    }

    void emitModRm_rmsib(RegisterID reg, RegisterID base, RegisterID index, int scale, int offset)
    {
        int shift = 0;
        while (scale >>= 1)
            shift++;
    
        if (CAN_SIGN_EXTEND_8_32(offset)) {
            m_buffer->putByte(MODRM(1, reg, X86::hasSib));
            m_buffer->putByte(SIB(shift, index, base));
            m_buffer->putByte(offset);
        } else {
            m_buffer->putByte(MODRM(2, reg, X86::hasSib));
            m_buffer->putByte(SIB(shift, index, base));
            m_buffer->putInt(offset);
        }
    }

    void emitModRm_opr(OpcodeID opcode, RegisterID rm)
    {
        m_buffer->ensureSpace(MAX_INSTRUCTION_SIZE);
        emitModRm_opr_Unchecked(opcode, rm);
    }

    void emitModRm_opr_Unchecked(OpcodeID opcode, RegisterID rm)
    {
        emitModRm_rr_Unchecked(static_cast<RegisterID>(opcode), rm);
    }

    void emitModRm_opm(OpcodeID opcode, RegisterID base)
    {
        emitModRm_rm(static_cast<RegisterID>(opcode), base);
    }

    void emitModRm_opm_Unchecked(OpcodeID opcode, RegisterID base, int offset)
    {
        emitModRm_rm_Unchecked(static_cast<RegisterID>(opcode), base, offset);
    }

    void emitModRm_opm(OpcodeID opcode, RegisterID base, int offset)
    {
        emitModRm_rm(static_cast<RegisterID>(opcode), base, offset);
    }

    void emitModRm_opm(OpcodeID opcode, void* addr)
    {
        emitModRm_rm(static_cast<RegisterID>(opcode), addr);
    }

    void emitModRm_opmsib(OpcodeID opcode, RegisterID base, RegisterID index, int scale, int offset)
    {
        emitModRm_rmsib(static_cast<RegisterID>(opcode), base, index, scale, offset);
    }

    JITCodeBuffer* m_buffer;
};

} // namespace JSC

#endif // ENABLE(MASM) && PLATFORM(X86)

#endif // X86Assembler_h
