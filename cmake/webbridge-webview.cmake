get_filename_component(_webbridge_runtime_root_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(EXISTS "${_webbridge_runtime_root_dir}/vendor/webview" AND NOT DEFINED FETCHCONTENT_SOURCE_DIR_WEBVIEW)
	set(FETCHCONTENT_SOURCE_DIR_WEBVIEW "${_webbridge_runtime_root_dir}/vendor/webview" CACHE PATH "" FORCE)
endif()
if(EXISTS "${_webbridge_runtime_root_dir}/vendor/webview2" AND NOT DEFINED FETCHCONTENT_SOURCE_DIR_MICROSOFT_WEB_WEBVIEW2)
	set(FETCHCONTENT_SOURCE_DIR_MICROSOFT_WEB_WEBVIEW2 "${_webbridge_runtime_root_dir}/vendor/webview2" CACHE PATH "" FORCE)
endif()

if(NOT TARGET webview::core)
	include(FetchContent)
	FetchContent_Declare(
		webview
		GIT_REPOSITORY https://github.com/webview/webview
		GIT_TAG 0.12.0)
	FetchContent_MakeAvailable(webview)
endif()

if(TARGET webbridge::webbridge AND TARGET webview::core)
	set_property(TARGET webbridge::webbridge APPEND PROPERTY
		INTERFACE_LINK_LIBRARIES webview::core)
endif()
