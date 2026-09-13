/**
 * @file aisyncclient.h
 * @brief Declaration of AISyncClient.
 *
 * Copyright (c) 2013, Aleric Inglewood.
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
 *   12/12/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AI_SYNC_CLIENT_H
#define AI_SYNC_CLIENT_H
#ifdef SYNC_TESTSUITE
#include <stdint.h>
#include <cassert>
typedef uint32_t U32;
typedef int32_t S32;
typedef uint64_t U64;
typedef float F32;
typedef double F64;
#define LL_COMMON_API
#define SHOW_ASSERT
#define ASSERT_ONLY_COMMA(...) , __VA_ARGS__
#define llassert assert
struct LLFrameTimer
{
  double mStartTime;
  double mExpiry;
  static double getCurrentTime(void);
  static U64 sFrameCount;
  static U64 getFrameCount() { return sFrameCount; }
  F64 getStartTime() const { return mStartTime; }
  void reset(double expiration) { mStartTime = getCurrentTime(); mExpiry = mStartTime + expiration; }
  bool hasExpired(void) const { return getCurrentTime() > mExpiry; }
};
template<typename T>
struct LLSingleton
{
  static T sInstance;
  static T& instance(void) { return sInstance; }
};
template<typename T>
T LLSingleton<T>::sInstance;
#else
#include "llsingleton.h"
#include "llframetimer.h"
#endif
#include <list>
#include <boost/intrusive_ptr.hpp>
enum syncgroups
{
#ifdef SYNC_TESTSUITE
  syncgroup_test1,
  syncgroup_test2,
#else
  syncgroup_motions,
#endif
  syncgroup_size
};
enum synckeytype_t
{
#ifdef SYNC_TESTSUITE
  synckeytype_test1a = 0x000 + syncgroup_test1,
  synckeytype_test1b = 0x100 + syncgroup_test1,
  synckeytype_test2a = 0x000 + syncgroup_test2,
  synckeytype_test2b = 0x100 + syncgroup_test2,
#else
  synckeytype_motion = syncgroup_motions
#endif
};
typedef U32 synceventset_t;
static F32 const sSyncKeyExpirationTime = 0.25;
class LL_COMMON_API AISyncKey
{
  private:
	LLFrameTimer mFrameTimer;
	U64 mStartFrameCount;
  public:
	AISyncKey(AISyncKey const* from_key) : mStartFrameCount(from_key ? from_key->mStartFrameCount : LLFrameTimer::getFrameCount())
	{
	  if (from_key)
	  {
		mFrameTimer.copy(from_key->mFrameTimer);
	  }
	  else
	  {
		mFrameTimer.reset(sSyncKeyExpirationTime);
	  }
	}
	virtual ~AISyncKey() { }
	bool expired(void) const
	{
	  return mFrameTimer.getFrameCount() > mStartFrameCount + 1 && mFrameTimer.hasExpired();
	}
	bool is_older_than(AISyncKey const& key) const
	{
	  return key.mStartFrameCount > mStartFrameCount + 1 && key.mFrameTimer.getStartTime() > mFrameTimer.getStartTime() + sSyncKeyExpirationTime;
	}
	F64 getCreationTime(void) const { return mFrameTimer.getStartTime(); }
	friend bool operator==(AISyncKey const& key1, AISyncKey const& key2);
	virtual synckeytype_t getkeytype(void) const = 0;
    virtual bool equals(AISyncKey const& key) const = 0;
};
class AISyncClient;
class AISyncServer;
LL_COMMON_API extern void intrusive_ptr_add_ref(AISyncServer* server);
LL_COMMON_API extern void intrusive_ptr_release(AISyncServer* server);
struct LL_COMMON_API AISyncClientData
{
  AISyncClient* mClientPtr;
  synceventset_t mReadyEvents;
  AISyncClientData(AISyncClient* client) : mClientPtr(client), mReadyEvents(0) { }
};
class LL_COMMON_API AISyncServer
{
  public:
	typedef std::list<AISyncClientData> client_list_t;
  private:
	int mRefCount;
	AISyncKey* mKey;
	client_list_t mClients;
	bool mSynchronized;
	synceventset_t mReadyEvents;
	synceventset_t mPendingEvents;
  public:
	AISyncServer(AISyncKey* key) : mRefCount(0), mKey(key), mSynchronized(false), mReadyEvents((synceventset_t)-1), mPendingEvents(0) { }
	~AISyncServer() { delete mKey; }
	void add(AISyncClient* client);
	void remove(AISyncClient* client);
	AISyncKey const& key(void) const { return *mKey; }
	void swapkey(AISyncKey*& key_ptr) { AISyncKey* tmp = key_ptr; key_ptr = mKey; mKey = tmp; }
	bool never_synced(void) const { return !mSynchronized; }
	void ready(synceventset_t events, synceventset_t yesno, AISyncClient* client);
	void unregister_last_client(void);
	synceventset_t events_with_all_clients_ready(void) const { return mReadyEvents; }
	synceventset_t events_with_at_least_one_client_ready(void) const { return mPendingEvents; }
	client_list_t const& getClients(void) const { return mClients; }
  private:
	void trigger(synceventset_t old_ready_events);
#ifdef SYNC_TESTSUITE
	void sanity_check(void) const;
#endif
  private:
    friend LL_COMMON_API void intrusive_ptr_add_ref(AISyncServer* server);
    friend LL_COMMON_API void intrusive_ptr_release(AISyncServer* server);
};
class LL_COMMON_API AISyncServerMap : public LLSingleton<AISyncServerMap>
{
  public:
	typedef boost::intrusive_ptr<AISyncServer> server_ptr_t;
	typedef std::list<server_ptr_t> server_list_t;
  private:
	server_list_t mServers;
  public:
	void register_client(AISyncClient* client, AISyncKey* new_key);
  private:
    friend LL_COMMON_API void intrusive_ptr_release(AISyncServer* server);
	void remove_server(AISyncServer* server);
};
class LL_COMMON_API AISyncClient
{
  private:
	friend class AISyncServer;
	boost::intrusive_ptr<AISyncServer> mServer;
  public:
#ifdef SHOW_ASSERT
	synceventset_t mReadyEvents;
	AISyncClient(void) : mReadyEvents(0) { }
#endif
	virtual ~AISyncClient() { llassert(!mServer); }
	virtual AISyncKey* createSyncKey(AISyncKey const* from_key = NULL) const = 0;
	virtual void event1_ready(void) = 0;
	virtual void event1_not_ready(void) = 0;
	virtual void deregistered(void)
	{
#ifdef SHOW_ASSERT
      mReadyEvents = 0;
#endif
	}
	AISyncServer* server(void) const { return mServer.get(); }
	void register_client(void) { AISyncServerMap::instance().register_client(this, createSyncKey()); }
	void unregister_client(void) { if (mServer) mServer->remove(this); }
	void ready(synceventset_t events, synceventset_t yesno)
    {
      if (!mServer)
      {
        register_client();
      }
      mServer->ready(events, yesno, this);
    }
};
#endif
