#pragma once

#include <filesystem>

#include "model/model.h"

namespace json_loader {
static const double sec_to_ms = 1000.0;
std::unique_ptr<model::Game> LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
