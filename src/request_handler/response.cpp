#include "response.h"
#include "defs.h"

namespace http_handler {
using namespace defs;
// Создаёт корректный HttpResponse с заданными параметрами
StringResponse Response::Make(http::status status, 
                            std::string_view body, 
                            std::string_view content_type,
                            std::string_view allow_field)
{
    StringResponse response;
    response.result(status);
    response.set(http::field::content_type, content_type);
    response.set(http::field::cache_control, MiscDefs::NO_CACHE);
    if(allow_field.size()) response.set(http::field::allow, allow_field);
    response.body() = body;
    response.content_length(body.size());
    return response;
}

StringResponse Response::MakeJSON(http::status status, std::string_view code, std::string_view message)
{
    boost::json::value jv = {
        { JsonField::CODE , code },
        { JsonField::MESSAGE, message }
    };

    return Make(status, boost::json::serialize(jv) );
}

StringResponse Response::MakeUnauthorizedErrorInvalidToken()
{
    return MakeJSON(http::status::unauthorized, 
                ErrorCode::INVALID_TOKEN, 
                ErrorMessage::INVALID_TOKEN);
}

StringResponse Response::MakeUnauthorizedErrorUnknownToken()
{
    return MakeJSON(http::status::unauthorized, 
                ErrorCode::UNKNOWN_TOKEN, 
                ErrorMessage::UNKNOWN_TOKEN);
}

StringResponse Response::MakeBadRequestInvalidArgument(std::string_view message)
{
    return MakeJSON(http::status::bad_request, 
                ErrorCode::INVALID_ARGUMENT, 
                message);
}

StringResponse Response::MakeMethodNotAllowed(std::string_view message, std::string_view allow)
{
    boost::json::value jv = {
        { JsonField::CODE , ErrorCode::INVALID_METHOD },
        { JsonField::MESSAGE, message }
    };

    return Response::Make(http::status::method_not_allowed, boost::json::serialize(jv), Response::ContentType::TEXT_JSON, allow);
}
}
