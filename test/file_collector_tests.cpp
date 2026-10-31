#include <gtest/gtest.h>
#include <file_collector/file_collector.hpp>
#include <chrono>
#include <thread>
#include <vector>
#include <cstdint>

#define FILE_ID   0
#define EVEN_SIZE 4
#define ODD_SIZE  3

TEST(FileCollectorTest, SimpleAssembly) {
    FileCollector fc;
    fc.CollectFile(FILE_ID, 6);

    fc.OnNewChunk(FILE_ID, 3, {4, 5, 6});
    fc.OnNewChunk(FILE_ID, 0, {1, 2, 3});
    fc.OnNewChunk(FILE_ID, 2, {3, 4});
    while (!fc.IsReady(FILE_ID)) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(1)
        );
    }

    ASSERT_TRUE(fc.IsReady(FILE_ID));
    EXPECT_EQ(
        fc.GetFile(FILE_ID),
        (std::vector<std::uint8_t>{1, 2, 3, 4, 5, 6})
    );
}

TEST(FileCollectorTest, bigFileWithIntersectedChunks) {
    FileCollector fc;
    fc.CollectFile(FILE_ID, 40000);
    std::vector<std::uint8_t> hugefile;
    hugefile.resize(40000);
    for (int i = 0; i < 40000; i++) hugefile[i] = i;
    

    // for (int i = 0; i < 35001; i+=5000) 
    //     fc.OnNewChunk(FILE_ID, i, std::vector<std::uint8_t>(hugefile.begin() + i, hugefile.begin() + i + 5000));
    fc.OnNewChunk(FILE_ID, 0, std::vector<std::uint8_t>(hugefile));

    while (!fc.IsReady(FILE_ID)) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(1)
        );
    }

    ASSERT_TRUE(fc.IsReady(FILE_ID));
    EXPECT_EQ(
        fc.GetFile(FILE_ID),
        hugefile
    );    
}



TEST(ChunkHandling, overlapping) {
    FileCollector fc;
    fc.CollectFile(FILE_ID, ODD_SIZE);

    fc.OnNewChunk(FILE_ID, 0, {1,2});
    fc.OnNewChunk(FILE_ID, 1, {2,2});  
    while (!fc.IsReady(FILE_ID)) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(1)
        );
    }

    const std::vector<std::uint8_t> res = fc.GetFile(FILE_ID);
    for (int i = 0; i < 1; i++) 
        ASSERT_EQ(res[i], 1);
    for (int i = 1; i < ODD_SIZE; i++) 
        ASSERT_EQ(res[i], 2);
}

TEST(ChunkHandling, oneChunkFile) {
    FileCollector fc;
    fc.CollectFile(FILE_ID, ODD_SIZE);

    fc.OnNewChunk(FILE_ID, 0, {0,1,2});
    while (!fc.IsReady(FILE_ID)) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(1)
        );
    }              

    const std::vector<std::uint8_t> res = fc.GetFile(FILE_ID);
    for (int i = 0; i < ODD_SIZE; i++) 
        ASSERT_EQ(res[i], i);
}

TEST(ChunkHandling, oneSymbolFile) {
    FileCollector fc;
    fc.CollectFile(FILE_ID, 1);

    fc.OnNewChunk(FILE_ID, 0, {1});
    while (!fc.IsReady(FILE_ID)) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(1)
        );
    }              

    const std::vector<std::uint8_t> res = fc.GetFile(FILE_ID);
    for (int i = 0; i < 1; i++) 
        ASSERT_EQ(res[i], 1);
}

TEST(ChunkHandling, chunksInReverseOrder) {
    FileCollector fc;
    fc.CollectFile(FILE_ID, EVEN_SIZE);

    fc.OnNewChunk(FILE_ID, 3, {3});
    fc.OnNewChunk(FILE_ID, 2, {2});
    fc.OnNewChunk(FILE_ID, 1, {1});
    fc.OnNewChunk(FILE_ID, 0, {0});
    while (!fc.IsReady(FILE_ID)) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(1)
        );
    }          
    
    const std::vector<std::uint8_t> res = fc.GetFile(FILE_ID);
    for (int i = 0; i < EVEN_SIZE; i++) 
        ASSERT_EQ(res[i], i);
}

TEST(ChunkHandling, chunksInForwardOrder) {
    FileCollector fc;
    fc.CollectFile(FILE_ID, EVEN_SIZE);

    fc.OnNewChunk(FILE_ID, 0, {0});
    fc.OnNewChunk(FILE_ID, 1, {1});
    fc.OnNewChunk(FILE_ID, 2, {2});
    fc.OnNewChunk(FILE_ID, 3, {3});
    while (!fc.IsReady(FILE_ID)) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(1)
        );
    }    
    
    const std::vector<std::uint8_t> res = fc.GetFile(FILE_ID);
    for (int i = 0; i < EVEN_SIZE; i++) 
        ASSERT_EQ(res[i], i);
}
