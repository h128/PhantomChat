#pragma once

#include <App.h>
#include <phantomchat/utils/CacheFileProvider.hpp>

namespace phantomchat::handlers {

template<typename ResponseType, typename RequestType>
void handleStaticFile(ResponseType *res, RequestType *req, const phantomchat::utils::CacheFileProvider &file_provider);

}// namespace phantomchat::handlers
