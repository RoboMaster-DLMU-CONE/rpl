#ifndef RPL_DEF_HPP
#define RPL_DEF_HPP
#include <cstdint>

namespace RPL
{
    static constexpr uint8_t FRAME_START_BYTE = 0xA5;
    static constexpr size_t FRAME_HEADER_SIZE = 7;
    static constexpr size_t FRAME_TAIL_SIZE = 2;
}

#endif //RPL_DEF_HPP
