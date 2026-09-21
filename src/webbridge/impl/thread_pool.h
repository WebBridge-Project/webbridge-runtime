#pragma once

/**
 * WebBridge Thread Pool Configuration
 *
 * This header allows configuring the thread pool used for async calls.
 *
 * USAGE:
 * ------
 * Before the first WebBridge call (e.g. in main.cpp):
 *
 *	#include "webbridge/impl/thread_pool.h"
 *
 *	// Optional: set the number of worker threads (default: CPU cores)
 *	webbridge::config::set_thread_pool_size(8);
 *
 *	// Later: get the thread pool
 *	auto& pool = webbridge::impl::get_thread_pool();
 *
 * WHAT HAPPENS WITH MORE REQUESTS THAN THREADS?
 * ----------------------------------------------
 * The thread pool uses a task queue (FIFO).
 *
 * Example: pool with 4 threads, 10 concurrent async calls:
 *
 *	1. Calls 1-4 are processed immediately by the 4 worker threads
 *	2. Calls 5-10 are queued and wait
 *	3. Once thread 1 finishes, it picks up call 5 from the queue
 *	4. Once thread 2 finishes, it picks up call 6 from the queue
 *	... and so on
 *
 * ADVANTAGES over std::thread().detach():
 * ----------------------------------------
 * - No thread creation per call (~50-100us saved per async call!)
 * - A bounded thread count prevents thread explosion
 * - Better CPU cache usage through thread reuse
 * - Controlled load on the system
 *
 * DISADVANTAGES / TRADE-OFFS:
 * ----------------------------
 * - Very long tasks can block short tasks
 * - The queue can grow under overload (memory usage)
 * - Too few threads: higher latency under load
 *
 * RECOMMENDED POOL SIZES:
 * ------------------------
 * - CPU-bound tasks: std::thread::hardware_concurrency() (default)
 * - I/O-bound tasks: 2x to 4x hardware_concurrency()
 * - Mixed workload: hardware_concurrency() + 2
 */

#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>

namespace webbridge::impl {

void set_thread_pool_size(size_t num_threads);
size_t get_thread_pool_size();


// =============================================================================
// Simple Thread Pool Implementation
// =============================================================================

class thread_pool {
public:
	explicit thread_pool(size_t num_threads = 0) {
		if (num_threads == 0) {
			num_threads = std::thread::hardware_concurrency();
			if (num_threads == 0) num_threads = 4; // Fallback
		}

		m_stop = false;
		m_workers.reserve(num_threads);

		for (size_t i = 0; i < num_threads; ++i) {
			m_workers.emplace_back([this]() {
				worker_loop();
			});
		}
	}

	~thread_pool() {
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_stop = true;
		}
		m_condition.notify_all();

		for (auto& worker : m_workers) {
			if (worker.joinable()) {
				worker.join();
			}
		}
	}

	// Non-copyable, non-movable
	thread_pool(const thread_pool&) = delete;
	thread_pool& operator=(const thread_pool&) = delete;
	thread_pool(thread_pool&&) = delete;
	thread_pool& operator=(thread_pool&&) = delete;

	/**
	 * Submit a task to the pool.
	 * The task will be executed by one of the worker threads.
	 * If all workers are busy, the task is queued (FIFO).
	 */
	void submit(std::function<void()> task) {
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_tasks.push(std::move(task));
		}
		m_condition.notify_one();
	}

	/**
	 * Returns the number of worker threads.
	 */
	size_t size() const {
		return m_workers.size();
	}

	/**
	 * Returns the approximate number of pending tasks in the queue.
	 */
	size_t pending() const {
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_tasks.size();
	}

private:
	void worker_loop() {
		while (true) {
			std::function<void()> task;

			{
				std::unique_lock<std::mutex> lock(m_mutex);
				m_condition.wait(lock, [this]() {
					return m_stop || !m_tasks.empty();
				});

				if (m_stop && m_tasks.empty()) {
					return;
				}

				task = std::move(m_tasks.front());
				m_tasks.pop();
			}

			// Execute task outside the lock
			task();
		}
	}

	std::vector<std::thread> m_workers;
	std::queue<std::function<void()>> m_tasks;
	mutable std::mutex m_mutex;
	std::condition_variable m_condition;
	bool m_stop;
};

// =============================================================================
// Global Thread Pool Access
// =============================================================================

/**
 * Returns the global thread pool instance.
 * Creates it on first call with the configured size.
 */
thread_pool& get_thread_pool();

} // namespace impl
