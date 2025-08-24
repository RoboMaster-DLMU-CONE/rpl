#ifndef RPL_RPL_HPP
#define RPL_RPL_HPP

#include <concepts>
#include <tl/expected.hpp>
#include "Error.hpp"
#include "Packet.hpp"

namespace rpl
{
    template<typename T>
    concept RPLPacket = std::derived_from<T, Packet>;

    template<RPLPacket T>
    class Parser
    {
    public:
        Parser();
        tl::expected<void, Error> serialize(const T& packet, char* buffer);
        tl::expected<T, Error> deserialize(char* buffer);

    private:

    };
}

#endif //RPL_RPL_HPP