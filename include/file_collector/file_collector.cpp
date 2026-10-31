
#include <stdexcept>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <map>
#include <mutex>
#include <thread>

#include "file_collector.hpp"


FileCollector::FileCollector() 
{
    threadsAvailable = std::thread::hardware_concurrency();
    if (threadsAvailable == 0) {
        threadsAvailable = 1;
    }
    threads_.reserve(threadsAvailable);
    for (int i = 0; i < threadsAvailable; i++) {
        threads_.emplace_back([this](){ work(); });
    }
}

FileCollector::~FileCollector() {
    {
        std::lock_guard lock(chunksQueryMutex);
        stopping_ = true;
    }

    chunksQueryCv.notify_all();

    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}


void FileCollector::CollectFile(
    const std::uint32_t fileId, 
    const size_t fileSize)
{
    if (files_.count(fileId) == 1) 
        throw std::invalid_argument("File ID already exists");

    files_[fileId].chunks.resize(fileSize);
    files_[fileId].ordseg.emplace(
        &chunksQuery, 
        fileSize
    );
    files_[fileId].ordseg->makeAsync(&chunksQueryMutex, &chunksQueryCv);
}


void FileCollector::OnNewChunk(
    const std::uint32_t fileId, 
    const size_t pos, 
    std::vector<uint8_t>&& chunk) 
{
    if (files_.count(fileId) == 0)  
        throw std::invalid_argument("File ID does not exist");
    if (pos + chunk.size() > files_[fileId].chunks.size())  
        throw std::out_of_range("Chunk size is out of file's bounds");

    if (chunk.size() > FileCollector::kMaxChunkSize) {
        for (size_t i = 0; i < chunk.size(); i += FileCollector::kMaxChunkSize) {
            size_t l = i;
            size_t r = l + std::min(FileCollector::kMaxChunkSize, chunk.size() - l);

            files_[fileId].ordseg->handleChunk({
                fileId,
                pos + l,
                std::vector<std::uint8_t>(
                    chunk.begin() + l,
                    chunk.begin() + r
                )
            });
        }
    } else {
        files_[fileId].ordseg->handleChunk({fileId, pos, chunk});
    }
}


void FileCollector::work() {
    while (stopping_ == false) {
        std::unique_lock queryLock(chunksQueryMutex);
        chunksQueryCv.wait(queryLock, [this](){
            return stopping_ || !chunksQuery.empty();
        });

        if (stopping_ && chunksQuery.empty()) {
            return;
        }

        auto chunk = chunksQuery.front();
        chunksQuery.pop();

        std::copy(
            chunk.data.begin(),
            chunk.data.end(),
            files_[chunk.fileId].chunks.begin() + chunk.pos
        );
    }
}

const std::vector<std::uint8_t>& 
FileCollector::GetFile(const std::uint32_t fileId) 
{
    if (files_.count(fileId) == 0)
        throw std::invalid_argument("File ID does not exist");
    if (IsReady(fileId) == false) 
        throw std::runtime_error("File is not ready yet");

    const std::vector<std::uint8_t>& view = files_.at(fileId).chunks;
    return view;
}


bool FileCollector::IsReady(const std::uint32_t fileId)  
{
    if (files_.count(fileId) == 0)
        throw std::invalid_argument("File ID does not exist");

    return files_.at(fileId).ordseg->isReady();
}