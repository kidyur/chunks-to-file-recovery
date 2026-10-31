#include "orderedseg.hpp"
#include <stdexcept>


void OrderedSegments::makeAsync(std::mutex* workMutex, std::condition_variable* alarm) {
    if (workMutex == nullptr || alarm == nullptr)
        throw std::invalid_argument("invalid arguments for async mode");
    workMutex_ = workMutex;
    alarm_ = alarm;
    isAsync_ = true;
}

void OrderedSegments::handleChunk(Chunk chunk) {
    if (chunk.data.empty()) return;

    std::lock_guard segLock(segmentsMutex);

    std::unique_lock<std::mutex> workLock;
    if (isAsync_) {
        workLock = std::unique_lock<std::mutex>(*workMutex_);
    }

    const size_t begin = chunk.pos;

    if (chunk.data.size() > SIZE_MAX - begin) {
        throw std::out_of_range("Chunk position overflow");
    }

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

        work->push({
            chunk.fileId,
            l,
            std::vector<std::uint8_t>(
                chunk.data.begin() + (l - begin),
                chunk.data.begin() + (r - begin)
            )
        });

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

    if (isAsync_ && queued) {
        workLock.unlock();
        alarm_->notify_all();
    }
}


bool OrderedSegments::isReady() {
    std::lock_guard seglock(segmentsMutex);
    if (segments.empty())
        return false;
    auto first = segments.begin();
    return first->l == 0 && first->r == fileSize_-1;
}