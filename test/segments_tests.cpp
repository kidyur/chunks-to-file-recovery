#include <gtest/gtest.h>
#include <iostream>
#include <queue>
#include <iostream>
#include <segment_handler/orderedseg.hpp>

#define FILE_ID 0

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

TEST(workQuery, oneInsert) 
{
    OrderedSegments os(new std::queue<Chunk>, 0);
    
    os.handleChunk({FILE_ID, 1, {0,1,2}});

    auto w = os.work->front();
    EXPECT_EQ(w.data.size(), 3);
    EXPECT_EQ(w.pos, 1);
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(w.data[i], i);
    }
    EXPECT_EQ(os.work->size(), 1);
}

TEST(workQuery, insertUnmergeable) 
{
    OrderedSegments os(new std::queue<Chunk>, 0);
    
    os.handleChunk({FILE_ID, 0, {0,1}});
    os.handleChunk({FILE_ID, 5, {0,1,2}});

    auto front = os.work->front();
    EXPECT_EQ(front.data.size(), 2);
    EXPECT_EQ(front.pos, 0);
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(front.data[i], i);
    }

    auto back = os.work->back();
    EXPECT_EQ(back.data.size(), 3);
    EXPECT_EQ(back.pos, 5);
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(back.data[i], i);
    }
    EXPECT_EQ(os.work->size(), 2);
}

TEST(workQuery, insertMergeableBackwards) 
{
    OrderedSegments os(new std::queue<Chunk>, 0);
    
    os.handleChunk({FILE_ID, 0, {0,1,2,3,4}});
    os.handleChunk({FILE_ID, 2, {2,3,4,5,6}});

    auto front = os.work->front();
    EXPECT_EQ(front.data.size(), 5);
    EXPECT_EQ(front.pos, 0);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(front.data[i], i);
    }

    auto back = os.work->back();
    EXPECT_EQ(back.data.size(), 2);
    EXPECT_EQ(back.pos, 5);
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(back.data[i], back.pos + i);
    }
    EXPECT_EQ(os.work->size(), 2);
}

TEST(segments, MnsertUnmergeableSegments) 
{
    OrderedSegments os(new std::queue<Chunk>, 0);
    
    os.handleChunk({FILE_ID, 0, {0,0,0}});
    os.handleChunk({FILE_ID, 5, {0,0}});
    os.handleChunk({FILE_ID, 9, {0}});

    auto it = os.segments.begin();
    EXPECT_EQ(*it, Segment({0, 2}));
    it++;
    EXPECT_EQ(*it, Segment({5, 6}));
    it++;
    EXPECT_EQ(*it, Segment({9, 9}));
    EXPECT_EQ(os.work->size(), 3);
}

TEST(segments, splitBackwards) 
{
    OrderedSegments os(new std::queue<Chunk>, 0);
    
    os.handleChunk({FILE_ID, 0, {0,0,0,0,0}});
    os.handleChunk({FILE_ID, 3, {0,0,0}});
    os.handleChunk({FILE_ID, 9, {0}});

    auto it = os.segments.begin();
    
    EXPECT_EQ(*it, Segment({0, 5}));
    it++;
    EXPECT_EQ(*it, Segment({9, 9}));
    EXPECT_EQ(os.work->size(), 3);
}

TEST(segments, splitForward) 
{
    OrderedSegments os(new std::queue<Chunk>, 0);
    
    os.handleChunk({FILE_ID, 0, {0,0,0}});
    os.handleChunk({FILE_ID, 4, {0,0,0,0,0}});
    os.handleChunk({FILE_ID, 6, {0,0,0,0}});

    auto it = os.segments.begin();
    EXPECT_EQ(*it, Segment({0, 2}));
    it++;
    EXPECT_EQ(*it, Segment({4, 9}));
    EXPECT_EQ(os.work->size(), 3);
}

TEST(segments, splitForwardsAndBackwards) 
{
    OrderedSegments os(new std::queue<Chunk>, 0);
    
    os.handleChunk({FILE_ID, 0, {0,0,0}});
    os.handleChunk({FILE_ID, 6, {0,0,0,0}});
    os.handleChunk({FILE_ID, 3, {0,1,2,0,0}});

    auto it = os.segments.begin();
    EXPECT_EQ(*it, Segment({0, 9}));

    auto fr = os.work->front();
    EXPECT_EQ(fr.pos, 0);

    auto bc = os.work->back();
    EXPECT_EQ(bc.pos, 3);
    EXPECT_EQ(bc.data.size(), 3);
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(bc.data[i], i);
    }
    EXPECT_EQ(os.work->size(), 3);
}

TEST(segments, OneByOne) 
{
    OrderedSegments os(new std::queue<Chunk>, 0);
    
    os.handleChunk({FILE_ID, 0, {0}});
    os.handleChunk({FILE_ID, 1, {1}});
    os.handleChunk({FILE_ID, 2, {2}});
    os.handleChunk({FILE_ID, 3, {3}});
    os.handleChunk({FILE_ID, 4, {4}});
    os.handleChunk({FILE_ID, 5, {5}});

    auto it = os.segments.begin();
    EXPECT_EQ(*it, Segment({0, 5}));

    auto fr = os.work->front();
    EXPECT_EQ(fr.pos, 0);
    EXPECT_EQ(fr.data.size(), 1);
    EXPECT_EQ(fr.data[0], 0);

    auto bc = os.work->back();
    EXPECT_EQ(bc.pos, 5);
    EXPECT_EQ(bc.data.size(), 1);
    EXPECT_EQ(bc.data[0], 5);
    EXPECT_EQ(os.work->size(), 6);
}

TEST(segments, duplicatesAddNoWorkAndSegment) 
{
    OrderedSegments os(new std::queue<Chunk>, 0);
    
    os.handleChunk({FILE_ID, 0, {0,1,2}});
    os.handleChunk({FILE_ID, 0, {0,1,2}});

    auto it = os.segments.begin();
    EXPECT_EQ(*it, Segment({0, 2}));

    auto bc = os.work->back();
    EXPECT_EQ(bc.pos, 0);
    os.work->pop();
    EXPECT_EQ(os.work->empty(), true);
    EXPECT_EQ(os.work->size(), 0);
}

TEST(segments, emptyChunk) 
{
    OrderedSegments os(new std::queue<Chunk>, 0);
    
    os.handleChunk({FILE_ID, 4, {}});

    EXPECT_EQ(os.segments.size(), 0);
    EXPECT_EQ(os.work->empty(), true);
    EXPECT_EQ(os.work->size(), 0);
}

TEST(segments, splitPoints) 
{
    OrderedSegments os(new std::queue<Chunk>, 0);
    
    os.handleChunk({FILE_ID, 0, {0}});
    os.handleChunk({FILE_ID, 2, {1}});
    os.handleChunk({FILE_ID, 4, {2}});
    os.handleChunk({FILE_ID, 0, {0,0,1,1,2}});
    
    auto it = os.segments.begin();
    EXPECT_EQ(*it, Segment({0, 4}));
    EXPECT_EQ(os.work->size(), 5);
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(os.work->front().data[0], i); 
        os.work->pop();
    }
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(os.work->front().data[0], i);
        os.work->pop();
    }
}

TEST(segments, splitIntervals) 
{
    OrderedSegments os(new std::queue<Chunk>, 0);
    
    os.handleChunk({FILE_ID, 0, {0, 1}});
    os.handleChunk({FILE_ID, 3, {3, 4}});
    os.handleChunk({FILE_ID, 6, {6, 7}});
    os.handleChunk({FILE_ID, 0, {0,1,2,3,4,5,6,7,8,9}});
    
    auto it = os.segments.begin();
    EXPECT_EQ(it->l, 0);
    EXPECT_EQ(it->r, 9);
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(os.work->front().data[0], i*3);
        EXPECT_EQ(os.work->front().data[1], i*3+1);
        os.work->pop();
    }

    EXPECT_EQ(os.work->front().data[0], 2);
    os.work->pop();
    EXPECT_EQ(os.work->front().data[0], 5);
    os.work->pop();
    EXPECT_EQ(os.work->front().data[0], 8);
    EXPECT_EQ(os.work->front().data[1], 9);
    os.work->pop();
    EXPECT_EQ(os.work->empty(), true);
}