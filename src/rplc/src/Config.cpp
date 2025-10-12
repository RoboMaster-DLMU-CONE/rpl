#include "Config.hpp"
#include <fstream>
#include <fmt/core.h>

namespace rplc
{
    std::optional<PacketConfig> ConfigLoader::load_from_file(const std::string& filename)
    {
        try
        {
            std::ifstream file(filename);
            if (!file.is_open())
            {
                fmt::print(stderr, "Error: Cannot open file '{}'\n", filename);
                return std::nullopt;
            }

            nlohmann::json j;
            file >> j;
            return parse_config(j);
        }
        catch (const nlohmann::json::parse_error& e)
        {
            fmt::print(stderr, "JSON parse error in '{}': {}\n", filename, e.what());
            return std::nullopt;
        }
        catch (const std::exception& e)
        {
            fmt::print(stderr, "Error reading file '{}': {}\n", filename, e.what());
            return std::nullopt;
        }
    }

    std::optional<PacketConfig> ConfigLoader::load_from_string(const std::string& json_str)
    {
        try
        {
            nlohmann::json j = nlohmann::json::parse(json_str);
            return parse_config(j);
        }
        catch (const nlohmann::json::parse_error& e)
        {
            fmt::print(stderr, "JSON parse error: {}\n", e.what());
            return std::nullopt;
        }
    }

    std::optional<PacketConfig> ConfigLoader::parse_config(const nlohmann::json& j)
    {
        try
        {
            PacketConfig config;

            // 检查必需字段
            if (!j.contains("packet_name") || !j.contains("command_id") ||
                !j.contains("fields"))
            {
                fmt::print(stderr, "Error: Missing required fields in configuration\n");
                return std::nullopt;
            }

            // 解析基本字段
            config.packet_name = j["packet_name"].get<std::string>();
            config.command_id = j["command_id"].get<std::string>();

            // 解析可选字段
            if (j.contains("namespace") && !j["namespace"].is_null())
            {
                config.name_space = j["namespace"].get<std::string>();
            }

            if (j.contains("packed"))
            {
                config.packed = j["packed"].get<bool>();
            }

            // 解析输出配置（可选）
            if (j.contains("output") && j["output"].is_object())
            {
                auto output = parse_output(j["output"]);
                if (!output)
                {
                    return std::nullopt;
                }
                config.output = *output;
            }
            else
            {
                // 默认输出配置
                config.output.header_file = ""; // 不再从配置决定输出路径
                config.output.header_guard = std::nullopt;
            }

            // 解析字段
            auto fields = parse_fields(j["fields"]);
            if (!fields)
            {
                return std::nullopt;
            }
            config.fields = *fields;

            if (config.fields.empty())
            {
                fmt::print(stderr, "Error: At least one field is required\n");
                return std::nullopt;
            }

            return config;
        }
        catch (const std::exception& e)
        {
            fmt::print(stderr, "Error parsing configuration: {}\n", e.what());
            return std::nullopt;
        }
    }

    std::optional<std::vector<Field>> ConfigLoader::parse_fields(const nlohmann::json& j)
    {
        if (!j.is_array())
        {
            fmt::print(stderr, "Error: 'fields' must be an array\n");
            return std::nullopt;
        }

        std::vector<Field> fields;
        for (size_t i = 0; i < j.size(); ++i)
        {
            const auto& field_json = j[i];

            if (!field_json.contains("name") || !field_json.contains("type"))
            {
                fmt::print(stderr, "Error: Field {} missing required 'name' or 'type'\n", i);
                return std::nullopt;
            }

            Field field;
            field.name = field_json["name"].get<std::string>();
            field.type = field_json["type"].get<std::string>();

            if (field_json.contains("comment"))
            {
                field.comment = field_json["comment"].get<std::string>();
            }

            fields.push_back(field);
        }

        return fields;
    }

    std::optional<OutputConfig> ConfigLoader::parse_output(const nlohmann::json& j)
    {
        OutputConfig output;

        // header_file 变为可选；缺省时设为空字符串
        if (j.contains("header_file") && !j["header_file"].is_null())
        {
            output.header_file = j["header_file"].get<std::string>();
        }
        else
        {
            output.header_file = "";
        }

        if (j.contains("header_guard") && !j["header_guard"].is_null())
        {
            output.header_guard = j["header_guard"].get<std::string>();
        }

        return output;
    }
} // namespace rplc
