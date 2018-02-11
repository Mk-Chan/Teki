/*
 * Teki, a free UCI-compliant chess engine
 * Copyright (C) 2017 Manik Charan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
    DEPTH_MASK = 0x7
};

struct TTEntry
{
    TTEntry();
    TTEntry(std::uint64_t move, std::uint64_t flag, std::uint64_t depth,
            std::uint64_t score, std::uint64_t key);
    std::uint64_t get_key() const;
    std::uint32_t get_move() const;
    int get_flag() const;
    int get_depth() const;
    int get_score() const;
    void clear();

private:
    std::uint64_t key;
    std::uint64_t data;
};

inline TTEntry::TTEntry() {}
inline TTEntry::TTEntry(std::uint64_t move, std::uint64_t flag, std::uint64_t depth,
                        std::uint64_t score, std::uint64_t key)
{
    data = move | (flag << FLAG_SHIFT) | (depth << DEPTH_SHIFT) | (score << SCORE_SHIFT);
    this->key = key;
}
inline std::uint64_t TTEntry::get_key() const { return key; }
inline std::uint32_t TTEntry::get_move() const { return std::uint32_t(data & MOVE_MASK); }
inline int TTEntry::get_flag() const { return (data >> FLAG_SHIFT) & FLAG_MASK; }
inline int TTEntry::get_depth() const { return (data >> DEPTH_SHIFT) & DEPTH_MASK; }
inline int TTEntry::get_score() const { return int(data >> SCORE_SHIFT); }
inline void TTEntry::clear() { key = data = 0; }

struct TranspositionTable
{
    TranspositionTable();
    ~TranspositionTable();
    TranspositionTable(int MB);
    void resize(int MB);
    TTEntry probe(std::uint64_t key) const;
    void write(TTEntry entry);
    void clear();
    int hash(std::uint64_t key) const;

private:
    TTEntry* table;
    int size;
};

inline TranspositionTable::TranspositionTable()
{
    size = (1 << 20) / sizeof(TTEntry);
    table = new TTEntry[size];
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

    size = ((1 << 20) / sizeof(TTEntry)) * MB;
    delete[] table;
    table = new TTEntry[size];
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
    return table[index];
}

inline void TranspositionTable::write(const TTEntry entry)
{
    int index = hash(entry.get_key());
    table[index] = entry;
}

extern TranspositionTable tt;

#endif
