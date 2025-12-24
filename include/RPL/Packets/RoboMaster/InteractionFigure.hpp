#ifndef RPL_INTERACTIONFIGURE_HPP
#define RPL_INTERACTIONFIGURE_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 客户端绘制一个图形 (子协议)
 */
struct InteractionFigure
{
    uint8_t[3] figure_name; ///< 图形索引名
    uint32_t operate_type : 3; ///< 0-空, 1-增加, 2-修改, 3-删除
    uint32_t figure_type : 3; ///< 0-直线, 1-矩形, 2-正圆, 3-椭圆, 4-圆弧, 5-浮点, 6-整型, 7-字符
    uint32_t layer : 4; ///< 图层 (0-9)
    uint32_t color : 4; ///< 0-红/蓝, 1-黄, 2-绿, 3-橙, 4-紫, 5-粉, 6-青, 7-黑, 8-白
    uint32_t details_a : 9; ///< 角度/字体大小/线宽等 (详见协议表 1-27)
    uint32_t details_b : 9; ///< 终点/半径/长度等
    uint32_t width : 10; ///< 线宽
    uint32_t start_x : 11; ///< 起点 X
    uint32_t start_y : 11; ///< 起点 Y
    uint32_t details_c : 10; ///< 浮点/半径等
    uint32_t details_d : 11; ///< 终点 X 等
    uint32_t details_e : 11; ///< 终点 Y 等
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<InteractionFigure> : PacketTraitsBase<PacketTraits<InteractionFigure>>
{
    static constexpr uint16_t cmd = 0x0101;
    static constexpr size_t size = sizeof(InteractionFigure);
};
#endif // RPL_INTERACTIONFIGURE_HPP
