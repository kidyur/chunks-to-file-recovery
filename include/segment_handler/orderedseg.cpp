#include "orderedseg.hpp"
#include <stdexcept>
#include <iostream>

void OrderedSegments::makeAsync(std::mutex* workMutex, std::condition_variable* alarm) {
    if (workMutex == nullptr || alarm == nullptr)
        throw std::invalid_argument("invalid arguments for async mode");
    std::lock_guard lockWork(*workMutex);
    std::lock_guard lockSegments(segmentsMutex);
    workMutex_ = workMutex;
    alarm_ = alarm;
    isAsync_ = true;
}

void OrderedSegments::handleChunk(Chunk chunk) {
    if (chunk.data.empty()) return;
    if (chunk.data.size() > SIZE_MAX - chunk.pos) 
        throw std::out_of_range("Chunk position overflow");
    std::unique_lock<std::mutex> segLock;
    if (isAsync_) segLock = std::unique_lock<std::mutex>(segmentsMutex);

    const size_t begin = chunk.pos;
    const size_t end = begin + chunk.data.size();
    Segment seg{begin, end - 1};
    auto it = segments.lower_bound(seg);

    if (it != segments.begin()) {
        auto prev = std::prev(it);
        if (prev->r >= begin ||
            (begin > 0 && prev->r == begin - 1)) {
            it = prev;
        }
    }

    size_t cursor = begin;
    bool queued = false;

    // Add only uncovered bytes to the queue
    auto addWork = [&](size_t l, size_t r) {
        if (l >= r) return;
        std::unique_lock<std::mutex> workLock;
        if (isAsync_) {
            workLock = std::unique_lock<std::mutex>(*workMutex_);
        }
        work->push({
            chunk.fileId,
            l,
            std::vector<std::uint8_t>(
                chunk.data.begin() + (l - begin),
                chunk.data.begin() + (r - begin)
            )
        });
        if (isAsync_) {
            workLock.unlock();
            alarm_->notify_one();
        }
        queued = true;
    };

    while (it != segments.end()) {
        // No overlap or adjacency
        if (it->l > seg.r && it->l - seg.r > 1) {
            break;
        }

        // Process only the part intersecting our chunk
        if (it->r >= begin && it->l < end) {
            addWork(cursor, std::min(it->l, end));

            if (it->r >= end - 1) {
                cursor = end;
            } else {
                cursor = std::max(cursor, it->r + 1);
            }
        }

        // Merge intervals
        seg.l = std::min(seg.l, it->l);
        seg.r = std::max(seg.r, it->r);

        it = segments.erase(it);
    }

    // Remaining uncovered bytes
    addWork(cursor, end);
    segments.insert(seg);
    std::cout << (int)segments.size() << "<< segments\n";
    // todo: notify only if add work
    if (isAsync_ && queued) {
        alarm_->notify_one();
    }
}
