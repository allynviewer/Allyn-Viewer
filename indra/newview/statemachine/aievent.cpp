/**
 * @file aievent.cpp
 * @brief Implementation of AIEvent.
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
#include "linden_common.h"
#include "aievent.h"
#include "aistatemachine.h"
#include <map>
struct AIRSData {
	bool mOneShot;
	bool mRunning;
	AIRSData(void) : mRunning(false) { }
	AIRSData(bool one_shot) : mOneShot(one_shot), mRunning(false) { }
	void operator=(bool one_shot) { mOneShot = one_shot; }
};
struct AIRegisteredStateMachines {
	typedef std::map<AIStateMachine*, AIRSData> rsm_type;
	rsm_type mRegisteredStateMachines;
	void Register(AIStateMachine* statemachine, bool one_shot);
	void Unregister(AIStateMachine* statemachine);
	void trigger(void);
};
inline void AIRegisteredStateMachines::Register(AIStateMachine* statemachine, bool one_shot)
{
	mRegisteredStateMachines[statemachine] = one_shot;
}
inline void AIRegisteredStateMachines::Unregister(AIStateMachine* statemachine)
{
	rsm_type::iterator sm = mRegisteredStateMachines.find(statemachine);
	if (sm != mRegisteredStateMachines.end())
	{
		if (!sm->second.mRunning)
		{
			mRegisteredStateMachines.erase(sm);
		}
		else
		{
			sm->second.mOneShot = true;
		}
	}
}
inline void AIRegisteredStateMachines::trigger(void)
{
	rsm_type::iterator sm = mRegisteredStateMachines.begin();
	while(sm != mRegisteredStateMachines.end())
	{
		sm->second.mRunning = true;
		sm->first->cont();
		sm->second.mRunning = false;
		if (sm->second.mOneShot)
			mRegisteredStateMachines.erase(sm++);
		else
			++sm;
	}
}
static AIThreadSafeSimpleDC<AIRegisteredStateMachines> registered_statemachines_list[AIEvent::number_of_events];
typedef AIAccess<AIRegisteredStateMachines> registered_statemachines_wat;
void AIEvent::Register(AIEvents event, AIStateMachine* statemachine, bool one_shot)
{
	registered_statemachines_wat registered_statemachines_w(registered_statemachines_list[event]);
	registered_statemachines_w->Register(statemachine, one_shot);
}
void AIEvent::Unregister(AIEvents event, AIStateMachine* statemachine)
{
	registered_statemachines_wat registered_statemachines_w(registered_statemachines_list[event]);
	registered_statemachines_w->Unregister(statemachine);
}
void AIEvent::trigger(AIEvents event)
{
	registered_statemachines_wat registered_statemachines_w(registered_statemachines_list[event]);
	registered_statemachines_w->trigger();
}
