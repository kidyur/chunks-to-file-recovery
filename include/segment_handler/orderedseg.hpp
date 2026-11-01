#pragma once

#ifndef ORDERED_SEGMENTS
#define ORDERED_SEGMENTS


#include <set>
#include <types.hpp>
#include <queue>
#include <condition_variable>
#include <mutex>

class OrderedSegmentsTest;

class OrderedSegments {
public:
    OrderedSegments(
        std::queue<Chunk>* q
    ) : work(q) { }
    OrderedSegments(OrderedSegments&&) = default;
    OrderedSegments& operator=(OrderedSegments&&) = default;
    
    
    void makeAsync(std::mutex* workMutex, std::condition_variable* alarm);


    void handleChunk(Chunk chunk);
    
private:
    friend class OrderedSegmentsTest;
    
    std::set<Segment, Comp> segments;
    std::queue<Chunk>* work;
    std::mutex segmentsMutex;
    bool isAsync_                   = false;
    std::condition_variable* alarm_ = nullptr;
    std::mutex* workMutex_          = nullptr;
};




#endif