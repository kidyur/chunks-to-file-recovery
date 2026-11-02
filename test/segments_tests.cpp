#include <gtest/gtest.h>
#include <iostream>
#include <queue>
#include <iostream>
#include <segment_handler/orderedseg.hpp>
#include <set>
#include <types.hpp>
#define FILE_ID 0

class OrderedSegmentsTest {
public:
    OrderedSegments os; 
    std::set<Segment, Comp>& segs() { return os.segments; }
    std::set<Chunk, CompChunks> handleChunk(Chunk c) { return os.handleChunk(c); };
};


template<typename T>
void showQueue(std::queue<T>& q) {
    int loop = q.size();
    std::cout << "[Queue] front:\n";
    while (loop > 0) {
        auto el = q.front();
        std::cout << '\t' << el;
        q.pop();
        q.push(el);
        loop -= 1;
    }
    std::cout << "[Queue] back.";
}

static std::ostream& operator<<(std::ostream& ostr, const Chunk& ch) {
    std::cout << "{ fileId = " << ch.fileId << "; "
              << "pos = " << ch.pos << "; "
              << "data = ";
    for (size_t i = 0; i < ch.data.size(); i++) {
        std::cout << (int)ch.data[i] << ' ';
    }
    std::cout << '}' << std::endl;
    return ostr;
}

static bool operator==(const Segment& s1, const Segment& s2) {
    return s1.l == s2.l && s1.r == s2.r;
}

TEST(workQuery, oneInsert) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    
    rez.merge(os.handleChunk({FILE_ID, 1, {0,1,2}}));

    auto w = *rez.begin();
    EXPECT_EQ(w.data.size(), 3);
    EXPECT_EQ(w.pos, 1);
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(w.data[i], i);
    }
    EXPECT_EQ(rez.size(), 1);
}

TEST(workQuery, insertUnmergeable) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    
    rez.merge(os.handleChunk({FILE_ID, 0, {0,1}}));
    rez.merge(os.handleChunk({FILE_ID, 5, {0,1,2}}));

    auto front = *rez.begin();
    EXPECT_EQ(front.data.size(), 2);
    EXPECT_EQ(front.pos, 0);
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(front.data[i], i);
    }

    auto back = *std::prev(rez.end());
    EXPECT_EQ(back.data.size(), 3);
    EXPECT_EQ(back.pos, 5);
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(back.data[i], i);
    }
    EXPECT_EQ(rez.size(), 2);
}

TEST(workQuery, insertMergeableBackwards) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    
    rez.merge(os.handleChunk({FILE_ID, 0, {0,1,2,3,4}}));
    rez.merge(os.handleChunk({FILE_ID, 2, {2,3,4,5,6}}));

    auto front = *rez.begin();
    EXPECT_EQ(front.data.size(), 5);
    EXPECT_EQ(front.pos, 0);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(front.data[i], i);
    }

    auto back = *std::prev(rez.end());
    EXPECT_EQ(back.data.size(), 2);
    EXPECT_EQ(back.pos, 5);
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(back.data[i], back.pos + i);
    }
    EXPECT_EQ(rez.size(), 2);
}

TEST(segments, MnsertUnmergeableSegments) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    
    rez.merge(os.handleChunk({FILE_ID, 0, {0,0,0}}));
    rez.merge(os.handleChunk({FILE_ID, 5, {0,0}}));
    rez.merge(os.handleChunk({FILE_ID, 9, {0}}));

    auto it = os.segs().begin();
    EXPECT_EQ(*it, Segment({0, 2}));
    it++;
    EXPECT_EQ(*it, Segment({5, 6}));
    it++;
    EXPECT_EQ(*it, Segment({9, 9}));
    EXPECT_EQ(rez.size(), 3);
}

TEST(segments, ChunkForDoneFile) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    rez.merge(os.handleChunk({FILE_ID, 3, {4, 5, 6}}));
    rez.merge(os.handleChunk({FILE_ID, 0, {1, 2, 3}}));
    
    rez.merge(os.handleChunk({FILE_ID, 2, {3, 4}}));

    auto it = os.segs().begin();
    EXPECT_EQ(*it, Segment({0, 5}));
    EXPECT_EQ(rez.size(), 2);
    EXPECT_EQ(rez.begin()->data, std::vector<uint8_t>({1, 2, 3}));
    EXPECT_EQ(std::prev(rez.end())->data, std::vector<uint8_t>({4, 5, 6}));
}

TEST(segments, splitBackwards) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    
    rez.merge(os.handleChunk({FILE_ID, 0, {0,0,0,0,0}}));
    rez.merge(os.handleChunk({FILE_ID, 3, {0,0,0}}));
    rez.merge(os.handleChunk({FILE_ID, 9, {0}}));

    auto it = os.segs().begin();
    
    EXPECT_EQ(*it, Segment({0, 5}));
    it++;
    EXPECT_EQ(*it, Segment({9, 9}));
    EXPECT_EQ(rez.size(), 3);
}

TEST(segments, splitForward) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    
    rez.merge(os.handleChunk({FILE_ID, 0, {0,0,0}}));
    rez.merge(os.handleChunk({FILE_ID, 4, {0,0,0,0,0}}));
    rez.merge(os.handleChunk({FILE_ID, 6, {0,0,0,0}}));

    auto it = os.segs().begin();
    EXPECT_EQ(*it, Segment({0, 2}));
    it++;
    EXPECT_EQ(*it, Segment({4, 9}));
    EXPECT_EQ(rez.size(), 3);
}

TEST(segments, splitForwardsAndBackwards) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    
    rez.merge(os.handleChunk({FILE_ID, 0, {0,0,0}}));
    rez.merge(os.handleChunk({FILE_ID, 6, {0,0,0,0}}));
    rez.merge(os.handleChunk({FILE_ID, 3, {0,1,2,0,0}}));

    auto it = os.segs().begin();
    EXPECT_EQ(*it, Segment({0, 9}));

    auto fr = *rez.begin();
    EXPECT_EQ(fr.pos, 0);

    auto bc = *std::prev(rez.end());
    EXPECT_EQ(bc.pos, 6);
    EXPECT_EQ(bc.data.size(), 4);
}

TEST(segments, OneByOne) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    
    rez.merge(os.handleChunk({FILE_ID, 0, {0}}));
    rez.merge(os.handleChunk({FILE_ID, 1, {1}}));
    rez.merge(os.handleChunk({FILE_ID, 2, {2}}));
    rez.merge(os.handleChunk({FILE_ID, 3, {3}}));
    rez.merge(os.handleChunk({FILE_ID, 4, {4}}));
    rez.merge(os.handleChunk({FILE_ID, 5, {5}}));

    auto it = os.segs().begin();
    EXPECT_EQ(*it, Segment({0, 5}));

    auto fr = *rez.begin();
    EXPECT_EQ(fr.pos, 0);
    EXPECT_EQ(fr.data.size(), 1);
    EXPECT_EQ(fr.data[0], 0);

    auto bc = *std::prev(rez.end());
    EXPECT_EQ(bc.pos, 5);
    EXPECT_EQ(bc.data.size(), 1);
    EXPECT_EQ(bc.data[0], 5);
    EXPECT_EQ(rez.size(), 6);
}

TEST(segments, duplicatesAddNoWorkAndSegment) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    
    rez.merge(os.handleChunk({FILE_ID, 0, {0,1,2}}));
    rez.merge(os.handleChunk({FILE_ID, 0, {0,1,2}}));

    auto it = os.segs().begin();
    EXPECT_EQ(*it, Segment({0, 2}));

    auto bc = *std::prev(rez.end());
    EXPECT_EQ(bc.pos, 0);
    EXPECT_EQ(rez.size(), 1);
}

TEST(segments, emptyChunk) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    
    rez.merge(os.handleChunk({FILE_ID, 4, {}}));

    EXPECT_EQ(os.segs().size(), 0);
    EXPECT_EQ(rez.empty(), true);
}

TEST(segments, splitPoints) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    
    rez.merge(os.handleChunk({FILE_ID, 0, {0}}));
    rez.merge(os.handleChunk({FILE_ID, 2, {2}}));
    rez.merge(os.handleChunk({FILE_ID, 4, {4}}));
    rez.merge(os.handleChunk({FILE_ID, 0, {0,1,2,3,4}}));
    
    auto it = os.segs().begin();
    EXPECT_EQ(*it, Segment({0, 4}));
    EXPECT_EQ(rez.size(), 5);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(rez.begin()->data[0], i); 
        EXPECT_EQ(rez.begin()->pos, i); 
        rez.erase(rez.begin());
    }
}

TEST(segments, splitIntervals) 
{
    OrderedSegmentsTest os;
    std::set<Chunk, CompChunks> rez;
    
    rez.merge(os.handleChunk({FILE_ID, 0, {0, 1}}));
    rez.merge(os.handleChunk({FILE_ID, 3, {3, 4}}));
    rez.merge(os.handleChunk({FILE_ID, 6, {6, 7}}));
    rez.merge(os.handleChunk({FILE_ID, 0, {0,1,2,3,4,5,6,7,8}}));
    
    auto it = os.segs().begin();
    EXPECT_EQ(it->l, 0);
    EXPECT_EQ(it->r, 8);

    EXPECT_EQ(rez.size(), 6);
    int idx = 0;
    for (int i = 0; i < 6; i++) {
        EXPECT_EQ(rez.begin()->pos, idx);
        for (int j = 0; j < rez.begin()->data.size(); j++)
            EXPECT_EQ(rez.begin()->data[j], idx++);
        rez.erase(rez.begin());
    }
}