#include <oead/byml.h>

#include <cstdint>
#include <exception>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {

std::vector<std::uint8_t> ReadStdin() {
  std::cin.sync_with_stdio(false);
  return {std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>()};
}

void WriteStdout(const std::vector<std::uint8_t>& data) {
  if (!data.empty())
    std::cout.write(reinterpret_cast<const char*>(data.data()),
                    static_cast<std::streamsize>(data.size()));
}

void WriteStdout(std::string_view data) {
  if (!data.empty())
    std::cout.write(data.data(), static_cast<std::streamsize>(data.size()));
}

}  // namespace

int main(int argc, char** argv) {
  try {
#ifdef _WIN32
    if (_setmode(_fileno(stdin), _O_BINARY) == -1 ||
        _setmode(_fileno(stdout), _O_BINARY) == -1)
      throw std::runtime_error("failed to configure binary standard streams");
#endif

    if (argc != 2)
      throw std::invalid_argument(
          "expected one command: byml_binary_to_text or byml_text_to_binary");

    const std::string_view command = argv[1];
    const auto input = ReadStdin();

    if (command == "byml_binary_to_text" || command == "byml-to-yaml") {
      const auto document = oead::Byml::FromBinaryUncheckedVersion(input);
      WriteStdout(document.ToText());
      return 0;
    }

    if (command == "byml_text_to_binary" || command == "yaml-to-byml") {
      const std::string yaml(input.begin(), input.end());
      const auto document = oead::Byml::FromText(yaml);
      WriteStdout(document.ToBinaryUncheckedVersion(false, 2));
      return 0;
    }

    throw std::invalid_argument("unknown command: " + std::string(command));
  } catch (const std::exception& error) {
    std::cout << "ERROR: " << error.what();
    return 1;
  } catch (...) {
    std::cout << "ERROR: unknown exception";
    return 1;
  }
}
