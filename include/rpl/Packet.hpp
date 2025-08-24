#ifndef RPL_PACKET_HPP
#define RPL_PACKET_HPP

#include <cstdint>
#include <string>

namespace rpl
{
    class __attribute__((packed)) Packet
    {
    public:
        Packet() = default;
        virtual ~Packet() = default;

        // 获取数据区大小（不包括帧头帧尾）
        [[nodiscard]] virtual constexpr size_t data_size() const = 0;

        // 获取完整帧大小（包括帧头帧尾）
        [[nodiscard]] constexpr size_t frame_size() const
        {
            return 5 + data_size() + 2; // 5字节帧头 + 数据 + 2字节帧尾
        }

        // 序列化数据区（不包括帧头帧尾）
        virtual void serialize_data(uint8_t* buffer) const = 0;
        // 反序列化数据区（不包括帧头帧尾）
        virtual void deserialize_data(const uint8_t* buffer) = 0;

        // 获取/设置包序号
        [[nodiscard]] uint8_t get_sequence_number() const { return sequence_number_; }
        void set_sequence_number(const uint8_t seq) { sequence_number_ = seq; }

    private:
        uint8_t sequence_number_ = 0;
    };
}

#endif //RPL_PACKET_HPP
