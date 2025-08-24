#ifndef RPL_SAMPLEPACKET_HPP
#define RPL_SAMPLEPACKET_HPP

#include <cstring>
#include <rpl/Packet.hpp>

class __attribute__((packed)) SamplePacket final : public rpl::Packet
{
public:
    // 构造函数
    SamplePacket() = default;

    // 构造函数，用于初始化所有字段
    SamplePacket(const float lift_val, const float stretch_val, const float shift_val,
                 const float suck_rotate_val, const float r1_val, const float r2_val, const float r3_val)
        : lift(lift_val), stretch(stretch_val), shift(shift_val)
          , suck_rotate(suck_rotate_val), r1(r1_val), r2(r2_val), r3(r3_val)
    {
    }

    // Getter和Setter方法
    [[nodiscard]] float get_lift() const { return lift; }
    void set_lift(const float value) { lift = value; }

    [[nodiscard]] float get_stretch() const { return stretch; }
    void set_stretch(const float value) { stretch = value; }

    [[nodiscard]] float get_shift() const { return shift; }
    void set_shift(const float value) { shift = value; }

    [[nodiscard]] float get_suck_rotate() const { return suck_rotate; }
    void set_suck_rotate(const float value) { suck_rotate = value; }

    [[nodiscard]] float get_r1() const { return r1; }
    void set_r1(const float value) { r1 = value; }

    [[nodiscard]] float get_r2() const { return r2; }
    void set_r2(const float value) { r2 = value; }

    [[nodiscard]] float get_r3() const { return r3; }
    void set_r3(const float value) { r3 = value; }

    // 重写基类虚函数
    [[nodiscard]] constexpr size_t data_size() const override
    {
        return sizeof(float) * 7; // 7个float字段
    }

    void serialize_data(uint8_t* buffer) const override
    {
        size_t offset = 0;
        std::memcpy(buffer + offset, &lift, sizeof(float));
        offset += sizeof(float);
        std::memcpy(buffer + offset, &stretch, sizeof(float));
        offset += sizeof(float);
        std::memcpy(buffer + offset, &shift, sizeof(float));
        offset += sizeof(float);
        std::memcpy(buffer + offset, &suck_rotate, sizeof(float));
        offset += sizeof(float);
        std::memcpy(buffer + offset, &r1, sizeof(float));
        offset += sizeof(float);
        std::memcpy(buffer + offset, &r2, sizeof(float));
        offset += sizeof(float);
        std::memcpy(buffer + offset, &r3, sizeof(float));
    }

    void deserialize_data(const uint8_t* buffer) override
    {
        size_t offset = 0;
        std::memcpy(&lift, buffer + offset, sizeof(float));
        offset += sizeof(float);
        std::memcpy(&stretch, buffer + offset, sizeof(float));
        offset += sizeof(float);
        std::memcpy(&shift, buffer + offset, sizeof(float));
        offset += sizeof(float);
        std::memcpy(&suck_rotate, buffer + offset, sizeof(float));
        offset += sizeof(float);
        std::memcpy(&r1, buffer + offset, sizeof(float));
        offset += sizeof(float);
        std::memcpy(&r2, buffer + offset, sizeof(float));
        offset += sizeof(float);
        std::memcpy(&r3, buffer + offset, sizeof(float));
    }

private:
    float lift = 0.0f;
    float stretch = 0.0f;
    float shift = 0.0f;
    float suck_rotate = 0.0f;
    float r1 = 0.0f;
    float r2 = 0.0f;
    float r3 = 0.0f;
};

#endif //RPL_SAMPLEPACKET_HPP
