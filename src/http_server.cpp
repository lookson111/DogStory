#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

std::string uriDecode(std::string_view src) {
    std::string out;
    for (int pos = 0; pos < src.length(); pos++) {
        if (src[pos] == '%') {
            int from_hex_char;
            [[maybe_unused]] auto s = sscanf(
                src.substr(pos + 1, 2).data(), "%x", &from_hex_char);
            out += static_cast<char>(from_hex_char);
            pos += 2;
        }
        else if (src[pos] == '+') {
            out += ' ';
            pos += 1;
        }
        else if (src[pos] >= 'A' && src[pos] <= 'Z') {
            out += src[pos] - 'A' + 'a';
        }
        else {
            out += src[pos];
        }
    }
    return out;
}

void SessionBase::Run() {
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
    using namespace std::literals;
    // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    request_ = {};
    stream_.expires_after(30s);
    // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    http::async_read(stream_, buffer_, request_,
        // По окончании операции будет вызван метод OnRead
        beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    using namespace std::literals;
    if (ec == http::error::end_of_stream) {
        // Нормальная ситуация - клиент закрыл соединение
        return Close();
    }
    if (ec) {
        LOGSRV().Error(ec, server_logging::Server::Where::read);
        return;
    }
    std::string uri = uriDecode(request_.target());
    auto rmeth = http::to_string(request_.method());
    LOGSRV().Request(stream_.socket().remote_endpoint().address().to_string(),
        uri, std::string_view(rmeth.data(), rmeth.size()));
    start_time_ = steady_clock::now();
    HandleRequest(std::move(request_));
}

void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
    if (ec) {
        LOGSRV().Error(ec, server_logging::Server::Where::write);
        return;
    }
    if (close) {
        // Семантика ответа требует закрыть соединение
        return Close();
    }
    // Считываем следующий запрос
    Read();
}

void SessionBase::Close() {
    stream_.socket().shutdown(tcp::socket::shutdown_send);
}

}  // namespace http_server
