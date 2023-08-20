#include "api_request.h"
#include <boost/regex.hpp>

namespace http_handler
{
using namespace std::literals;
using Token = security::Token;
http::status ApiRequestHandler::ErrorCodeToStatus(app::error_code ec) const {
    http::status stat = http::status::ok;
    switch (ec) {
    case app::error_code::InvalidToken:
        stat = http::status::unauthorized;
        break;
    case app::error_code::UnknownToken:
        stat = http::status::unauthorized;
        break;
    case app::error_code::InvalidArgument:
        stat = http::status::bad_request;
        break;
    case app::error_code::None:
        stat = http::status::ok;
        break;
    }
    return stat;
}

void ApiRequestHandler::LinkJoinWithoutAuthorize() {
    auto ptr = uri_handler_.AddEndpoint(Endpoint::JOIN_GAME);
    if(ptr) {
        ptr->SetNeedAuthorization(false)
            .SetAllowedMethods({http::verb::post}, ErrorMessage::POST_IS_EXPECTED, MiscMessage::ALLOWED_POST_METHOD)
            .SetProcessFunction([&](std::string_view body){
                return ProcessPostEndpoitWithoutAuthorization(body); 
            });
    }
}
void ApiRequestHandler::LinkPlayersToUriHandler() {
    auto ptr = uri_handler_.AddEndpoint(Endpoint::PLAYERS_LIST);
    if(ptr) {
        ptr->SetNeedAuthorization(true)
            .SetAllowedMethods({http::verb::get, http::verb::head}, ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD)
            .SetProcessFunction([&](const Token& token, std::string_view body){
                return ExecuteAuthorizedPost(token, body, [&](const Token& token, std::string_view) {
                    return app_.GetPlayers(token);
            });
        });
    }
}

void ApiRequestHandler::LinkMapsAndMaps() {
    auto ptr = uri_handler_.AddEndpoint(Endpoint::MAPS);
    if (ptr) {
        ptr->SetNeedAuthorization(false)
            .SetNeedTargetProcessing(true)
            .SetAllowedMethods({ http::verb::get, http::verb::head }, ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD)
            .SetProcessFunction([&](std::string_view target, std::string_view body) {
                // finded maps
                std::string_view res;
                auto sz_maps = Endpoint::MAPS.size();
                if (target.size() != sz_maps)
                    res = target.substr(sz_maps + 1, target.size() - sz_maps);
                http::status stat;
                auto [text, flag] = app_.GetMapBodyJson(res);
                stat = flag ? http::status::ok : http::status::not_found;
                return Response::Make(stat, text);
            });
    }
}

void ApiRequestHandler::LinkGameState() {
    auto ptr = uri_handler_.AddEndpoint(Endpoint::GAME_STATE);
    if (ptr) {
        ptr->SetNeedAuthorization(true)
            .SetAllowedMethods({ http::verb::get, http::verb::head }, ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD)
            .SetProcessFunction([&](const Token& token, std::string_view body) {
            return ExecuteAuthorizedPost(token, body, [&](const Token& token, std::string_view) {
                return app_.GetState(token);
            });
        });
    }
}

void ApiRequestHandler::LinkGameActionMove() {
    auto ptr = uri_handler_.AddEndpoint(Endpoint::GAME_ACTION);
    if (ptr) {
        ptr->SetNeedAuthorization(true)
            .SetAllowedMethods({ http::verb::post }, ErrorMessage::POST_IS_EXPECTED, MiscMessage::ALLOWED_POST_METHOD)
            .SetProcessFunction([&](const Token& token, std::string_view body) {
            return ExecuteAuthorizedPost(token, body, [&](const Token& token, std::string_view body) {
                return app_.ActionMove(token, body);
            });
        });
    }
}

void ApiRequestHandler::LinkGameTick() {
    auto ptr = uri_handler_.AddEndpoint(Endpoint::GAME_TICK);
    if (ptr) {
        ptr->SetNeedAuthorization(false)
            .SetAllowedMethods({ http::verb::post }, ErrorMessage::POST_IS_EXPECTED, MiscMessage::ALLOWED_POST_METHOD)
            .SetProcessFunction([&](std::string_view body) {
                auto [text, err] = app_.Tick(body);
                return Response::Make(ErrorCodeToStatus(err), text);
        });
    }
}

void ApiRequestHandler::LinkRetiredPlayers() {
    auto ptr = uri_handler_.AddEndpoint(Endpoint::RECORDS);
    if (ptr) {
        ptr->SetNeedAuthorization(false)
            .SetAllowedMethods({ http::verb::get, http::verb::head }, ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD)
            .SetProcessFunction([&](std::string_view params) {
            std::string str{params.data(), params.size()};
            int start = GetIntUrlParam(str, "start"s, Param::START);
            int max_items = GetIntUrlParam(str, "max[Ii]tems"s, Param::MAX_ITEMS);
            if (start == -1 || max_items == -1 || max_items > Param::MAX_ITEMS)
                return Response::Make(http::status::bad_request, "");
            auto [text, err] = app_.GetRecords(start, max_items);
            return Response::Make(ErrorCodeToStatus(err), text);
        });
    }
}

int ApiRequestHandler::GetIntUrlParam(const std::string& params, 
        const std::string& name, int def_value) const {
    int res = def_value;
    boost::regex expr{"(\\?|&|^|,)"s + name + "=(\\w+)(\\&|$)"s };
    boost::smatch what;
    if (boost::regex_search(params, what, expr)) {
        try {
            res = std::stoi(what[2]);
        } catch (std::exception&) {
            LOGSRV().Msg("error", "error parse uri params");
            return -1;
        }
    }
    return res;

}

StringResponse ApiRequestHandler::ProcessPostEndpoitWithoutAuthorization(
        std::string_view body) {
    auto [text, err] = app_.ResponseJoin(body);
    http::status stat;
    switch (err) {
    case app::JoinError::BadJson:
    case app::JoinError::InvalidName:
        stat = http::status::bad_request;
        break;
    case app::JoinError::MapNotFound:
        stat = http::status::not_found;
        break;
    default:
        stat = http::status::ok;
    }
    return Response::Make(stat, text);
}

}
