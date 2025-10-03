#include "include/CLI.hpp"
#include <cstdlib>

int main(int argc, char* argv[])
{
    const auto options = rplc::CLI::parse_arguments(argc, argv);
    if (!options)
    {
        // 解析失败或显示版本信息
        return static_cast<int>(rplc::CLIResult::INVALID_ARGUMENTS);
    }

    auto result = rplc::CLI::execute(*options);
    return static_cast<int>(result);
}
