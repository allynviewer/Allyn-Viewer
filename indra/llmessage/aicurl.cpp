/**
 * @file aicurl.cpp
 * @brief Implementation of AICurl.
 *
 * Copyright (c) 2012, Aleric Inglewood.
 * Copyright (C) 2010, Linden Research, Inc.
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
 *   17/03/2012
 *   Initial version, written by Aleric Inglewood @ SL
 *
 *   20/03/2012
 *   Added copyright notice for Linden Lab for those parts that were
 *   copied or derived from llcurl.cpp. The code of those parts are
 *   already in their own llcurl.cpp, so they do not ever need to
 *   even look at this file; the reason I added the copyright notice
 *   is to make clear that I am not the author of 100% of this code
 *   and hence I cannot change the license of it.
 */
#include "linden_common.h"
#if LL_WINDOWS
#include <winsock2.h>
#endif
#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include "aicurl.h"
#include "llbufferstream.h"
#include "llsdserialize.h"
#include "aithreadsafe.h"
#include "llqueuedthread.h"
#include "llproxy.h"
#include "llhttpstatuscodes.h"
#include "aihttpheaders.h"
#include "aihttptimeoutpolicy.h"
#include "aicurleasyrequeststatemachine.h"
#include "aicurlperservice.h"
bool gNoVerifySSLCert;
namespace {
struct CertificateAuthority {
  std::string file;
  std::string path;
};
AIThreadSafeSimpleDC<CertificateAuthority> gCertificateAuthority;
typedef AIAccess<CertificateAuthority> CertificateAuthority_wat;
typedef AIAccessConst<CertificateAuthority> CertificateAuthority_rat;
enum gSSLlib_type {
  ssl_unknown,
  ssl_openssl,
  ssl_gnutls,
  ssl_nss
};
gSSLlib_type gSSLlib;
bool gSetoptParamsNeedDup;
}
#define HAVE_CRYPTO_THREADID (OPENSSL_VERSION_NUMBER >= (1 << 28))
struct CRYPTO_dynlock_value
{
  AIRWLock rwlock;
};
namespace {
AIRWLock* ssl_rwlock_array;
void ssl_locking_function(int mode, int n, char const* file, int line)
{
  if ((mode & CRYPTO_LOCK))
  {
	if ((mode & CRYPTO_READ))
	  ssl_rwlock_array[n].rdlock();
	else
	  ssl_rwlock_array[n].wrlock();
  }
  else
  {
    if ((mode & CRYPTO_READ))
	  ssl_rwlock_array[n].rdunlock();
	else
	  ssl_rwlock_array[n].wrunlock();
  }
}
#if HAVE_CRYPTO_THREADID
void ssl_id_function(CRYPTO_THREADID* thread_id)
{
#if LL_WINDOWS || LL_DARWIN
  CRYPTO_THREADID_set_pointer(thread_id, apr_os_thread_current());
#else
  CRYPTO_THREADID_set_numeric(thread_id, apr_os_thread_current());
#endif
}
#endif
CRYPTO_dynlock_value* ssl_dyn_create_function(char const* file, int line)
{
  return new CRYPTO_dynlock_value;
}
void ssl_dyn_destroy_function(CRYPTO_dynlock_value* l, char const* file, int line)
{
  delete l;
}
void ssl_dyn_lock_function(int mode, CRYPTO_dynlock_value* l, char const* file, int line)
{
  if ((mode & CRYPTO_LOCK))
  {
	if ((mode & CRYPTO_READ))
	  l->rwlock.rdlock();
	else
	  l->rwlock.wrlock();
  }
  else
  {
    if ((mode & CRYPTO_READ))
	  l->rwlock.rdunlock();
	else
	  l->rwlock.wrunlock();
  }
}
typedef void (*ssl_locking_function_type)(int, int, char const*, int);
#if HAVE_CRYPTO_THREADID
typedef void (*ssl_id_function_type)(CRYPTO_THREADID*);
#else
typedef unsigned long (*ulong_thread_id_function_type)(void);
#endif
typedef CRYPTO_dynlock_value* (*ssl_dyn_create_function_type)(char const*, int);
typedef void (*ssl_dyn_destroy_function_type)(CRYPTO_dynlock_value*, char const*, int);
typedef void (*ssl_dyn_lock_function_type)(int, CRYPTO_dynlock_value*, char const*, int);
ssl_locking_function_type     old_ssl_locking_function;
#if HAVE_CRYPTO_THREADID
ssl_id_function_type          old_ssl_id_function;
#else
ulong_thread_id_function_type old_ulong_thread_id_function;
#endif
ssl_dyn_create_function_type  old_ssl_dyn_create_function;
ssl_dyn_destroy_function_type old_ssl_dyn_destroy_function;
ssl_dyn_lock_function_type    old_ssl_dyn_lock_function;
#if LL_WINDOWS && !HAVE_CRYPTO_THREADID
static unsigned long __cdecl apr_os_thread_current_wrapper()
{
	return (unsigned long)(HANDLE)apr_os_thread_current();
}
#endif
static bool need_renegotiation_hack = false;
void ssl_init(void)
{
  int const compiled_openSSL_major = (OPENSSL_VERSION_NUMBER >> 28) & 0xff;
  int const compiled_openSSL_minor = (OPENSSL_VERSION_NUMBER >> 20) & 0xff;
  unsigned long const ssleay = SSLeay();
  int const linked_openSSL_major = (ssleay >> 28) & 0xff;
  int const linked_openSSL_minor = (ssleay >> 20) & 0xff;
  if (linked_openSSL_major != compiled_openSSL_major ||
	  (linked_openSSL_major == 0 && linked_openSSL_minor != compiled_openSSL_minor))
  {
	LL_ERRS() << "The viewer was compiled against " << OPENSSL_VERSION_TEXT <<
	    " but linked against " << SSLeay_version(SSLEAY_VERSION) <<
		". Those versions are not compatible." << LL_ENDL;
  }
  ssl_rwlock_array = new AIRWLock[CRYPTO_num_locks()];
  old_ssl_locking_function = CRYPTO_get_locking_callback();
#if HAVE_CRYPTO_THREADID
  old_ssl_id_function = CRYPTO_THREADID_get_callback();
#else
  old_ulong_thread_id_function = CRYPTO_get_id_callback();
#endif
  CRYPTO_set_locking_callback(&ssl_locking_function);
#if HAVE_CRYPTO_THREADID
  CRYPTO_THREADID_set_callback(&ssl_id_function);
#else
#if LL_WINDOWS
  CRYPTO_set_id_callback(&apr_os_thread_current_wrapper);
#else
  CRYPTO_set_id_callback(&apr_os_thread_current);
#endif
#endif
  old_ssl_dyn_create_function = CRYPTO_get_dynlock_create_callback();
  old_ssl_dyn_lock_function = CRYPTO_get_dynlock_lock_callback();
  old_ssl_dyn_destroy_function = CRYPTO_get_dynlock_destroy_callback();
  CRYPTO_set_dynlock_create_callback(&ssl_dyn_create_function);
  CRYPTO_set_dynlock_lock_callback(&ssl_dyn_lock_function);
  CRYPTO_set_dynlock_destroy_callback(&ssl_dyn_destroy_function);
  need_renegotiation_hack = (0x10001000UL <= ssleay);
  LL_INFOS() << "Successful initialization of " <<
	  SSLeay_version(SSLEAY_VERSION) << " (0x" << std::hex << SSLeay() << std::dec << ")." << LL_ENDL;
}
void ssl_cleanup(void)
{
  CRYPTO_set_dynlock_destroy_callback(old_ssl_dyn_destroy_function);
  CRYPTO_set_dynlock_lock_callback(old_ssl_dyn_lock_function);
  CRYPTO_set_dynlock_create_callback(old_ssl_dyn_create_function);
#if HAVE_CRYPTO_THREADID
  CRYPTO_THREADID_set_callback(old_ssl_id_function);
#else
  CRYPTO_set_id_callback(old_ulong_thread_id_function);
#endif
  CRYPTO_set_locking_callback(old_ssl_locking_function);
  delete [] ssl_rwlock_array;
}
}
static unsigned int encoded_version(int major, int minor, int patch)
{
  return (major << 16) | (minor << 8) | patch;
}
#undef AICurlPrivate
namespace AICurlInterface {
LLAtomicU32 Stats::easy_calls;
LLAtomicU32 Stats::easy_errors;
LLAtomicU32 Stats::easy_init_calls;
LLAtomicU32 Stats::easy_init_errors;
LLAtomicU32 Stats::easy_cleanup_calls;
LLAtomicU32 Stats::multi_calls;
LLAtomicU32 Stats::multi_errors;
LLAtomicU32 Stats::running_handles;
LLAtomicU32 Stats::AICurlEasyRequest_count;
LLAtomicU32 Stats::AICurlEasyRequestStateMachine_count;
LLAtomicU32 Stats::BufferedCurlEasyRequest_count;
LLAtomicU32 Stats::ResponderBase_count;
LLAtomicU32 Stats::ThreadSafeBufferedCurlEasyRequest_count;
LLAtomicU32 Stats::status_count[100];
LLAtomicU32 Stats::llsd_body_count;
LLAtomicU32 Stats::llsd_body_parse_error;
LLAtomicU32 Stats::raw_body_count;
U32 Stats::status2index(U32 status)
{
  return (status - 100) / 100 * 20 + status % 100;
}
U32 Stats::index2status(U32 index)
{
  return 100 + (index / 20) * 100 + index % 20;
}
void initCurl(void)
{
  DoutEntering(dc::curl, "AICurlInterface::initCurl()");
  llassert(LLThread::getRunning() == 0);
  CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
  if (res != CURLE_OK)
  {
	LL_ERRS() << "curl_global_init(CURL_GLOBAL_ALL) failed." << LL_ENDL;
  }
  {
	curl_version_info_data* version_info = curl_version_info(CURLVERSION_NOW);
	llassert_always(version_info->age >= 0);
	if (version_info->age < 1)
	{
	  LL_WARNS() << "libcurl's age is 0; no ares support." << LL_ENDL;
	}
	llassert_always((version_info->features & CURL_VERSION_SSL));
	if (!(version_info->features & CURL_VERSION_ASYNCHDNS))
	{
	  LL_WARNS() << "libcurl was not compiled with support for asynchronous name lookups!" << LL_ENDL;
	}
	if (!version_info->ssl_version)
	{
	  LL_ERRS() << "This libcurl has no SSL support!" << LL_ENDL;
	}
	LL_INFOS() << "Successful initialization of libcurl " <<
		version_info->version << " (0x" << std::hex << version_info->version_num << std::dec << "), (" <<
	    version_info->ssl_version;
	if (version_info->libz_version)
	{
	  LL_CONT << ", libz/" << version_info->libz_version;
	}
	LL_CONT << ")." << LL_ENDL;
	gSSLlib = ssl_unknown;
	std::string ssl_version(version_info->ssl_version);
	if (ssl_version.find("OpenSSL") != std::string::npos)
	  gSSLlib = ssl_openssl;
	else if (ssl_version.find("GnuTLS") != std::string::npos)
	  gSSLlib = ssl_gnutls;
	else if (ssl_version.find("NSS") != std::string::npos)
	  gSSLlib = ssl_nss;
	switch (gSSLlib)
	{
	  case ssl_unknown:
	  {
		LL_ERRS() << "Unknown SSL library \"" << version_info->ssl_version << "\", required actions for thread-safe handling are unknown! Bailing out." << LL_ENDL;
	  }
	  case ssl_openssl:
	  {
#ifndef OPENSSL_THREADS
		LL_ERRS() << "OpenSSL was not configured with thread support! Bailing out." << LL_ENDL;
#endif
		ssl_init();
	  }
	  case ssl_gnutls:
	  {
		break;
	  }
	  case ssl_nss:
	  {
	    break;
	  }
	}
	gSetoptParamsNeedDup = (version_info->version_num < encoded_version(7, 17, 0));
	if (gSetoptParamsNeedDup)
	{
	  LL_WARNS() << "Your libcurl version is too old." << LL_ENDL;
	}
	llassert_always(!gSetoptParamsNeedDup);
  }
}
void shutdownCurl(void)
{
  using namespace AICurlPrivate;
  DoutEntering(dc::curl, "AICurlInterface::shutdownCurl()");
  BufferedCurlEasyRequest::shutdown();
}
void cleanupCurl(void)
{
  using namespace AICurlPrivate;
  DoutEntering(dc::curl, "AICurlInterface::cleanupCurl()");
  stopCurlThread();
  if (CurlMultiHandle::getTotalMultiHandles() != 0)
	LL_WARNS() << "Not all CurlMultiHandle objects were destroyed!" << LL_ENDL;
  gMainThreadEngine.flush();
  gStateMachineThreadEngine.flush();
  clearCommandQueue();
  Stats::print();
  ssl_cleanup();
  llassert(LLThread::getRunning() <= (curlThreadIsRunning() ? 1 : 0));
  curl_global_cleanup();
}
std::string getVersionString(void)
{
  return curl_version();
}
void setCAFile(std::string const& file)
{
  CertificateAuthority_wat CertificateAuthority_w(gCertificateAuthority);
  CertificateAuthority_w->file = file;
}
void setCAPath(std::string const& path)
{
  CertificateAuthority_wat CertificateAuthority_w(gCertificateAuthority);
  CertificateAuthority_w->path = path;
}
U32 getNumHTTPRunning(void)
{
  return Stats::running_handles;
}
void Stats::print(void)
{
  int const easy_handles = easy_init_calls - easy_init_errors - easy_cleanup_calls;
  LL_INFOS_NF() << "============ CURL  STATS ============" << LL_ENDL;
  LL_INFOS_NF() << "  Curl multi       errors/calls      : " << std::dec << multi_errors << "/" << multi_calls << LL_ENDL;
  LL_INFOS_NF() << "  Curl easy        errors/calls      : " << std::dec << easy_errors << "/" << easy_calls << LL_ENDL;
  LL_INFOS_NF() << "  curl_easy_init() errors/calls      : " << std::dec << easy_init_errors << "/" << easy_init_calls << LL_ENDL;
  LL_INFOS_NF() << "  Current number of curl easy handles: " << std::dec << easy_handles << LL_ENDL;
#ifdef DEBUG_CURLIO
  LL_INFOS_NF() << "  Current number of BufferedCurlEasyRequest objects: " << BufferedCurlEasyRequest_count << LL_ENDL;
  LL_INFOS_NF() << "  Current number of ThreadSafeBufferedCurlEasyRequest objects: " << ThreadSafeBufferedCurlEasyRequest_count << LL_ENDL;
  LL_INFOS_NF() << "  Current number of AICurlEasyRequest objects: " << AICurlEasyRequest_count << LL_ENDL;
  LL_INFOS_NF() << "  Current number of AICurlEasyRequestStateMachine objects: " << AICurlEasyRequestStateMachine_count << LL_ENDL;
#endif
  LL_INFOS_NF() << "  Current number of Responders: " << ResponderBase_count << LL_ENDL;
  LL_INFOS_NF() << "  Received HTTP bodies   LLSD / LLSD parse errors / non-LLSD: " << llsd_body_count << "/" << llsd_body_parse_error << "/" << raw_body_count << LL_ENDL;
  LL_INFOS_NF() << "  Received HTTP status codes: status (count) [...]: ";
  bool first = true;
  for (U32 index = 0; index < 100; ++index)
  {
	if (status_count[index] > 0)
	{
	  if (!first)
	  {
		LL_CONT << ", ";
	  }
	  else
	  {
		first = false;
	  }
	  LL_CONT << index2status(index) << " (" << status_count[index] << ')';
	}
  }
  LL_CONT << LL_ENDL;
  LL_INFOS_NF() << "========= END OF CURL STATS =========" << LL_ENDL;
  llassert(easy_handles == BufferedCurlEasyRequest_count);
  llassert(BufferedCurlEasyRequest_count == ThreadSafeBufferedCurlEasyRequest_count);
  llassert(AICurlEasyRequest_count >= AICurlEasyRequestStateMachine_count);
  llassert(easy_handles <= S32(ResponderBase_count));
}
}
namespace AICurlPrivate {
using AICurlInterface::Stats;
#ifdef CWDEBUG
extern int debug_callback(CURL*, curl_infotype infotype, char* buf, size_t size, void* user_ptr);
#endif
void handle_multi_error(CURLMcode code)
{
  Stats::multi_errors++;
  LL_INFOS() << "curl multi error detected: " << curl_multi_strerror(code) <<
	  "; (errors/calls = " << Stats::multi_errors << "/" << Stats::multi_calls << ")" << LL_ENDL;
}
void CurlEasyHandle::handle_easy_error(CURLcode code)
{
  char* error_buffer = LLThreadLocalData::tldata().mCurlErrorBuffer;
  LL_INFOS() << "curl easy error detected: " << curl_easy_strerror(code);
  if (error_buffer && *error_buffer != '\0')
  {
	LL_CONT << ": " << error_buffer;
  }
  Stats::easy_errors++;
  LL_CONT << "; (errors/calls = " << Stats::easy_errors << "/" << Stats::easy_calls << ")" << LL_ENDL;
}
CurlEasyHandle::CurlEasyHandle(void) : mActiveMultiHandle(NULL), mErrorBuffer(NULL), mQueuedForRemoval(false)
#ifdef DEBUG_CURLIO
	, mDebug(false)
#endif
#ifdef SHOW_ASSERT
	, mRemovedPerCommand(true)
#endif
{
  mEasyHandle = curl_easy_init();
#if 0
  static int count = 0;
  if (mEasyHandle && (++count % 10) == 5)
  {
    curl_easy_cleanup(mEasyHandle);
	mEasyHandle = NULL;
  }
#endif
  Stats::easy_init_calls++;
  if (!mEasyHandle)
  {
	Stats::easy_init_errors++;
	throw AICurlNoEasyHandle("curl_easy_init() returned NULL");
  }
}
#if 0
CurlEasyHandle::CurlEasyHandle(CurlEasyHandle const& orig) : mActiveMultiHandle(NULL), mErrorBuffer(NULL)
#ifdef SHOW_ASSERT
		, mRemovedPerCommand(true)
#endif
{
  mEasyHandle = curl_easy_duphandle(orig.mEasyHandle);
  Stats::easy_init_calls++;
  if (!mEasyHandle)
  {
	Stats::easy_init_errors++;
	throw AICurlNoEasyHandle("curl_easy_duphandle() returned NULL");
  }
}
#endif
CurlEasyHandle::~CurlEasyHandle()
{
  llassert(!mActiveMultiHandle);
  curl_easy_cleanup(mEasyHandle);
  Stats::easy_cleanup_calls++;
#ifdef DEBUG_CURLIO
  if (mDebug)
  {
	debug_curl_remove_easy(mEasyHandle);
  }
#endif
}
char* CurlEasyHandle::getTLErrorBuffer(void)
{
  LLThreadLocalData& tldata = LLThreadLocalData::tldata();
  if (!tldata.mCurlErrorBuffer)
  {
	tldata.mCurlErrorBuffer = new char[CURL_ERROR_SIZE];
  }
  return tldata.mCurlErrorBuffer;
}
void CurlEasyHandle::setErrorBuffer(void) const
{
  char* error_buffer = getTLErrorBuffer();
  if (mErrorBuffer != error_buffer)
  {
	mErrorBuffer = error_buffer;
	CURLcode res = curl_easy_setopt(mEasyHandle, CURLOPT_ERRORBUFFER, error_buffer);
	if (res != CURLE_OK)
	{
	  LL_WARNS() << "curl_easy_setopt(" << (void*)mEasyHandle << "CURLOPT_ERRORBUFFER, " << (void*)error_buffer << ") failed with error " << res << LL_ENDL;
	  mErrorBuffer = NULL;
	}
  }
  if (mErrorBuffer)
  {
	mErrorBuffer[0] = '\0';
  }
}
CURLcode CurlEasyHandle::getinfo_priv(CURLINFO info, void* data) const
{
  setErrorBuffer();
  return check_easy_code(curl_easy_getinfo(mEasyHandle, info, data));
}
char* CurlEasyHandle::escape(char* url, int length)
{
  return curl_easy_escape(mEasyHandle, url, length);
}
char* CurlEasyHandle::unescape(char* url, int inlength , int* outlength)
{
  return curl_easy_unescape(mEasyHandle, url, inlength, outlength);
}
CURLcode CurlEasyHandle::perform(void)
{
  llassert(!mActiveMultiHandle);
  setErrorBuffer();
  return check_easy_code(curl_easy_perform(mEasyHandle));
}
CURLcode CurlEasyHandle::pause(int bitmask)
{
  setErrorBuffer();
  return check_easy_code(curl_easy_pause(mEasyHandle, bitmask));
}
CURLMcode CurlEasyHandle::add_handle_to_multi(AICurlEasyRequest_wat& curl_easy_request_w, CURLM* multi)
{
  llassert_always(!mActiveMultiHandle && multi);
  mActiveMultiHandle = multi;
  CURLMcode res = check_multi_code(curl_multi_add_handle(multi, mEasyHandle));
  added_to_multi_handle(curl_easy_request_w);
  return res;
}
CURLMcode CurlEasyHandle::remove_handle_from_multi(AICurlEasyRequest_wat& curl_easy_request_w, CURLM* multi)
{
  llassert_always(mActiveMultiHandle && mActiveMultiHandle == multi);
  mActiveMultiHandle = NULL;
  CURLMcode res = check_multi_code(curl_multi_remove_handle(multi, mEasyHandle));
  removed_from_multi_handle(curl_easy_request_w);
  mPostField = NULL;
  return res;
}
void intrusive_ptr_add_ref(ThreadSafeBufferedCurlEasyRequest* threadsafe_curl_easy_request)
{
  threadsafe_curl_easy_request->mReferenceCount++;
}
void intrusive_ptr_release(ThreadSafeBufferedCurlEasyRequest* threadsafe_curl_easy_request)
{
  if (--threadsafe_curl_easy_request->mReferenceCount == 0)
  {
	delete threadsafe_curl_easy_request;
  }
}
CURLcode CurlEasyHandle::setopt(CURLoption option, long parameter)
{
  llassert((CURLOPTTYPE_LONG  <= option && option < CURLOPTTYPE_LONG  + 1000) ||
		   (sizeof(curl_off_t) == sizeof(long) &&
			CURLOPTTYPE_OFF_T <= option && option < CURLOPTTYPE_OFF_T + 1000));
  llassert(!mActiveMultiHandle);
  setErrorBuffer();
  return check_easy_code(curl_easy_setopt(mEasyHandle, option, parameter));
}
CURLcode CurlEasyHandle::setopt(CURLoption option, long long parameter)
{
  llassert(sizeof(curl_off_t) == sizeof(long long) &&
		   CURLOPTTYPE_OFF_T <= option && option < CURLOPTTYPE_OFF_T + 1000);
  llassert(!mActiveMultiHandle);
  setErrorBuffer();
  return check_easy_code(curl_easy_setopt(mEasyHandle, option, parameter));
}
CURLcode CurlEasyHandle::setopt(CURLoption option, void const* parameter)
{
  llassert(CURLOPTTYPE_OBJECTPOINT <= option && option < CURLOPTTYPE_OBJECTPOINT + 1000);
  setErrorBuffer();
  return check_easy_code(curl_easy_setopt(mEasyHandle, option, parameter));
}
#define DEFINE_FUNCTION_SETOPT1(function_type, opt1) \
	CURLcode CurlEasyHandle::setopt(CURLoption option, function_type parameter) \
	{ \
	  llassert(option == opt1); \
	  setErrorBuffer(); \
	  return check_easy_code(curl_easy_setopt(mEasyHandle, option, parameter)); \
	}
#define DEFINE_FUNCTION_SETOPT3(function_type, opt1, opt2, opt3) \
	CURLcode CurlEasyHandle::setopt(CURLoption option, function_type parameter) \
	{ \
	  llassert(option == opt1 || option == opt2 || option == opt3); \
	  setErrorBuffer(); \
	  return check_easy_code(curl_easy_setopt(mEasyHandle, option, parameter)); \
	}
#define DEFINE_FUNCTION_SETOPT4(function_type, opt1, opt2, opt3, opt4) \
	CURLcode CurlEasyHandle::setopt(CURLoption option, function_type parameter) \
	{ \
	  llassert(option == opt1 || option == opt2 || option == opt3 || option == opt4); \
	  setErrorBuffer(); \
	  return check_easy_code(curl_easy_setopt(mEasyHandle, option, parameter)); \
	}
DEFINE_FUNCTION_SETOPT1(curl_debug_callback, CURLOPT_DEBUGFUNCTION)
DEFINE_FUNCTION_SETOPT4(curl_write_callback, CURLOPT_HEADERFUNCTION, CURLOPT_WRITEFUNCTION, CURLOPT_INTERLEAVEFUNCTION, CURLOPT_READFUNCTION)
DEFINE_FUNCTION_SETOPT1(curl_ssl_ctx_callback, CURLOPT_SSL_CTX_FUNCTION)
DEFINE_FUNCTION_SETOPT3(curl_conv_callback, CURLOPT_CONV_FROM_NETWORK_FUNCTION, CURLOPT_CONV_TO_NETWORK_FUNCTION, CURLOPT_CONV_FROM_UTF8_FUNCTION)
DEFINE_FUNCTION_SETOPT1(curl_progress_callback, CURLOPT_PROGRESSFUNCTION)
#if 0
DEFINE_FUNCTION_SETOPT1(curl_seek_callback, CURLOPT_SEEKFUNCTION)
DEFINE_FUNCTION_SETOPT1(curl_ioctl_callback, CURLOPT_IOCTLFUNCTION)
DEFINE_FUNCTION_SETOPT1(curl_sockopt_callback, CURLOPT_SOCKOPTFUNCTION)
DEFINE_FUNCTION_SETOPT1(curl_opensocket_callback, CURLOPT_OPENSOCKETFUNCTION)
DEFINE_FUNCTION_SETOPT1(curl_closesocket_callback, CURLOPT_CLOSESOCKETFUNCTION)
DEFINE_FUNCTION_SETOPT1(curl_sshkeycallback, CURLOPT_SSH_KEYFUNCTION)
DEFINE_FUNCTION_SETOPT1(curl_chunk_bgn_callback, CURLOPT_CHUNK_BGN_FUNCTION)
DEFINE_FUNCTION_SETOPT1(curl_chunk_end_callback, CURLOPT_CHUNK_END_FUNCTION)
DEFINE_FUNCTION_SETOPT1(curl_fnmatch_callback, CURLOPT_FNMATCH_FUNCTION)
#endif
void CurlEasyRequest::setoptString(CURLoption option, std::string const& value)
{
  llassert(!gSetoptParamsNeedDup);
  setopt(option, value.c_str());
}
void CurlEasyRequest::setPut(U32 size, bool keepalive)
{
  DoutCurl("PUT size is " << size << " bytes.");
  mContentLength = size;
  addHeader("Expect:");
  if (size > 0 && keepalive)
  {
	addHeader("Connection: keep-alive");
	addHeader("Keep-alive: 300");
  }
  setopt(CURLOPT_UPLOAD, 1);
  setopt(CURLOPT_INFILESIZE, size);
}
void CurlEasyRequest::setPatch(U32 size, bool keepalive)
{
	DoutCurl("PATCH size is " << size << " bytes.");
	mContentLength = size;
	addHeader("Expect:");
	if (size > 0 && keepalive)
	{
		addHeader("Connection: keep-alive");
		addHeader("Keep-alive: 300");
	}
	setopt(CURLOPT_UPLOAD, 1);
	setopt(CURLOPT_INFILESIZE, size);\
	setopt(CURLOPT_CUSTOMREQUEST, "PATCH");
}
void CurlEasyRequest::setPost(AIPostFieldPtr const& postdata, U32 size, bool keepalive)
{
  llassert_always(postdata->data());
  DoutCurl("POST size is " << size << " bytes: \"" << libcwd::buf2str(postdata->data(), size) << "\".");
  setPostField(postdata);
  setPost_raw(size, postdata->data(), keepalive);
}
void CurlEasyRequest::setPost_raw(U32 size, char const* data, bool keepalive)
{
  if (!data)
  {
	DoutCurl("POST size is " << size << " bytes.");
  }
  mContentLength = size;
  addHeader("Expect:");
  if (size > 0 && keepalive)
  {
	addHeader("Connection: keep-alive");
	addHeader("Keep-alive: 300");
  }
  setopt(CURLOPT_POSTFIELDSIZE, size);
  setopt(CURLOPT_POSTFIELDS, data);
}
size_t CurlEasyRequest::headerCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
  CurlEasyRequest* self = static_cast<CurlEasyRequest*>(userdata);
  ThreadSafeBufferedCurlEasyRequest* lockobj = self->get_lockobj();
  AICurlEasyRequest_wat lock_self(*lockobj);
  return self->mHeaderCallback(ptr, size, nmemb, self->mHeaderCallbackUserData);
}
void CurlEasyRequest::setHeaderCallback(curl_write_callback callback, void* userdata)
{
  mHeaderCallback = callback;
  mHeaderCallbackUserData = userdata;
  setopt(CURLOPT_HEADERFUNCTION, callback ? &CurlEasyRequest::headerCallback : NULL);
  setopt(CURLOPT_WRITEHEADER, userdata ? this : NULL);
}
size_t CurlEasyRequest::writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
  CurlEasyRequest* self = static_cast<CurlEasyRequest*>(userdata);
  ThreadSafeBufferedCurlEasyRequest* lockobj = self->get_lockobj();
  AICurlEasyRequest_wat lock_self(*lockobj);
  return self->mWriteCallback(ptr, size, nmemb, self->mWriteCallbackUserData);
}
void CurlEasyRequest::setWriteCallback(curl_write_callback callback, void* userdata)
{
  mWriteCallback = callback;
  mWriteCallbackUserData = userdata;
  setopt(CURLOPT_WRITEFUNCTION, callback ? &CurlEasyRequest::writeCallback : NULL);
  setopt(CURLOPT_WRITEDATA, userdata ? this : NULL);
}
size_t CurlEasyRequest::readCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
  CurlEasyRequest* self = static_cast<CurlEasyRequest*>(userdata);
  ThreadSafeBufferedCurlEasyRequest* lockobj = self->get_lockobj();
  AICurlEasyRequest_wat lock_self(*lockobj);
  return self->mReadCallback(ptr, size, nmemb, self->mReadCallbackUserData);
}
void CurlEasyRequest::setReadCallback(curl_read_callback callback, void* userdata)
{
  mReadCallback = callback;
  mReadCallbackUserData = userdata;
  setopt(CURLOPT_READFUNCTION, callback ? &CurlEasyRequest::readCallback : NULL);
  setopt(CURLOPT_READDATA, userdata ? this : NULL);
}
CURLcode CurlEasyRequest::SSLCtxCallback(CURL* curl, void* sslctx, void* userdata)
{
  CurlEasyRequest* self = static_cast<CurlEasyRequest*>(userdata);
  ThreadSafeBufferedCurlEasyRequest* lockobj = self->get_lockobj();
  AICurlEasyRequest_wat lock_self(*lockobj);
  return self->mSSLCtxCallback(curl, sslctx, self->mSSLCtxCallbackUserData);
}
void CurlEasyRequest::setSSLCtxCallback(curl_ssl_ctx_callback callback, void* userdata)
{
  mSSLCtxCallback = callback;
  mSSLCtxCallbackUserData = userdata;
  setopt(CURLOPT_SSL_CTX_FUNCTION, callback ? &CurlEasyRequest::SSLCtxCallback : NULL);
  setopt(CURLOPT_SSL_CTX_DATA, this);
}
int CurlEasyRequest::progressCallback(void* userdata, double dltotal, double dlnow, double ultotal, double ulnow)
{
  CurlEasyRequest* self = static_cast<CurlEasyRequest*>(userdata);
  ThreadSafeBufferedCurlEasyRequest* lockobj = self->get_lockobj();
  AICurlEasyRequest_wat lock_self(*lockobj);
  return self->mProgressCallback(self->mProgressCallbackUserData, dltotal, dlnow, ultotal, ulnow);
}
void CurlEasyRequest::setProgressCallback(curl_progress_callback callback, void* userdata)
{
  mProgressCallback = callback;
  mProgressCallbackUserData = userdata;
  setopt(CURLOPT_PROGRESSFUNCTION, callback ? &CurlEasyRequest::progressCallback : NULL);
  setopt(CURLOPT_PROGRESSDATA, userdata ? this : NULL);
}
#define llmaybewarns lllog(LLApp::isExiting() ? LLError::LEVEL_INFO : LLError::LEVEL_WARN, false, true)
static size_t noHeaderCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
  llmaybewarns << "Calling noHeaderCallback(); curl session aborted." << LL_ENDL;
  return 0;
}
static size_t noWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
  llmaybewarns << "Calling noWriteCallback(); curl session aborted." << LL_ENDL;
  return 0;
}
static size_t noReadCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
  llmaybewarns << "Calling noReadCallback(); curl session aborted." << LL_ENDL;
  return CURL_READFUNC_ABORT;
}
static CURLcode noSSLCtxCallback(CURL* curl, void* sslctx, void* parm)
{
  llmaybewarns << "Calling noSSLCtxCallback(); curl session aborted." << LL_ENDL;
  return CURLE_ABORTED_BY_CALLBACK;
}
static int noProgressCallback(void* userdata, double, double, double, double)
{
  llmaybewarns << "Calling noProgressCallback(); curl session aborted." << LL_ENDL;
  return -1;
}
void CurlEasyRequest::revokeCallbacks(void)
{
  if (mHeaderCallback == &noHeaderCallback &&
	  mWriteCallback == &noWriteCallback &&
	  mReadCallback == &noReadCallback &&
	  mSSLCtxCallback == &noSSLCtxCallback &&
	  mProgressCallback == &noProgressCallback)
  {
	return;
  }
  mHeaderCallback = &noHeaderCallback;
  mWriteCallback = &noWriteCallback;
  mReadCallback = &noReadCallback;
  mSSLCtxCallback = &noSSLCtxCallback;
  mProgressCallback = &noProgressCallback;
  if (active() && !no_warning())
  {
	LL_WARNS() << "Revoking callbacks on a still active CurlEasyRequest object!" << LL_ENDL;
  }
  curl_easy_setopt(getEasyHandle(), CURLOPT_HEADERFUNCTION, &noHeaderCallback);
  curl_easy_setopt(getEasyHandle(), CURLOPT_WRITEHEADER, &noWriteCallback);
  curl_easy_setopt(getEasyHandle(), CURLOPT_READFUNCTION, &noReadCallback);
  curl_easy_setopt(getEasyHandle(), CURLOPT_SSL_CTX_FUNCTION, &noSSLCtxCallback);
  curl_easy_setopt(getEasyHandle(), CURLOPT_PROGRESSFUNCTION, &noProgressCallback);
}
CurlEasyRequest::~CurlEasyRequest()
{
  send_handle_events_to(NULL);
  revokeCallbacks();
  if (mPerServicePtr)
  {
	 AIPerService::release(mPerServicePtr);
  }
  curl_slist_free_all(mHeaders);
}
void CurlEasyRequest::resetState(void)
{
  revokeCallbacks();
  reset();
  curl_slist_free_all(mHeaders);
  mHeaders = NULL;
  mTimeoutPolicy = NULL;
  mTimeout = NULL;
  mHandleEventsTarget = NULL;
  mResult = CURLE_FAILED_INIT;
  applyDefaultOptions();
}
void CurlEasyRequest::addHeader(char const* header)
{
  llassert(!mTimeoutPolicy);
  mHeaders = curl_slist_append(mHeaders, header);
}
void CurlEasyRequest::addHeaders(AIHTTPHeaders const& headers)
{
  llassert(!mTimeoutPolicy);
  headers.append_to(mHeaders);
}
void CurlEasyRequest::applyProxySettings(void)
{
  LLProxy& proxy = *LLProxy::getInstance();
  if (proxy.HTTPProxyEnabled())
  {
	LLProxy::Shared_crat proxy_r(proxy.shared_lockobj());
	if (proxy.HTTPProxyEnabled())
	{
	  setopt(CURLOPT_PROXY, proxy.getHTTPProxy(proxy_r).getIPString().c_str());
	  setopt(CURLOPT_PROXYPORT, proxy.getHTTPProxy(proxy_r).getPort());
	  if (proxy.getHTTPProxyType(proxy_r) == LLPROXY_SOCKS)
	  {
		setopt(CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
		if (proxy.getSelectedAuthMethod(proxy_r) == METHOD_PASSWORD)
		{
		  std::string auth_string = proxy.getSocksUser(proxy_r) + ":" + proxy.getSocksPwd(proxy_r);
		  setopt(CURLOPT_PROXYUSERPWD, auth_string.c_str());
		}
	  }
	  else
	  {
		setopt(CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
	  }
	}
  }
}
CURLcode CurlEasyRequest::curlCtxCallback(CURL* curl, void* sslctx, void* parm)
{
  DoutEntering(dc::curl, "CurlEasyRequest::curlCtxCallback((CURL*)" << (void*)curl << ", " << sslctx << ", " << parm << ")");
  SSL_CTX* ctx = (SSL_CTX*)sslctx;
  long options = SSL_OP_NO_SSLv2;
  SSL_CTX_set_options(ctx, options);
  return CURLE_OK;
}
void CurlEasyRequest::applyDefaultOptions(void)
{
  CertificateAuthority_rat CertificateAuthority_r(gCertificateAuthority);
  setoptString(CURLOPT_CAINFO, CertificateAuthority_r->file);
  if (gSSLlib == ssl_openssl)
  {
	setSSLCtxCallback(&curlCtxCallback, NULL);
  }
  setopt(CURLOPT_NOSIGNAL, 1);
  setopt(CURLOPT_DNS_CACHE_TIMEOUT, 3600);
  setopt(CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
  setopt(CURLOPT_SSL_SESSIONID_CACHE, 0);
  setopt(CURLOPT_NOPROGRESS, 0);
  applyProxySettings();
  Debug(
	if (dc::curlio.is_on())
	{
	  setopt(CURLOPT_VERBOSE, 1);
	  setopt(CURLOPT_DEBUGFUNCTION, &debug_callback);
	  setopt(CURLOPT_DEBUGDATA, this);
	}
  );
}
void CurlEasyRequest::finalizeRequest(std::string const& url, AIHTTPTimeoutPolicy const& policy, AICurlEasyRequestStateMachine* state_machine)
{
  DoutCurlEntering("CurlEasyRequest::finalizeRequest(\"" << url << "\", " << policy.name() << ", " << (void*)state_machine << ")");
  llassert(!mTimeoutPolicy);
  mResult = CURLE_FAILED_INIT;
  mIsHttps = strncmp(url.c_str(), "https:", 6) == 0;
#ifdef SHOW_ASSERT
  int content_type_count = 0;
  for (curl_slist* list = mHeaders; list; list = list->next)
  {
	if (strncmp(list->data, "Content-Type:", 13) == 0)
	{
	  ++content_type_count;
	}
  }
  if (content_type_count > 1)
  {
	LL_WARNS() << "Requesting: \"" << url << "\": " << content_type_count << " Content-Type: headers!" << LL_ENDL;
  }
#endif
  setopt(CURLOPT_HTTPHEADER, mHeaders);
  setoptString(CURLOPT_URL, url);
  llassert(!mPerServicePtr);
  mLowercaseServicename = AIPerService::extract_canonical_servicename(url);
  mTimeoutPolicy = &policy;
  state_machine->setTotalDelayTimeout(policy.getTotalDelay());
  setopt(CURLOPT_PRIVATE, get_lockobj());
}
void CurlEasyRequest::set_timeout_opts(void)
{
  U16 connect_timeout = mTimeoutPolicy->getConnectTimeout(getLowercaseHostname());
  if (mIsHttps && connect_timeout < 30)
  {
	DoutCurl("Incrementing CURLOPT_CONNECTTIMEOUT of \"" << mTimeoutPolicy->name() << "\" from " << connect_timeout << " to 30 seconds.");
	connect_timeout = 30;
  }
  setopt(CURLOPT_CONNECTTIMEOUT, connect_timeout);
  setopt(CURLOPT_TIMEOUT, mTimeoutPolicy->getCurlTransaction());
}
void CurlEasyRequest::create_timeout_object(void)
{
  ThreadSafeBufferedCurlEasyRequest* lockobj = NULL;
#ifdef CWDEBUG
  lockobj = static_cast<BufferedCurlEasyRequest*>(this)->get_lockobj();
#endif
  mTimeout = new curlthread::HTTPTimeout(mTimeoutPolicy, lockobj);
}
LLPointer<curlthread::HTTPTimeout>& CurlEasyRequest::get_timeout_object(void)
{
  if (mTimeoutIsOrphan)
  {
	mTimeoutIsOrphan = false;
	llassert_always(mTimeout);
  }
  else
  {
	create_timeout_object();
  }
  return mTimeout;
}
void CurlEasyRequest::print_curl_timings(void) const
{
  double t;
  getinfo(CURLINFO_NAMELOOKUP_TIME, &t);
  DoutCurl("CURLINFO_NAMELOOKUP_TIME = " << t);
  getinfo(CURLINFO_CONNECT_TIME, &t);
  DoutCurl("CURLINFO_CONNECT_TIME = " << t);
  getinfo(CURLINFO_APPCONNECT_TIME, &t);
  DoutCurl("CURLINFO_APPCONNECT_TIME = " << t);
  getinfo(CURLINFO_PRETRANSFER_TIME, &t);
  DoutCurl("CURLINFO_PRETRANSFER_TIME = " << t);
  getinfo(CURLINFO_STARTTRANSFER_TIME, &t);
  DoutCurl("CURLINFO_STARTTRANSFER_TIME = " << t);
}
void CurlEasyRequest::getTransferInfo(AITransferInfo* info)
{
  double size, total_time, speed;
  getinfo(CURLINFO_SIZE_DOWNLOAD, &size);
  getinfo(CURLINFO_TOTAL_TIME, &total_time);
  getinfo(CURLINFO_SPEED_DOWNLOAD, &speed);
  info->mSizeDownload = size;
  info->mTotalTime = total_time;
  info->mSpeedDownload = speed;
}
void CurlEasyRequest::getResult(CURLcode* result, AITransferInfo* info)
{
  *result = mResult;
  if (info && mResult != CURLE_FAILED_INIT)
  {
	getTransferInfo(info);
  }
}
void CurlEasyRequest::added_to_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w)
{
  if (mHandleEventsTarget)
	mHandleEventsTarget->added_to_multi_handle(curl_easy_request_w);
}
void CurlEasyRequest::finished(AICurlEasyRequest_wat& curl_easy_request_w)
{
  if (mHandleEventsTarget)
	mHandleEventsTarget->finished(curl_easy_request_w);
}
void CurlEasyRequest::removed_from_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w)
{
  if (mHandleEventsTarget)
	mHandleEventsTarget->removed_from_multi_handle(curl_easy_request_w);
}
void CurlEasyRequest::bad_file_descriptor(AICurlEasyRequest_wat& curl_easy_request_w)
{
  if (mHandleEventsTarget)
	mHandleEventsTarget->bad_file_descriptor(curl_easy_request_w);
}
#ifdef SHOW_ASSERT
void CurlEasyRequest::queued_for_removal(AICurlEasyRequest_wat& curl_easy_request_w)
{
  if (mHandleEventsTarget)
	mHandleEventsTarget->queued_for_removal(curl_easy_request_w);
}
#endif
AIPerServicePtr CurlEasyRequest::getPerServicePtr(void)
{
  if (!mPerServicePtr)
  {
	mPerServicePtr = AIPerService::instance(mLowercaseServicename);
  }
  return mPerServicePtr;
}
bool CurlEasyRequest::removeFromPerServiceQueue(AICurlEasyRequest const& easy_request, AICapabilityType capability_type) const
{
  return mPerServicePtr && PerService_wat(*mPerServicePtr)->cancel(easy_request, capability_type);
}
std::string CurlEasyRequest::getLowercaseHostname(void) const
{
  return mLowercaseServicename.substr(0, mLowercaseServicename.find_last_of(':'));
}
static int const HTTP_REDIRECTS_DEFAULT = 16;
LLChannelDescriptors const BufferedCurlEasyRequest::sChannels;
LLGlobalMutex BufferedCurlEasyRequest::sResponderCallbackMutex;
bool BufferedCurlEasyRequest::sShuttingDown = false;
AIAverage BufferedCurlEasyRequest::sHTTPBandwidth(25);
BufferedCurlEasyRequest::BufferedCurlEasyRequest() :
	mRequestTransferedBytes(0), mTotalRawBytes(0), mStatus(HTTP_INTERNAL_ERROR_OTHER), mBufferEventsTarget(NULL), mCapabilityType(number_of_capability_types)
{
  AICurlInterface::Stats::BufferedCurlEasyRequest_count++;
}
#define llmaybeerrs lllog(LLApp::isRunning() ? LLError::LEVEL_ERROR : LLError::LEVEL_WARN, false, true)
BufferedCurlEasyRequest::~BufferedCurlEasyRequest()
{
  send_buffer_events_to(NULL);
  revokeCallbacks();
  if (mResponder)
  {
	llmaybeerrs << "Calling ~BufferedCurlEasyRequest() with active responder!" << LL_ENDL;
	if (!LLApp::isRunning())
	{
	  mResponder = NULL;
	}
	else
	{
	  aborted(HTTP_INTERNAL_ERROR_OTHER, "BufferedCurlEasyRequest destructed with active responder");
	}
  }
  --AICurlInterface::Stats::BufferedCurlEasyRequest_count;
}
void BufferedCurlEasyRequest::aborted(U32 http_status, std::string const& reason)
{
  if (mResponder)
  {
	mResponder->finished(CURLE_OK, http_status, reason, sChannels, mOutput);
	if (mResponder->needsHeaders())
	{
	  send_buffer_events_to(NULL);
	}
	mResponder = NULL;
  }
}
#ifdef CWDEBUG
static AIPerServicePtr sConnections[64];
void BufferedCurlEasyRequest::connection_established(int connectionnr)
{
  PerService_rat per_service_r(*mPerServicePtr);
  int n = per_service_r->connection_established();
  llassert(sConnections[connectionnr] == NULL);
  llassert_always(connectionnr < 64);
  sConnections[connectionnr] = mPerServicePtr;
  Dout(dc::curlio, (void*)get_lockobj() << " Connection established (#" << connectionnr << "). Now " << n << " connections [" << (void*)&*per_service_r << "].");
  llassert(sConnections[connectionnr] != NULL);
}
void BufferedCurlEasyRequest::connection_closed(int connectionnr)
{
  if (sConnections[connectionnr] == NULL)
  {
	Dout(dc::curlio, "Closing connection that never connected (#" << connectionnr << ").");
	return;
  }
  PerService_rat per_service_r(*sConnections[connectionnr]);
  int n = per_service_r->connection_closed();
  sConnections[connectionnr] = NULL;
  Dout(dc::curlio, (void*)get_lockobj() << " Connection closed (#" << connectionnr << "); " << n << " connections remaining [" << (void*)&*per_service_r << "].");
}
#endif
void BufferedCurlEasyRequest::resetState(void)
{
  llassert(!mResponder);
  CurlEasyRequest::resetState();
  mOutput.reset();
  mInput.reset();
  mRequestTransferedBytes = 0;
  mTotalRawBytes = 0;
  mBufferEventsTarget = NULL;
  mStatus = HTTP_INTERNAL_ERROR_OTHER;
}
void BufferedCurlEasyRequest::print_diagnostics(CURLcode code)
{
  char* eff_url;
  getinfo(CURLINFO_EFFECTIVE_URL, &eff_url);
  if (code == CURLE_OPERATION_TIMEDOUT)
  {
	if (mTimeout)
	{
	  mTimeout->print_diagnostics(this, eff_url);
	}
  }
  else
  {
	LL_WARNS() << "Curl returned error code " << code << " (" << curl_easy_strerror(code) << ") for HTTP request to \"" << eff_url << "\"." << LL_ENDL;
  }
}
ThreadSafeBufferedCurlEasyRequest* BufferedCurlEasyRequest::get_lockobj(void)
{
  return static_cast<ThreadSafeBufferedCurlEasyRequest*>(AIThreadSafeSimple<BufferedCurlEasyRequest>::wrapper_cast(this));
}
ThreadSafeBufferedCurlEasyRequest const* BufferedCurlEasyRequest::get_lockobj(void) const
{
  return static_cast<ThreadSafeBufferedCurlEasyRequest const*>(AIThreadSafeSimple<BufferedCurlEasyRequest>::wrapper_cast(this));
}
void BufferedCurlEasyRequest::prepRequest(AICurlEasyRequest_wat& curl_easy_request_w, AIHTTPHeaders const& headers, LLHTTPClient::ResponderPtr responder)
{
  mInput.reset(new LLBufferArray);
  mInput->setThreaded(true);
  mLastRead = NULL;
  mOutput.reset(new LLBufferArray);
  mOutput->setThreaded(true);
  ThreadSafeBufferedCurlEasyRequest* lockobj = get_lockobj();
  curl_easy_request_w->setWriteCallback(&curlWriteCallback, lockobj);
  curl_easy_request_w->setReadCallback(&curlReadCallback, lockobj);
  curl_easy_request_w->setHeaderCallback(&curlHeaderCallback, lockobj);
  curl_easy_request_w->setProgressCallback(&curlProgressCallback, lockobj);
  bool allow_cookies = headers.hasHeader("Cookie");
  if (!responder->pass_redirect_status())
  {
	curl_easy_request_w->setopt(CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_request_w->setopt(CURLOPT_MAXREDIRS, HTTP_REDIRECTS_DEFAULT);
	allow_cookies = true;
  }
  if (responder->forbidReuse())
  {
	curl_easy_request_w->setopt(CURLOPT_FORBID_REUSE, 1);
  }
  if (allow_cookies)
  {
	curl_easy_request_w->setopt(CURLOPT_COOKIEFILE, "");
  }
  mResponder = responder;
  mCapabilityType = responder->capability_type();
  mIsEventPoll = responder->is_event_poll();
  if (mResponder->needsHeaders())
  {
	  send_buffer_events_to(mResponder.get());
  }
  curl_easy_request_w->addHeaders(headers);
}
LLAtomicU32 CurlMultiHandle::sTotalMultiHandles;
CurlMultiHandle::CurlMultiHandle(void)
{
  DoutEntering(dc::curl, "CurlMultiHandle::CurlMultiHandle() [" << (void*)this << "].");
  mMultiHandle = curl_multi_init();
  Stats::multi_calls++;
  if (!mMultiHandle)
  {
	Stats::multi_errors++;
	throw AICurlNoMultiHandle("curl_multi_init() returned NULL");
  }
  sTotalMultiHandles++;
}
CurlMultiHandle::~CurlMultiHandle()
{
  curl_multi_cleanup(mMultiHandle);
  Stats::multi_calls++;
#ifdef CWDEBUG
  int total = --sTotalMultiHandles;
  Dout(dc::curl, "Called CurlMultiHandle::~CurlMultiHandle() [" << (void*)this << "], " << total << " remaining.");
#else
	--sTotalMultiHandles;
#endif
}
}
