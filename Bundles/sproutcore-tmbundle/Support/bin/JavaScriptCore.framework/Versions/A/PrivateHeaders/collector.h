/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef KJSCOLLECTOR_H_
#define KJSCOLLECTOR_H_

#include <string.h>
#include <wtf/HashCountedSet.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/Threading.h>

// This is supremely lame that we require pthreads to build on windows.
#if ENABLE(JSC_MULTIPLE_THREADS)
#include <pthread.h>
#endif

#define ASSERT_CLASS_FITS_IN_CELL(class) COMPILE_ASSERT(sizeof(class) <= CELL_SIZE, class_fits_in_cell)

namespace JSC {

    class ArgList;
    class CollectorBlock;
    class JSCell;
    class JSGlobalData;
    class JSValue;

    enum OperationInProgress { NoOperation, Allocation, Collection };

    struct CollectorHeap {
        CollectorBlock** blocks;
        size_t numBlocks;
        size_t usedBlocks;
        size_t firstBlockWithPossibleSpace;

        size_t numLiveObjects;
        size_t numLiveObjectsAtLastCollect;
        size_t extraCost;

        OperationInProgress operationInProgress;
    };

    class Heap : Noncopyable {
    public:
        class Thread;
        enum HeapType { PrimaryHeap, NumberHeap };

        void destroy();

#ifdef JAVASCRIPTCORE_BUILDING_ALL_IN_ONE_FILE
        // We can inline these functions because everything is compiled as
        // one file, so the heapAllocate template definitions are available.
        // However, allocateNumber is used via jsNumberCell outside JavaScriptCore.
        // Thus allocateNumber needs to provide a non-inline version too.
        void* inlineAllocateNumber(size_t s) { return heapAllocate<NumberHeap>(s); }
        void* inlineAllocate(size_t s) { return heapAllocate<PrimaryHeap>(s); }
#endif
        void* allocateNumber(size_t);
        void* allocate(size_t);

        bool collect();
        bool isBusy(); // true if an allocation or collection is in progress

        static const size_t minExtraCostSize = 256;

        void reportExtraMemoryCost(size_t cost);

        size_t size();

        void setGCProtectNeedsLocking();
        void protect(JSValue*);
        void unprotect(JSValue*);

        static Heap* heap(const JSValue*); // 0 for immediate values

        size_t globalObjectCount();
        size_t protectedObjectCount();
        size_t protectedGlobalObjectCount();
        HashCountedSet<const char*>* protectedObjectTypeCounts();

        void registerThread(); // Only needs to be called by clients that can use the same heap from multiple threads.

        static bool isCellMarked(const JSCell*);
        static void markCell(JSCell*);

        void markConservatively(void* start, void* end);

        HashSet<ArgList*>& markListSet() { if (!m_markListSet) m_markListSet = new HashSet<ArgList*>; return *m_markListSet; }

        JSGlobalData* globalData() const { return m_globalData; }
        static bool isNumber(JSCell*);

    private:
        template <Heap::HeapType heapType> void* heapAllocate(size_t);
        template <Heap::HeapType heapType> size_t sweep();
        static const CollectorBlock* cellBlock(const JSCell*);
        static CollectorBlock* cellBlock(JSCell*);
        static size_t cellOffset(const JSCell*);

        friend class JSGlobalData;
        Heap(JSGlobalData*);
        ~Heap();

        void recordExtraCost(size_t);
        void markProtectedObjects();
        void markCurrentThreadConservatively();
        void markCurrentThreadConservativelyInternal();
        void markOtherThreadConservatively(Thread*);
        void markStackObjectsConservatively();

        typedef HashCountedSet<JSCell*> ProtectCountSet;

        CollectorHeap primaryHeap;
        CollectorHeap numberHeap;

        OwnPtr<Mutex> m_protectedValuesMutex; // Only non-null if the client explicitly requested it via setGCPrtotectNeedsLocking().
        ProtectCountSet m_protectedValues;

        HashSet<ArgList*>* m_markListSet;

#if ENABLE(JSC_MULTIPLE_THREADS)
        static void unregisterThread(void*);
        void unregisterThread();

        Mutex m_registeredThreadsMutex;
        Thread* m_registeredThreads;
        pthread_key_t m_currentThreadRegistrar;
#endif

        JSGlobalData* m_globalData;
    };

    // tunable parameters
    template<size_t bytesPerWord> struct CellSize;

    // cell size needs to be a power of two for certain optimizations in collector.cpp
    template<> struct CellSize<sizeof(uint32_t)> { static const size_t m_value = 32; }; // 32-bit
    template<> struct CellSize<sizeof(uint64_t)> { static const size_t m_value = 64; }; // 64-bit
    const size_t BLOCK_SIZE = 16 * 4096; // 64k

    // derived constants
    const size_t BLOCK_OFFSET_MASK = BLOCK_SIZE - 1;
    const size_t BLOCK_MASK = ~BLOCK_OFFSET_MASK;
    const size_t MINIMUM_CELL_SIZE = CellSize<sizeof(void*)>::m_value;
    const size_t CELL_ARRAY_LENGTH = (MINIMUM_CELL_SIZE / sizeof(double)) + (MINIMUM_CELL_SIZE % sizeof(double) != 0 ? sizeof(double) : 0);
    const size_t CELL_SIZE = CELL_ARRAY_LENGTH * sizeof(double);
    const size_t SMALL_CELL_SIZE = CELL_SIZE / 2;
    const size_t CELL_MASK = CELL_SIZE - 1;
    const size_t CELL_ALIGN_MASK = ~CELL_MASK;
    const size_t CELLS_PER_BLOCK = (BLOCK_SIZE * 8 - sizeof(uint32_t) * 8 - sizeof(void *) * 8 - 2 * (7 + 3 * 8)) / (CELL_SIZE * 8 + 2);
    const size_t SMALL_CELLS_PER_BLOCK = 2 * CELLS_PER_BLOCK;
    const size_t BITMAP_SIZE = (CELLS_PER_BLOCK + 7) / 8;
    const size_t BITMAP_WORDS = (BITMAP_SIZE + 3) / sizeof(uint32_t);
  
    struct CollectorBitmap {
        uint32_t bits[BITMAP_WORDS];
        bool get(size_t n) const { return !!(bits[n >> 5] & (1 << (n & 0x1F))); } 
        void set(size_t n) { bits[n >> 5] |= (1 << (n & 0x1F)); } 
        void clear(size_t n) { bits[n >> 5] &= ~(1 << (n & 0x1F)); } 
        void clearAll() { memset(bits, 0, sizeof(bits)); }
    };
  
    struct CollectorCell {
        union {
            double memory[CELL_ARRAY_LENGTH];
            struct {
                void* zeroIfFree;
                ptrdiff_t next;
            } freeCell;
        } u;
    };

    struct SmallCollectorCell {
        union {
            double memory[CELL_ARRAY_LENGTH / 2];
            struct {
                void* zeroIfFree;
                ptrdiff_t next;
            } freeCell;
        } u;
    };

    class CollectorBlock {
    public:
        CollectorCell cells[CELLS_PER_BLOCK];
        uint32_t usedCells;
        CollectorCell* freeList;
        CollectorBitmap marked;
        Heap* heap;
        Heap::HeapType type;
    };

    class SmallCellCollectorBlock {
    public:
        SmallCollectorCell cells[SMALL_CELLS_PER_BLOCK];
        uint32_t usedCells;
        SmallCollectorCell* freeList;
        CollectorBitmap marked;
        Heap* heap;
        Heap::HeapType type;
    };
    
    inline bool Heap::isNumber(JSCell* cell)
    {
        CollectorBlock* block = Heap::cellBlock(cell);
        return block && block->type == NumberHeap;
    }

    inline const CollectorBlock* Heap::cellBlock(const JSCell* cell)
    {
        return reinterpret_cast<const CollectorBlock*>(reinterpret_cast<uintptr_t>(cell) & BLOCK_MASK);
    }

    inline CollectorBlock* Heap::cellBlock(JSCell* cell)
    {
        return const_cast<CollectorBlock*>(cellBlock(const_cast<const JSCell*>(cell)));
    }

    inline size_t Heap::cellOffset(const JSCell* cell)
    {
        return (reinterpret_cast<uintptr_t>(cell) & BLOCK_OFFSET_MASK) / CELL_SIZE;
    }

    inline bool Heap::isCellMarked(const JSCell* cell)
    {
        return cellBlock(cell)->marked.get(cellOffset(cell));
    }

    inline void Heap::markCell(JSCell* cell)
    {
        cellBlock(cell)->marked.set(cellOffset(cell));
    }

    inline void Heap::reportExtraMemoryCost(size_t cost)
    {
        if (cost > minExtraCostSize) 
            recordExtraCost(cost / (CELL_SIZE * 2)); 
    }

} // namespace JSC

#endif /* KJSCOLLECTOR_H_ */
