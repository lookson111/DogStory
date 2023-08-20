#pragma once
#include <filesystem>
#include "response.h"
#include "defs.h"
#include "base_request.h"

namespace http_handler {
namespace fs = std::filesystem;
using namespace defs;
	
class FileRequestHandler : public BaseRequestHandler {
private:
	const fs::path static_path_;

public:
	FileRequestHandler(const fs::path& static_path)
		: static_path_{ CheckStaticPath(static_path) } {}
    virtual ~FileRequestHandler() {}
private: 
    TypeRequest ParseTarget(std::string_view target, std::string& res) const;
    virtual FileRequestResult MakeGetResponse(
        const StringRequest& req, bool with_body) const override;
	virtual FileRequestResult MakePostResponse(
        const StringRequest& req) const override;
    virtual FileRequestResult MakeOptionsResponse(
        const StringRequest& req) const override;
    virtual FileRequestResult MakePutResponse(
        const StringRequest& req) const override;
    virtual FileRequestResult MakePatchResponse(
        const StringRequest& req) const override;
    virtual FileRequestResult MakeDeleteResponse(
        const StringRequest& req) const override;
	static fs::path CheckStaticPath(const fs::path& path_static);
	FileRequestResult StaticFilesResponse(
		std::string_view responseText, bool with_body,
		unsigned http_version, bool keep_alive) const;
	bool CheckFileExist(std::string& file) const;
	bool FileInRootStaticDir(std::string_view file) const;
};
} // namespace http_handler
