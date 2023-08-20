#include "file_request.h"
#include <boost/algorithm/string.hpp>
#include "../app.h"
#include "../http_server.h"

namespace http_handler {
TypeRequest FileRequestHandler::ParseTarget(std::string_view target, std::string& res) const {
    std::string_view api = Endpoint::API;
    res = "";
    std::string uriDecode = http_server::uriDecode(target);
    // request stitic files
    auto pos = target.find(api);
    if (pos == target.npos) {
        res = target;
        return TypeRequest::StaticFiles;
    }
    return TypeRequest::None;
}

fs::path FileRequestHandler::CheckStaticPath(const fs::path& path_static) {
    auto path = fs::weakly_canonical(path_static);
    if (!fs::is_directory(path)) {
        std::string msgError = "Static path "s + path.generic_string() +
            " is not exist";
        throw std::invalid_argument(msgError);
    }
    return path;
}

bool FileRequestHandler::CheckFileExist(std::string& file) const {
    std::string fileName = fs::path(file).filename().string();
    auto path = fs::path(file).parent_path();
    for (const auto& entry : fs::directory_iterator{ path }) {
        std::string fn = entry.path().filename().string();
        if (boost::iequals(fn, fileName)) {
            file = entry.path().string();
            return true;
        }
    }
    return false;
}
bool FileRequestHandler::FileInRootStaticDir(std::string_view file) const {
    auto path = fs::weakly_canonical(file).string();
    if (path.find(static_path_.string()) == 0)
        return true;
    return false;
}

FileRequestResult FileRequestHandler::StaticFilesResponse(
    std::string_view target, bool with_body,
    unsigned http_version, bool keep_alive) const {
    const auto text_response = [&](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, http_version,
            keep_alive, ContentType::TEXT_PLAIN);
    };
    if (target == "/")
        target = "/index.html";
    std::string fullName = static_path_.string() + target.data();
    if (!FileInRootStaticDir(fullName))
        return text_response(http::status::bad_request, "Bad Request");
    if (!CheckFileExist(fullName))
        return text_response(http::status::not_found, "File not found");
    http::file_body::value_type file;
    if (sys::error_code ec; file.open(fullName.c_str(), beast::file_mode::read, ec), ec) {
        return text_response(http::status::gone,
            app::JsonMessage("Gone"sv, "Failed to open file "s + fullName));
    }
    http::response<http::file_body> res;
    res.version(http_version);
    res.result(http::status::ok);
    std::string ext = fs::path(fullName).extension().string();
    res.set(http::field::content_type, ContentType::get(ext).data());

    if (with_body)
        res.body() = std::move(file);
    // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
    // в зависимости от свойств тела сообщения
    res.prepare_payload();
    return std::move(res);
}


FileRequestResult FileRequestHandler::MakeGetResponse(const StringRequest& req, bool with_body) const {
    const auto text_response = [&](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive());
    };
    std::string target;
    switch (ParseTarget(req.target(), target)) {
    case TypeRequest::StaticFiles:
        return StaticFilesResponse(target, with_body, req.version(),
            req.keep_alive());
    default:
        // if bad URI
        return text_response(http::status::bad_request,
            with_body ? app::JsonMessage("badRequest", "Get/Head tagret(in file) not found") : ""s
        );
    }
}

FileRequestResult FileRequestHandler::MakePostResponse(const StringRequest& req) const {
    return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
}
FileRequestResult FileRequestHandler::MakeOptionsResponse(const StringRequest& req) const {
    return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
}
FileRequestResult FileRequestHandler::MakePutResponse(const StringRequest& req) const {
    return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
}
FileRequestResult FileRequestHandler::MakePatchResponse(const StringRequest& req) const {
    return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
}
FileRequestResult FileRequestHandler::MakeDeleteResponse(const StringRequest& req) const {
    return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
}
}
