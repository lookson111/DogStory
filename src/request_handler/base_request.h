#pragma once
#include "../sdk.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <filesystem>
#include <iostream>
#include <variant>

namespace http_handler {
namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http; 
namespace fs = std::filesystem;
namespace sys = boost::system;
using namespace std::literals;
// «апрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// ќтвет, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;
using EmptyResponse = http::response<http::empty_body>;
using FileRequestResult = std::variant<EmptyResponse, StringResponse, FileResponse>;


enum class TypeRequest {
    None,
    StaticFiles,
    Maps,
    Map,
    Join,
    Players,
    State,
    Action,
    BadVersion,
    Tick
};

struct FileExtension {
    // file extension
    constexpr static std::string_view TEXT_HTML_1 = ".htm"sv;
    constexpr static std::string_view TEXT_HTML_2 = ".html"sv;
    constexpr static std::string_view TEXT_CSS    = ".css"sv;
    constexpr static std::string_view TEXT_PLAIN  = ".txt"sv;
    constexpr static std::string_view TEXT_JS     = ".js"sv;
    constexpr static std::string_view APP_JSON    = ".json"sv;
    constexpr static std::string_view APP_XML     = ".xml"sv;
    constexpr static std::string_view IMAGE_PNG   = ".png"sv;
    constexpr static std::string_view IMAGE_JPG_1 = ".jpg"sv;
    constexpr static std::string_view IMAGE_JPG_2 = ".jpe"sv;
    constexpr static std::string_view IMAGE_JPG_3 = ".jpeg"sv;
    constexpr static std::string_view IMAGE_GIF   = ".gif"sv;
    constexpr static std::string_view IMAGE_BMP   = ".bmp"sv;
    constexpr static std::string_view IMAGE_ICO   = ".ico"sv;
    constexpr static std::string_view IMAGE_TIFF_1 = ".tiff"sv;
    constexpr static std::string_view IMAGE_TIFF_2 = ".tif"sv;
    constexpr static std::string_view IMAGE_SVG_1 = ".svg"sv;
    constexpr static std::string_view IMAGE_SVG_2 = ".svgz"sv;
    constexpr static std::string_view AUDIO_MP3   = ".mp3"sv;
    constexpr static std::string_view EMPTY       = ""sv;
};
struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML  = "text/html"sv;       // .htm, .html
    constexpr static std::string_view TEXT_CSS   = "text/css"sv;        // .css
    constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;      // .txt
    constexpr static std::string_view TEXT_JS    = "text/javascript"sv; // .js
    constexpr static std::string_view APP_JSON   = "application/json"sv;// .json
    constexpr static std::string_view APP_XML    = "application/xml"sv; // .xml
    constexpr static std::string_view IMAGE_PNG  = "image/png"sv;       // .png
    constexpr static std::string_view IMAGE_JPG  = "image/jpeg"sv;      // .jpg, .jpe, .jpeg
    constexpr static std::string_view IMAGE_GIF  = "image/gif"sv;       // .gif
    constexpr static std::string_view IMAGE_BMP  = "image/bmp"sv;       // .bmp
    constexpr static std::string_view IMAGE_ICO  = "image/vnd.microsoft.icon"sv; // .ico
    constexpr static std::string_view IMAGE_TIFF = "image/tiff"sv;      // .tiff, .tif
    constexpr static std::string_view IMAGE_SVG  = "image/svg+xml"sv;   // .svg, .svgz
    constexpr static std::string_view AUDIO_MP3  = "audio/mpeg"sv;      // .mp3
    constexpr static std::string_view EMPTY      = "application/octet-stream"sv;

    static std::string_view get(std::string_view key) {
        if (!type.contains(key))
            return EMPTY;
        return type.at(key);
    }
private:
    typedef FileExtension FE;
    static std::unordered_map<std::string_view, std::string_view> type;
};

class BaseRequestHandler {
public:
    virtual ~BaseRequestHandler() {}
    FileRequestResult Handle(const StringRequest& req) const;
    static StringResponse MakeStringResponse(
        http::status status, std::string_view requestTarget, unsigned http_version,
        bool keep_alive, std::string_view content_type = ContentType::APP_JSON,
        bool no_cache = false);

protected:
    virtual FileRequestResult MakeGetResponse(const StringRequest& req, bool with_body) const = 0;
    virtual FileRequestResult MakePostResponse(const StringRequest& req) const = 0;
    virtual FileRequestResult MakeOptionsResponse(const StringRequest& req) const = 0;
    virtual FileRequestResult MakePutResponse(const StringRequest& req) const = 0;
    virtual FileRequestResult MakePatchResponse(const StringRequest& req) const = 0;
    virtual FileRequestResult MakeDeleteResponse(const StringRequest& req) const = 0;
    StringResponse MakeBadResponse(
        http::status status, unsigned http_version,
        bool keep_alive, std::string_view content_type = ContentType::APP_JSON) const;
    StringResponse MakeInvalidMethod(std::string_view allow_methods, unsigned http_version,
        bool keep_alive) const;
    StringResponse MakeInvalidApiVersion(unsigned http_version, bool keep_alive) const;
};
} // namespace http_handler
