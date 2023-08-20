#pragma once

#include <chrono>
#include "app_serialization.h"

namespace infrastructure {
using milliseconds = std::chrono::milliseconds;
using namespace std::literals;

class SerializingListiner {
public:
    SerializingListiner(std::shared_ptr<app::App> &app, const std::string& state_file, 
        milliseconds save_period);
    void OnTick(milliseconds time_delta_ms);
    void Save();
    void Load();

private:
    std::shared_ptr<app::App> &app_;
    const std::string state_file_;
    milliseconds save_period_;
    milliseconds time_since_save_;
};

} // namespace infrastructure
