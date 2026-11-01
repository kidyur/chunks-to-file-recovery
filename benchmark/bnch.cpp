#include <benchmark/benchmark.h>
#include <file_collector/file_collector.hpp>
#define FILE_ID 0
#define SIZE 40000

static void BM_SomeFunction(benchmark::State& state) {
    FileCollector fc;
    fc.CollectFile(FILE_ID, SIZE);
    std::vector<std::uint8_t> hugefile;
    hugefile.resize(SIZE);
    for (int i = 0; i < SIZE; i++) hugefile[i] = i;

    for (auto _ : state) {
        for (int i = 0; i < 35001; i+=5000) 
            fc.OnNewChunk(FILE_ID, i, std::vector<std::uint8_t>(hugefile.begin() + i, hugefile.begin() + i + 5000));

        while (!fc.IsReady(FILE_ID)) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1)
            );
        }
    }
}

BENCHMARK(BM_SomeFunction);
BENCHMARK_MAIN();