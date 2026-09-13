/**
 * @file aicondition.h
 * @brief Condition variable for statemachines.
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
 *   14/10/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AICONDITION_H
#define AICONDITION_H
#include <deque>
#include <llpointer.h>
#include "aithreadsafe.h"
class AIStateMachine;
class LLMutex;
class AIConditionBase
{
  public:
	virtual ~AIConditionBase() { }
	void signal(int n = 1);
	void broadcast(void) { signal(mWaitingStateMachines.size()); }
  private:
	friend class AIStateMachine;
	void wait(AIStateMachine* state_machine);
	void remove(AIStateMachine* state_machine);
  protected:
	virtual LLMutex& mutex(void)  = 0;
  protected:
	typedef std::deque<LLPointer<AIStateMachine> > queue_t;
	queue_t mWaitingStateMachines;
};
template<typename T>
class AICondition : public AIThreadSafeSimpleDC<T>, public AIConditionBase
{
  protected:
	LLMutex& mutex(void) { return this->mMutex; }
};
#endif
