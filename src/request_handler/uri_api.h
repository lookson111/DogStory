#pragma once

#include <string_view>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

#include "defs.h"
#include "response.h"
#include "../util/token.h"
#include "../http_server.h"

namespace uri_api
{
namespace beast = boost::beast;
namespace http = beast::http;
using namespace defs;
using FunctionWithAuthorize = std::function<http_handler::StringResponse(const security::Token& token, std::string_view body)>;
using FunctionWithoutAuthorize = std::function<http_handler::StringResponse(std::string_view body)>;
using FunctionTargetProcessing = std::function<http_handler::StringResponse(std::string_view target, std::string_view body)>;

template <typename Fn, typename Body, typename Allocator>
http_handler::StringResponse ExecuteAuthorized(const http::request<Body, http::basic_fields<Allocator>>& req, Fn&& action) { 
    if (auto token = security::ExtractTokenFromStringViewAndCheckIt(req.base()[http::field::authorization])) {
        return action(*token, req.body());
    } else {
        return http_handler::Response::MakeUnauthorizedErrorInvalidToken();
    }
}

class UriElement {
    struct AllowedMethods {
        std::vector<http::verb> data_;
        std::string_view error_;
        std::string_view allowed_;

        AllowedMethods()
        : data_()
        , error_()
        , allowed_()
        {};
    };

    struct AuthorizeData {
        bool need_;
        AuthorizeData()
        : need_(false)
        {};
    };

    struct TargetProcessing {
        bool need_;
        TargetProcessing()
            : need_(false)
        {};
    };

    struct ContentType {
        bool need_to_check_;
        std::string_view value_;
        std::string_view error_;

        ContentType()
        : need_to_check_(false)
        {};
    };
public:
    UriElement()
    : methods_()
    , authorize_()
    , content_type_()
    , target_processing_()
    {};
    UriElement& SetAllowedMethods(std::vector<http::verb> methods, std::string_view method_error_message, std::string_view allowed_methods) {
        methods_.data_ = std::move(methods);
        methods_.error_ = method_error_message;
        methods_.allowed_ = allowed_methods;

        return *this;
    }

    UriElement& SetNeedAuthorization(bool need_authorize_) {
        authorize_.need_ = need_authorize_;
        return *this;
    }

    UriElement& SetNeedTargetProcessing(bool need_target_processing_) {
        target_processing_.need_ = need_target_processing_;
        return *this;
    }

    UriElement& SetProcessFunction(FunctionWithAuthorize f) {
        process_function_ = std::move(f);
        return *this;
    }

    UriElement& SetProcessFunction(FunctionWithoutAuthorize f) {
        process_function_without_authorize_ = std::move(f);
        return *this;
    }

    UriElement& SetProcessFunction(FunctionTargetProcessing f) {
        process_function_target_processing_ = std::move(f);
        return *this;
    }

    UriElement& SetContentType(std::string_view type, std::string_view error_message) {
        content_type_.need_to_check_ = true;
        content_type_.value_ = type;
        content_type_.error_ = error_message;
        return *this;
    }

    template <typename Body, typename Allocator>
    http_handler::StringResponse ProcessRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
        if( methods_.data_.empty() || std::find(methods_.data_.begin(), 
            methods_.data_.end(), req.method()) != methods_.data_.end()) {
            //Check content type if necessary
            if(content_type_.need_to_check_ && content_type_.value_ 
                != req.base()[boost::beast::http::field::content_type]) {
                return http_handler::Response::MakeBadRequestInvalidArgument(content_type_.error_);
            }
            //Check authorization if necessary and call necessary processing function
            if(authorize_.need_) {
                return ExecuteAuthorized(req, [&](const security::Token& token, std::string_view body){
                    return process_function_(token, body);
                }); 
            }
            std::string target = http_server::uriDecode(req.target());
            if (target_processing_.need_)
                return process_function_target_processing_(target, req.body());
            auto stop = target.find('?');
            if ( stop != std::string::npos) {
                return process_function_without_authorize_(target.substr(stop + 1));
            }
            return process_function_without_authorize_(req.body());
        }            
        return http_handler::Response::MakeMethodNotAllowed(methods_.error_, methods_.allowed_);
    }
private:
    AllowedMethods methods_;
    AuthorizeData authorize_;
    ContentType content_type_;
    TargetProcessing target_processing_;
    FunctionWithAuthorize process_function_;
    FunctionWithoutAuthorize process_function_without_authorize_;
    FunctionTargetProcessing process_function_target_processing_;
};

class UriData {
public:
    UriData() = default;
    UriElement* AddEndpoint(std::string_view uri) {
        auto [it, ret] = data_.try_emplace(std::string(uri));
        return &it->second;
    }
    template <typename Body, typename Allocator>
    http_handler::StringResponse Process(const http::request<Body, http::basic_fields<Allocator>>& req) {
        std::string target = http_server::uriDecode(req.target());
        auto stop = target.find('?');
        target = std::string(target.substr(0, stop));
        if (target.find(Endpoint::MAPS) == 0)
            target = std::string{ Endpoint::MAPS };
        auto it = data_.find(target);
        if(it != data_.end()) {
            return it->second.ProcessRequest(req);
        }
        return http_handler::Response::MakeJSON(http::status::bad_request, ErrorCode::BAD_REQUEST, ErrorMessage::INVALID_ENDPOINT);
    }
private:
    std::unordered_map<std::string, UriElement> data_;
};
}
