/*
MIT License

Copyright (c) 2018 Manik Charan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef TT_H
#define TT_H

#include <cinttypes>
#include <memory>

enum TTConstants
{
    FLAG_EXACT = 1,
    FLAG_UPPER = 2,
    FLAG_LOWER = 3,

    FLAG_SHIFT = 21,
    DEPTH_SHIFT = 23,
    SCORE_SHIFT = 32,

    MOVE_MASK = 0x1fffff,
    FLAG_MASK = 0x3,
    DEPTH_MASK = 0x7,

    CLUSTER_SIZE = 4
};

struct TTEntry
{
    TTEntry();
    std::uint64_t get_key() const;
    std::uint32_t get_move() const;
    void set(std::uint64_t move, std::uint64_t flag, std::uint64_t depth,
             std::uint64_t score, std::uint64_t key);
    int get_flag() const;
    int get_depth() const;
    int get_score() const;
    void clear();

private:
    std::uint64_t key;
    std::uint64_t data;
};

inline TTEntry::TTEntry() {}
inline void TTEntry::set(std::uint64_t move, std::uint64_t flag,
                         std::uint64_t depth, std::uint64_t score,
                         std::uint64_t key)
{
    data = move | (flag << FLAG_SHIFT) | (depth << DEPTH_SHIFT) | (score << SCORE_SHIFT);
    this->key = key ^ data;
}
inline std::uint64_t TTEntry::get_key() const { return key ^ data; }
inline std::uint32_t TTEntry::get_move() const { return std::uint32_t(data & MOVE_MASK); }
inline int TTEntry::get_flag() const { return (data >> FLAG_SHIFT) & FLAG_MASK; }
inline int TTEntry::get_depth() const { return (data >> DEPTH_SHIFT) & DEPTH_MASK; }
inline int TTEntry::get_score() const { return int(data >> SCORE_SHIFT); }
inline void TTEntry::clear() { key = data = 0; }

struct TTCluster
{
    TTEntry& get_entry(std::uint64_t key);
    void clear();

private:
    TTEntry entries[CLUSTER_SIZE];
};

inline TTEntry& TTCluster::get_entry(std::uint64_t key)
{
    // If any entry key matches, return it
    for (TTEntry& entry : entries) {
        if (entry.get_key() == key)
            return entry;
    }
    // Otherwise, return entry with minimum depth in cluster
    int min_depth_index = 0;
    for (int i = 1; i < CLUSTER_SIZE; ++i) {
        if (entries[i].get_depth() < entries[min_depth_index].get_depth())
            min_depth_index = i;
    }
    return entries[min_depth_index];
}

inline void TTCluster::clear()
{
    for (TTEntry& entry : entries)
        entry.clear();
}

struct TranspositionTable
{
    TranspositionTable();
    ~TranspositionTable();
    TranspositionTable(int MB);
    void resize(int MB);
    TTEntry probe(std::uint64_t key) const;
    void write(std::uint64_t move, std::uint64_t flag, std::uint64_t depth,
               std::uint64_t score, std::uint64_t key);
    void clear();
    int hash(std::uint64_t key) const;

private:
    TTCluster* table;
    int size;
};

inline TranspositionTable::TranspositionTable()
{
    size = (1 << 20) / sizeof(TTCluster);
    table = new TTCluster[size];
    clear();
}

inline TranspositionTable::~TranspositionTable()
{
    delete[] table;
}

inline TranspositionTable::TranspositionTable(int MB)
{
    resize(MB);
}

inline void TranspositionTable::resize(int MB)
{
    if (MB <= 0)
        MB = 1;

    size = ((1 << 20) / sizeof(TTCluster)) * MB;
    delete[] table;
    table = new TTCluster[size];
    clear();
}

inline void TranspositionTable::clear()
{
    for (int i = 0; i < size; ++i)
        table[i].clear();
}

inline int TranspositionTable::hash(std::uint64_t key) const
{
    return key % size;
}

inline TTEntry TranspositionTable::probe(std::uint64_t key) const
{
    int index = hash(key);
    return table[index].get_entry(key);
}

inline void TranspositionTable::write(
        std::uint64_t move, std::uint64_t flag, std::uint64_t depth,
        std::uint64_t score, std::uint64_t key
        )
{
    int index = hash(key);
    table[index].get_entry(key).set(move, flag, depth, score, key);
}

inline TranspositionTable tt;

#endif
