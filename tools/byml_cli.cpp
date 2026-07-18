#include <oead/byml.h>

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::vector<oead::u8> ReadBinary(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if (!stream)
    throw std::runtime_error("cannot open input file: " + path.string());
  const auto size = stream.tellg();
  if (size < 0)
    throw std::runtime_error("cannot determine input size: " + path.string());
  std::vector<oead::u8> data(static_cast<std::size_t>(size));
  stream.seekg(0);
  if (!data.empty() && !stream.read(reinterpret_cast<char*>(data.data()), size))
    throw std::runtime_error("cannot read input file: " + path.string());
  return data;
}

std::string ReadText(const std::filesystem::path& path) {
  const auto bytes = ReadBinary(path);
  return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

void WriteBinary(const std::filesystem::path& path, const std::vector<oead::u8>& data) {
  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  if (!stream || (!data.empty() && !stream.write(reinterpret_cast<const char*>(data.data()),
                                                  static_cast<std::streamsize>(data.size()))))
    throw std::runtime_error("cannot write output file: " + path.string());
}

void WriteText(const std::filesystem::path& path, std::string_view text) {
  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  if (!stream || !stream.write(text.data(), static_cast<std::streamsize>(text.size())))
    throw std::runtime_error("cannot write output file: " + path.string());
}

oead::u16 ParseVersion(std::string_view text) {
  unsigned int value = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, 10);
  if (error != std::errc{} || end != text.data() + text.size() || value > UINT16_MAX)
    throw std::invalid_argument("version must be an integer from 0 to 65535");
  return static_cast<oead::u16>(value);
}

void PrintUsage() {
  std::cerr << "Usage:\n"
            << "  oead_byml byml-to-yaml <input.byml> <output.yaml>\n"
            << "  oead_byml yaml-to-byml <input.yaml> <output.byml> [--big-endian] "
               "[--version <0..65535>]\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 4) {
      PrintUsage();
      return 2;
    }
    const std::string_view operation = argv[1];
    const std::filesystem::path input = argv[2];
    const std::filesystem::path output = argv[3];

    if (operation == "byml-to-yaml") {
      if (argc != 4) {
        PrintUsage();
        return 2;
      }
      const auto data = ReadBinary(input);
      const auto document = oead::Byml::FromBinaryUncheckedVersion(data);
      WriteText(output, document.ToText());
      return 0;
    }

    if (operation == "yaml-to-byml") {
      bool big_endian = false;
      oead::u16 version = 2;
      for (int i = 4; i < argc; ++i) {
        const std::string_view option = argv[i];
        if (option == "--big-endian") {
          big_endian = true;
        } else if (option == "--version" && i + 1 < argc) {
          version = ParseVersion(argv[++i]);
        } else {
          throw std::invalid_argument("unknown or incomplete option: " + std::string(option));
        }
      }
      const auto document = oead::Byml::FromText(ReadText(input));
      WriteBinary(output, document.ToBinaryUncheckedVersion(big_endian, version));
      return 0;
    }

    PrintUsage();
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "oead_byml: " << error.what() << '\n';
    return 1;
  }
}
