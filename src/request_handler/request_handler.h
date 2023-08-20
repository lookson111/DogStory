#pragma once
#include "../sdk.h"
#include <iostream>
#include <string_view>
#include <string>
#include <boost/json.hpp>
#include <functional>
#include <map>
#include <variant>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/strand.hpp>

#include "defs.h"
#include "../http_server.h"
#include "response.h"
#include "api_request.h"
#include "file_request.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace defs;

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
    using Strand = net::strand<net::io_context::executor_type>;
public:
    RequestHandler(const fs::path& static_path, Strand api_strand, app::App &app, bool on_tick_api)
        : file_handler{ static_path }
        , api_strand_(api_strand)
        , api_handler_(api_strand, app, on_tick_api) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(tcp::endpoint, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send)
	{
        auto version = req.version();
        auto keep_alive = req.keep_alive();
        std::string target{StringRequest(req).target().data(), StringRequest(req).target().size()};
        target = http_server::uriDecode(target);
        auto api = Endpoint::API;
        bool is_target = target.size() > api.size() && (target.substr(0, api.size()) == api);
        try {
            /*req относится к API*/
            if (is_target) {
                auto handle = [self = shared_from_this(), send,
                    req = std::forward<decltype(req)>(req), version, keep_alive] {
                    try {
                        return send(std::move(self->api_handler_.Handle(req)));
                    }
                    catch (...) {
                        send(self->ReportServerError(version, keep_alive));
                    }
                };
                return net::dispatch(api_strand_, handle);
            }
            // Возвращаем результат обработки запроса к файлу
            return std::visit(
                [&send](auto&& result) {
                    send(std::forward<decltype(result)>(result));
                },
                file_handler.Handle(req)
            );
        }
        catch (...) {
            send(ReportServerError(version, keep_alive));
        }
    }
private:
    FileRequestHandler file_handler;
    Strand api_strand_;
	ApiRequestHandler api_handler_;
    StringResponse ReportServerError(unsigned version, bool keep_alive) const;
};

}  // namespace http_handler
