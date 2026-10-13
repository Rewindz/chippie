
#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include <fstream>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <format>

#include "lexer.hpp"
#include "parser.hpp"

int main(int argc, char** argv)
{
    argparse::ArgumentParser program("chipc", "0.0.1");

    spdlog::set_pattern("[%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);

    program.add_argument("-o", "--output")
        .default_value(std::string("a.ch8"))
        .help("sets the output file");

    program.add_argument("inputs")
        .help("input assembly files to compile")
        .nargs(argparse::nargs_pattern::at_least_one);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        spdlog::error("{}\n{}", e.what(), program.help().str());
        return 1;
    }

    auto inputFiles = program.get<std::vector<std::string>>("inputs");
    auto outputFile = program.get<std::string>("-o");

    bool badFile{false};
    for(const auto& file : inputFiles) {
        if(!std::filesystem::exists(file)) {
            spdlog::critical("Input file {} does not exist!", file);
            badFile = true;
        }
    }
    if(badFile) return 1;

    try {
        std::ostringstream ss;

        for (const auto& file : inputFiles) {
            std::ifstream inputSrc{file};
            if(!inputSrc.is_open())
                throw std::runtime_error(std::format("Failed to open source file {}", file));
            ss << inputSrc.rdbuf() << '\n';
        }

        auto tokens = lex(ss.view());
        Parser parser {std::move(tokens)};
        auto rom = parser.parse();

        std::ofstream out{outputFile, std::ios::binary | std::ios::out | std::ios::trunc};
        if(!out.is_open())
            throw std::runtime_error(std::format("Failed to open output file {}", outputFile));

        out.write(reinterpret_cast<const char*>(rom.data()), rom.size());

    } catch (const std::exception& e) {
        spdlog::error("Compiler error: {}", e.what());
        return 1;
    }

    return 0;
}
