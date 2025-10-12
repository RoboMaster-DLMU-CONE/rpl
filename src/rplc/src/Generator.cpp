#include "Generator.hpp"
#include <sstream>
#include <algorithm>
#include <fmt/core.h>

namespace rplc
{
    std::optional<std::string> PacketHeaderGenerator::generate(const PacketConfig& config) const
    {
        try
        {
            std::ostringstream oss;

            // 头文件保护宏开始
            const std::string header_guard = generate_header_guard(config);
            oss << fmt::format("#ifndef {}\n", header_guard);
            oss << fmt::format("#define {}\n\n", header_guard);

            // 包含语句
            oss << generate_includes() << "\n";

            // 命名空间开始
            oss << generate_namespace_begin(config.name_space);

            // 结构体定义
            oss << generate_struct(config);

            // 命名空间结束
            oss << generate_namespace_end(config.name_space);

            // PacketTraits特化
            oss << generate_packet_traits(config);

            // 头文件保护宏结束
            oss << fmt::format("\n#endif //{}\n", header_guard);

            return oss.str();
        }
        catch (const std::exception& e)
        {
            // 代码生成失败
            return std::nullopt;
        }
    }

    std::string PacketHeaderGenerator::generate_header_guard(const PacketConfig& config) const
    {
        if (config.header_guard)
        {
            return *config.header_guard;
        }

        // 自动生成头文件保护宏：RPL_PACKETNAME_HPP
        std::string guard = "RPL_" + config.packet_name + "_HPP";
        std::ranges::transform(guard, guard.begin(), ::toupper);
        return guard;
    }

    std::string PacketHeaderGenerator::generate_includes() const
    {
        return "#include <cstdint>\n#include <RPL/Meta/PacketTraits.hpp>";
    }

    std::string PacketHeaderGenerator::generate_namespace_begin(const std::optional<std::string>& ns) const
    {
        if (!ns || ns->empty())
        {
            return "";
        }

        return fmt::format("namespace {} {{\n\n", *ns);
    }

    std::string PacketHeaderGenerator::generate_namespace_end(const std::optional<std::string>& ns) const
    {
        if (!ns || ns->empty())
        {
            return "";
        }

        return fmt::format("}} // namespace {}\n", *ns);
    }

    std::string PacketHeaderGenerator::generate_struct(const PacketConfig& config) const
    {
        std::ostringstream oss;

        // 结构体声明
        if (config.packed)
        {
            oss << fmt::format("struct __attribute__((packed)) {}\n", config.packet_name);
        }
        else
        {
            oss << fmt::format("struct {}\n", config.packet_name);
        }
        oss << "{\n";

        // 字段定义 - 计算对齐
        const size_t max_type_width = calculate_max_type_width(config.fields);
        for (const auto& field : config.fields)
        {
            oss << "    " << generate_field(field, max_type_width) << "\n";
        }

        oss << "};\n";

        return oss.str();
    }

    std::string PacketHeaderGenerator::generate_packet_traits(const PacketConfig& config) const
    {
        const std::string qualified_name = get_qualified_struct_name(config);
        const std::string cmd_id = format_command_id(config.command_id);

        std::ostringstream oss;
        oss << "template <>\n";
        oss << fmt::format("struct RPL::Meta::PacketTraits<{}> : PacketTraitsBase<PacketTraits<{}>>\n",
                           qualified_name, qualified_name);
        oss << "{\n";
        oss << fmt::format("    static constexpr uint16_t cmd = {};\n", cmd_id);
        oss << fmt::format("    static constexpr size_t size = sizeof({});\n", qualified_name);
        oss << "};";

        return oss.str();
    }

    std::string PacketHeaderGenerator::format_command_id(const std::string& cmd_id) const
    {
        // 如果已经是0x格式，直接返回
        if (cmd_id.starts_with("0x") || cmd_id.starts_with("0X"))
        {
            return cmd_id;
        }

        // 转换为十六进制格式
        try
        {
            unsigned long value = std::stoul(cmd_id, nullptr, 10);
            return fmt::format("0x{:04X}", static_cast<uint16_t>(value));
        }
        catch (const std::exception&)
        {
            // 如果转换失败，返回原始字符串（验证阶段应该已经捕获这种错误）
            return cmd_id;
        }
    }

    std::string PacketHeaderGenerator::get_qualified_struct_name(const PacketConfig& config) const
    {
        if (!config.name_space || config.name_space->empty())
        {
            return config.packet_name;
        }
        return *config.name_space + "::" + config.packet_name;
    }

    std::string PacketHeaderGenerator::generate_field(const Field& field, size_t max_type_width) const
    {
        std::string result;

        // 类型名对齐
        if (max_type_width > 0)
        {
            result = fmt::format("{:<{}} {}", field.type, max_type_width, field.name);
        }
        else
        {
            result = fmt::format("{} {}", field.type, field.name);
        }

        result += ";";

        // 添加注释
        if (!field.comment.empty())
        {
            // 计算注释对齐位置（至少在字段定义后面留2个空格）
            const size_t comment_pos = std::max(result.length() + 2, size_t(40));
            while (result.length() < comment_pos - 2)
            {
                result += " ";
            }
            result += fmt::format("  // {}", field.comment);
        }

        return result;
    }

    size_t PacketHeaderGenerator::calculate_max_type_width(const std::vector<Field>& fields) const
    {
        size_t max_width = 0;
        for (const auto& field : fields)
        {
            max_width = std::max(max_width, field.type.length());
        }
        return max_width;
    }

    // Factory implementation
    std::unique_ptr<IGenerator> GeneratorFactory::create_packet_header_generator()
    {
        return std::make_unique<PacketHeaderGenerator>();
    }
} // namespace rplc
