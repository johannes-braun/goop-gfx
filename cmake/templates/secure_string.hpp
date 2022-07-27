#pragma once

#include <array>
#include <string>

namespace security
{
    constexpr auto key = static_cast<char>(
        __TIME__[0] + 
        __TIME__[1] * 3 +
        __TIME__[3] * 7 +
        __TIME__[4] * 11 +
        __TIME__[6] * 13 +
        __TIME__[7] * 17);

    template<size_t S>
    class secure_const_string
    {
    public:
        constexpr secure_const_string(char const (&literal)[S])
        {
            char k = key;
            for(size_t i = 0; i < S; ++i)
            {
                _data[i] = literal[i] ^ k;
                k *= 7;
            }
        }

        std::string str() const 
        {
            std::array<char, S> copy;
            
            char k = key;
            for(size_t i = 0; i < S; ++i)
            {
                copy[i] = _data[i] ^ k;
                k *= 7;
            }
            return std::string(copy.data(), S-1);
        }

    private:
        std::array<char, S> _data{};
    };
}