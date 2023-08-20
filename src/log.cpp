#include "log.h"
#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр 
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/date_time.hpp>
#include <string_view>

namespace server_logging {
using namespace std::literals;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(file, "File", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(line, "Line", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(data, "Data", std::string_view)
BOOST_LOG_ATTRIBUTE_KEYWORD(msg, "Msg", std::string_view)

static void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    // Момент времени приходится вручную конвертировать в строку.
    // Для получени¤ истинного значения атрибута нужно добавить
    // разыменование. 
    auto ts = *rec[timestamp];
    strm << "{\"timestamp\":\"" << to_iso_extended_string(ts) << "\",";
    strm << "\"data\":" << rec[data] << ",";
    strm << "\"message\":\"" << rec[msg] << "\"}" << std::endl;
}

void InitBoostLogFilter() {
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info
    );
    logging::add_common_attributes();
    /*logging::add_file_log(
        keywords::file_name = "/var/log/sample_%N.log",
        keywords::format = &JsonFormatter,
        keywords::open_mode = std::ios_base::app | std::ios_base::out,
        // ротируем по достижению размера 10 мегабайт
        keywords::rotation_size = 10 * 1024 * 1024,
        // ротируем ежедневно в полдень
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(12, 0, 0)
    );*/
    logging::add_console_log(
        std::cout,
        keywords::format = &JsonFormatter,
        keywords::auto_flush = true
    );
}

void Log::Info(std::string_view data_, std::string_view message_) {
    BOOST_LOG_TRIVIAL(info)
        << logging::add_value(data, data_)
        << logging::add_value(msg, message_);
}

void Server::Start(std::string_view address, int port) {
    json::object mapEl;
    mapEl["port"] = port;
    mapEl["address"] = address.data();
    log_.Info(serialize(mapEl), "server started"sv);
}

void Server::End(const boost::system::error_code& err) {
    json::object mapEl;
    mapEl["code"] = err.value();
    
    if (err)
        mapEl["exception"] = err.what();
    log_.Info(serialize(mapEl), "server exited"sv);
}

void Server::Error(const sys::error_code& ec, Where where)
{
    boost::string_view svWhere;
    switch (where) {
    case Where::read:
        svWhere = "read";
        break;
    case Where::write:
        svWhere = "write";
        break;
    case Where::accept:
        svWhere = "accept";
        break;
    }
    json::object mapEl;
    mapEl["code"] = ec.value();
    mapEl["text"] = ec.message();
    mapEl["where"] = svWhere;
    log_.Info(serialize(mapEl), "error"sv);
}

static auto to_booststr = [](std::string_view str) {
    return boost::string_view(str.data(), str.size());
};

void Server::Request(std::string_view address, std::string_view uri, std::string_view method)
{
    json::object mapEl;
    mapEl["address"] = to_booststr(address);
    mapEl["URI"] = to_booststr(uri);
    mapEl["method"] = to_booststr(method);
    log_.Info(serialize(mapEl), "request received"sv);
}

void Server::Response(long long response_time, unsigned status_code, std::string_view content_type)
{
    json::object mapEl;
    mapEl["response_time"] = response_time;
    mapEl["code"] = status_code;
    if (content_type.empty())
        mapEl["content_type"] = "null";
    else {
        auto ct = content_type.substr(0, content_type.find("\\r"));
        mapEl["content_type"] = to_booststr(ct);
    }
    log_.Info(serialize(mapEl), "response sent"sv);
}
void Server::Msg(std::string_view header, std::string_view message) {
    json::object mapEl;
    mapEl["header"] = to_booststr(header);
    mapEl["message"] = to_booststr(message);
    log_.Info(serialize(mapEl), "response sent"sv);
}

}
