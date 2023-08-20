#include "infrastructure.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <fstream>
#include <filesystem>
#include "../log.h"

namespace infrastructure {
using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

SerializingListiner::SerializingListiner(std::shared_ptr<app::App> &app,
        const std::string& state_file, milliseconds save_period) 
    : app_(app)
    , state_file_(state_file)
    , save_period_(save_period) 
{
    time_since_save_ = 0ms;
}
void SerializingListiner::OnTick(milliseconds time_delta_ms) {
    using namespace std::chrono_literals;    
    if (save_period_ == 0ms)
        return;
    time_since_save_ += time_delta_ms;
    if (time_since_save_ >= save_period_) {
        LOGSRV().Msg("Tick", "serializing "s + std::to_string(time_since_save_.count()) +
            " out "s + std::to_string(save_period_.count()));
        Save();
        time_since_save_ = 0ms;
    }
}

void SerializingListiner::Load() {
    if (!std::filesystem::exists(state_file_))
        return;
    try {
        std::ifstream archive_{state_file_};
        InputArchive input_archive{ archive_ };
        serialization::AppRepr repr;
        input_archive >> repr;
        repr.Restore(*app_);
    }
    catch (const std::exception& ex) {
        LOGSRV().Msg("Error load save", ex.what());
        throw std::logic_error("game save corrupted");
    }
    catch (...) {
        LOGSRV().Msg("Error load serialize", "");
        std::cout << "error" << std::endl;
        throw;
    }
}

void SerializingListiner::Save() {
    if (state_file_.empty())
        return;
    try {
        std::ofstream archive_{state_file_};
        OutputArchive output_archive{archive_};
        output_archive << serialization::AppRepr{*app_};
    }
    catch (const std::exception& ex) {
        LOGSRV().Msg("Error save serialize", ex.what());
        throw;
    }
    catch (...) {
        LOGSRV().Msg("Error save serialize", "");
        std::cout << "error" << std::endl;
        throw;
    }
}

} // namespace infrastructure
