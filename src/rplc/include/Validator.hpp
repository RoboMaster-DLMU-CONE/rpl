#pragma once

#include "Config.hpp"
#include <string>
#include <vector>
#include <set>

namespace rplc
{
    /**
     * 验证结果
     */
    struct ValidationResult
    {
        bool valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;

        void add_error(const std::string& error)
        {
            valid = false;
            errors.push_back(error);
        }

        void add_warning(const std::string& warning)
        {
            warnings.push_back(warning);
        }
    };

    /**
     * 配置验证器
     */
    class Validator
    {
    public:
        /**
         * 验证包配置
         * @param config 包配置
         * @return 验证结果
         */
        static ValidationResult validate(const PacketConfig& config);

    private:
        /**
         * 验证包名称
         */
        static void validate_packet_name(const std::string& name, ValidationResult& result);

        /**
         * 验证命令ID
         */
        static void validate_command_id(const std::string& cmd_id, ValidationResult& result);

        /**
         * 验证命名空间
         */
        static void validate_namespace(const std::optional<std::string>& ns, ValidationResult& result);

        static void validate_header_guard(const std::optional<std::string>& hg, ValidationResult& result);

        /**
         * 验证字段列表
         */
        static void validate_fields(const std::vector<Field>& fields, ValidationResult& result);

        /**
         * 验证单个字段
         */
        static void validate_field(const Field& field, ValidationResult& result);

        /**
         * 检查标识符是否合法
         */
        static bool is_valid_identifier(const std::string& identifier);

        /**
         * 检查是否为C++关键字
         */
        static bool is_cpp_keyword(const std::string& word);

        /**
         * 检查数据类型是否支持
         */
        static bool is_supported_type(const std::string& type);

        /**
         * 解析命令ID（十六进制或十进制）
         */
        static std::optional<uint16_t> parse_command_id(const std::string& cmd_id);

        /**
         * 获取支持的数据类型集合
         */
        static const std::set<std::string>& get_supported_types();

        /**
         * 获取C++关键字集合
         */
        static const std::set<std::string>& get_cpp_keywords();
    };
} // namespace rplc
