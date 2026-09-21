#pragma once

#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>
#include "webview/webview.h"
#include "dispatcher.h"

namespace webbridge::impl {

using obj_deleter_fun = std::function<void(const std::string&)>;

void init_webview(webview::webview* ptr, obj_deleter_fun fun);

bool is_webview_initialized(webview::webview* ptr);

std::string generate_js_class_wrapper(
	std::string_view type_name,
	const std::vector<std::string>& sync_methods,
	const std::vector<std::string>& async_methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events,
	const std::vector<std::string>& instance_constants,
	const nlohmann::json& static_constants);

} // namespace webbridge::impl