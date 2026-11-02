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
    
    void makeAsync(std::mutex* workMutex, std::condition_variable* alarm);


    std::set<Chunk, CompChunks> handleChunk(Chunk chunk);

    
    size_t lastRestore() const { return lastRestore_; }; // TODO: mutex

private:
    friend class OrderedSegmentsTest;

    size_t lastRestore_ = 0;
    std::set<Segment, Comp> segments;
    std::mutex segmentsMutex;
    bool isAsync_                   = false;
    std::condition_variable* alarm_ = nullptr;
    std::mutex* workMutex_          = nullptr;
};




#endif