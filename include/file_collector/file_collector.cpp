
#include <stdexcept>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <map>
#include <mutex>
#include <iostream>
#include <thread>

#include "file_collector.hpp"


FileCollector::FileCollector() 
{
    threadsAvailable = std::thread::hardware_concurrency();
    if (threadsAvailable == 0) {
        threadsAvailable = FileCollector::kDefThreadsNumber;
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

    files_[fileId].fileSize = fileSize;
    files_[fileId].dataRestored = 0;
    files_[fileId].ordseg.emplace(&chunksQuery);
    files_[fileId].ordseg->makeAsync(&chunksQueryMutex, &chunksQueryCv);
}


void FileCollector::OnNewChunk(
    const std::uint32_t fileId, 
    const size_t pos, 
    std::vector<uint8_t>&& chunk) 
{
    if (files_.count(fileId) == 0)  
        throw std::invalid_argument("File ID does not exist");
    if (pos + chunk.size() > files_[fileId].fileSize)  
        throw std::out_of_range("Chunk size is out of file's bounds");
    if (files_[fileId].dataRestored == files_[fileId].fileSize)
        return;

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

        std::uint64_t id = chunksQuery.front().fileId;
        std::uint64_t size = chunksQuery.front().data.size();
        files_[id].chunks.insert(std::move(chunksQuery.front()));
        chunksQuery.pop();

        files_[id].dataRestored += size;
    }
}

std::vector<std::uint8_t> FileCollector::GetFile(const std::uint32_t fileId) 
{
    if (files_.count(fileId) == 0)
        throw std::invalid_argument("File ID does not exist");
    if (IsReady(fileId) == false) 
        return {};

    auto it = files_.at(fileId).chunks.begin();
    std::vector<std::uint8_t> file;
    file.resize(files_.at(fileId).fileSize);
    size_t pos = 0;
    while (pos != files_.at(fileId).fileSize) {
        for (size_t i = 0; i < it->data.size(); i++)  {
            file[pos + i] = it->data[i];
        }
        pos += it->data.size();
        it++;
    }
    return file;
}


bool FileCollector::IsReady(const std::uint32_t fileId)  
{
    if (files_.count(fileId) == 0)
        throw std::invalid_argument("File ID does not exist");
    auto& file = files_.at(fileId);
    return file.dataRestored == file.fileSize;
}