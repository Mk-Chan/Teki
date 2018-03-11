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

#ifndef SCORE_H
#define SCORE_H

#include "definitions.h"

struct Score
{
    Score() : mg(0), eg(0) {}
    Score(int val) : mg(val), eg(val) {}
    Score(int mg, int eg) : mg(mg), eg(eg) {}

    int value(int phase, int max_phase) const;
    int value() const;

    const Score operator-() const;

    const Score operator+(const Score& rhs) const;
    const Score operator-(const Score& rhs) const;
    const Score operator*(const Score& rhs) const;
    const Score operator/(const Score& rhs) const;

    const Score operator+(const int rhs) const;
    const Score operator-(const int rhs) const;
    const Score operator*(const int rhs) const;
    const Score operator/(const int rhs) const;

    const Score& operator+=(const Score& rhs);
    const Score& operator-=(const Score& rhs);
    const Score& operator*=(const Score& rhs);
    const Score& operator/=(const Score& rhs);

    const Score& operator+=(const int rhs);
    const Score& operator-=(const int rhs);
    const Score& operator*=(const int rhs);
    const Score& operator/=(const int rhs);

private:
    int mg;
    int eg;
};

inline int Score::value() const
{
    return mg;
}
inline int Score::value(int phase, int max_phase) const
{
    return ((mg * phase) + (eg * (max_phase - phase))) / max_phase;
}

inline const Score Score::operator-() const { return Score(-mg, -eg); }

inline const Score Score::operator+(const Score& rhs) const { return Score(mg + rhs.mg, eg + rhs.eg); }
inline const Score Score::operator-(const Score& rhs) const { return Score(mg - rhs.mg, eg - rhs.eg); }
inline const Score Score::operator*(const Score& rhs) const { return Score(mg * rhs.mg, eg * rhs.eg); }
inline const Score Score::operator/(const Score& rhs) const { return Score(mg / rhs.mg, eg / rhs.eg); }

inline const Score Score::operator+(const int rhs) const { return Score(mg + rhs, eg + rhs); }
inline const Score Score::operator-(const int rhs) const { return Score(mg - rhs, eg - rhs); }
inline const Score Score::operator*(const int rhs) const { return Score(mg * rhs, eg * rhs); }
inline const Score Score::operator/(const int rhs) const { return Score(mg / rhs, eg / rhs); }

inline const Score& Score::operator+=(const Score& rhs)
{
    mg += rhs.mg;
    eg += rhs.eg;
    return *this;
}
inline const Score& Score::operator-=(const Score& rhs)
{
    mg -= rhs.mg;
    eg -= rhs.eg;
    return *this;
}
inline const Score& Score::operator*=(const Score& rhs)
{
    mg *= rhs.mg;
    eg *= rhs.eg;
    return *this;
}
inline const Score& Score::operator/=(const Score& rhs)
{
    mg /= rhs.mg;
    eg /= rhs.eg;
    return *this;
}

inline const Score& Score::operator+=(const int rhs)
{
    mg += rhs;
    eg += rhs;
    return *this;
}
inline const Score& Score::operator-=(const int rhs)
{
    mg -= rhs;
    eg -= rhs;
    return *this;
}
inline const Score& Score::operator*=(const int rhs)
{
    mg *= rhs;
    eg *= rhs;
    return *this;
}
inline const Score& Score::operator/=(const int rhs)
{
    mg /= rhs;
    eg /= rhs;
    return *this;
}

inline Score S(int val) { return Score(val); }
inline Score S(int mg, int eg) { return Score(mg, eg); }

#endif
