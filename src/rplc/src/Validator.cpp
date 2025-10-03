#include "../include/Validator.hpp"
#include <regex>
#include <algorithm>
#include <cctype>
#include <fmt/core.h>

namespace rplc {

ValidationResult Validator::validate(const PacketConfig& config) {
    ValidationResult result;

    validate_packet_name(config.packet_name, result);
    validate_command_id(config.command_id, result);
    validate_namespace(config.name_space, result);
    validate_fields(config.fields, result);
    validate_output(config.output, result);

    return result;
}

void Validator::validate_packet_name(const std::string& name, ValidationResult& result) {
    if (name.empty()) {
        result.add_error("Packet name cannot be empty");
        return;
    }

    if (!is_valid_identifier(name)) {
        result.add_error(fmt::format("Invalid packet name '{}': must be a valid C++ identifier", name));
    }

    if (is_cpp_keyword(name)) {
        result.add_error(fmt::format("Packet name '{}' is a C++ keyword", name));
    }
}

void Validator::validate_command_id(const std::string& cmd_id, ValidationResult& result) {
    auto parsed = parse_command_id(cmd_id);
    if (!parsed) {
        result.add_error(fmt::format("Invalid command ID '{}': must be a valid 16-bit unsigned integer (0-65535)", cmd_id));
    }
}

void Validator::validate_namespace(const std::optional<std::string>& ns, ValidationResult& result) {
    if (!ns) return;

    const auto& namespace_str = *ns;
    if (namespace_str.empty()) {
        result.add_warning("Empty namespace specified, will be treated as global namespace");
        return;
    }

    // 检查命名空间格式（支持::分隔的多级命名空间）
    std::regex ns_regex(R"(^[a-zA-Z_][a-zA-Z0-9_]*(?:::[a-zA-Z_][a-zA-Z0-9_]*)*$)");
    if (!std::regex_match(namespace_str, ns_regex)) {
        result.add_error(fmt::format("Invalid namespace '{}': must be valid C++ namespace format", namespace_str));
    }

    // 检查各级命名空间是否为关键字
    std::string temp = namespace_str;
    size_t pos = 0;
    while ((pos = temp.find("::")) != std::string::npos) {
        std::string ns_part = temp.substr(0, pos);
        if (is_cpp_keyword(ns_part)) {
            result.add_error(fmt::format("Namespace part '{}' is a C++ keyword", ns_part));
        }
        temp.erase(0, pos + 2);
    }
    if (is_cpp_keyword(temp)) {
        result.add_error(fmt::format("Namespace part '{}' is a C++ keyword", temp));
    }
}

void Validator::validate_fields(const std::vector<Field>& fields, ValidationResult& result) {
    if (fields.empty()) {
        result.add_error("At least one field is required");
        return;
    }

    std::set<std::string> field_names;
    for (size_t i = 0; i < fields.size(); ++i) {
        validate_field(fields[i], result);

        // 检查字段名重复
        if (field_names.count(fields[i].name)) {
            result.add_error(fmt::format("Duplicate field name '{}' at position {}", fields[i].name, i));
        } else {
            field_names.insert(fields[i].name);
        }
    }
}

void Validator::validate_field(const Field& field, ValidationResult& result) {
    // 验证字段名
    if (field.name.empty()) {
        result.add_error("Field name cannot be empty");
    } else {
        if (!is_valid_identifier(field.name)) {
            result.add_error(fmt::format("Invalid field name '{}': must be a valid C++ identifier", field.name));
        }
        if (is_cpp_keyword(field.name)) {
            result.add_error(fmt::format("Field name '{}' is a C++ keyword", field.name));
        }
    }

    // 验证数据类型
    if (field.type.empty()) {
        result.add_error("Field type cannot be empty");
    } else if (!is_supported_type(field.type)) {
        result.add_error(fmt::format("Unsupported field type '{}' for field '{}'", field.type, field.name));
    }
}

void Validator::validate_output(const OutputConfig& output, ValidationResult& result) {
    // 验证头文件路径
    if (output.header_file.empty()) {
        result.add_error("Header file path cannot be empty");
    } else {
        // 检查文件扩展名
        if (!output.header_file.ends_with(".hpp") && !output.header_file.ends_with(".h")) {
            result.add_warning("Header file should have .hpp or .h extension");
        }

        // 建议使用正斜杠
        if (output.header_file.find('\\') != std::string::npos) {
            result.add_warning("Consider using forward slashes (/) in header file path for better cross-platform compatibility");
        }
    }

    // 验证头文件保护宏
    if (output.header_guard && !output.header_guard->empty()) {
        const std::string& guard = *output.header_guard;
        if (!is_valid_identifier(guard)) {
            result.add_error(fmt::format("Invalid header guard '{}': must be a valid C++ identifier", guard));
        }
        
        // 建议使用大写
        bool all_upper = std::all_of(guard.begin(), guard.end(), [](char c) {
            return std::isupper(c) || std::isdigit(c) || c == '_';
        });
        if (!all_upper) {
            result.add_warning("Header guard should typically be in UPPERCASE");
        }
    }
}

bool Validator::is_valid_identifier(const std::string& identifier) {
    if (identifier.empty()) return false;
    
    // 首字符必须是字母或下划线
    if (!std::isalpha(identifier[0]) && identifier[0] != '_') {
        return false;
    }
    
    // 其余字符必须是字母、数字或下划线
    return std::all_of(identifier.begin() + 1, identifier.end(), [](char c) {
        return std::isalnum(c) || c == '_';
    });
}

bool Validator::is_cpp_keyword(const std::string& word) {
    return get_cpp_keywords().count(word) > 0;
}

bool Validator::is_supported_type(const std::string& type) {
    return get_supported_types().count(type) > 0;
}

std::optional<uint16_t> Validator::parse_command_id(const std::string& cmd_id) {
    try {
        if (cmd_id.starts_with("0x") || cmd_id.starts_with("0X")) {
            // 十六进制格式
            unsigned long value = std::stoul(cmd_id, nullptr, 16);
            if (value > 0xFFFF) return std::nullopt;
            return static_cast<uint16_t>(value);
        } else {
            // 十进制格式
            unsigned long value = std::stoul(cmd_id, nullptr, 10);
            if (value > 65535) return std::nullopt;
            return static_cast<uint16_t>(value);
        }
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

const std::set<std::string>& Validator::get_supported_types() {
    static const std::set<std::string> types = {
        "uint8_t", "int8_t",
        "uint16_t", "int16_t", 
        "uint32_t", "int32_t",
        "uint64_t", "int64_t",
        "int", "float", "double"
    };
    return types;
}

const std::set<std::string>& Validator::get_cpp_keywords() {
    static const std::set<std::string> keywords = {
        // C++98 keywords
        "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
        "case", "catch", "char", "class", "compl", "const", "const_cast", "continue",
        "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit",
        "export", "extern", "false", "float", "for", "friend", "goto", "if",
        "inline", "int", "long", "mutable", "namespace", "new", "not", "not_eq",
        "operator", "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast",
        "return", "short", "signed", "sizeof", "static", "static_cast", "struct", "switch",
        "template", "this", "throw", "true", "try", "typedef", "typeid", "typename",
        "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while",
        "xor", "xor_eq",
        
        // C++11 and later
        "alignas", "alignof", "char16_t", "char32_t", "constexpr", "decltype", "noexcept",
        "nullptr", "static_assert", "thread_local",
        
        // C++20
        "concept", "requires", "co_await", "co_return", "co_yield"
    };
    return keywords;
}

} // namespace rplc
