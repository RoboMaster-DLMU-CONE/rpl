#include "FileWriter.hpp"
#include <fstream>
#include <algorithm>
#include <fmt/core.h>

namespace rplc
{
    bool FileWriter::write_file(const std::string& filepath,
                                const std::string& content,
                                bool create_directories)
    {
        try
        {
            // 规范化路径
            const std::string normalized_path = normalize_path(filepath);

            // 创建目录
            if (create_directories)
            {
                const std::string dir = get_directory(normalized_path);
                if (!dir.empty() && !create_directory(dir))
                {
                    fmt::print(stderr, "Error: Failed to create directory '{}'\n", dir);
                    return false;
                }
            }

            // 写入文件
            std::ofstream file(normalized_path, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                fmt::print(stderr, "Error: Cannot open file '{}' for writing\n", normalized_path);
                return false;
            }

            file << content;
            if (file.fail())
            {
                fmt::print(stderr, "Error: Failed to write to file '{}'\n", normalized_path);
                return false;
            }

            file.close();
            fmt::print("Generated file: {}\n", normalized_path);
            return true;
        }
        catch (const std::exception& e)
        {
            fmt::print(stderr, "Error writing file '{}': {}\n", filepath, e.what());
            return false;
        }
    }

    bool FileWriter::file_exists(const std::string& filepath)
    {
        try
        {
            return std::filesystem::exists(filepath);
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool FileWriter::create_directory(const std::string& directory_path)
    {
        try
        {
            if (std::filesystem::exists(directory_path))
            {
                return std::filesystem::is_directory(directory_path);
            }

            return std::filesystem::create_directories(directory_path);
        }
        catch (const std::exception& e)
        {
            fmt::print(stderr, "Error creating directory '{}': {}\n", directory_path, e.what());
            return false;
        }
    }

    std::string FileWriter::get_directory(const std::string& filepath)
    {
        try
        {
            std::filesystem::path path(filepath);
            if (path.has_parent_path())
            {
                return path.parent_path().string();
            }
            return "";
        }
        catch (const std::exception&)
        {
            // 如果解析失败，手动查找最后一个目录分隔符
            size_t pos = filepath.find_last_of("/\\");
            if (pos != std::string::npos)
            {
                return filepath.substr(0, pos);
            }
            return "";
        }
    }

    bool FileWriter::is_absolute_path(const std::string& path)
    {
        try
        {
            return std::filesystem::path(path).is_absolute();
        }
        catch (const std::exception&)
        {
            // 简单检查：Unix风格的绝对路径以/开头，Windows风格可能以盘符开头
            if (path.empty()) return false;
            if (path[0] == '/') return true;
            if (path.length() >= 3 && path[1] == ':' && (path[2] == '/' || path[2] == '\\'))
            {
                return true;
            }
            return false;
        }
    }

    std::string FileWriter::normalize_path(const std::string& path)
    {
        try
        {
            std::filesystem::path fs_path(path);
            return fs_path.lexically_normal().string();
        }
        catch (const std::exception&)
        {
            // 如果std::filesystem失败，进行简单的路径清理
            std::string result = path;

            // 将反斜杠转换为正斜杠（Unix风格）
            std::replace(result.begin(), result.end(), '\\', '/');

            // 移除重复的斜杠
            size_t pos = 0;
            while ((pos = result.find("//", pos)) != std::string::npos)
            {
                result.erase(pos, 1);
            }

            return result;
        }
    }

    bool FileWriter::backup_file(const std::string& filepath)
    {
        try
        {
            if (!file_exists(filepath))
            {
                return true; // 文件不存在，不需要备份
            }

            const std::string backup_path = filepath + ".bak";
            std::filesystem::copy_file(filepath, backup_path,
                                       std::filesystem::copy_options::overwrite_existing);

            fmt::print("Created backup: {}\n", backup_path);
            return true;
        }
        catch (const std::exception& e)
        {
            fmt::print(stderr, "Warning: Failed to create backup for '{}': {}\n", filepath, e.what());
            return false;
        }
    }
} // namespace rplc
