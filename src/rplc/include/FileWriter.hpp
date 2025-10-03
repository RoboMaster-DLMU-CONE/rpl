#pragma once

#include <string>
#include <filesystem>

namespace rplc {

/**
 * 文件写入器
 */
class FileWriter {
public:
    /**
     * 写入内容到文件
     * @param filepath 文件路径
     * @param content 文件内容
     * @param create_directories 是否自动创建目录
     * @return 成功返回true
     */
    static bool write_file(const std::string& filepath, 
                          const std::string& content,
                          bool create_directories = true);

    /**
     * 检查文件是否存在
     * @param filepath 文件路径
     * @return 文件存在返回true
     */
    static bool file_exists(const std::string& filepath);

    /**
     * 创建目录
     * @param directory_path 目录路径
     * @return 成功返回true
     */
    static bool create_directory(const std::string& directory_path);

    /**
     * 获取文件的目录部分
     * @param filepath 文件路径
     * @return 目录路径
     */
    static std::string get_directory(const std::string& filepath);

    /**
     * 检查路径是否为绝对路径
     * @param path 路径
     * @return 是绝对路径返回true
     */
    static bool is_absolute_path(const std::string& path);

    /**
     * 规范化路径（转换为标准格式）
     * @param path 原始路径
     * @return 规范化后的路径
     */
    static std::string normalize_path(const std::string& path);

    /**
     * 备份现有文件（如果存在）
     * @param filepath 文件路径
     * @return 成功返回true
     */
    static bool backup_file(const std::string& filepath);
};

} // namespace rplc
