#include "CLI.hpp"
#include "CLI/CLI.hpp"
#include "Config.hpp"
#include "FileWriter.hpp"
#include "Generator.hpp"
#include "Validator.hpp"
#include "Version.hpp"
#include <CLI/CLI.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <iostream>
#include <optional>
#include <string_view>

namespace rplc {
std::optional<CLIOptions> CLI::parse_arguments(int argc, char *argv[]) {
  ::CLI::App app{"RPLC - RPL Packet Compiler", "rplc"};
  app.require_subcommand(0, 1); // 允许0或1个子命令

  CLIOptions options;

  // Generate 子命令
  auto *generate_cmd = app.add_subcommand(
      "generate", "Generate packet header file from configuration");
  auto *gen_config_group =
      generate_cmd->add_option_group("Counfiguration Source");

  gen_config_group
      ->add_option("config", options.config_path, "JSON configuration file")
      ->required(false)
      ->check(::CLI::ExistingFile);

  gen_config_group->add_option("--string-input", options.config_string,
                               "JSON configuration string");
  gen_config_group->require_option(1);

  generate_cmd->add_option("-o,--output", options.output_path,
                           "Output file path OR directory (default: "
                           "<config_name>.hpp next to config file)");

  generate_cmd->add_flag("-b,--backup", options.backup_files,
                         "Create backup of existing files before overwriting");
  generate_cmd->add_flag("-f,--force", options.force_overwrite,
                         "Force overwrite existing files without confirmation");

  generate_cmd->add_flag("--verbose", options.verbose, "Enable verbose output");

  generate_cmd->add_flag("--string-output", options.output_string,
                         "Output HPP file as string");

  // Validate 子命令
  auto *validate_cmd =
      app.add_subcommand("validate", "Validate configuration file only");

  auto *val_config_group = validate_cmd->add_option_group("Validation Source");

  val_config_group
      ->add_option("config", options.config_path, "JSON configuration file")
      ->required(false)
      ->check(::CLI::ExistingFile);
  val_config_group->add_option("--string-input", options.config_string,
                               "JSON configuration string");
  val_config_group->require_option(1);

  validate_cmd->add_flag("--verbose", options.verbose, "Enable verbose output");

  // 全局选项
  app.add_flag("--version", "Print version information");

  // 解析参数
  try {
    app.parse(argc, argv);

    // 处理版本信息
    if (app.count("--version")) {
      print_version();
      return std::nullopt;
    }

    // 判断使用哪个子命令
    if (generate_cmd->parsed()) {
      options.subcommand = SubCommand::GENERATE;
    } else if (validate_cmd->parsed()) {
      options.subcommand = SubCommand::VALIDATE;
    } else {
      // 没有子命令，打印帮助信息
      fmt::print("{}\n", app.help());
      return std::nullopt;
    }

    return options;
  } catch (const ::CLI::ParseError &e) {
    fmt::print(stderr, "Argument parsing error: {}\n", e.what());
    print_usage();
    return std::nullopt;
  }
}

CLIResult CLI::execute(const CLIOptions &options) {
  std::string config_source = !options.config_string.empty() ? options.config_string : options.config_path;
  if (options.verbose) {
    fmt::print("RPLC - RPL Packet Compiler\n");
    fmt::print("Processing configuration: {}\n\n", config_source);
  }

  // 仅验证模式
  if (options.subcommand == SubCommand::VALIDATE) {
    return validate_config(options);
  }

  // 生成模式
  return generate_code(options);
}

void CLI::print_version() {
  fmt::print("RPLC (RPL Packet Compiler) version {}\n", VERSION);
}

void CLI::print_usage() {
  fmt::print("Usage: rplc <subcommand> [options]\n\n");
  fmt::print("Subcommands:\n");
  fmt::print("  generate [config]        Generate packet header file from configuration\n");
  fmt::print("  validate [config]        Validate configuration file only\n\n");
  fmt::print("Configuration Options:\n");
  fmt::print("  config                   JSON configuration file path\n");
  fmt::print("  --string-input           JSON configuration as string\n\n");
  fmt::print("Generate Options:\n");
  fmt::print("  -o, --output PATH        Output file path OR directory "
             "(default: <config_name>.hpp next to config file)\n");
  fmt::print("  -b, --backup             Create backup of existing files\n");
  fmt::print(
      "  -f, --force              Force overwrite without confirmation\n");
  fmt::print("      --verbose            Enable verbose output\n");
  fmt::print("      --string-output      Output generated code to stdout\n\n");
  fmt::print("Validate Options:\n");
  fmt::print("      --verbose            Enable verbose output\n\n");
  fmt::print("Global Options:\n");
  fmt::print("      --version            Print version information\n");
  fmt::print("  -h, --help               Print this help message\n\n");
  fmt::print("Examples:\n");
  fmt::print("  rplc generate TestPacket.json                        Generate "
             "./TestPacket.hpp next to json\n");
  fmt::print("  rplc generate TestPacket.json -o ./include/          Put into "
             "directory ./include as ./include/TestPacket.hpp\n");
  fmt::print("  rplc generate TestPacket.json -o ./include/Test.hpp  Generate "
             "as ./include/Test.hpp\n");
  fmt::print("  rplc generate --string-input '{{\"packet_name\":\"Test\",\"command_id\":\"0x0101\",\"fields\":[{{\"name\":\"value\",\"type\":\"int\"}}]}}' --string-output\n");
  fmt::print("  rplc validate TestPacket.json --verbose              Verbose "
             "validation output\n");
  fmt::print("  rplc validate --string-input '{{\"packet_name\":\"Test\",\"command_id\":\"0x0101\",\"fields\":[{{\"name\":\"value\",\"type\":\"int\"}}]}}'\n");
}

CLIResult CLI::validate_config(const CLIOptions &options) {
  // 加载配置
  std::optional<PacketConfig> config{};
  std::string_view sw;
  if (!options.config_string.empty()) {
    config = ConfigLoader::load_from_string(options.config_string);
    sw = options.config_string;
  } else {
    config = ConfigLoader::load_from_file(options.config_path);
    sw = options.config_path;
  }
  if (!config) {
    fmt::print(stderr, "Failed to load configuration from '{}'\n", sw);
    return CLIResult::CONFIG_NOT_FOUND;
  }

  if (options.verbose) {
    fmt::print("Configuration loaded successfully\n");
    fmt::print("Packet name: {}\n", config->packet_name);
    fmt::print("Command ID: {}\n", config->command_id);
    fmt::print("Fields count: {}\n\n", config->fields.size());
  }

  // 验证配置
  auto validation_result = Validator::validate(*config);
  print_validation_result(validation_result, options.verbose);

  if (validation_result.valid) {
    fmt::print("✓ Configuration is valid\n");
    return CLIResult::SUCCESS;
  } else {
    fmt::print("✗ Configuration validation failed\n");
    return CLIResult::VALIDATION_FAILED;
  }
}

CLIResult CLI::generate_code(const CLIOptions &options) {
  // 加载和验证配置
  std::optional<PacketConfig> config{};
  if (!options.config_string.empty()) {
    config = ConfigLoader::load_from_string(options.config_string);
  } else {
    config = ConfigLoader::load_from_file(options.config_path);
  }
  std::string_view config_source = !options.config_string.empty() ? options.config_string : options.config_path;
  if (!config) {
    fmt::print(stderr, "Failed to load configuration from '{}'\n",
               config_source);
    return CLIResult::CONFIG_NOT_FOUND;
  }

  auto validation_result = Validator::validate(*config);
  if (!validation_result.valid) {
    fmt::print(stderr, "Configuration validation failed:\n");
    print_validation_result(validation_result, true);
    return CLIResult::VALIDATION_FAILED;
  }

  if (options.verbose) {
    print_validation_result(validation_result, true);
    fmt::print("✓ Configuration is valid\n\n");
  }

  if (options.output_string) {
    // 生成代码
    const auto generator = GeneratorFactory::create_packet_header_generator();
    const auto generated_code = generator->generate(*config);

    if (!generated_code) {
      fmt::print(stderr, "Failed to generate code\n");
      return CLIResult::GENERATION_FAILED;
    }

    fmt::print(stdout, "{}", *generated_code);
    return CLIResult::SUCCESS;
  }

  // 计算默认文件名：以配置文件名为基准（仅在文件输入时）
  std::string default_filename = "generated_packet.hpp";  // 默认名称
  if (!options.config_path.empty()) {
    const std::filesystem::path cfg_path(options.config_path);
    default_filename = (cfg_path.stem().string() + ".hpp");
  }

  // 确定输出路径
  std::filesystem::path output_path;
  if (!options.output_path.empty()) {
    std::filesystem::path out_hint(options.output_path);

    // 判断是否应作为目录对待：存在且为目录，或用户以/或\结尾
    bool treat_as_dir = false;
    try {
      treat_as_dir = std::filesystem::exists(out_hint) &&
                     std::filesystem::is_directory(out_hint);
    } catch (...) {
      treat_as_dir = false;
    }
    if (!treat_as_dir) {
      const char last = options.output_path.back();
      if (last == '/' || last == '\\')
        treat_as_dir = true;
    }

    if (treat_as_dir) {
      output_path = out_hint / default_filename;
      if (options.verbose) {
        fmt::print("Using output directory hint: {} => {}\n", out_hint.string(),
                   output_path.string());
      }
    } else {
      output_path = out_hint; // 直接使用用户指定的文件路径
      if (options.verbose) {
        fmt::print("Using explicit output file path: {}\n",
                   output_path.string());
      }
    }
  } else {
    // 默认：与配置文件同目录，同名（.hpp扩展）
    std::filesystem::path cfg_path(options.config_path);
    std::filesystem::path out_dir = cfg_path.parent_path();
    if (out_dir.empty()) {
      out_dir = std::filesystem::current_path();
    }
    output_path = out_dir / default_filename;
    if (options.verbose) {
      fmt::print("Using default output path: {}\n", output_path.string());
    }
  }

  // 检查文件覆盖
  if (FileWriter::file_exists(output_path.string())) {
    if (!confirm_overwrite(output_path.string(), options.force_overwrite)) {
      fmt::print("Operation cancelled by user\n");
      return CLIResult::SUCCESS;
    }

    if (options.backup_files) {
      FileWriter::backup_file(output_path.string());
    }
  }

  // 生成代码
  const auto generator = GeneratorFactory::create_packet_header_generator();
  const auto generated_code = generator->generate(*config);

  if (!generated_code) {
    fmt::print(stderr, "Failed to generate code\n");
    return CLIResult::GENERATION_FAILED;
  }

  if (options.verbose) {
    fmt::print("Code generation completed\n");
  }

  // 写入文件（需要创建目录）
  if (!FileWriter::write_file(output_path.string(), *generated_code,
                              /*create_directories=*/true)) {
    fmt::print(stderr, "Failed to write output file\n");
    return CLIResult::OUTPUT_FAILED;
  }

  fmt::print("✓ Successfully generated: {}\n", output_path.string());
  return CLIResult::SUCCESS;
}

void CLI::print_validation_result(const ValidationResult &result,
                                  bool verbose) {
  if (!result.errors.empty()) {
    fmt::print("Errors:\n");
    for (const auto &error : result.errors) {
      fmt::print("  ✗ {}\n", error);
    }
    fmt::print("\n");
  }

  if (verbose && !result.warnings.empty()) {
    fmt::print("Warnings:\n");
    for (const auto &warning : result.warnings) {
      fmt::print("  ⚠ {}\n", warning);
    }
    fmt::print("\n");
  }
}

bool CLI::confirm_overwrite(const std::string &filepath, bool force_overwrite) {
  if (force_overwrite) {
    return true;
  }

  fmt::print("File '{}' already exists. Overwrite? (y/N): ", filepath);
  std::string response;
  std::getline(std::cin, response);

  return response == "y" || response == "Y" || response == "yes" ||
         response == "YES";
}

std::string CLI::generate_default_output_path(const std::string &packet_name) {
  return packet_name + ".hpp";
}
} // namespace rplc
