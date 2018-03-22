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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <functional>
#include <unordered_map>

struct SpinOption
{
    SpinOption() {}
    SpinOption(int value, int min, int max, std::function<void(int)> handler)
        : value(value), min(min), max(max), handler(handler) {}

    void setoption(int value)
    {
        if (value >= min && value <= max)
        {
            this->value = value;
            if (handler)
                handler(value);
        }
    }

    int value;
    int min;
    int max;
    std::function<void(int)> handler;
};

struct CheckOption
{
    CheckOption() {}
    CheckOption(bool default_value, std::function<void(bool)> handler)
        : default_value(default_value), handler(handler) {}

    void setoption(bool value)
    {
        if (handler)
            handler(value);
    }

    bool default_value;
    std::function<void(bool)> handler;
};

struct StringOption
{
    StringOption() {}
    StringOption(std::string value, std::function<void(std::string)> handler)
        : value(value), handler(handler) {}

    void setoption(std::string& value)
    {
        this->value = value;
        if (handler)
            handler(value);
    }

    std::string value;
    std::function<void(std::string)> handler;
};

namespace options
{
    extern std::unordered_map<std::string, SpinOption> spins;
    extern std::unordered_map<std::string, CheckOption> checks;
    extern std::unordered_map<std::string, StringOption> strings;
}

#endif
