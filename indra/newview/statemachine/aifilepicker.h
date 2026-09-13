/**
 * @file aifilepicker.h
 * @brief File picker State machine
 *
 * Copyright (c) 2010, Aleric Inglewood.
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
 *   02/12/2010
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AIFILEPICKER_H
#define AIFILEPICKER_H
#include "aistatemachine.h"
#include "llpluginclassmedia.h"
#include "llviewerpluginmanager.h"
#include <vector>
enum ELoadFilter
{
	DF_DIRECTORY,
	FFLOAD_ALL,
	FFLOAD_WAV,
	FFLOAD_IMAGE,
	FFLOAD_ANIM,
	FFLOAD_XML,
	FFLOAD_SLOBJECT,
	FFLOAD_RAW,
	FFLOAD_MODEL,
	FFLOAD_COLLADA,
	FFLOAD_SCRIPT,
	FFLOAD_DICTIONARY,
	FFLOAD_INVGZ,
	FFLOAD_AO,
	FFLOAD_BLACKLIST
};
enum ESaveFilter
{
	FFSAVE_ALL,
	FFSAVE_WAV,
	FFSAVE_TGA,
	FFSAVE_BMP,
	FFSAVE_AVI,
	FFSAVE_ANIM,
	FFSAVE_XML,
	FFSAVE_COLLADA,
	FFSAVE_RAW,
	FFSAVE_J2C,
	FFSAVE_PNG,
	FFSAVE_JPEG,
	FFSAVE_SCRIPT,
	FFSAVE_ANIMATN,
	FFSAVE_OGG,
	FFSAVE_NOTECARD,
	FFSAVE_GESTURE,
	FFSAVE_SHAPE,
	FFSAVE_SKIN,
	FFSAVE_HAIR,
	FFSAVE_EYES,
	FFSAVE_SHIRT,
	FFSAVE_PANTS,
	FFSAVE_SHOES,
	FFSAVE_SOCKS,
	FFSAVE_JACKET,
	FFSAVE_GLOVES,
	FFSAVE_UNDERSHIRT,
	FFSAVE_UNDERPANTS,
	FFSAVE_SKIRT,
	FFSAVE_INVGZ,
	FFSAVE_LANDMARK,
	FFSAVE_AO,
	FFSAVE_BLACKLIST,
	FFSAVE_PHYSICS,
	FFSAVE_IMAGE,
};
class AIFilePicker : public AIStateMachine {
	LOG_CLASS(AIFilePicker);
protected:
	typedef AIStateMachine direct_base_type;
	enum filepicker_state_type {
		AIFilePicker_initialize_plugin = direct_base_type::max_state,
		AIFilePicker_plugin_running,
		AIFilePicker_canceled,
		AIFilePicker_done
	};
public:
	static state_type const max_state = AIFilePicker_done + 1;
public:
	AIFilePicker(CWD_ONLY(bool debug = false));
	static AIFilePicker* create(void) { AIFilePicker* filepicker = new AIFilePicker; return filepicker; }
	void open(std::string const& filename, ESaveFilter filter = FFSAVE_ALL, std::string const& default_path = "", std::string const& context = "savefile");
	void open(ELoadFilter filter = FFLOAD_ALL, std::string const& default_path = "", std::string const& context = "openfile", bool multiple = false);
	bool isCanceled(void) const { return mCanceled; }
	bool hasFilename(void) const { return *this && !mCanceled; }
	std::string const& getFilename(void) const;
	std::string getFolder(void) const;
	std::vector<std::string> const& getFilenames(void) const { return mFilenames; }
	const char* getName() const { return "AIFilePicker"; }
	static bool loadFile(std::string const& filename);
	static bool saveFile(std::string const& filename);
private:
	friend class AIPluginFilePicker;
    void receivePluginMessage(LLPluginMessage const& message);
public:
	enum open_type { save, load, load_multiple };
	static AIFilePicker* activePicker;
private:
	LLPointer<LLViewerPluginManager> mPluginManager;
	typedef std::map<std::string, std::string> context_map_type;
	static AIThreadSafeSimpleDC<context_map_type> sContextMap;
	std::string mContext;
	open_type mOpenType;
	std::string mFilter;
	std::string mFilename;
	std::string mFolder;
	bool mCanceled;
	std::vector<std::string> mFilenames;
	static void store_folder(std::string const& context, std::string const& folder);
	static std::string get_folder(std::string const& default_path, std::string const& context);
protected:
	~AIFilePicker() { LL_DEBUGS("Plugin") << "Calling AIFilePicker::~AIFilePicker()" << LL_ENDL; }
	void initialize_impl(void);
	void multiplex_impl(state_type run_state);
	void finish_impl(void);
	char const* state_str_impl(state_type run_state) const;
};
class AIPluginFilePicker : public LLPluginClassBasic {
	LOG_CLASS(AIPluginFilePicker);
public:
	AIPluginFilePicker(AIFilePicker* state_machine) : mStateMachine(state_machine) { }
	~AIPluginFilePicker() { LL_DEBUGS("Plugin") << "Calling AIPluginFilePicker::~AIPluginFilePicker()" << LL_ENDL; }
	static std::string launcher_name(void) { return gDirUtilp->getLLPluginLauncher(); }
	static char const* plugin_basename(void) { return "basic_plugin_filepicker"; }
	void receivePluginMessage(LLPluginMessage const& message) { mStateMachine->receivePluginMessage(message); }
	void receivedShutdown(void) { }
private:
	AIFilePicker* mStateMachine;
};
#endif
