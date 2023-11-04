#pragma once

#include <filesystem>
#include <string>
#include <optional>

std::optional<std::string> loadFile(std::filesystem::path const& file);