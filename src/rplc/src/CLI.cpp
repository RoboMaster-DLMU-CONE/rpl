#include "../include/CLI.hpp"
#include "../include/Config.hpp"
#include "../include/Validator.hpp"
#include "../include/Generator.hpp"
#include "../include/FileWriter.hpp"
#include <CLI/CLI.hpp>
#include <iostream>
#include <fmt/core.h>

namespace rplc
{
    std::optional<CLIOptions> CLI::parse_arguments(int argc, char* argv[])
    {
        ::CLI::App app{"RPLC - RPL Packet Compiler", "rplc"};

        CLIOptions options;

        // 必需参数：配置文件
        app.add_option("config", options.config_file, "JSON configuration file")
          ->check(::CLI::ExistingFile);

        // 可选参数
        app.add_option("-o,--output", options.output_path,
                       "Output file path (default: <packet_name>.hpp in current directory)");

        app.add_flag("-v,--validate", options.validate_only,
                     "Validate configuration only, don't generate code");

        app.add_flag("-b,--backup", options.backup_files,
                     "Create backup of existing files before overwriting");

        app.add_flag("--verbose", options.verbose,
                     "Enable verbose output");

        app.add_flag("-f,--force", options.force_overwrite,
                     "Force overwrite existing files without confirmation");

        app.add_flag("--version", "Print version information");

        // 解析参数
        try
        {
            app.parse(argc, argv);

            // 处理版本信息
            if (app.count("--version"))
            {
                print_version();
                return std::nullopt;
            }

            // 检查配置文件是否提供
            if (options.config_file.empty())
            {
                fmt::print(stderr, "Error: configuration file is required\n");
                print_usage();
                return std::nullopt;
            }

            return options;
        }
        catch (const ::CLI::ParseError& e)
        {
            fmt::print(stderr, "Argument parsing error: {}\n", e.what());
            print_usage();
            return std::nullopt;
        }
    }

    CLIResult CLI::execute(const CLIOptions& options)
    {
        if (options.verbose)
        {
            fmt::print("RPLC - RPL Packet Compiler\n");
            fmt::print("Processing configuration: {}\n\n", options.config_file);
        }

        // 仅验证模式
        if (options.validate_only)
        {
            return validate_config(options.config_file, options.verbose);
        }

        // 生成模式
        return generate_code(options);
    }

    void CLI::print_version()
    {
        fmt::print("RPLC (RPL Packet Compiler) version 0.1.0\n");
    }

    void CLI::print_usage()
    {
        fmt::print("Usage: rplc <config_file> [options]\n\n");
        fmt::print("Arguments:\n");
        fmt::print("  config_file              JSON configuration file\n\n");
        fmt::print("Options:\n");
        fmt::print("  -o, --output FILE        Output file path (default: <packet_name>.hpp)\n");
        fmt::print("  -v, --validate           Validate configuration only\n");
        fmt::print("  -b, --backup             Create backup of existing files\n");
        fmt::print("  -f, --force              Force overwrite without confirmation\n");
        fmt::print("      --verbose            Enable verbose output\n");
        fmt::print("      --version            Print version information\n");
        fmt::print("  -h, --help               Print this help message\n\n");
        fmt::print("Examples:\n");
        fmt::print("  rplc config.json                    Generate header file\n");
        fmt::print("  rplc config.json --validate         Validate configuration only\n");
        fmt::print("  rplc config.json -o MyPacket.hpp    Specify output file path\n");
        fmt::print("  rplc config.json --backup --force   Backup and force overwrite\n");
    }

    CLIResult CLI::validate_config(const std::string& config_file, bool verbose)
    {
        // 加载配置
        auto config = ConfigLoader::load_from_file(config_file);
        if (!config)
        {
            fmt::print(stderr, "Failed to load configuration from '{}'\n", config_file);
            return CLIResult::CONFIG_NOT_FOUND;
        }

        if (verbose)
        {
            fmt::print("Configuration loaded successfully\n");
            fmt::print("Packet name: {}\n", config->packet_name);
            fmt::print("Command ID: {}\n", config->command_id);
            fmt::print("Fields count: {}\n\n", config->fields.size());
        }

        // 验证配置
        auto validation_result = Validator::validate(*config);
        print_validation_result(validation_result, verbose);

        if (validation_result.valid)
        {
            fmt::print("✓ Configuration is valid\n");
            return CLIResult::SUCCESS;
        }
        else
        {
            fmt::print("✗ Configuration validation failed\n");
            return CLIResult::VALIDATION_FAILED;
        }
    }

    CLIResult CLI::generate_code(const CLIOptions& options)
    {
        // 加载和验证配置
        auto config = ConfigLoader::load_from_file(options.config_file);
        if (!config)
        {
            fmt::print(stderr, "Failed to load configuration from '{}'\n", options.config_file);
            return CLIResult::CONFIG_NOT_FOUND;
        }

        auto validation_result = Validator::validate(*config);
        if (!validation_result.valid)
        {
            fmt::print(stderr, "Configuration validation failed:\n");
            print_validation_result(validation_result, true);
            return CLIResult::VALIDATION_FAILED;
        }

        if (options.verbose)
        {
            print_validation_result(validation_result, true);
            fmt::print("✓ Configuration is valid\n\n");
        }

        // 确定输出路径
        std::string output_path;
        if (!options.output_path.empty())
        {
            // 用户指定了输出路径
            output_path = options.output_path;
            if (options.verbose)
            {
                fmt::print("Using user-specified output path: {}\n", output_path);
            }
        }
        else
        {
            // 使用默认路径：当前目录下的<packet_name>.hpp
            output_path = generate_default_output_path(config->packet_name);
            if (options.verbose)
            {
                fmt::print("Using default output path: {}\n", output_path);
            }
        }

        // 检查文件覆盖
        if (FileWriter::file_exists(output_path))
        {
            if (!confirm_overwrite(output_path, options.force_overwrite))
            {
                fmt::print("Operation cancelled by user\n");
                return CLIResult::SUCCESS;
            }

            if (options.backup_files)
            {
                FileWriter::backup_file(output_path);
            }
        }

        // 生成代码
        const auto generator = GeneratorFactory::create_packet_header_generator();
        const auto generated_code = generator->generate(*config);

        if (!generated_code)
        {
            fmt::print(stderr, "Failed to generate code\n");
            return CLIResult::GENERATION_FAILED;
        }

        if (options.verbose)
        {
            fmt::print("Code generation completed\n");
        }

        // 写入文件
        if (!FileWriter::write_file(output_path, *generated_code))
        {
            fmt::print(stderr, "Failed to write output file\n");
            return CLIResult::OUTPUT_FAILED;
        }

        fmt::print("✓ Successfully generated: {}\n", output_path);
        return CLIResult::SUCCESS;
    }

    void CLI::print_validation_result(const ValidationResult& result, bool verbose)
    {
        if (!result.errors.empty())
        {
            fmt::print("Errors:\n");
            for (const auto& error : result.errors)
            {
                fmt::print("  ✗ {}\n", error);
            }
            fmt::print("\n");
        }

        if (verbose && !result.warnings.empty())
        {
            fmt::print("Warnings:\n");
            for (const auto& warning : result.warnings)
            {
                fmt::print("  ⚠ {}\n", warning);
            }
            fmt::print("\n");
        }
    }

    bool CLI::confirm_overwrite(const std::string& filepath, bool force_overwrite)
    {
        if (force_overwrite)
        {
            return true;
        }

        fmt::print("File '{}' already exists. Overwrite? (y/N): ", filepath);
        std::string response;
        std::getline(std::cin, response);

        return response == "y" || response == "Y" || response == "yes" || response == "YES";
    }

    std::string CLI::generate_default_output_path(const std::string& packet_name)
    {
        return packet_name + ".hpp";
    }
} // namespace rplc
