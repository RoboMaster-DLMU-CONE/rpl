#pragma once

#include <string>
#include <vector>
#include <optional>

namespace rplc {

/**
 * CLI选项配置
 */
struct CLIOptions {
    std::string config_file;         // 配置文件路径
    std::string output_dir;          // 输出目录（可选，覆盖配置文件中的路径）
    bool validate_only = false;      // 仅验证配置，不生成代码
    bool backup_files = false;       // 是否备份现有文件
    bool verbose = false;            // 详细输出
    bool force_overwrite = false;    // 强制覆盖现有文件
};

/**
 * CLI操作结果
 */
enum class CLIResult {
    SUCCESS = 0,
    INVALID_ARGUMENTS = 1,
    CONFIG_NOT_FOUND = 2,
    VALIDATION_FAILED = 3,
    GENERATION_FAILED = 4,
    OUTPUT_FAILED = 5
};

/**
 * 命令行界面处理器
 */
class CLI {
public:
    /**
     * 解析命令行参数
     * @param argc 参数个数
     * @param argv 参数数组
     * @return 解析成功返回选项，失败返回nullopt
     */
    static std::optional<CLIOptions> parse_arguments(int argc, char* argv[]);

    /**
     * 执行主要操作
     * @param options CLI选项
     * @return 操作结果
     */
    static CLIResult execute(const CLIOptions& options);

    /**
     * 打印版本信息
     */
    static void print_version();

    /**
     * 打印使用说明
     */
    static void print_usage();

private:
    /**
     * 执行验证操作
     * @param config_file 配置文件路径
     * @param verbose 是否详细输出
     * @return 验证结果
     */
    static CLIResult validate_config(const std::string& config_file, bool verbose);

    /**
     * 执行生成操作
     * @param options CLI选项
     * @return 生成结果
     */
    static CLIResult generate_code(const CLIOptions& options);

    /**
     * 打印验证结果
     * @param result 验证结果
     * @param verbose 是否详细输出
     */
    static void print_validation_result(const class ValidationResult& result, bool verbose);

    /**
     * 检查文件覆盖确认
     * @param filepath 文件路径
     * @param force_overwrite 是否强制覆盖
     * @return 可以覆盖返回true
     */
    static bool confirm_overwrite(const std::string& filepath, bool force_overwrite);

    /**
     * 调整输出路径
     * @param original_path 原始路径
     * @param output_dir 输出目录
     * @return 调整后的路径
     */
    static std::string adjust_output_path(const std::string& original_path, 
                                         const std::string& output_dir);
};

} // namespace rplc
