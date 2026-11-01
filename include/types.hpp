#pragma once

#include <vector>
#include <set>
#include <cstdint>

struct Chunk {
    std::uint32_t fileId;
    size_t pos;
    std::vector<std::uint8_t> data;
};

struct CompChunks {
    bool operator()(const Chunk& lhs, const Chunk& rhs) const {
        return lhs.pos < rhs.pos;
    }
};

struct Segment {
    size_t l;
    size_t r;
};

struct Comp {
    bool operator()(const Segment& lhs, const Segment& rhs) const {
        return lhs.l < rhs.l;
    }
};







