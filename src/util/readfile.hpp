#pragma once

#include <fstream>
#include <string>
#include <vector>

static auto readFile(const std::string& filename) -> std::vector<char> {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(std::string("Failed to open file " + filename));
    }

    const auto fileSize = static_cast<std::streamsize>(file.tellg());
    std::vector<char> buffer(static_cast<size_t>(fileSize));

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}