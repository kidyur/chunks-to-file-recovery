
#ifndef SAFE_BOARD_FILE_COLLECTOR
#define SAFE_BOARD_FILE_COLLECTOR

#include <cstdint>
#include <unordered_map>
#include <thread>
#include <segment_handler/orderedseg.hpp>
#include "types.hpp"
#include <mutex>
#include <vector>
#include <condition_variable>
#include <optional>
#include <atomic>
#include <set>

class FileCollector {
public:
    FileCollector();
    ~FileCollector();

    void CollectFile(
        const std::uint32_t fileId, 
        const size_t fileSize);


    void OnNewChunk(
        const std::uint32_t fileId, 
        const size_t pos, 
        std::vector<std::uint8_t>&& chunk);


    std::vector<std::uint8_t> GetFile(const std::uint32_t fileId);
    

    bool IsReady(const std::uint32_t fileId);

private:
    static constexpr size_t kMaxChunkSize = 4096;
    static constexpr size_t kDefThreadsNumber = 4;
    std::uint32_t threadsAvailable = 0;

    bool stopping_ = false;

    void work();
    struct File {
        std::set<Chunk, CompChunks> chunks;  
        std::optional<OrderedSegments> ordseg;
        std::atomic<std::int64_t> dataRestored;
        size_t fileSize;
    };
    std::vector<std::thread> threads_;

    std::condition_variable chunksQueryCv;
    std::mutex chunksQueryMutex;
    std::queue<Chunk> chunksQuery;

    std::unordered_map<std::uint32_t, File> files_;
};


#endif