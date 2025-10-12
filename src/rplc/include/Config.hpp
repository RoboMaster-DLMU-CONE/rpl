#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace rplc
{
    /**
     * 表示包中的一个字段
     */
    struct Field
    {
        std::string name; // 字段名称
        std::string type; // 数据类型
        std::string comment; // 注释（可选）
    };

    /**
     * 包配置结构
     */
    struct PacketConfig
    {
        std::string packet_name; // 包名称
        std::string command_id; // 命令ID（字符串形式，支持0x格式）
        std::optional<std::string> name_space; // 命名空间（可选）
        bool packed = true; // 是否使用packed属性
        std::optional<std::string> header_guard; // 头文件保护宏（可选）
        std::vector<Field> fields; // 字段列表
    };

    /**
     * 配置加载器和解析器
     */
    class ConfigLoader
    {
    public:
        /**
         * 从JSON文件加载配置
         * @param filename JSON配置文件路径
         * @return 解析成功返回配置对象，失败返回nullopt
         */
        static std::optional<PacketConfig> load_from_file(const std::string& filename);

        /**
         * 从JSON字符串加载配置
         * @param json_str JSON字符串
         * @return 解析成功返回配置对象，失败返回nullopt
         */
        static std::optional<PacketConfig> load_from_string(const std::string& json_str);

    private:
        /**
         * 从nlohmann::json对象解析配置
         * @param j JSON对象
         * @return 解析成功返回配置对象，失败返回nullopt
         */
        static std::optional<PacketConfig> parse_config(const nlohmann::json& j);

        /**
         * 解析字段配置
         * @param j JSON数组
         * @return 解析成功返回字段列表，失败返回nullopt
         */
        static std::optional<std::vector<Field>> parse_fields(const nlohmann::json& j);
    };
} // namespace rplc
