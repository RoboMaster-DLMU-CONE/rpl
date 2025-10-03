#pragma once

#include "Config.hpp"
#include <string>
#include <optional>

namespace rplc
{
    /**
     * 代码生成器接口
     */
    class IGenerator
    {
    public:
        virtual ~IGenerator() = default;

        /**
         * 生成代码
         * @param config 包配置
         * @return 生成的代码，失败返回nullopt
         */
        virtual std::optional<std::string> generate(const PacketConfig& config) const = 0;
    };

    /**
     * RPL包头文件生成器
     */
    class PacketHeaderGenerator : public IGenerator
    {
    public:
        std::optional<std::string> generate(const PacketConfig& config) const override;

    private:
        /**
         * 生成头文件保护宏
         */
        std::string generate_header_guard(const PacketConfig& config) const;

        /**
         * 生成包含语句
         */
        std::string generate_includes() const;

        /**
         * 生成命名空间开始
         */
        std::string generate_namespace_begin(const std::optional<std::string>& ns) const;

        /**
         * 生成命名空间结束
         */
        std::string generate_namespace_end(const std::optional<std::string>& ns) const;

        /**
         * 生成结构体定义
         */
        std::string generate_struct(const PacketConfig& config) const;

        /**
         * 生成PacketTraits特化
         */
        std::string generate_packet_traits(const PacketConfig& config) const;

        /**
         * 转换命令ID为C++格式
         */
        std::string format_command_id(const std::string& cmd_id) const;

        /**
         * 获取完全限定的结构体名称
         */
        std::string get_qualified_struct_name(const PacketConfig& config) const;

        /**
         * 生成字段定义
         */
        std::string generate_field(const Field& field, size_t max_type_width = 0) const;

        /**
         * 计算最大类型名宽度（用于对齐）
         */
        size_t calculate_max_type_width(const std::vector<Field>& fields) const;
    };

    /**
     * 代码生成器工厂
     */
    class GeneratorFactory
    {
    public:
        /**
         * 创建包头文件生成器
         */
        static std::unique_ptr<IGenerator> create_packet_header_generator();
    };
} // namespace rplc
