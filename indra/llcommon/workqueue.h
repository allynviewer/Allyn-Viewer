/**
 * @file workqueue.h
 * @brief Minimal LL::WorkQueue shim for WebRTC voice (Allyn).
 *
 * Official viewer uses a full workqueue; here we schedule onto the main
 * idle loop via doOnIdleOneTime.
 */
#ifndef LL_WORKQUEUE_H
#define LL_WORKQUEUE_H
#include <functional>
#include <memory>
#include <string>
#include "llcallbacklist.h"
namespace LL
{
class WorkQueue : public std::enable_shared_from_this<WorkQueue>
{
public:
	using weak_t = std::weak_ptr<WorkQueue>;
	using ptr_t = std::shared_ptr<WorkQueue>;
	static ptr_t getInstance(const std::string& )
	{
		static ptr_t sInstance = std::make_shared<WorkQueue>();
		return sInstance;
	}
	template <typename CALLABLE>
	static void postMaybe(const weak_t& queue, CALLABLE&& callable)
	{
		if (queue.expired())
		{
			return;
		}
		CALLABLE fn(std::forward<CALLABLE>(callable));
		doOnIdleOneTime(nullary_func_t(fn));
	}
};
}
#endif
