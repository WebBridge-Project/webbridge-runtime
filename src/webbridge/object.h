#pragma once

#include "impl/property_impl.h"
#include "impl/event_impl.h"
#include "impl/thread_pool.h"
#include <webview/webview.h>
#include <memory>

namespace webbridge {

class object
{
public:
	template<typename T>
	using property = impl::property<T>;
	
	template<typename... Args>
	using event = impl::event<Args...>;
};

// =========================================
// Type Registration API
// =========================================

template<typename T>
void register_type(webview::webview* w) {
	static_assert(sizeof(T) == 0, "register_type<T> must be specialized. Include the generated _registration.h file.");
}

inline void set_thread_pool_size(size_t num_threads) {
	impl::set_thread_pool_size(num_threads);
}

inline size_t get_thread_pool_size() {
	return impl::get_thread_pool_size();
}

}
