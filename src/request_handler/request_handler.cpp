#include "request_handler.h"

namespace http_handler {
StringResponse RequestHandler::ReportServerError(unsigned version, bool keep_alive) const
{
	auto text = app::JsonMessage("ServerError"sv,
		"request processing error"sv);
	return BaseRequestHandler::MakeStringResponse(http::status::bad_request, text, version, keep_alive);
}
}  // namespace http_handler
