/**
 * @file aidirpicker.h
 * @brief Directory picker State machine
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
 *   10/05/2011
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AIDIRPICKER_H
#define AIDIRPICKER_H
#include "aifilepicker.h"
class AIDirPicker : protected AIFilePicker {
	LOG_CLASS(AIDirPicker);
public:
	AIDirPicker(std::string const& default_path = "", std::string const& context = "openfile") { open(default_path, context); }
	void open(std::string const& default_path = "", std::string const& context = "openfile") { AIFilePicker::open(DF_DIRECTORY, default_path, context); }
	bool hasDirname(void) const { return hasFilename(); }
	std::string const& getDirname(void) const { return getFilename(); }
	const char* getName() const { return "AIDirPicker"; }
public:
	using AIStateMachine::state_type;
	using AIFilePicker::isCanceled;
	using AIStateMachine::run;
protected:
	~AIDirPicker() { LL_DEBUGS("Plugin") << "Calling AIDirPicker::~AIDirPicker()" << LL_ENDL; }
};
#endif
