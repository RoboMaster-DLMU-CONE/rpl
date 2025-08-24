#ifndef RPL_SAMPLEPACKET_HPP
#define RPL_SAMPLEPACKET_HPP

#include <cstring>
#include <rpl/Packet.hpp>

class __attribute__((packed)) SamplePacket final : public rpl::Packet
{
private:
    struct __attribute__((packed)) DataFields
    {
        float lift{};
        float stretch{};
        float shift{};
        float suck_rotate{};
        float r1{};
        float r2{};
        float r3{};
    } data_;

    static_assert(std::is_trivially_copyable_v<DataFields>, "DataFields must be trivially copyable");

public:
    SamplePacket() = default;

    SamplePacket(const float lift_val, const float stretch_val, const float shift_val,
                 const float suck_rotate_val, const float r1_val, const float r2_val, const float r3_val)
        : data_{lift_val, stretch_val, shift_val, suck_rotate_val, r1_val, r2_val, r3_val}
    {
    }

    explicit SamplePacket(const DataFields& fields)
        : data_(fields)
    {
    }

    [[nodiscard]] constexpr size_t data_size() const override
    {
        return sizeof(DataFields);
    }

    void serialize_data(uint8_t* buffer) const override
    {
        std::memcpy(buffer, &data_, sizeof(DataFields));
    }

    void deserialize_data(const uint8_t* buffer) override
    {
        std::memcpy(&data_, buffer, sizeof(DataFields));
    }

    [[nodiscard]] float get_lift() const { return data_.lift; }
    void set_lift(const float value) { data_.lift = value; }

    [[nodiscard]] float get_stretch() const { return data_.stretch; }
    void set_stretch(const float value) { data_.stretch = value; }

    [[nodiscard]] float get_shift() const { return data_.shift; }
    void set_shift(const float value) { data_.shift = value; }

    [[nodiscard]] float get_suck_rotate() const { return data_.suck_rotate; }
    void set_suck_rotate(const float value) { data_.suck_rotate = value; }

    [[nodiscard]] float get_r1() const { return data_.r1; }
    void set_r1(const float value) { data_.r1 = value; }

    [[nodiscard]] float get_r2() const { return data_.r2; }
    void set_r2(const float value) { data_.r2 = value; }

    [[nodiscard]] float get_r3() const { return data_.r3; }
    void set_r3(const float value) { data_.r3 = value; }
};

#endif //RPL_SAMPLEPACKET_HPP
