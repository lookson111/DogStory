#pragma once 
#include <ostream>
#include <iostream>
#include <boost/json.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>

#define LOG() server_logging::Log::GetInstance()
#define LOGSRV() server_logging::Server::GetInstance()

namespace server_logging {

namespace sys = boost::system;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;
    
void InitBoostLogFilter();

// Для синхронного вывода в Boost.Asio могут использовать любой тип, удовлетворяющий требованиям,
// описанным в документе:
// https://www.boost.org/doc/libs/1_78_0/doc/html/boost_asio/reference/SyncWriteStream.html
class Log {
    Log() = default;
    Log(const Log&) = delete;
public:
    static Log& GetInstance() {
        static Log obj;
        return obj;
    }
    static void Info(std::string_view data, std::string_view message);

    template <typename ConstBufferSequence>
    size_t WriteSome(const ConstBufferSequence& cbs, sys::error_code& ec) {
        const size_t total_size = net::buffer_size(cbs);
        if (total_size == 0) {
            ec = {};
            return 0;
        }
        size_t bytes_written = 0;
        for (const auto& cb : cbs) {
            const size_t size = cb.size();
            const char* const data = reinterpret_cast<const char*>(cb.data());
            if (size > 0) {
                if (!os_.write(reinterpret_cast<const char*>(data), size)) {
                    ec = make_error_code(boost::system::errc::io_error);
                    return bytes_written;
                }
                bytes_written += size;
            }
        }
        ec = {};
        return bytes_written;
    }

    template <typename ConstBufferSequence>
    size_t WriteSome(const ConstBufferSequence& cbs) {
        sys::error_code ec;
        const size_t bytes_written = write_some(cbs, ec);
        if (ec) {
            throw std::runtime_error("Failed to write");
        }
        return bytes_written;
    }

private:
    std::ostream& os_ = std::cout;
};

class Server {
private:
    Log& log_; 
    Server() : log_(Log::GetInstance()) {}
public:
    enum class Where {
        read,
        write,
        accept
    };
    static Server& GetInstance() {
        static Server obj;
        return obj;
    }
    void Start(std::string_view address, int port);
    void End(const sys::error_code& ec);
    void Error(const sys::error_code& ec, Where where);
    void Request(std::string_view address, std::string_view uri, std::string_view method);
    void Msg(std::string_view address, std::string_view uri);
    void Response(long long response_time, unsigned status_code, std::string_view content_type);
};

template<class SomeRequestHandler>
class LoggingRequestHandler {
public:
    LoggingRequestHandler(SomeRequestHandler&& requestHandler) : 
        decorated_(std::forward<SomeRequestHandler>(requestHandler)) {
    }
    template <typename Body, typename Allocator, typename Send>
     void operator () (tcp::endpoint ep, 
            http::request<Body, http::basic_fields<Allocator>>&& req, 
            Send&& send) {
        decorated_(ep, std::forward<decltype(req)>(req),
        std::forward<decltype(send)>(send));
    }

private:
    SomeRequestHandler decorated_;
};

} // namespace logger
