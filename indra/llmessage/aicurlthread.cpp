/**
 * @file aicurlthread.cpp
 * @brief Implementation of AICurl, curl thread functions.
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
#include "linden_common.h"
#include "aicurlthread.h"
#include "aihttptimeoutpolicy.h"
#include "aihttptimeout.h"
#include "aicurlperservice.h"
#include "aiaverage.h"
#include "aicurltimer.h"
#include "lltimer.h"
#include "llhttpstatuscodes.h"
#include "llbuffer.h"
#include "llcontrol.h"
#include <sys/types.h>
#if !LL_WINDOWS
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include <deque>
#include <cctype>
#define WINDOWS_CODE LL_WINDOWS
#undef AICurlPrivate
namespace AICurlPrivate {
enum command_st {
  cmd_none,
  cmd_add,
  cmd_boost,
  cmd_remove
};
class Command {
  public:
	Command(void) : mCommand(cmd_none) { }
	Command(AICurlEasyRequest const& easy_request, command_st command) : mCurlEasyRequest(easy_request.get_ptr()), mCommand(command) { }
	command_st command(void) const { return mCommand; }
	BufferedCurlEasyRequestPtr const& easy_request(void) const { return mCurlEasyRequest; }
	bool operator==(AICurlEasyRequest const& easy_request) const { return mCurlEasyRequest == easy_request.get_ptr(); }
	void reset(void);
  private:
	BufferedCurlEasyRequestPtr mCurlEasyRequest;
	command_st mCommand;
};
void Command::reset(void)
{
  mCurlEasyRequest.reset();
  mCommand = cmd_none;
}
struct command_queue_st {
  std::deque<Command> commands;
  size_t size;
};
AIThreadSafeSimpleDC<command_queue_st> command_queue;
typedef AIAccess<command_queue_st> command_queue_wat;
typedef AIAccess<command_queue_st> command_queue_rat;
AIThreadSafeDC<Command> command_being_processed;
typedef AIWriteAccess<Command> command_being_processed_wat;
typedef AIReadAccess<Command> command_being_processed_rat;
namespace curlthread {
int const empty = 0x1;
int const complete = 0x2;
enum refresh_t {
  not_complete_not_empty = 0,
  complete_not_empty = complete,
  empty_and_complete = complete|empty
};
class CurlSocketInfo
{
  public:
	CurlSocketInfo(MultiHandle& multi_handle, ASSERT_ONLY(CURL* easy,) curl_socket_t s, int action, ThreadSafeBufferedCurlEasyRequest* lockobj);
	~CurlSocketInfo();
	void set_action(int action);
	void mark_dead(void) { set_action(CURL_POLL_NONE); mDead = true; }
	curl_socket_t getSocketFd(void) const { return mSocketFd; }
	AICurlEasyRequest& getEasyRequest(void) { return mEasyRequest; }
  private:
	MultiHandle& mMultiHandle;
	curl_socket_t mSocketFd;
	int mAction;
	bool mDead;
	AICurlEasyRequest mEasyRequest;
	LLPointer<HTTPTimeout> mTimeout;
};
class PollSet
{
  public:
	PollSet(void);
	void add(CurlSocketInfo* sp);
	void remove(CurlSocketInfo* sp);
	refresh_t refresh(void);
	fd_set* access(void) { return &mFdSet; }
#if !WINDOWS_CODE
	curl_socket_t get_max_fd(void) const { return mMaxFdSet; }
#endif
	CurlSocketInfo* contains(curl_socket_t s) const;
	bool is_set(curl_socket_t s) const;
	void clr(curl_socket_t fd);
	void reset(void);
	curl_socket_t get(void) const;
	void next(void);
  private:
	CurlSocketInfo** mFileDescriptors;
	int mNrFds;
	int mNext;
	fd_set mFdSet;
#if !WINDOWS_CODE
	curl_socket_t mMaxFd;
	curl_socket_t mMaxFdSet;
	std::vector<curl_socket_t> mCopiedFileDescriptors;
	std::vector<curl_socket_t>::iterator mIter;
#else
	unsigned int mIter;
#endif
};
static size_t const MAXSIZE = llmax(1024, FD_SETSIZE);
PollSet::PollSet(void) : mFileDescriptors(new CurlSocketInfo* [MAXSIZE]),
                         mNrFds(0), mNext(0)
#if !WINDOWS_CODE
						 , mMaxFd(-1), mMaxFdSet(-1)
#endif
{
  FD_ZERO(&mFdSet);
}
void PollSet::add(CurlSocketInfo* sp)
{
  llassert_always(mNrFds < (int)MAXSIZE);
  mFileDescriptors[mNrFds++] = sp;
#if !WINDOWS_CODE
  mMaxFd = llmax(mMaxFd, sp->getSocketFd());
#endif
}
void PollSet::remove(CurlSocketInfo* sp)
{
  llassert(mNrFds > 0);
  int i = --mNrFds;
  curl_socket_t const s = sp->getSocketFd();
  CurlSocketInfo* cur = mFileDescriptors[i];
#if !WINDOWS_CODE
  curl_socket_t max = -1;
#endif
  while (cur != sp)
  {
	llassert(i > 0);
	CurlSocketInfo* next = mFileDescriptors[--i];
	mFileDescriptors[i] = cur;
#if !WINDOWS_CODE
	max = llmax(max, cur->getSocketFd());
#endif
	cur = next;
  }
  llassert(cur == sp);
  if (mNext > i)
	--mNext;
#if !WINDOWS_CODE
  if (s == mMaxFd)
  {
	while (i > 0)
	{
	  CurlSocketInfo* next = mFileDescriptors[--i];
	  max = llmax(max, next->getSocketFd());
	}
	mMaxFd = max;
	llassert(mMaxFd < s);
	llassert((mMaxFd == -1) == (mNrFds == 0));
  }
#endif
#if !WINDOWS_CODE
  clr(s);
#else
  if (FD_ISSET(s, &mFdSet))
  {
	llassert(mFdSet.fd_count > 0);
	unsigned int i = --mFdSet.fd_count;
	curl_socket_t cur = mFdSet.fd_array[i];
	while (cur != s)
	{
	  llassert(i > 0);
	  curl_socket_t next = mFdSet.fd_array[--i];
	  mFdSet.fd_array[i] = cur;
	  cur = next;
	}
	if (mIter > i)
	  --mIter;
	llassert(mIter <= mFdSet.fd_count);
  }
#endif
}
CurlSocketInfo* PollSet::contains(curl_socket_t fd) const
{
  for (int i = 0; i < mNrFds; ++i)
	if (mFileDescriptors[i]->getSocketFd() == fd)
	  return mFileDescriptors[i];
  return NULL;
}
inline bool PollSet::is_set(curl_socket_t fd) const
{
  return FD_ISSET(fd, &mFdSet);
}
inline void PollSet::clr(curl_socket_t fd)
{
  FD_CLR(fd, &mFdSet);
}
refresh_t PollSet::refresh(void)
{
  FD_ZERO(&mFdSet);
#if !WINDOWS_CODE
  mCopiedFileDescriptors.clear();
#endif
  if (mNrFds == 0)
  {
#if !WINDOWS_CODE
	mMaxFdSet = -1;
#endif
	return empty_and_complete;
  }
  llassert_always(mNext < mNrFds);
  if (mNrFds >= FD_SETSIZE)
  {
	LL_WARNS() << "PollSet::reset: More than FD_SETSIZE (" << FD_SETSIZE << ") file descriptors active!" << LL_ENDL;
#if !WINDOWS_CODE
	int max = -1, i = mNext, count = 0;
	while (++count < FD_SETSIZE) { max = llmax(max, mFileDescriptors[i]->getSocketFd()); if (++i == mNrFds) i = 0; }
	mMaxFdSet = max;
#endif
  }
  else
  {
	mNext = 0;
#if !WINDOWS_CODE
	mMaxFdSet = mMaxFd;
#endif
  }
  int count = 0;
  int i = mNext;
  for(;;)
  {
	if (++count == FD_SETSIZE)
	{
	  mNext = i;
	  return not_complete_not_empty;
	}
	FD_SET(mFileDescriptors[i]->getSocketFd(), &mFdSet);
#if !WINDOWS_CODE
	mCopiedFileDescriptors.push_back(mFileDescriptors[i]->getSocketFd());
#endif
	if (++i == mNrFds)
	{
	  if (mNext == 0)
		break;
	  i = 0;
	}
  }
  return complete_not_empty;
}
void PollSet::reset(void)
{
#if WINDOWS_CODE
  mIter = 0;
#else
  if (mCopiedFileDescriptors.empty())
	mIter = mCopiedFileDescriptors.end();
  else
  {
	mIter = mCopiedFileDescriptors.begin();
	if (!FD_ISSET(*mIter, &mFdSet))
	  next();
  }
#endif
}
inline curl_socket_t PollSet::get(void) const
{
#if WINDOWS_CODE
  return (mIter >= mFdSet.fd_count) ? CURL_SOCKET_BAD : mFdSet.fd_array[mIter];
#else
  return (mIter == mCopiedFileDescriptors.end()) ? CURL_SOCKET_BAD : *mIter;
#endif
}
void PollSet::next(void)
{
#if WINDOWS_CODE
  llassert(mIter < mFdSet.fd_count);
  ++mIter;
#else
  llassert(mIter != mCopiedFileDescriptors.end());
  while (++mIter != mCopiedFileDescriptors.end() && !FD_ISSET(*mIter, &mFdSet));
#endif
}
class MergeIterator
{
  public:
	MergeIterator(PollSet* readPollSet, PollSet* writePollSet);
	bool next(curl_socket_t& fd_out, int& ev_bitmask_out);
  private:
	PollSet* mReadPollSet;
	PollSet* mWritePollSet;
};
MergeIterator::MergeIterator(PollSet* readPollSet, PollSet* writePollSet) :
    mReadPollSet(readPollSet), mWritePollSet(writePollSet)
{
  mReadPollSet->reset();
  mWritePollSet->reset();
}
bool MergeIterator::next(curl_socket_t& fd_out, int& ev_bitmask_out)
{
  curl_socket_t rfd = mReadPollSet->get();
  curl_socket_t wfd = mWritePollSet->get();
  if (rfd == CURL_SOCKET_BAD && wfd == CURL_SOCKET_BAD)
	return false;
  if (rfd == wfd)
  {
	fd_out = rfd;
	ev_bitmask_out = CURL_CSELECT_IN | CURL_CSELECT_OUT;
	mReadPollSet->next();
  }
  else if (wfd == CURL_SOCKET_BAD || (rfd != CURL_SOCKET_BAD && rfd < wfd))
  {
	fd_out = rfd;
	ev_bitmask_out = CURL_CSELECT_IN;
	mReadPollSet->next();
	if (wfd != CURL_SOCKET_BAD && mWritePollSet->is_set(rfd))
	{
	  ev_bitmask_out |= CURL_CSELECT_OUT;
	  mWritePollSet->clr(rfd);
	}
  }
  else
  {
	fd_out = wfd;
	ev_bitmask_out = CURL_CSELECT_OUT;
	mWritePollSet->next();
	if (rfd != CURL_SOCKET_BAD && mReadPollSet->is_set(wfd))
	{
	  ev_bitmask_out |= CURL_CSELECT_IN;
	  mReadPollSet->clr(wfd);
	}
  }
  return true;
}
#ifdef CWDEBUG
#undef AI_CASE_RETURN
#define AI_CASE_RETURN(x) case x: return #x;
static char const* action_str(int action)
{
  switch(action)
  {
	AI_CASE_RETURN(CURL_POLL_NONE);
	AI_CASE_RETURN(CURL_POLL_IN);
	AI_CASE_RETURN(CURL_POLL_OUT);
	AI_CASE_RETURN(CURL_POLL_INOUT);
	AI_CASE_RETURN(CURL_POLL_REMOVE);
  }
  return "<unknown action>";
}
struct DebugFdSet {
  int nfds;
  fd_set* fdset;
  DebugFdSet(int n, fd_set* p) : nfds(n), fdset(p) { }
};
std::ostream& operator<<(std::ostream& os, DebugFdSet const& s)
{
  if (!s.fdset)
	return os << "NULL";
  bool first = true;
  os << '{';
  for (int fd = 0; fd < s.nfds; ++fd)
  {
	if (FD_ISSET(fd, s.fdset))
	{
	  if (!first)
		os << ", ";
	  os << fd;
	  first = false;
	}
  }
  os << '}';
  return os;
}
#endif
CurlSocketInfo::CurlSocketInfo(MultiHandle& multi_handle, ASSERT_ONLY(CURL* easy,) curl_socket_t s, int action, ThreadSafeBufferedCurlEasyRequest* lockobj) :
    mMultiHandle(multi_handle), mSocketFd(s), mAction(CURL_POLL_NONE), mDead(false), mEasyRequest(lockobj)
{
  llassert(*AICurlEasyRequest_wat(*mEasyRequest) == easy);
  mMultiHandle.assign(s, this);
  llassert(!mMultiHandle.mReadPollSet->contains(s));
  llassert(!mMultiHandle.mWritePollSet->contains(s));
  set_action(action);
  AICurlEasyRequest_wat easy_request_w(*lockobj);
  mTimeout = easy_request_w->get_timeout_object();
}
CurlSocketInfo::~CurlSocketInfo()
{
  set_action(CURL_POLL_NONE);
}
void CurlSocketInfo::set_action(int action)
{
  if (mDead)
  {
	return;
  }
  Dout(dc::curl, "CurlSocketInfo::set_action(" << action_str(mAction) << " --> " << action_str(action) << ") [" << (void*)mEasyRequest.get_ptr().get() << "]");
  int toggle_action = mAction ^ action;
  mAction = action;
  if ((toggle_action & CURL_POLL_IN))
  {
	if ((action & CURL_POLL_IN))
	  mMultiHandle.mReadPollSet->add(this);
	else
	  mMultiHandle.mReadPollSet->remove(this);
  }
  if ((toggle_action & CURL_POLL_OUT))
  {
	if ((action & CURL_POLL_OUT))
	{
	  mMultiHandle.mWritePollSet->add(this);
	  if (mTimeout)
	  {
		  mTimeout->upload_starting();
	  }
	}
	else
	{
	  mMultiHandle.mWritePollSet->remove(this);
	  AICurlEasyRequest_wat curl_easy_request_w(*mEasyRequest);
	  double pretransfer_time;
	  curl_easy_request_w->getinfo(CURLINFO_PRETRANSFER_TIME, &pretransfer_time);
	  if (pretransfer_time > 0)
	  {
		mTimeout->upload_finished();
	  }
	}
  }
}
class AICurlThread : public LLThread
{
  public:
	static AICurlThread* sInstance;
	LLMutex mWakeUpMutex;
	LLMutex mWakeUpFlagMutex;
	bool mWakeUpFlag;
  public:
	AICurlThread(void);
	virtual ~AICurlThread();
	void wakeup_thread(bool stop_thread = false);
	apr_status_t join_thread(void);
  protected:
	virtual void run(void);
	void wakeup(AICurlMultiHandle_wat const& multi_handle_w);
	void process_commands(AICurlMultiHandle_wat const& multi_handle_w);
  private:
	void create_wakeup_fds(void);
	void cleanup_wakeup_fds(void);
	curl_socket_t mWakeUpFd_in;
	curl_socket_t mWakeUpFd;
	int mZeroTimeout;
	volatile bool mRunning;
};
AICurlThread* AICurlThread::sInstance = NULL;
AICurlThread::AICurlThread(void) : LLThread("AICurlThread"),
    mWakeUpFd_in(CURL_SOCKET_BAD),
	mWakeUpFd(CURL_SOCKET_BAD),
	mZeroTimeout(0), mWakeUpFlag(false), mRunning(true)
{
  create_wakeup_fds();
  sInstance = this;
}
AICurlThread::~AICurlThread()
{
  sInstance = NULL;
  cleanup_wakeup_fds();
}
#if LL_WINDOWS
static std::string formatWSAError(int e = WSAGetLastError())
{
	std::ostringstream r;
	LPTSTR error_str = 0;
	r << e;
	if(FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, e, 0, (LPTSTR)&error_str, 0, NULL))
	{
		r << " " << utf16str_to_utf8str(error_str);
		LocalFree(error_str);
	}
	else
	{
		r << " Unknown WinSock error";
	}
	return r.str();
}
#elif WINDOWS_CODE
static std::string formatWSAError(int e = errno)
{
	return strerror(e);
}
#endif
#if LL_WINDOWS
static int dumb_socketpair(SOCKET socks[2], bool make_overlapped)
{
    union {
       struct sockaddr_in inaddr;
       struct sockaddr addr;
    } a;
    SOCKET listener;
    int e;
    socklen_t addrlen = sizeof(a.inaddr);
    DWORD flags = (make_overlapped ? WSA_FLAG_OVERLAPPED : 0);
    int reuse = 1;
    if (socks == 0) {
      WSASetLastError(WSAEINVAL);
      return SOCKET_ERROR;
    }
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET)
        return SOCKET_ERROR;
    memset(&a, 0, sizeof(a));
    a.inaddr.sin_family = AF_INET;
    a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.inaddr.sin_port = 0;
    socks[0] = socks[1] = INVALID_SOCKET;
    do {
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
               (char*) &reuse, (socklen_t) sizeof(reuse)) == -1)
            break;
        if  (bind(listener, &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR)
            break;
        memset(&a, 0, sizeof(a));
        if  (getsockname(listener, &a.addr, &addrlen) == SOCKET_ERROR)
            break;
        a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.inaddr.sin_family = AF_INET;
        if (listen(listener, 1) == SOCKET_ERROR)
            break;
        socks[0] = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, flags);
        if (socks[0] == INVALID_SOCKET)
            break;
        if (connect(socks[0], &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR)
            break;
        socks[1] = accept(listener, NULL, NULL);
        if (socks[1] == INVALID_SOCKET)
            break;
        closesocket(listener);
        return 0;
    } while (0);
    e = WSAGetLastError();
    closesocket(listener);
    closesocket(socks[0]);
    closesocket(socks[1]);
    WSASetLastError(e);
    return SOCKET_ERROR;
}
#elif WINDOWS_CODE
int dumb_socketpair(int socks[2], int dummy)
{
    (void) dummy;
    return socketpair(AF_LOCAL, SOCK_STREAM, 0, socks);
}
#endif
void AICurlThread::create_wakeup_fds(void)
{
#if WINDOWS_CODE
	curl_socket_t socks[2];
	if (dumb_socketpair(socks, false) == SOCKET_ERROR)
	{
		LL_ERRS() << "Failed to generate wake-up socket pair" << formatWSAError() << LL_ENDL;
		return;
	}
	u_long nonblocking_enable = TRUE;
	int error = ioctlsocket(socks[0], FIONBIO, &nonblocking_enable);
	if(error)
	{
		LL_ERRS() << "Failed to set wake-up socket nonblocking: " << formatWSAError() << LL_ENDL;
	}
	llassert(nonblocking_enable);
	error = ioctlsocket(socks[1], FIONBIO, &nonblocking_enable);
	if(error)
	{
		LL_ERRS() << "Failed to set wake-up input socket nonblocking: " << formatWSAError() << LL_ENDL;
	}
	mWakeUpFd = socks[0];
	mWakeUpFd_in = socks[1];
#else
  int pipefd[2];
  if (pipe(pipefd))
  {
	LL_ERRS() << "Failed to create wakeup pipe: " << strerror(errno) << LL_ENDL;
  }
  int const flags = O_NONBLOCK;
  for (int i = 0; i < 2; ++i)
  {
	if (fcntl(pipefd[i], F_SETFL, flags))
	{
	  LL_ERRS() << "Failed to set pipe to non-blocking: " << strerror(errno) << LL_ENDL;
	}
  }
  mWakeUpFd = pipefd[0];
  mWakeUpFd_in = pipefd[1];
#endif
}
void AICurlThread::cleanup_wakeup_fds(void)
{
#if WINDOWS_CODE
	if (mWakeUpFd != CURL_SOCKET_BAD)
	{
		int error = closesocket(mWakeUpFd);
		if (error)
		{
			LL_WARNS() << "Error closing wake-up socket" << formatWSAError() << LL_ENDL;
		}
	}
	if (mWakeUpFd_in != CURL_SOCKET_BAD)
	{
		int error = closesocket(mWakeUpFd_in);
		if (error)
		{
			LL_WARNS() << "Error closing wake-up input socket" << formatWSAError() << LL_ENDL;
		}
	}
#else
  if (mWakeUpFd_in != CURL_SOCKET_BAD)
	close(mWakeUpFd_in);
  if (mWakeUpFd != CURL_SOCKET_BAD)
	close(mWakeUpFd);
#endif
}
void AICurlThread::wakeup_thread(bool stop_thread)
{
  DoutEntering(dc::curl, "AICurlThread::wakeup_thread");
  if (!mRunning)
	return;
  if (stop_thread)
	mRunning = false;
  if (!mWakeUpMutex.try_lock())
  {
	return;
  }
  if (mWakeUpFlagMutex.try_lock())
  {
	mWakeUpFlag = true;
	mWakeUpFlagMutex.unlock();
	mWakeUpMutex.unlock();
	return;
  }
#if WINDOWS_CODE
  int len = send(mWakeUpFd_in, "!", 1, 0);
  if (len == SOCKET_ERROR)
  {
	  LL_ERRS() << "Send to wake-up socket failed: " << formatWSAError() << LL_ENDL;
  }
  llassert_always(len == 1);
#else
  ssize_t len;
  do
  {
    len = write(mWakeUpFd_in, "!", 1);
    if (len == -1 && errno == EAGAIN)
	{
	  mWakeUpMutex.unlock();
	  return;
	}
  }
  while(len == -1 && errno == EINTR);
  if (len == -1)
  {
	LL_ERRS() << "write(3) to mWakeUpFd_in: " << strerror(errno) << LL_ENDL;
  }
  llassert_always(len == 1);
#endif
  mWakeUpMutex.unlock();
}
apr_status_t AICurlThread::join_thread(void)
{
	apr_status_t retval = APR_SUCCESS;
	if (sInstance)
	{
		apr_thread_join(&retval, sInstance->mAPRThreadp);
		delete sInstance;
	}
	return retval;
}
void AICurlThread::wakeup(AICurlMultiHandle_wat const& multi_handle_w)
{
  DoutEntering(dc::curl, "AICurlThread::wakeup");
#if WINDOWS_CODE
  char buf[256];
  bool got_data = false;
  for(;;)
  {
	int len = recv(mWakeUpFd, buf, sizeof(buf), 0);
	if (len > 0)
	{
	  got_data = true;
	  if (len < sizeof(buf))
		break;
	}
	else if (len == SOCKET_ERROR)
	{
	  if (errno == EWOULDBLOCK)
	  {
		if (got_data)
		  break;
		return;
	  }
	  else if (errno == EINTR)
	  {
		continue;
	  }
	  else
	  {
		LL_ERRS() << "read(3) from mWakeUpFd: " << formatWSAError() << LL_ENDL;
		return;
	  }
	}
	else
	{
	  LL_WARNS() << "read(3) from mWakeUpFd returned 0, indicating that the pipe on the other end was closed! Shutting down curl thread." << LL_ENDL;
	  closesocket(mWakeUpFd);
	  mWakeUpFd = CURL_SOCKET_BAD;
	  mRunning = false;
	  return;
	}
  }
#else
  char buf[256];
  bool got_data = false;
  for(;;)
  {
	ssize_t len = read(mWakeUpFd, buf, sizeof(buf));
	if (len > 0)
	{
	  got_data = true;
	  if (len < sizeof(buf))
		break;
	}
	else if (len == -1)
	{
	  if (errno == EAGAIN)
	  {
		if (got_data)
		  break;
		return;
	  }
	  else if (errno == EINTR)
	  {
		continue;
	  }
	  else
	  {
		LL_ERRS() << "read(3) from mWakeUpFd: " << strerror(errno) << LL_ENDL;
		return;
	  }
	}
	else
	{
	  LL_WARNS() << "read(3) from mWakeUpFd returned 0, indicating that the pipe on the other end was closed! Shutting down curl thread." << LL_ENDL;
	  close(mWakeUpFd);
	  mWakeUpFd = CURL_SOCKET_BAD;
	  mRunning = false;
	  return;
	}
  }
#endif
  process_commands(multi_handle_w);
}
void AICurlThread::process_commands(AICurlMultiHandle_wat const& multi_handle_w)
{
  DoutEntering(dc::curl, "AICurlThread::process_commands(void)");
  mWakeUpMutex.lock();
  mWakeUpMutex.unlock();
  for(;;)
  {
	{
	  command_queue_wat command_queue_w(command_queue);
	  if (command_queue_w->commands.empty())
	  {
		mWakeUpFlagMutex.lock();
		mWakeUpFlag = false;
		mWakeUpFlagMutex.unlock();
		break;
	  }
	  command_st command;
	  {
		command_being_processed_wat command_being_processed_w(command_being_processed);
		*command_being_processed_w = command_queue_w->commands.front();
		command = command_being_processed_w->command();
	  }
	  command_queue_w->commands.pop_front();
	  if (command == cmd_add)
	  {
		command_queue_w->size--;
	  }
	  else if (command == cmd_remove)
	  {
		command_queue_w->size++;
	  }
	}
	{
	  command_being_processed_rat command_being_processed_r(command_being_processed);
	  AICapabilityType capability_type;
	  AIPerServicePtr per_service;
	  {
		AICurlEasyRequest_wat easy_request_w(*command_being_processed_r->easy_request());
		capability_type = easy_request_w->capability_type();
		per_service = easy_request_w->getPerServicePtr();
	  }
	  switch(command_being_processed_r->command())
	  {
		case cmd_none:
		case cmd_boost:
		  break;
		case cmd_add:
		{
		  multi_handle_w->add_easy_request(AICurlEasyRequest(command_being_processed_r->easy_request()), false);
		  PerService_wat(*per_service)->removed_from_command_queue(capability_type);
		  break;
		}
		case cmd_remove:
		{
		  PerService_wat(*per_service)->added_to_command_queue(capability_type);
		  multi_handle_w->remove_easy_request(AICurlEasyRequest(command_being_processed_r->easy_request()), true);
		  break;
		}
	  }
	  command_being_processed_wat command_being_processed_w(command_being_processed_r);
	  command_being_processed_w->reset();
	}
  }
}
static bool is_bad(curl_socket_t fd, bool for_writing)
{
  fd_set tmp;
  FD_ZERO(&tmp);
  FD_SET(fd, &tmp);
  fd_set* readfds = for_writing ? NULL : &tmp;
  fd_set* writefds = for_writing ? &tmp : NULL;
#if !WINDOWS_CODE
  int nfds = fd + 1;
#else
  int nfds = 64;
#endif
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 10;
  int ret = select(nfds, readfds, writefds, NULL, &timeout);
  return ret == -1;
}
void AICurlThread::run(void)
{
  DoutEntering(dc::curl, "AICurlThread::run()");
  {
	AICurlMultiHandle_wat multi_handle_w(AICurlMultiHandle::getInstance());
	while(mRunning)
	{
	  llassert(mWakeUpFd != CURL_SOCKET_BAD);
	  for(;;)
	  {
		mWakeUpFlagMutex.lock();
		if (mWakeUpFlag)
		{
		  mWakeUpFlagMutex.unlock();
		  process_commands(multi_handle_w);
		  continue;
		}
		break;
	  }
	  if (!mRunning)
	  {
		mWakeUpFlagMutex.unlock();
		break;
	  }
	  multi_handle_w->mReadPollSet->refresh();
	  refresh_t wres = multi_handle_w->mWritePollSet->refresh();
	  fd_set* read_fd_set = multi_handle_w->mReadPollSet->access();
	  FD_SET(mWakeUpFd, read_fd_set);
	  fd_set* write_fd_set = ((wres & empty)) ? NULL : multi_handle_w->mWritePollSet->access();
#if !WINDOWS_CODE
	  curl_socket_t const max_rfd = llmax(multi_handle_w->mReadPollSet->get_max_fd(), mWakeUpFd);
	  curl_socket_t const max_wfd = multi_handle_w->mWritePollSet->get_max_fd();
	  int nfds = llmax(max_rfd, max_wfd) + 1;
	  llassert(1 <= nfds && nfds <= FD_SETSIZE);
	  llassert((max_rfd == -1) == (read_fd_set == NULL) &&
			   (max_wfd == -1) == (write_fd_set == NULL));
	  llassert((max_rfd == -1 || multi_handle_w->mReadPollSet->is_set(max_rfd)) &&
			   (max_wfd == -1 || multi_handle_w->mWritePollSet->is_set(max_wfd)));
#else
	  int nfds = 64;
#endif
	  int ready = 0;
	  struct timeval timeout;
	  AICurlTimer::sTime_1ms = get_clock_count() * AICurlTimer::sClockWidth_1ms;
	  Dout(dc::curl, "AICurlTimer::sTime_1ms = " << AICurlTimer::sTime_1ms);
	  long timeout_ms = multi_handle_w->getTimeout();
	  bool libcurl_timeout = timeout_ms == 0 || (timeout_ms > 0 && !AICurlTimer::expiresBefore(timeout_ms));
	  if (LL_UNLIKELY(timeout_ms < 0))
		timeout_ms = 4000;
	  if (AICurlTimer::expiresBefore(timeout_ms))
	  {
		timeout_ms = AICurlTimer::nextExpiration();
	  }
	  if (LL_UNLIKELY(timeout_ms <= 0))
	  {
		if (mZeroTimeout >= 1000)
		{
		  if (mZeroTimeout % 10000 == 0)
			LL_WARNS() << "Detected " << mZeroTimeout << " zero-timeout calls of select() by curl thread (more than 101 seconds)!" << LL_ENDL;
		  timeout_ms = 10;
		}
		else if (mZeroTimeout >= 100)
		  timeout_ms = 1;
		else
		  timeout_ms = 0;
	  }
	  else
	  {
		if (LL_UNLIKELY(mZeroTimeout >= 10000))
		  LL_INFOS() << "Timeout of select() call by curl thread reset (to " << timeout_ms << " ms)." << LL_ENDL;
		mZeroTimeout = 0;
	  }
	  timeout.tv_sec = timeout_ms / 1000;
	  timeout.tv_usec = (timeout_ms % 1000) * 1000;
#ifdef CWDEBUG
#ifdef DEBUG_CURLIO
	  Dout(dc::curl|flush_cf|continued_cf, "select(" << nfds << ", " << DebugFdSet(nfds, read_fd_set) << ", " << DebugFdSet(nfds, write_fd_set) << ", NULL, timeout = " << timeout_ms << " ms) = ");
#else
	  static int last_nfds = -1;
	  static long last_timeout_ms = -1;
	  static int same_count = 0;
	  bool same = (nfds == last_nfds && timeout_ms == last_timeout_ms);
	  if (!same)
	  {
		if (same_count > 1)
		  Dout(dc::curl, "Last select() call repeated " << same_count << " times.");
		Dout(dc::curl|flush_cf|continued_cf, "select(" << nfds << ", ..., timeout = " << timeout_ms << " ms) = ");
		same_count = 1;
	  }
	  else
	  {
		++same_count;
	  }
#endif
#endif
	  ready = select(nfds, read_fd_set, write_fd_set, NULL, &timeout);
	  mWakeUpFlagMutex.unlock();
#ifdef CWDEBUG
#ifdef DEBUG_CURLIO
	  Dout(dc::finish|cond_error_cf(ready == -1), ready);
#else
	  static int last_ready = -2;
	  static int last_errno = 0;
	  if (!same)
		Dout(dc::finish|cond_error_cf(ready == -1), ready);
	  else if (ready != last_ready || (ready == -1 && errno != last_errno))
	  {
		if (same_count > 1)
		  Dout(dc::curl, "Last select() call repeated " << same_count << " times.");
		Dout(dc::curl|cond_error_cf(ready == -1), "select(" << last_nfds << ", ..., timeout = " << last_timeout_ms << " ms) = " << ready);
		same_count = 1;
	  }
	  last_nfds = nfds;
	  last_timeout_ms = timeout_ms;
	  last_ready = ready;
	  if (ready == -1)
		last_errno = errno;
#endif
#endif
	  if (ready == -1)
	  {
		LL_WARNS() << "select() failed: " << errno << ", " << strerror(errno) << LL_ENDL;
		if (errno == EBADF)
		{
		  llassert_always(!is_bad(mWakeUpFd, false));
		  PollSet* found = NULL;
		  multi_handle_w->mReadPollSet->refresh();
		  multi_handle_w->mReadPollSet->reset();
		  curl_socket_t fd;
		  while ((fd = multi_handle_w->mReadPollSet->get()) != CURL_SOCKET_BAD)
		  {
			if (is_bad(fd, false))
			{
			  found = multi_handle_w->mReadPollSet;
			  break;
			}
			multi_handle_w->mReadPollSet->next();
		  }
		  if (!found)
		  {
			refresh_t wres = multi_handle_w->mWritePollSet->refresh();
			if (!(wres & empty))
			{
			  multi_handle_w->mWritePollSet->reset();
			  while ((fd = multi_handle_w->mWritePollSet->get()) != CURL_SOCKET_BAD)
			  {
				if (is_bad(fd, true))
				{
				  found = multi_handle_w->mWritePollSet;
				  break;
				}
				multi_handle_w->mWritePollSet->next();
			  }
			}
		  }
		  llassert_always(found);
		  CurlSocketInfo* sp = found->contains(fd);
		  llassert_always(sp);
		  sp->mark_dead();
		  AICurlEasyRequest_wat curl_easy_request_w(*sp->getEasyRequest());
		  curl_easy_request_w->pause(CURLPAUSE_ALL);
		  curl_easy_request_w->bad_file_descriptor(curl_easy_request_w);
		}
		continue;
	  }
	  AICurlTimer::sTime_1ms = get_clock_count() * AICurlTimer::sClockWidth_1ms;
	  Dout(dc::curl, "AICurlTimer::sTime_1ms = " << AICurlTimer::sTime_1ms);
	  HTTPTimeout::sTime_10ms = AICurlTimer::sTime_1ms / 10;
	  if (ready == 0)
	  {
		if (libcurl_timeout)
		{
		  multi_handle_w->socket_action(CURL_SOCKET_TIMEOUT, 0);
		}
		else
		{
		  multi_handle_w->update_timeout(timeout_ms);
		  Dout(dc::curl, "MultiHandle::mTimeout set to " << multi_handle_w->getTimeout() << " ms.");
		}
		if (AICurlTimer::expiresBefore(1))
		{
		  AICurlTimer::handleExpiration();
		}
		multi_handle_w->handle_stalls();
	  }
	  else
	  {
		if (multi_handle_w->mReadPollSet->is_set(mWakeUpFd))
		{
		  wakeup(multi_handle_w);
		  --ready;
		}
		MergeIterator iter(multi_handle_w->mReadPollSet, multi_handle_w->mWritePollSet);
		curl_socket_t fd;
		int ev_bitmask;
		while (ready > 0 && iter.next(fd, ev_bitmask))
		{
		  ready -= (ev_bitmask == (CURL_CSELECT_IN|CURL_CSELECT_OUT)) ? 2 : 1;
		  multi_handle_w->socket_action(fd, ev_bitmask);
		  llassert(ready >= 0);
		}
	  }
	  multi_handle_w->check_msg_queue();
	}
	AIPerService::purge();
  }
  AICurlMultiHandle::destroyInstance();
}
LLAtomicU32 MultiHandle::sTotalAdded;
MultiHandle::MultiHandle(void) : mTimeout(-1), mReadPollSet(NULL), mWritePollSet(NULL)
{
  mReadPollSet = new PollSet;
  mWritePollSet = new PollSet;
  check_multi_code(curl_multi_setopt(mMultiHandle, CURLMOPT_SOCKETFUNCTION, &MultiHandle::socket_callback));
  check_multi_code(curl_multi_setopt(mMultiHandle, CURLMOPT_SOCKETDATA, this));
  check_multi_code(curl_multi_setopt(mMultiHandle, CURLMOPT_TIMERFUNCTION, &MultiHandle::timer_callback));
  check_multi_code(curl_multi_setopt(mMultiHandle, CURLMOPT_TIMERDATA, this));
}
MultiHandle::~MultiHandle()
{
  LL_INFOS() << "Destructing MultiHandle with " << mAddedEasyRequests.size() << " active curl easy handles." << LL_ENDL;
  for(addedEasyRequests_type::iterator iter = mAddedEasyRequests.begin(); iter != mAddedEasyRequests.end(); iter = mAddedEasyRequests.begin())
  {
	finish_easy_request(*iter, CURLE_GOT_NOTHING);
	remove_easy_request(*iter);
  }
  delete mWritePollSet;
  delete mReadPollSet;
}
void MultiHandle::handle_stalls(void)
{
  for(addedEasyRequests_type::iterator iter = mAddedEasyRequests.begin(); iter != mAddedEasyRequests.end();)
  {
	if (AICurlEasyRequest_wat(**iter)->has_stalled())
	{
	  Dout(dc::curl, "MultiHandle::handle_stalls(): Easy request stalled! [" << (void*)iter->get_ptr().get() << "]");
	  finish_easy_request(*iter, CURLE_OPERATION_TIMEDOUT);
	  remove_easy_request(iter++, false);
	}
	else
	  ++iter;
  }
}
int MultiHandle::socket_callback(CURL* easy, curl_socket_t s, int action, void* userp, void* socketp)
{
#ifdef CWDEBUG
  ThreadSafeBufferedCurlEasyRequest* lockobj = NULL;
  curl_easy_getinfo(easy, CURLINFO_PRIVATE, &lockobj);
  DoutEntering(dc::curl, "MultiHandle::socket_callback((CURL*)" << (void*)easy << ", " << s <<
	  ", " << action_str(action) << ", " << (void*)userp << ", " << (void*)socketp << ") [CURLINFO_PRIVATE = " << (void*)lockobj << "]");
#endif
  MultiHandle& self = *static_cast<MultiHandle*>(userp);
  CurlSocketInfo* sock_info = static_cast<CurlSocketInfo*>(socketp);
  if (action == CURL_POLL_REMOVE)
  {
	delete sock_info;
  }
  else
  {
	if (!sock_info)
	{
	  ThreadSafeBufferedCurlEasyRequest* ptr;
	  CURLcode rese = curl_easy_getinfo(easy, CURLINFO_PRIVATE, &ptr);
	  llassert_always(rese == CURLE_OK);
	  sock_info = new CurlSocketInfo(self, ASSERT_ONLY(easy,) s, action, ptr);
	}
	else
	{
	  sock_info->set_action(action);
	}
  }
  return 0;
}
int MultiHandle::timer_callback(CURLM* multi, long timeout_ms, void* userp)
{
  MultiHandle& self = *static_cast<MultiHandle*>(userp);
  llassert(multi == self.mMultiHandle);
  self.mTimeout = timeout_ms;
  Dout(dc::curl, "MultiHandle::timer_callback(): timeout set to " << timeout_ms << " ms.");
  return 0;
}
CURLMcode MultiHandle::socket_action(curl_socket_t sockfd, int ev_bitmask)
{
  int running_handles;
  CURLMcode res;
  do
  {
    res = check_multi_code(curl_multi_socket_action(mMultiHandle, sockfd, ev_bitmask, &running_handles));
  }
  while(res == CURLM_CALL_MULTI_PERFORM);
  llassert(mAddedEasyRequests.size() >= (size_t)running_handles);
  AICurlInterface::Stats::running_handles = running_handles;
  return res;
}
CURLMcode MultiHandle::assign(curl_socket_t sockfd, void* sockptr)
{
  return check_multi_code(curl_multi_assign(mMultiHandle, sockfd, sockptr));
}
CURLMsg const* MultiHandle::info_read(int* msgs_in_queue) const
{
  CURLMsg const* ret = curl_multi_info_read(mMultiHandle, msgs_in_queue);
  if (ret)
	AICurlInterface::Stats::multi_calls++;
  return ret;
}
U32 curl_max_total_concurrent_connections = 32;
bool MultiHandle::add_easy_request(AICurlEasyRequest const& easy_request, bool from_queue)
{
  bool throttled = true;
  AICapabilityType capability_type;
  bool event_poll;
  AIPerServicePtr per_service;
  {
	AICurlEasyRequest_wat curl_easy_request_w(*easy_request);
	capability_type = curl_easy_request_w->capability_type();
	event_poll = curl_easy_request_w->is_event_poll();
	per_service = curl_easy_request_w->getPerServicePtr();
	if (!from_queue)
	{
	  PerService_wat per_service_w(*per_service);
	  if (per_service_w->queue(easy_request, capability_type, false))
	  {
#ifdef SHOW_ASSERT
		curl_easy_request_w->mRemovedPerCommand = false;
#endif
		if (per_service_w->nothing_added(capability_type))
		{
		  per_service_w->add_queued_to(this);
		}
		return true;
	  }
	}
	bool too_much_bandwidth = !curl_easy_request_w->approved() && AIPerService::checkBandwidthUsage(per_service, get_clock_count() * HTTPTimeout::sClockWidth_40ms);
	PerService_wat per_service_w(*per_service);
	if (!too_much_bandwidth && sTotalAdded < curl_max_total_concurrent_connections && !per_service_w->throttled(capability_type))
	{
	  curl_easy_request_w->set_timeout_opts();
	  if (curl_easy_request_w->add_handle_to_multi(curl_easy_request_w, mMultiHandle) == CURLM_OK)
	  {
		per_service_w->added_to_multi_handle(capability_type, event_poll);
		throttled = false;
	  }
	}
  }
  if (!throttled)
  {
#ifdef SHOW_ASSERT
	std::pair<addedEasyRequests_type::iterator, bool> res =
#endif
		mAddedEasyRequests.insert(easy_request);
	llassert(res.second);
	sTotalAdded++;
	llassert(sTotalAdded == mAddedEasyRequests.size());
	Dout(dc::curl, "MultiHandle::add_easy_request: Added AICurlEasyRequest " << (void*)easy_request.get_ptr().get() <<
		"; now processing " << mAddedEasyRequests.size() << " easy handles [running_handles = " << AICurlInterface::Stats::running_handles << "].");
	return true;
  }
  if (from_queue)
  {
	return false;
  }
  PerService_wat(*per_service)->queue(easy_request, capability_type);
#ifdef SHOW_ASSERT
  AICurlEasyRequest_wat(*easy_request)->mRemovedPerCommand = false;
#endif
  return true;
}
CURLMcode MultiHandle::remove_easy_request(AICurlEasyRequest const& easy_request, bool as_per_command)
{
  AICurlEasyRequest_wat easy_request_w(*easy_request);
  addedEasyRequests_type::iterator iter = mAddedEasyRequests.find(easy_request);
  if (iter == mAddedEasyRequests.end())
  {
#ifdef SHOW_ASSERT
	bool removed =
#endif
	easy_request_w->removeFromPerServiceQueue(easy_request, easy_request_w->capability_type());
#ifdef SHOW_ASSERT
	if (removed)
	{
	  AICurlEasyRequest_wat(*easy_request)->mRemovedPerCommand = true;
	}
#endif
	return (CURLMcode)-2;
  }
  return remove_easy_request(iter, as_per_command);
}
CURLMcode MultiHandle::remove_easy_request(addedEasyRequests_type::iterator const& iter, bool as_per_command)
{
  CURLMcode res;
  AICapabilityType capability_type;
  bool event_poll;
  AIPerServicePtr per_service;
  {
	AICurlEasyRequest_wat curl_easy_request_w(**iter);
	bool downloaded_something = curl_easy_request_w->received_data();
	bool success = curl_easy_request_w->success();
	res = curl_easy_request_w->remove_handle_from_multi(curl_easy_request_w, mMultiHandle);
	capability_type = curl_easy_request_w->capability_type();
	event_poll = curl_easy_request_w->is_event_poll();
	per_service = curl_easy_request_w->getPerServicePtr();
	PerService_wat(*per_service)->removed_from_multi_handle(capability_type, event_poll, downloaded_something, success);
#ifdef SHOW_ASSERT
	curl_easy_request_w->mRemovedPerCommand = as_per_command;
#endif
  }
#if CWDEBUG
  ThreadSafeBufferedCurlEasyRequest* lockobj = iter->get_ptr().get();
#endif
  mAddedEasyRequests.erase(iter);
  --sTotalAdded;
  llassert(sTotalAdded == mAddedEasyRequests.size());
#if CWDEBUG
  Dout(dc::curl, "MultiHandle::remove_easy_request: Removed AICurlEasyRequest " << (void*)lockobj <<
	  "; now processing " << mAddedEasyRequests.size() << " easy handles [running_handles = " << AICurlInterface::Stats::running_handles << "].");
#endif
  PerService_wat(*per_service)->add_queued_to(this);
  return res;
}
void MultiHandle::check_msg_queue(void)
{
  CURLMsg const* msg;
  int msgs_left;
  while ((msg = info_read(&msgs_left)))
  {
	if (msg->msg == CURLMSG_DONE)
	{
	  CURL* easy = msg->easy_handle;
	  ThreadSafeBufferedCurlEasyRequest* ptr;
	  CURLcode rese = curl_easy_getinfo(easy, CURLINFO_PRIVATE, &ptr);
	  llassert_always(rese == CURLE_OK);
	  AICurlEasyRequest easy_request(ptr);
	  llassert(*AICurlEasyRequest_wat(*easy_request) == easy);
	  finish_easy_request(easy_request, msg->data.result);
	  CURLMcode res = remove_easy_request(easy_request);
	  llassert(res == CURLM_OK);
	  if (res == CURLM_OK)
	  {
	  }
	  else if (res == -2)
	  {
		LL_WARNS() << "Curl easy handle returned by curl_multi_info_read() that is not (anymore) in MultiHandle::mAddedEasyRequests!?!" << LL_ENDL;
	  }
	}
  }
}
void MultiHandle::finish_easy_request(AICurlEasyRequest const& easy_request, CURLcode result)
{
  AICurlEasyRequest_wat curl_easy_request_w(*easy_request);
  curl_easy_request_w->update_body_bandwidth();
  curl_easy_request_w->storeResult(result);
#ifdef CWDEBUG
  char* eff_url;
  curl_easy_request_w->getinfo(CURLINFO_EFFECTIVE_URL, &eff_url);
  double namelookup_time, connect_time, appconnect_time, pretransfer_time, starttransfer_time;
  curl_easy_request_w->getinfo(CURLINFO_NAMELOOKUP_TIME, &namelookup_time);
  curl_easy_request_w->getinfo(CURLINFO_CONNECT_TIME, &connect_time);
  curl_easy_request_w->getinfo(CURLINFO_APPCONNECT_TIME, &appconnect_time);
  curl_easy_request_w->getinfo(CURLINFO_PRETRANSFER_TIME, &pretransfer_time);
  curl_easy_request_w->getinfo(CURLINFO_STARTTRANSFER_TIME, &starttransfer_time);
  if (appconnect_time - connect_time <= 1e-6)
  {
	appconnect_time = 0;
  }
  if (connect_time - namelookup_time <= 1e-6)
  {
	connect_time = 0;
  }
  if (namelookup_time < 500e-6)
  {
	namelookup_time = 0;
  }
  Dout(dc::curl|continued_cf, "Finished: " << eff_url << " (" << curl_easy_strerror(result));
  if (result != CURLE_OK)
  {
	long os_error;
	curl_easy_request_w->getinfo(CURLINFO_OS_ERRNO, &os_error);
	if (os_error)
	{
#if WINDOWS_CODE
	  Dout(dc::continued, ": " << formatWSAError(os_error));
#else
	  Dout(dc::continued, ": " << strerror(os_error));
#endif
	}
  }
  Dout(dc::continued, "); ");
  if (namelookup_time)
  {
    Dout(dc::continued, "namelookup time: " << namelookup_time << ", ");
  }
  if (connect_time)
  {
    Dout(dc::continued, "connect_time: " << connect_time << ", ");
  }
  if (appconnect_time)
  {
	Dout(dc::continued, "appconnect_time: " << appconnect_time << ", ");
  }
  Dout(dc::finish, "pretransfer_time: " << pretransfer_time << ", starttransfer_time: " << starttransfer_time <<
	  ". [CURLINFO_PRIVATE = " << (void*)easy_request.get_ptr().get() << "]");
#endif
  curl_easy_request_w->done(curl_easy_request_w, result);
}
}
}
void AICurlMultiHandle::destroyInstance(void)
{
  LLThreadLocalData& tldata = LLThreadLocalData::tldata();
  Dout(dc::curl, "Destroying AICurlMultiHandle [" << (void*)tldata.mCurlMultiHandle << "] for thread \"" << tldata.mName << "\".");
  delete tldata.mCurlMultiHandle;
  tldata.mCurlMultiHandle = NULL;
}
AICurlMultiHandle& AICurlMultiHandle::getInstance(void)
{
  LLThreadLocalData& tldata = LLThreadLocalData::tldata();
  if (!tldata.mCurlMultiHandle)
  {
	tldata.mCurlMultiHandle = new AICurlMultiHandle;
	Dout(dc::curl, "Created AICurlMultiHandle [" << (void*)tldata.mCurlMultiHandle << "] for thread \"" << tldata.mName << "\".");
  }
  return *static_cast<AICurlMultiHandle*>(tldata.mCurlMultiHandle);
}
namespace AICurlPrivate {
bool curlThreadIsRunning(void)
{
  using curlthread::AICurlThread;
  return AICurlThread::sInstance && !AICurlThread::sInstance->isStopped();
}
void wakeUpCurlThread(void)
{
  using curlthread::AICurlThread;
  if (AICurlThread::sInstance)
	AICurlThread::sInstance->wakeup_thread();
}
void stopCurlThread(void)
{
  using curlthread::AICurlThread;
  if (AICurlThread::sInstance)
  {
	AICurlThread::sInstance->wakeup_thread(true);
	int count = 401;
	while(--count && !AICurlThread::sInstance->isStopped())
	{
	  ms_sleep(10);
	}
	if (AICurlThread::sInstance->isStopped())
	{
	  AICurlThread::sInstance->join_thread();
	}
	LL_INFOS() << "Curl thread" << (curlThreadIsRunning() ? " not" : "") << " stopped after " << ((400 - count) * 10) << "ms." << LL_ENDL;
  }
}
void clearCommandQueue(void)
{
	command_queue_wat command_queue_w(command_queue);
	command_queue_w->commands.clear();
	command_queue_w->size = 0;
}
void BufferedCurlEasyRequest::setStatusAndReason(U32 status, std::string const& reason)
{
  mStatus = status;
  mReason = reason;
  if (status >= 100 && status < 600 && (status % 100) < 20)
  {
	AICurlInterface::Stats::status_count[AICurlInterface::Stats::status2index(mStatus)]++;
  }
  if ((status >= 300 && status < 400) && mResponder && !mResponder->redirect_status_ok())
  {
	LL_ERRS() << "Received " << status << " (" << reason << ") for responder \"" << mResponder->getName() << "\" which does not allow redirection!" << LL_ENDL;
  }
}
void BufferedCurlEasyRequest::processOutput(void)
{
  U32 responseCode = 0;
  std::string responseReason;
  CURLcode code;
  AITransferInfo info;
  getResult(&code, &info);
  if (code == CURLE_OK && !is_internal_http_error(mStatus))
  {
	getinfo(CURLINFO_RESPONSE_CODE, &responseCode);
	llassert(responseCode == mStatus);
	if (responseCode == mStatus)
	  responseReason = mReason;
	else
	  responseReason = "Unknown reason.";
  }
  else
  {
	responseReason = (code == CURLE_OK) ? mReason : std::string(curl_easy_strerror(code));
	switch (code)
	{
	  case CURLE_FAILED_INIT:
		responseCode = HTTP_INTERNAL_ERROR_OTHER;
		break;
	  case CURLE_OPERATION_TIMEDOUT:
		responseCode = HTTP_INTERNAL_ERROR_CURL_TIMEOUT;
		break;
	  case CURLE_WRITE_ERROR:
		responseCode = HTTP_INTERNAL_ERROR_LOW_SPEED;
		break;
	  default:
		responseCode = HTTP_INTERNAL_ERROR_CURL_OTHER;
		break;
	}
	if (responseCode == HTTP_INTERNAL_ERROR_LOW_SPEED)
	{
		responseReason = llformat("Connection to \"%s\" stalled: download speed dropped below %u bytes/s for %u seconds (up till that point, %s received a total of %lu bytes). "
			"To change these values, go to Advanced --> Debug Settings and change CurlTimeoutLowSpeedLimit and CurlTimeoutLowSpeedTime respectively.",
			mResponder->getURL().c_str(), mResponder->getHTTPTimeoutPolicy().getLowSpeedLimit(), mResponder->getHTTPTimeoutPolicy().getLowSpeedTime(),
			mResponder->getName(), mTotalRawBytes);
	}
	setopt(CURLOPT_FRESH_CONNECT, TRUE);
  }
  if (code != CURLE_OK)
  {
	print_diagnostics(code);
  }
  sResponderCallbackMutex.lock();
  if (!sShuttingDown)
  {
	if (mBufferEventsTarget)
	{
	  llassert(mBufferEventsTarget == mResponder.get());
	  mBufferEventsTarget->completed_headers(responseCode, responseReason, (code == CURLE_FAILED_INIT) ? NULL : &info);
	}
	mResponder->finished(code, responseCode, responseReason, sChannels, mOutput);
  }
  sResponderCallbackMutex.unlock();
  mResponder = NULL;
}
void BufferedCurlEasyRequest::shutdown(void)
{
  sResponderCallbackMutex.lock();
  sShuttingDown = true;
  sResponderCallbackMutex.unlock();
}
void BufferedCurlEasyRequest::received_HTTP_header(void)
{
  if (mBufferEventsTarget)
	mBufferEventsTarget->received_HTTP_header();
}
void BufferedCurlEasyRequest::received_header(std::string const& key, std::string const& value)
{
  if (mBufferEventsTarget)
	mBufferEventsTarget->received_header(key, value);
}
void BufferedCurlEasyRequest::completed_headers(U32 status, std::string const& reason, AITransferInfo* info)
{
  if (mBufferEventsTarget)
	mBufferEventsTarget->completed_headers(status, reason, info);
}
size_t BufferedCurlEasyRequest::curlWriteCallback(char* data, size_t size, size_t nmemb, void* user_data)
{
  ThreadSafeBufferedCurlEasyRequest* lockobj = static_cast<ThreadSafeBufferedCurlEasyRequest*>(user_data);
  AICurlEasyRequest_wat self_w(*lockobj);
  S32 bytes = size * nmemb;
  self_w->getOutput()->append(sChannels.in(), (U8 const*)data, bytes);
  self_w->update_body_bandwidth();
  if (self_w->httptimeout()->data_received(bytes))
  {
	return 0;
  }
  return bytes;
}
void BufferedCurlEasyRequest::update_body_bandwidth(void)
{
  double size_download;
  getinfo(CURLINFO_SIZE_DOWNLOAD, &size_download);
  size_t total_raw_bytes = size_download;
  size_t raw_bytes = total_raw_bytes - mTotalRawBytes;
  if (mTotalRawBytes == 0 && total_raw_bytes > 0)
  {
	PerService_wat per_service_w(*mPerServicePtr);
	per_service_w->download_started(mCapabilityType);
  }
  mTotalRawBytes = total_raw_bytes;
  if (raw_bytes > 0)
  {
	U64 const sTime_40ms = curlthread::HTTPTimeout::sTime_10ms >> 2;
	AIAverage& http_bandwidth(PerService_wat(*getPerServicePtr())->bandwidth());
	http_bandwidth.addData(raw_bytes, sTime_40ms);
	sHTTPBandwidth.addData(raw_bytes, sTime_40ms);
  }
}
size_t BufferedCurlEasyRequest::curlReadCallback(char* data, size_t size, size_t nmemb, void* user_data)
{
  ThreadSafeBufferedCurlEasyRequest* lockobj = static_cast<ThreadSafeBufferedCurlEasyRequest*>(user_data);
  AICurlEasyRequest_wat self_w(*lockobj);
  S32 bytes = size * nmemb;
  self_w->mLastRead = self_w->getInput()->readAfter(sChannels.out(), self_w->mLastRead, (U8*)data, bytes);
  self_w->mRequestTransferedBytes += bytes;
  llassert(self_w->mRequestTransferedBytes <= self_w->mContentLength);
  if (self_w->httptimeout()->data_sent(bytes, self_w->mRequestTransferedBytes >= self_w->mContentLength))
  {
	return CURL_READFUNC_ABORT;
  }
  return bytes;
}
size_t BufferedCurlEasyRequest::curlHeaderCallback(char* data, size_t size, size_t nmemb, void* user_data)
{
  ThreadSafeBufferedCurlEasyRequest* lockobj = static_cast<ThreadSafeBufferedCurlEasyRequest*>(user_data);
  AICurlEasyRequest_wat self_w(*lockobj);
  char const* const header_line = static_cast<char const*>(data);
  size_t const header_len = size * nmemb;
  if (!header_len)
  {
	return header_len;
  }
  std::string header(header_line, header_len);
  bool being_redirected = false;
  bool done = false;
  if (!LLStringUtil::_isASCII(header))
  {
	done = true;
  }
  else if (header.substr(0, 5) == "HTTP/")
  {
	std::string::iterator const begin = header.begin();
	std::string::iterator const end = header.end();
	std::string::iterator pos1 = std::find(begin, end, ' ');
	if (pos1 != end) ++pos1;
	std::string::iterator pos2 = std::find(pos1, end, ' ');
	if (pos2 != end) ++pos2;
	std::string::iterator pos3 = std::find(pos2, end, '\r');
	U32 status = 0;
	std::string reason;
	if (pos3 != end && LLStringOps::isDigit(*pos1))
	{
	  status = atoi(&header_line[pos1 - begin]);
	  reason.assign(pos2, pos3);
	}
	if (!(status >= 100 && status < 600 && (status % 100) < 20))
	{
	  if (status == 0)
	  {
		reason = "Header parse error.";
		LL_WARNS() << "Received broken header line from server: \"" << header << "\"" << LL_ENDL;
	  }
	  else
	  {
		LL_WARNS() << "Received unexpected status value from server (" << status << "): \"" << header << "\"" << LL_ENDL;
	  }
	  if (!status) status = HTTP_INTERNAL_ERROR_OTHER;
	}
	self_w->received_HTTP_header();
	self_w->setStatusAndReason(status, reason);
	done = true;
	if (status >= 300 && status < 400)
	{
	  being_redirected = true;
	}
  }
  U64 const sTime_40ms = curlthread::HTTPTimeout::sTime_10ms >> 2;
  AIAverage& http_bandwidth(PerService_wat(*self_w->getPerServicePtr())->bandwidth());
  http_bandwidth.addData(header_len, sTime_40ms);
  sHTTPBandwidth.addData(header_len, sTime_40ms);
  if (self_w->httptimeout()->data_received(header_len ASSERT_ONLY_COMMA(self_w->upload_error_status())))
  {
	return 0;
  }
  if (being_redirected)
  {
	  self_w->httptimeout()->being_redirected();
  }
  if (done)
  {
	return header_len;
  }
  std::string::iterator sep = std::find(header.begin(), header.end(), ':');
  if (sep != header.end())
  {
	std::string key(header.begin(), sep);
	std::string value(sep + 1, header.end());
	key = utf8str_tolower(utf8str_trim(key));
	value = utf8str_trim(value);
	self_w->received_header(key, value);
  }
  else
  {
	LLStringUtil::trim(header);
	if (!header.empty())
	{
	  LL_WARNS() << "Unable to parse header: " << header << LL_ENDL;
	}
  }
  return header_len;
}
int BufferedCurlEasyRequest::curlProgressCallback(void* user_data, double dltotal, double dlnow, double ultotal, double ulnow)
{
  if (ultotal > 0)
  {
	ThreadSafeBufferedCurlEasyRequest* lockobj = static_cast<ThreadSafeBufferedCurlEasyRequest*>(user_data);
	DoutEntering(dc::curl, "BufferedCurlEasyRequest::curlProgressCallback(" << (void*)lockobj << ", " << dltotal << ", " << dlnow << ", " << ultotal << ", " << ulnow << ")");
	if (ulnow == ultotal)
	{
	  AICurlEasyRequest_wat self_w(*lockobj);
	  self_w->httptimeout()->upload_finished();
	}
  }
  return 0;
}
#ifdef CWDEBUG
int debug_callback(CURL* handle, curl_infotype infotype, char* buf, size_t size, void* user_ptr)
{
  BufferedCurlEasyRequest* request = (BufferedCurlEasyRequest*)user_ptr;
  if (infotype == CURLINFO_HEADER_OUT && size >= 5 && (strncmp(buf, "GET ", 4) == 0 || strncmp(buf, "HEAD ", 5) == 0))
  {
	request->mDebugIsHeadOrGetMethod = true;
  }
  if (infotype == CURLINFO_TEXT)
  {
	if (!strncmp(buf, "STATE: WAITCONNECT => ", 22))
	{
	  if (buf[22] == 'P' || buf[22] == 'D')
	  {
		int n = size - 1;
		while (buf[n] != ')')
		{
		  llassert(n > 56);
		  --n;
		}
		int connectionnr = 0;
		int factor = 1;
		do
		{
		  llassert(n > 56);
		  --n;
		  connectionnr += factor * (buf[n] - '0');
		  factor *= 10;
		}
		while(buf[n - 1] != '#');
		request->connection_established(connectionnr);
	  }
	  else
	  {
	  	llassert(buf[22] == 'C');
	  }
	}
	else if (!strncmp(buf, "Closing connection", 18))
	{
	  int n = size - 1;
	  while (!std::isdigit(buf[n]))
	  {
		llassert(n > 20);
		--n;
	  }
	  int connectionnr = 0;
	  int factor = 1;
	  do
	  {
		llassert(n > 19);
		connectionnr += factor * (buf[n] - '0');
		factor *= 10;
		--n;
	  }
	  while(buf[n] != '#');
	  request->connection_closed(connectionnr);
	}
  }
#ifdef DEBUG_CURLIO
  if (!debug_curl_print_debug(handle))
  {
	return 0;
  }
#endif
  using namespace ::libcwd;
  std::ostringstream marker;
  marker << (void*)request->get_lockobj() << ' ';
  libcw_do.push_marker();
  libcw_do.marker().assign(marker.str().data(), marker.str().size());
  if (!debug::channels::dc::curlio.is_on())
	debug::channels::dc::curlio.on();
  LibcwDoutScopeBegin(LIBCWD_DEBUGCHANNELS, libcw_do, dc::curlio|cond_nonewline_cf(infotype == CURLINFO_TEXT))
  switch (infotype)
  {
	case CURLINFO_TEXT:
	  LibcwDoutStream << "* ";
	  break;
	case CURLINFO_HEADER_IN:
	  LibcwDoutStream << "H> ";
	  break;
	case CURLINFO_HEADER_OUT:
	  LibcwDoutStream << "H< ";
	  break;
	case CURLINFO_DATA_IN:
	  LibcwDoutStream << "D> ";
	  break;
	case CURLINFO_DATA_OUT:
	  LibcwDoutStream << "D< ";
	  break;
	case CURLINFO_SSL_DATA_IN:
	  LibcwDoutStream << "S> ";
	  break;
	case CURLINFO_SSL_DATA_OUT:
	  LibcwDoutStream << "S< ";
	  break;
	default:
	  LibcwDoutStream << "?? ";
  }
  if (infotype == CURLINFO_TEXT)
	LibcwDoutStream.write(buf, size);
  else if (infotype == CURLINFO_HEADER_IN || infotype == CURLINFO_HEADER_OUT)
	LibcwDoutStream << libcwd::buf2str(buf, size);
  else if (infotype == CURLINFO_DATA_IN)
  {
	LibcwDoutStream << size << " bytes";
	bool finished = false;
	size_t i = 0;
	while (i < size)
	{
	  char c = buf[i];
	  if (!('0' <= c && c <= '9') && !('a' <= c && c <= 'f'))
	  {
		if (0 < i && i + 1 < size && buf[i] == '\r' && buf[i + 1] == '\n')
		{
		  LibcwDoutStream << ": \"" << libcwd::buf2str(buf, i + 2) << "\"...";
		  finished = true;
		}
		break;
	  }
	  ++i;
	}
	if (!finished && size > 9 && buf[0] == '<')
	{
	  if (!strncmp(buf, "<!DOCTYPE", 9) || !strncmp(buf, "<?xml", 5) || !strncmp(buf, "<llsd>", 6))
	  {
		LibcwDoutStream << ": \"" << libcwd::buf2str(buf, size) << '"';
		finished = true;
	  }
	}
	if (!finished)
	{
	  if (size > 40UL)
	  {
		LibcwDoutStream << ": \"" << libcwd::buf2str(buf, 20) << "\"...\"" << libcwd::buf2str(&buf[size - 20], 20) << '"';
	  }
	  else
	  {
		LibcwDoutStream << ": \"" << libcwd::buf2str(buf, size) << '"';
	  }
	}
  }
  else if (infotype == CURLINFO_DATA_OUT)
	LibcwDoutStream << size << " bytes: \"" << libcwd::buf2str(buf, size) << '"';
  else
	LibcwDoutStream << size << " bytes";
  LibcwDoutScopeEnd;
  libcw_do.pop_marker();
  return 0;
}
#endif
}
void AICurlEasyRequest::addRequest(void)
{
  using namespace AICurlPrivate;
  {
	command_queue_wat command_queue_w(command_queue);
#ifdef SHOW_ASSERT
	command_st cmd = cmd_none;
	for (std::deque<Command>::iterator iter = command_queue_w->commands.begin(); iter != command_queue_w->commands.end(); ++iter)
	{
	  if (*iter == *this)
	  {
		cmd = iter->command();
		break;
	  }
	}
	llassert(cmd == cmd_none || cmd == cmd_remove);
	if (cmd == cmd_none)
	{
	  command_being_processed_rat command_being_processed_r(command_being_processed);
	  if (*command_being_processed_r == *this)
	  {
		llassert(command_being_processed_r->command() == cmd_remove);
	  }
	  else
	  {
		llassert(!AICurlEasyRequest_wat(*get())->active());
	  }
	}
#endif
	command_queue_w->commands.push_back(Command(*this, cmd_add));
	command_queue_w->size++;
	AICurlEasyRequest_wat curl_easy_request_w(*get());
	PerService_wat(*curl_easy_request_w->getPerServicePtr())->added_to_command_queue(curl_easy_request_w->capability_type());
	curl_easy_request_w->add_queued();
  }
  wakeUpCurlThread();
}
void AICurlEasyRequest::removeRequest(void)
{
  using namespace AICurlPrivate;
  {
	command_queue_wat command_queue_w(command_queue);
#ifdef SHOW_ASSERT
	command_st cmd = cmd_none;
	for (std::deque<Command>::iterator iter = command_queue_w->commands.begin(); iter != command_queue_w->commands.end(); ++iter)
	{
	  if (*iter == *this)
	  {
		cmd = iter->command();
		break;
	  }
	}
	llassert(cmd == cmd_none || cmd != cmd_remove);
	if (cmd == cmd_none)
	{
	  command_being_processed_rat command_being_processed_r(command_being_processed);
	  if (*command_being_processed_r == *this)
	  {
		llassert(command_being_processed_r->command() != cmd_remove);
	  }
	  else
	  {
		{
		  AICurlEasyRequest_wat curl_easy_request_w(*get());
		  llassert(curl_easy_request_w->active() || !curl_easy_request_w->mRemovedPerCommand);
		}
	  }
	}
	{
	  AICurlEasyRequest_wat curl_easy_request_w(*get());
	  curl_easy_request_w->queued_for_removal(curl_easy_request_w);
	}
#endif
	command_queue_w->commands.push_back(Command(*this, cmd_remove));
	command_queue_w->size--;
	AICurlEasyRequest_wat curl_easy_request_w(*get());
	PerService_wat(*curl_easy_request_w->getPerServicePtr())->removed_from_command_queue(curl_easy_request_w->capability_type());
	curl_easy_request_w->remove_queued();
  }
  wakeUpCurlThread();
}
namespace AICurlInterface {
LLControlGroup* sConfigGroup;
void startCurlThread(LLControlGroup* control_group)
{
  using namespace AICurlPrivate;
  using namespace AICurlPrivate::curlthread;
  llassert(is_main_thread());
  sConfigGroup = control_group;
  curl_max_total_concurrent_connections = sConfigGroup->getU32("CurlMaxTotalConcurrentConnections");
  CurlConcurrentConnectionsPerService = (U16)sConfigGroup->getU32("CurlConcurrentConnectionsPerService");
  gNoVerifySSLCert = sConfigGroup->getBOOL("NoVerifySSLCert");
  AIPerService::setMaxPipelinedRequests(curl_max_total_concurrent_connections);
  AIPerService::setHTTPThrottleBandwidth(sConfigGroup->getF32("HTTPThrottleBandwidth"));
  AICurlThread::sInstance = new AICurlThread;
  AICurlThread::sInstance->start();
}
bool handleCurlMaxTotalConcurrentConnections(LLSD const& newvalue)
{
  using namespace AICurlPrivate;
  using namespace AICurlPrivate::curlthread;
  U32 old = curl_max_total_concurrent_connections;
  curl_max_total_concurrent_connections = newvalue.asInteger();
  AIPerService::incrementMaxPipelinedRequests(curl_max_total_concurrent_connections - old);
  LL_INFOS() << "CurlMaxTotalConcurrentConnections set to " << curl_max_total_concurrent_connections << LL_ENDL;
  return true;
}
bool handleCurlConcurrentConnectionsPerService(LLSD const& newvalue)
{
  using namespace AICurlPrivate;
  U16 new_concurrent_connections = (U16)newvalue.asInteger();
  U16 const maxCurlConcurrentConnectionsPerService = 32;
  if (new_concurrent_connections < 1 || new_concurrent_connections > maxCurlConcurrentConnectionsPerService)
  {
	sConfigGroup->setU32("CurlConcurrentConnectionsPerService", static_cast<U32>((new_concurrent_connections < 1) ? 1 : maxCurlConcurrentConnectionsPerService));
  }
  else
  {
	int increment = new_concurrent_connections - CurlConcurrentConnectionsPerService;
	CurlConcurrentConnectionsPerService = new_concurrent_connections;
	AIPerService::adjust_concurrent_connections(increment);
	LL_INFOS() << "CurlConcurrentConnectionsPerService set to " << CurlConcurrentConnectionsPerService << LL_ENDL;
  }
  return true;
}
bool handleNoVerifySSLCert(LLSD const& newvalue)
{
  gNoVerifySSLCert = newvalue.asBoolean();
  return true;
}
U32 getNumHTTPCommands(void)
{
  using namespace AICurlPrivate;
  command_queue_rat command_queue_r(command_queue);
  return command_queue_r->size;
}
U32 getNumHTTPQueued(void)
{
  return AIPerService::total_approved_queue_size();
}
U32 getNumHTTPAdded(void)
{
  return AICurlPrivate::curlthread::MultiHandle::total_added_size();
}
U32 getMaxHTTPAdded(void)
{
  return AICurlPrivate::curlthread::curl_max_total_concurrent_connections;
}
size_t getHTTPBandwidth(void)
{
  using namespace AICurlPrivate;
  U64 const sTime_40ms = get_clock_count() * curlthread::HTTPTimeout::sClockWidth_40ms;
  return BufferedCurlEasyRequest::sHTTPBandwidth.truncateData(sTime_40ms);
}
}
AIThreadSafeSimpleDC<AIPerService::MaxPipelinedRequests> AIPerService::sMaxPipelinedRequests;
AIThreadSafeSimpleDC<AIPerService::ThrottleFraction> AIPerService::sThrottleFraction;
LLAtomicU32 AIPerService::sHTTPThrottleBandwidth125(250000);
bool AIPerService::sNoHTTPBandwidthThrottling;
AIPerService::Approvement* AIPerService::approveHTTPRequestFor(AIPerServicePtr const& per_service, AICapabilityType capability_type)
{
  using namespace AICurlPrivate;
  using namespace AICurlPrivate::curlthread;
  U64 const sTime_40ms = get_clock_count() * HTTPTimeout::sClockWidth_40ms;
  bool starvation, decrement_threshold;
  S32 total_approved_queuedapproved_or_added = MultiHandle::total_added_size();
  {
	TotalQueued_wat total_queued_w(sTotalQueued);
	total_approved_queuedapproved_or_added += total_queued_w->approved;
	starvation = total_queued_w->starvation;
	decrement_threshold = total_queued_w->full && !total_queued_w->empty;
	total_queued_w->starvation = total_queued_w->empty = total_queued_w->full = false;
  }
  if (decrement_threshold)
  {
	MaxPipelinedRequests_wat max_pipelined_requests_w(sMaxPipelinedRequests);
	if (max_pipelined_requests_w->threshold > (S32)curl_max_total_concurrent_connections &&
		sTime_40ms > max_pipelined_requests_w->last_decrement)
	{
	  max_pipelined_requests_w->threshold--;
	  max_pipelined_requests_w->last_decrement = sTime_40ms;
	}
  }
  bool reject, equal, increment_threshold;
  {
	PerService_wat per_service_w(*per_service);
	CapabilityType& ct(per_service_w->mCapabilityType[capability_type]);
	S32 const pipelined_requests_per_capability_type = ct.pipelined_requests();
	reject = pipelined_requests_per_capability_type >= (S32)ct.mMaxPipelinedRequests;
	equal = pipelined_requests_per_capability_type == ct.mMaxPipelinedRequests;
	increment_threshold = ct.mFlags & ctf_starvation;
	decrement_threshold = (ct.mFlags & (ctf_empty | ctf_full)) == ctf_full;
	ct.mFlags &= ~(ctf_empty|ctf_full|ctf_starvation);
	if (decrement_threshold)
	{
	  if ((int)ct.mMaxPipelinedRequests > ct.mConcurrentConnections)
	  {
		ct.mMaxPipelinedRequests--;
	  }
	}
	else if (increment_threshold && reject)
	{
	  if ((int)ct.mMaxPipelinedRequests < 2 * ct.mConcurrentConnections)
	  {
		ct.mMaxPipelinedRequests++;
		reject = !equal;
	  }
	}
	if (!reject)
	{
	  ct.mApprovedRequests++;
	  per_service_w->mApprovedRequests++;
	}
	total_approved_queuedapproved_or_added += per_service_w->mApprovedRequests;
  }
  if (reject)
  {
	return NULL;
  }
  if (checkBandwidthUsage(per_service, sTime_40ms))
  {
	PerService_wat per_service_w(*per_service);
	per_service_w->mCapabilityType[capability_type].mApprovedRequests--;
	per_service_w->mApprovedRequests--;
	return NULL;
  }
  S32 const pipelined_requests = command_queue_rat(command_queue)->size + total_approved_queuedapproved_or_added;
  MaxPipelinedRequests_wat max_pipelined_requests_w(sMaxPipelinedRequests);
  reject = pipelined_requests >= max_pipelined_requests_w->threshold;
  equal = pipelined_requests == max_pipelined_requests_w->threshold;
  increment_threshold = starvation;
  if (increment_threshold && reject)
  {
	if (max_pipelined_requests_w->threshold < 2 * (S32)curl_max_total_concurrent_connections &&
		sTime_40ms > max_pipelined_requests_w->last_increment)
	{
	  max_pipelined_requests_w->threshold++;
	  max_pipelined_requests_w->last_increment = sTime_40ms;
	  reject = !equal;
	}
  }
  if (reject)
  {
	PerService_wat per_service_w(*per_service);
	per_service_w->mCapabilityType[capability_type].mApprovedRequests--;
	per_service_w->mApprovedRequests--;
	return NULL;
  }
  return new Approvement(per_service, capability_type);
}
bool AIPerService::checkBandwidthUsage(AIPerServicePtr const& per_service, U64 sTime_40ms)
{
  if (sNoHTTPBandwidthThrottling)
	return false;
  using namespace AICurlPrivate;
  size_t const max_bandwidth = AIPerService::getHTTPThrottleBandwidth125();
  size_t const total_bandwidth = BufferedCurlEasyRequest::sHTTPBandwidth.truncateData(sTime_40ms);
  size_t const service_bandwidth = PerService_wat(*per_service)->bandwidth().truncateData(sTime_40ms);
  ThrottleFraction_wat throttle_fraction_w(sThrottleFraction);
  if (sTime_40ms > throttle_fraction_w->last_add)
  {
	throttle_fraction_w->average.addData(throttle_fraction_w->fraction, sTime_40ms);
	throttle_fraction_w->last_add = sTime_40ms;
  }
  double fraction_avg = throttle_fraction_w->average.getAverage(1024.0);
  if (total_bandwidth == 0)
	throttle_fraction_w->fraction = 1024;
  else
  {
	throttle_fraction_w->fraction = llmin(1024., fraction_avg * max_bandwidth / total_bandwidth + 0.5);
  }
  if (total_bandwidth > max_bandwidth)
  {
	throttle_fraction_w->fraction *= 0.95;
  }
  return (service_bandwidth > (max_bandwidth * throttle_fraction_w->fraction / 1024));
}
