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

#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <random>
#include <cinttypes>
#include <vector>
#include <algorithm>

typedef std::int64_t time_ms;

namespace utils
{
    inline time_ms curr_time()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    inline std::uint64_t rand_u64(std::uint64_t low_incl, std::uint64_t high_incl)
    {
        static std::mt19937_64 rng(88349201835);
        std::uniform_int_distribution<std::uint64_t> uni(low_incl, high_incl);
        return uni(rng);
    }

    inline std::uint32_t rand_u32(std::uint32_t low_incl, std::uint32_t high_incl)
    {
        std::mt19937 rng(curr_time());
        std::uniform_int_distribution<std::uint32_t> uni(low_incl, high_incl);
        return uni(rng);
    }

    inline std::int32_t rand_int(std::int32_t low_incl, std::int32_t high_incl)
    {
        std::mt19937 rng(curr_time());
        std::uniform_int_distribution<std::int32_t> uni(low_incl, high_incl);
        return uni(rng);
    }

    template <typename T>
    inline void remove_from_vec(T& val, std::vector<T>& vec)
    {
        vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end());
    }
}

#endif
