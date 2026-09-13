/**
 * @file aisyncclient.cpp
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
 *   13/12/2013
 *   - Initial version, written by Aleric Inglewood @ SL
 */
#include "sys.h"
#include "aisyncclient.h"
#include <cmath>
#include <algorithm>
#include "debug.h"
bool operator==(AISyncKey const& key1, AISyncKey const& key2)
{
  if (std::abs((S64)(key1.mStartFrameCount - key2.mStartFrameCount)) > 1 &&
	  std::abs(key1.mFrameTimer.getStartTime() - key2.mFrameTimer.getStartTime()) >= sSyncKeyExpirationTime)
  {
	return false;
  }
  return key1.equals(key2);
}
#ifdef CWDEBUG
struct SyncEventSet {
  synceventset_t mBits;
  SyncEventSet(synceventset_t bits) : mBits(bits) { }
};
std::ostream& operator<<(std::ostream& os, SyncEventSet const& ses)
{
  for (int b = sizeof(ses.mBits) * 8 - 1; b >= 0; --b)
  {
	int m = 1 << b;
	os << ((ses.mBits & m) ? '1' : '0');
  }
  return os;
}
void print_clients(AISyncServer const* server, AISyncServer::client_list_t const& client_list)
{
  Dout(dc::notice, "Clients of server " << server << ": ");
  for (AISyncServer::client_list_t::const_iterator iter = client_list.begin(); iter != client_list.end(); ++ iter)
  {
	llassert(iter->mClientPtr->mReadyEvents == iter->mReadyEvents);
	Dout(dc::notice, "-> " << iter->mClientPtr << " : " << SyncEventSet(iter->mReadyEvents));
  }
}
#endif
void AISyncServerMap::register_client(AISyncClient* client, AISyncKey* new_key)
{
  llassert(client->server() == NULL);
  llassert(!client->mReadyEvents);
  AISyncServer* server = NULL;
  for (server_list_t::iterator iter = mServers.begin(); iter != mServers.end();)
  {
    boost::intrusive_ptr<AISyncServer>& server_ptr = *iter++;
	AISyncKey const& server_key(server_ptr->key());
	if (server_key.is_older_than(*new_key))
	{
	  if (server_ptr->never_synced())
	  {
		server_ptr->unregister_last_client();
	  }
	  continue;
	}
	if (*new_key == server_key)
	{
	  server = server_ptr.get();
	  break;
	}
  }
  if (server)
  {
	if (new_key->getkeytype() > server->key().getkeytype())
	{
	  server->swapkey(new_key);
	}
	delete new_key;
  }
  else
  {
	server = new AISyncServer(new_key);
	server_list_t::iterator where = mServers.end();
	server_list_t::iterator new_where = where;
	while (where != mServers.begin())
	{
	  --new_where;
	  if (new_key->getCreationTime() > (*new_where)->key().getCreationTime())
	  {
		break;
	  }
	  where = new_where;
	}
	server_ptr_t server_ptr = server;
	mServers.insert(where, server_ptr_t())->swap(server_ptr);
  }
  server->add(client);
}
#ifdef SYNC_TESTSUITE
void AISyncServer::sanity_check(void) const
{
  synceventset_t ready_events = (synceventset_t)-1;
  client_list_t::const_iterator client_iter = mClients.begin();
  while (client_iter != mClients.end())
  {
	ready_events &= client_iter->mReadyEvents;
	++client_iter;
  }
  synceventset_t pending_events = 0;
  client_iter = mClients.begin();
  while (client_iter != mClients.end())
  {
	pending_events |= client_iter->mReadyEvents;
	++client_iter;
  }
  llassert(ready_events == mReadyEvents);
  llassert(pending_events == mPendingEvents);
}
#endif
void AISyncServer::add(AISyncClient* client)
{
#ifdef SYNC_TESTSUITE
  sanity_check();
#endif
  llassert(!client->mReadyEvents);
  synceventset_t old_ready_events = mReadyEvents;
  mReadyEvents = 0;
  if (!mSynchronized && mClients.size() > 0)
  {
	mSynchronized = true;
  }
  trigger(old_ready_events);
  mClients.push_back(client);
  client->mServer = this;
#ifdef SYNC_TESTSUITE
  sanity_check();
#endif
}
void AISyncServer::remove(AISyncClient* client)
{
#ifdef SYNC_TESTSUITE
  sanity_check();
#endif
  client_list_t::iterator client_iter = mClients.begin();
  synceventset_t remaining_ready_events = (synceventset_t)-1;
  synceventset_t remaining_pending_events = 0;
  client_list_t::iterator found_client = mClients.end();
  while (client_iter != mClients.end())
  {
	if (client_iter->mClientPtr == client)
	{
	  found_client = client_iter;
	}
	else
	{
	  remaining_ready_events &= client_iter->mReadyEvents;
	  remaining_pending_events |= client_iter->mReadyEvents;
	}
	++client_iter;
  }
  llassert(found_client != mClients.end());
  llassert(found_client->mReadyEvents == client->mReadyEvents);
  mClients.erase(found_client);
  synceventset_t old_ready_events = mReadyEvents;
  mReadyEvents = remaining_ready_events;
  mPendingEvents = remaining_pending_events;
  trigger(old_ready_events);
  client->mServer.reset();
  client->deregistered();
#ifdef SYNC_TESTSUITE
  sanity_check();
#endif
}
void AISyncServer::unregister_last_client(void)
{
#ifdef SYNC_TESTSUITE
  sanity_check();
#endif
  llassert(!mSynchronized && mClients.size() == 1);
  AISyncClient* client = mClients.begin()->mClientPtr;
  mClients.clear();
  client->mServer.reset();
  llassert(mReadyEvents == client->mReadyEvents);
  llassert(mPendingEvents == mReadyEvents);
  client->deregistered();
#ifdef SYNC_TESTSUITE
  sanity_check();
#endif
}
void AISyncServer::trigger(synceventset_t old_ready_events)
{
  if (((old_ready_events ^ mReadyEvents) & 1))
  {
	for (client_list_t::iterator client_iter = mClients.begin(); client_iter != mClients.end(); ++client_iter)
	{
	  if ((mReadyEvents & 1))
	  {
		client_iter->mClientPtr->event1_ready();
	  }
	  else
	  {
		client_iter->mClientPtr->event1_not_ready();
	  }
	}
  }
}
void AISyncServer::ready(synceventset_t events, synceventset_t yesno, AISyncClient* client)
{
#ifdef SYNC_TESTSUITE
  sanity_check();
#endif
  synceventset_t added_events = events & yesno;
  synceventset_t removed_events = events & ~yesno;
  synceventset_t remaining_ready_events = (synceventset_t)-1;
  synceventset_t remaining_pending_events = 0;
  client_list_t::iterator found_client = mClients.end();
  for (client_list_t::iterator client_iter = mClients.begin(); client_iter != mClients.end(); ++client_iter)
  {
	if (client_iter->mClientPtr == client)
	{
	  found_client = client_iter;
	}
	else
	{
	  remaining_ready_events &= client_iter->mReadyEvents;
	  remaining_pending_events |= client_iter->mReadyEvents;
	}
  }
  llassert(mReadyEvents == (remaining_ready_events & found_client->mReadyEvents));
  llassert(mPendingEvents == (remaining_pending_events | found_client->mReadyEvents));
  found_client->mReadyEvents &= ~removed_events;
  found_client->mReadyEvents |= added_events;
#ifdef SHOW_ASSERT
  client->mReadyEvents = found_client->mReadyEvents;
#endif
  synceventset_t old_ready_events = mReadyEvents;
  mReadyEvents = remaining_ready_events & found_client->mReadyEvents;
  mPendingEvents = remaining_pending_events | found_client->mReadyEvents;
  trigger(old_ready_events);
#ifdef SYNC_TESTSUITE
  sanity_check();
#endif
}
void intrusive_ptr_add_ref(AISyncServer* server)
{
  server->mRefCount++;
}
void intrusive_ptr_release(AISyncServer* server)
{
  llassert(server->mRefCount > 0);
  server->mRefCount--;
  if (server->mRefCount == 0)
  {
	delete server;
  }
  else if (server->mRefCount == 1)
  {
	AISyncServerMap::instance().remove_server(server);
  }
}
void AISyncServerMap::remove_server(AISyncServer* server)
{
  for (server_list_t::iterator iter = mServers.begin(); iter != mServers.end(); ++iter)
  {
	if (server == iter->get())
	{
	  mServers.erase(iter);
	  return;
	}
  }
  llassert(false);
}
#ifdef SYNC_TESTSUITE
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <boost/io/ios_state.hpp>
U64 LLFrameTimer::sFrameCount;
double innerloop_count = 0;
double LLFrameTimer::getCurrentTime()
{
  return innerloop_count * 0.001;
}
template<synckeytype_t synckeytype>
class TestsuiteKey : public AISyncKey
{
  private:
	int mIndex;
  public:
	TestsuiteKey(int index) : mIndex(index) { }
	int getIndex(void) const { return mIndex; }
  public:
	synckeytype_t getkeytype(void) const
	{
	  return synckeytype;
	}
	bool equals(AISyncKey const& key) const
	{
	  synckeytype_t const theotherkey = (synckeytype_t)(synckeytype ^ 0x100);
	  switch (key.getkeytype())
	  {
		case synckeytype:
		{
		  TestsuiteKey<synckeytype> const& test_key = static_cast<TestsuiteKey<synckeytype> const&>(key);
		  return (mIndex & 1) == (test_key.mIndex & 1);
		}
		case theotherkey:
		{
		  TestsuiteKey<theotherkey> const& test_key = static_cast<TestsuiteKey<theotherkey> const&>(key);
		  return (mIndex & 2) == (test_key.getIndex() & 2);
		}
		default:
		  break;
	  }
	  return false;
	}
};
template<synckeytype_t synckeytype>
class TestsuiteClient : public AISyncClient
{
  protected:
	AISyncKey* createSyncKey(void) const
	{
	  return new TestsuiteKey<synckeytype>(mIndex);
	}
  private:
	int mIndex;
	bool mRequestedRegistered;
	synceventset_t mRequestedReady;
	bool mActualReady1;
  public:
	TestsuiteClient() : mIndex(-1), mRequestedRegistered(false), mRequestedReady(0), mActualReady1(false) { }
	~TestsuiteClient() { if (is_registered()) this->ready(mRequestedReady, (synceventset_t)0); }
	void setIndex(int index) { mIndex = index; }
  protected:
	void event1_ready(void)
	{
#ifdef DEBUG_SYNCOUTPUT
	  Dout(dc::notice, "Calling TestsuiteClient<" << synckeytype << ">::event1_ready() (mIndex = " << mIndex << ") of client " << this);
#endif
	  llassert(!mActualReady1);
	  mActualReady1 = true;
	}
	void event1_not_ready(void)
	{
#ifdef DEBUG_SYNCOUTPUT
	  Dout(dc::notice, "Calling TestsuiteClient<" << synckeytype << ">::event1_not_ready() (mIndex = " << mIndex << ") of client " << this);
#endif
	  llassert(mActualReady1);
	  mActualReady1 = false;
	}
	void deregistered(void)
	{
#ifdef DEBUG_SYNCOUTPUT
	  DoutEntering(dc::notice, "TestsuiteClient<" << synckeytype << ">::deregistered(), with this = " << this);
#endif
	  mRequestedRegistered = false;
	  mRequestedReady = 0;
	  mActualReady1 = false;
	  this->mReadyEvents = 0;
	}
  private:
	bool is_registered(void) const { return this->server(); }
  public:
	void change_state(unsigned long r);
	bool getRequestedRegistered(void) const { return mRequestedRegistered; }
	synceventset_t getRequestedReady(void) const { return mRequestedReady; }
};
TestsuiteClient<synckeytype_test1a>* client1ap;
TestsuiteClient<synckeytype_test1b>* client1bp;
TestsuiteClient<synckeytype_test2a>* client2ap;
TestsuiteClient<synckeytype_test2b>* client2bp;
int const number_of_clients_per_syncgroup = 8;
template<synckeytype_t synckeytype>
void TestsuiteClient<synckeytype>::change_state(unsigned long r)
{
  bool change_registered = r & 1;
  r >>= 1;
  synceventset_t toggle_events = r & 15;
  r >>= 4;
  if (change_registered)
  {
	if (mRequestedRegistered && !mRequestedReady)
	{
	  mRequestedRegistered = false;
	  this->unregister_client();
	}
  }
  else if (toggle_events)
  {
	mRequestedReady ^= toggle_events;
	mRequestedRegistered = true;
	this->ready(toggle_events, mRequestedReady & toggle_events);
  }
  llassert(mRequestedRegistered == is_registered());
  AISyncServer* server = this->server();
  if (mRequestedRegistered)
  {
	synceventset_t all_ready = synceventset_t(-1);
	synceventset_t any_ready = 0;
	int nr = 0;
	for (int cl = 0; cl < number_of_clients_per_syncgroup; ++cl)
	{
	  switch ((synckeytype & 0xff))
	  {
		case syncgroup_test1:
		{
		  if (client1ap[cl].server() == server)
		  {
			if (client1ap[cl].getRequestedRegistered())
			{
			  ++nr;
			  all_ready &= client1ap[cl].getRequestedReady();
			  any_ready |= client1ap[cl].getRequestedReady();
			}
		  }
		  if (client1bp[cl].server() == server)
		  {
			if (client1bp[cl].getRequestedRegistered())
			{
			  ++nr;
			  all_ready &= client1bp[cl].getRequestedReady();
			  any_ready |= client1bp[cl].getRequestedReady();
			}
		  }
		  break;
		}
		case syncgroup_test2:
		{
		  if (client2ap[cl].server() == server)
		  {
			if (client2ap[cl].getRequestedRegistered())
			{
			  ++nr;
			  all_ready &= client2ap[cl].getRequestedReady();
			  any_ready |= client2ap[cl].getRequestedReady();
			}
		  }
		  if (client2bp[cl].server() == server)
		  {
			if (client2bp[cl].getRequestedRegistered())
			{
			  ++nr;
			  all_ready &= client2bp[cl].getRequestedReady();
			  any_ready |= client2bp[cl].getRequestedReady();
			}
		  }
		  break;
		}
	  }
	}
	llassert(nr == server->getClients().size());
	llassert(!!(all_ready & 1) == mActualReady1);
	llassert(this->server()->events_with_all_clients_ready() == all_ready);
	llassert(this->server()->events_with_at_least_one_client_ready() == any_ready);
	llassert(nr == 0 || (any_ready & all_ready) == all_ready);
  }
  llassert(mRequestedReady == this->mReadyEvents);
}
int main()
{
  Debug(libcw_do.on());
  Debug(dc::notice.on());
  Debug(libcw_do.set_ostream(&std::cout));
  Debug(list_channels_on(libcw_do));
  unsigned short seed16v[3] = { 0x1234, 0xfedc, 0x7091 };
  for (int k = 0;; ++k)
  {
	std::cout << "Loop: " << k << "; SEED: " << std::hex << seed16v[0] << ", " << seed16v[1] << ", " << seed16v[2] << std::dec << std::endl;
	++LLFrameTimer::sFrameCount;
	seed48(seed16v);
	seed16v[0] = lrand48() & 0xffff;
	seed16v[1] = lrand48() & 0xffff;
	seed16v[2] = lrand48() & 0xffff;
	TestsuiteClient<synckeytype_test1a> client1a[number_of_clients_per_syncgroup];
	TestsuiteClient<synckeytype_test1b> client1b[number_of_clients_per_syncgroup];
	TestsuiteClient<synckeytype_test2a> client2a[number_of_clients_per_syncgroup];
	TestsuiteClient<synckeytype_test2b> client2b[number_of_clients_per_syncgroup];
	client1ap = client1a;
	client1bp = client1b;
	client2ap = client2a;
	client2bp = client2b;
	for (int i = 0; i < number_of_clients_per_syncgroup; ++i)
	{
	  client1a[i].setIndex(i);
	  client1b[i].setIndex(i);
	  client2a[i].setIndex(i);
	  client2b[i].setIndex(i);
	}
	for (int j = 0; j < 1000000; ++j)
	{
	  innerloop_count += 1;
#ifdef DEBUG_SYNCOUTPUT
	  Dout(dc::notice, "Innerloop: " << j);
#endif
	  unsigned long r = lrand48();
	  synckeytype_t keytype = (r & 1) ? ((r & 2) ? synckeytype_test1a : synckeytype_test1b) : ((r & 2) ? synckeytype_test2a : synckeytype_test2b);
	  r >>= 2;
	  int cl = (r & 255) % number_of_clients_per_syncgroup;
	  r >>= 8;
	  switch (keytype)
	  {
		case synckeytype_test1a:
		  client1a[cl].change_state(r);
		  break;
		case synckeytype_test1b:
		  client1b[cl].change_state(r);
		  break;
		case synckeytype_test2a:
		  client2a[cl].change_state(r);
		  break;
		case synckeytype_test2b:
		  client2b[cl].change_state(r);
		  break;
	  }
	}
  }
}
#endif
