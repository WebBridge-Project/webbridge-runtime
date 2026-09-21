#include "thread_pool.h"

namespace webbridge {

namespace impl {

static size_t g_thread_pool_size = 0; // 0 = auto

void set_thread_pool_size(size_t num_threads) {
	g_thread_pool_size = num_threads;
}

size_t get_thread_pool_size() {
	return g_thread_pool_size;
}

thread_pool& get_thread_pool() {
	static thread_pool pool(get_thread_pool_size());
	return pool;
}

} // namespace impl
} // namespace webbridge
