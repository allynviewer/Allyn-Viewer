/**
 * @file aifetchinventoryfolder.h
 * @brief Fetch an inventory folder
 *
 * Copyright (c) 2011, Aleric Inglewood.
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
 *   19/05/2011
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AIFETCHINVENTORYFOLDER_H
#define AIFETCHINVENTORYFOLDER_H
#include "aistatemachine.h"
#include "lluuid.h"
#include <map>
class AIFetchInventoryFolder : public AIStateMachine {
  private:
	std::string mFolderName;
	bool mCreate;
	bool mFetchContents;
	LLUUID mParentFolder;
	LLUUID mFolderUUID;
	bool mExists;
	bool mCreated;
	bool mNeedNotifyObservers;
  public:
	AIFetchInventoryFolder(CWD_ONLY(bool debug = false)) :
#ifdef CWDEBUG
		AIStateMachine(debug),
#endif
		mCreate(false), mFetchContents(false), mExists(false), mCreated(false)
        { Dout(dc::statemachine(mSMDebug), "Calling AIFetchInventoryFolder constructor [" << (void*)this << "]"); }
	void fetch(LLUUID const& parentUUID, std::string const& foldername, bool create = false, bool fetch_contents = true)
	{
	  mParentFolder = parentUUID;
	  mCreate = create;
	  mFetchContents = fetch_contents;
	  if (mFolderName != foldername)
	  {
		mFolderName = foldername;
		mFolderUUID.setNull();
	  }
	}
	void fetch(std::string const& foldername, bool create = false, bool fetch_contents = true);
	void fetch(LLUUID const& folderUUID, bool fetch_contents = true)
	{
	  mFetchContents = fetch_contents;
	  if (mFolderUUID != folderUUID)
	  {
		mFolderName.clear();
		mFolderUUID = folderUUID;
	  }
	}
	std::string const& name(void) const { return mFolderName; }
	bool exists(void) const { return mExists; }
	bool created(void) const { return mCreated; }
	LLUUID const& UUID(void) const { llassert(mExists || mFolderUUID.isNull()); return mFolderUUID; }
  protected:
	~AIFetchInventoryFolder() { Dout(dc::statemachine(mSMDebug), "Calling ~AIFetchInventoryFolder() [" << (void*)this << "]"); }
	void initialize_impl(void);
	void multiplex_impl(state_type run_state);
	void finish_impl(void);
	char const* state_str_impl(state_type run_state) const;
};
#endif
