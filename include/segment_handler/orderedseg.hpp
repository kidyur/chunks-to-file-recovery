#pragma once

#ifndef ORDERED_SEGMENTS
#define ORDERED_SEGMENTS


#include <set>
#include <types.hpp>
#include <condition_variable>
#include <mutex>

class OrderedSegmentsTest;

class OrderedSegments {
public:
    
    std::set<Chunk, CompChunks> handleChunk(Chunk chunk);

    
    size_t lastRestore() const { return lastRestore_; }; // TODO: mutex

private:
    friend class OrderedSegmentsTest;

    size_t lastRestore_ = 0;
    std::set<Segment, Comp> segments;
    std::mutex segmentsMutex;
};




#endif