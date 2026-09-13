/**
 * @file aicurlthread.h
 * @brief Thread safe wrapper for libcurl.
 *
 * Copyright (c) 2012, Aleric Inglewood.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * CHANGELOG
 *   and additional copyright holders.
 *
 *   28/04/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AICURLTHREAD_H
#define AICURLTHREAD_H
#include "aicurl.h"
#include <vector>
#undef AICurlPrivate
namespace AICurlPrivate {
namespace curlthread {
extern U32 curl_max_total_concurrent_connections;
class PollSet;
struct AICurlEasyRequestCompare {
  bool operator()(AICurlEasyRequest const& h1, AICurlEasyRequest const& h2) const { return h1.get() < h2.get(); }
};
class MultiHandle : public CurlMultiHandle
{
  public:
	MultiHandle(void);
	~MultiHandle();
	bool add_easy_request(AICurlEasyRequest const& easy_request, bool from_queue);
	CURLMcode remove_easy_request(AICurlEasyRequest const& easy_request, bool as_per_command = false);
	CURLMcode socket_action(curl_socket_t sockfd, int ev_bitmask);
	CURLMcode assign(curl_socket_t sockfd, void* sockptr);
	CURLMsg const* info_read(int* msgs_in_queue) const;
  private:
	typedef std::set<AICurlEasyRequest, AICurlEasyRequestCompare> addedEasyRequests_type;
	addedEasyRequests_type mAddedEasyRequests;
	long mTimeout;
	static LLAtomicU32 sTotalAdded;
  private:
	void finish_easy_request(AICurlEasyRequest const& easy_request, CURLcode result);
	CURLMcode remove_easy_request(addedEasyRequests_type::iterator const& iter, bool as_per_command);
    static int socket_callback(CURL* easy, curl_socket_t s, int action, void* userp, void* socketp);
    static int timer_callback(CURLM* multi, long timeout_ms, void* userp);
  public:
	int getTimeout(void) const { return mTimeout; }
	void update_timeout(long delta_ms) { mTimeout -= delta_ms; }
	void check_msg_queue(void);
	void handle_stalls(void);
	static U32 total_added_size(void) { return sTotalAdded; }
	static bool added_maximum(void) { return sTotalAdded >= curl_max_total_concurrent_connections; }
  public:
	PollSet* mReadPollSet;
	PollSet* mWritePollSet;
};
}
}
class AICurlMultiHandle : public AIThreadSafeSingleThreadDC<AICurlPrivate::curlthread::MultiHandle>, public LLThreadLocalDataMember {
  public:
	static AICurlMultiHandle& getInstance(void);
	static void destroyInstance(void);
  private:
	AICurlMultiHandle(void) { }
};
typedef AISTAccessConst<AICurlPrivate::curlthread::MultiHandle> AICurlMultiHandle_rat;
typedef AISTAccess<AICurlPrivate::curlthread::MultiHandle> AICurlMultiHandle_wat;
#define AICurlPrivate DONTUSE_AICurlPrivate
#endif
