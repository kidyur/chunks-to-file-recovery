#pragma once

#ifndef ORDERED_SEGMENTS
#define ORDERED_SEGMENTS


#include <set>
#include <cstdint>
#include <vector>
#include <types.hpp>
#include <queue>
#include <condition_variable>
#include <mutex>


class OrderedSegments {
public:
    OrderedSegments(
        std::queue<Chunk>* q
    ) : work(q) { }

    std::mutex segmentsMutex;
    std::set<Segment, Comp> segments;
    std::queue<Chunk>* work;
    
    
    void makeAsync(std::mutex* workMutex, std::condition_variable* alarm);


    void handleChunk(Chunk chunk);

private:
    bool isAsync_                   = false;
    std::condition_variable* alarm_ = nullptr;
    std::mutex* workMutex_          = nullptr;
};



#endif