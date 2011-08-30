/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RegisterFile_h
#define RegisterFile_h

#include "Register.h"
#include "collector.h"
#if HAVE(MMAP)
#include <sys/mman.h>
#endif
#include <wtf/Noncopyable.h>

namespace JSC {

/*
    A register file is a stack of register frames. We represent a register
    frame by its offset from "base", the logical first entry in the register
    file. The bottom-most register frame's offset from base is 0.

    In a program where function "a" calls function "b" (global code -> a -> b),
    the register file might look like this:

    |       global frame     |        call frame      |        call frame      |     spare capacity     |
    -----------------------------------------------------------------------------------------------------
    |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 |    |    |    |    |    | <-- index in buffer
    -----------------------------------------------------------------------------------------------------
    | -3 | -2 | -1 |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 |    |    |    |    |    | <-- index relative to base
    -----------------------------------------------------------------------------------------------------
    |    <-globals | temps-> |  <-vars | temps->      |                 <-vars |
       ^              ^                   ^                                       ^
       |              |                   |                                       |
     buffer    base (frame 0)          frame 1                                 frame 2

    Since all variables, including globals, are accessed by negative offsets
    from their register frame pointers, to keep old global offsets correct, new
    globals must appear at the beginning of the register file, shifting base
    to the right.

    If we added one global variable to the register file depicted above, it
    would look like this:

    |         global frame        |<                                                                    >
    ------------------------------->                                                                    <
    |  0 |  1 |  2 |  3 |  4 |  5 |<                             >snip<                                 > <-- index in buffer
    ------------------------------->                                                                    <
    | -4 | -3 | -2 | -1 |  0 |  1 |<                                                                    > <-- index relative to base
    ------------------------------->                                                                    <
    |         <-globals | temps-> |
       ^                   ^
       |                   |
     buffer         base (frame 0)

    As you can see, global offsets relative to base have stayed constant,
    but base itself has moved. To keep up with possible changes to base,
    clients keep an indirect pointer, so their calculations update
    automatically when base changes.

    For client simplicity, the RegisterFile measures size and capacity from
    "base", not "buffer".
*/

    class JSGlobalObject;

    class RegisterFile : Noncopyable {
    public:
        enum CallFrameHeaderEntry {
            CallFrameHeaderSize = 9,

            CodeBlock = -9,
            ScopeChain = -8,
            CallerRegisters = -7,
            ReturnPC = -6,
            ReturnValueRegister = -5,
            ArgumentCount = -4,
            Callee = -3,
            OptionalCalleeActivation = -2,
            OptionalCalleeArguments = -1,
        };

        enum { ProgramCodeThisRegister = -CallFrameHeaderSize - 1 };
        enum { ArgumentsRegister = 0 };

        static const size_t defaultCapacity = 524288;
        static const size_t defaultMaxGlobals = 8192;

        RegisterFile(size_t capacity = defaultCapacity, size_t maxGlobals = defaultMaxGlobals)
            : m_numGlobals(0)
            , m_maxGlobals(maxGlobals)
            , m_start(0)
            , m_end(0)
            , m_max(0)
            , m_buffer(0)
            , m_globalObject(0)
        {
            size_t bufferLength = (capacity + maxGlobals) * sizeof(Register);
#if HAVE(MMAP)
            m_buffer = static_cast<Register*>(mmap(0, bufferLength, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0));
            ASSERT(reinterpret_cast<intptr_t>(m_buffer) != -1);
#elif HAVE(VIRTUALALLOC)
            // FIXME: Use VirtualAlloc, and commit pages as we go.
            m_buffer = static_cast<Register*>(fastMalloc(bufferLength));
#else
            #error "Don't know how to reserve virtual memory on this platform."
#endif
            m_start = m_buffer + maxGlobals;
            m_end = m_start;
            m_max = m_start + capacity;
        }

        ~RegisterFile();

        Register* start() const { return m_start; }
        Register* end() const { return m_end; }
        size_t size() const { return m_end - m_start; }

        void setGlobalObject(JSGlobalObject* globalObject) { m_globalObject = globalObject; }
        JSGlobalObject* globalObject() { return m_globalObject; }

        void shrink(Register* newEnd)
        {
            if (newEnd < m_end)
                m_end = newEnd;
        }

        bool grow(Register* newEnd)
        {
            if (newEnd > m_end) {
                if (newEnd > m_max)
                    return false;
#if !HAVE(MMAP) && HAVE(VIRTUALALLOC)
                // FIXME: Use VirtualAlloc, and commit pages as we go.
#endif
                m_end = newEnd;
            }
            return true;
        }
        
        void setNumGlobals(size_t numGlobals) { m_numGlobals = numGlobals; }
        int numGlobals() const { return m_numGlobals; }
        size_t maxGlobals() const { return m_maxGlobals; }

        Register* lastGlobal() const { return m_start - m_numGlobals; }
        
        void markGlobals(Heap* heap) { heap->markConservatively(lastGlobal(), m_start); }
        void markCallFrames(Heap* heap) { heap->markConservatively(m_start, m_end); }

    private:
        size_t m_numGlobals;
        const size_t m_maxGlobals;
        Register* m_start;
        Register* m_end;
        Register* m_max;
        Register* m_buffer;
        JSGlobalObject* m_globalObject; // The global object whose vars are currently stored in the register file.
    };

} // namespace JSC

#endif // RegisterFile_h
