/**
* @file llfloaterpathfindingcharacters.cpp
* @brief "Pathfinding characters" floater, allowing for identification of pathfinding characters and their cpu usage.
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/
#include "llviewerprecompiledheaders.h"
#include "llfloaterpathfindingcharacters.h"
#include "llpathfindingcharacter.h"
#include "llpathfindingcharacterlist.h"
#include "lluictrlfactory.h"
void LLFloaterPathfindingCharacters::openCharactersWithSelectedObjects()
{
	LLFloaterPathfindingCharacters* charactersFloater = getInstance();
	charactersFloater->showFloaterWithSelectionObjects();
}
LLFloaterPathfindingCharacters::LLFloaterPathfindingCharacters(const LLSD& pSeed)
	: LLFloaterPathfindingObjects(),
	mSelectedCharacterId(),
	mBeaconColor()
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_pathfinding_characters.xml");
}
LLFloaterPathfindingCharacters::~LLFloaterPathfindingCharacters()
{
}
BOOL LLFloaterPathfindingCharacters::postBuild()
{
	mBeaconColor = LLUI::sColorsGroup->getColor("PathfindingCharacterBeaconColor");
	return LLFloaterPathfindingObjects::postBuild();
}
void LLFloaterPathfindingCharacters::requestGetObjects()
{
	LLPathfindingManager::getInstance()->requestGetCharacters(getNewRequestId(), boost::bind(&LLFloaterPathfindingCharacters::handleNewObjectList, this, _1, _2, _3));
}
void LLFloaterPathfindingCharacters::buildObjectsScrollList(const LLPathfindingObjectListPtr pObjectListPtr)
{
	llassert(pObjectListPtr != NULL);
	llassert(!pObjectListPtr->isEmpty());
	for (LLPathfindingObjectList::const_iterator objectIter = pObjectListPtr->begin();	objectIter != pObjectListPtr->end(); ++objectIter)
	{
		const LLPathfindingObjectPtr objectPtr = objectIter->second;
		const LLPathfindingCharacter *characterPtr = dynamic_cast<const LLPathfindingCharacter *>(objectPtr.get());
		llassert(characterPtr != NULL);
		LLSD scrollListItemData = buildCharacterScrollListItemData(characterPtr);
		addObjectToScrollList(objectPtr, scrollListItemData);
	}
}
S32 LLFloaterPathfindingCharacters::getNameColumnIndex() const
{
	return 0;
}
S32 LLFloaterPathfindingCharacters::getOwnerNameColumnIndex() const
{
	return 2;
}
std::string LLFloaterPathfindingCharacters::getOwnerName(const LLPathfindingObject *pObject) const
{
	return (pObject->hasOwner()
		? (pObject->hasOwnerName()
		? (pObject->isGroupOwned()
		? (pObject->getOwnerName() + " " + getString("character_owner_group"))
		: pObject->getOwnerName())
		: getString("character_owner_loading"))
		: getString("character_owner_unknown"));
}
const LLColor4 &LLFloaterPathfindingCharacters::getBeaconColor() const
{
	return mBeaconColor;
}
LLPathfindingObjectListPtr LLFloaterPathfindingCharacters::getEmptyObjectList() const
{
	LLPathfindingObjectListPtr objectListPtr(new LLPathfindingCharacterList());
	return objectListPtr;
}
LLSD LLFloaterPathfindingCharacters::buildCharacterScrollListItemData(const LLPathfindingCharacter *pCharacterPtr) const
{
	LLSD columns = LLSD::emptyArray();
	columns[0]["column"] = "name";
	columns[0]["value"] = pCharacterPtr->getName();
	columns[1]["column"] = "description";
	columns[1]["value"] = pCharacterPtr->getDescription();
	columns[2]["column"] = "owner";
	columns[2]["value"] = getOwnerName(pCharacterPtr);
	S32 cpuTime = ll_round(pCharacterPtr->getCPUTime());
	std::string cpuTimeString = llformat("%d", cpuTime);
	LLStringUtil::format_map_t string_args;
	string_args["[CPU_TIME]"] = cpuTimeString;
	columns[3]["column"] = "cpu_time";
	columns[3]["value"] = getString("character_cpu_time", string_args);
	columns[4]["column"] = "altitude";
	columns[4]["value"] = llformat("%1.0f m", pCharacterPtr->getLocation()[2]);
	return columns;
}
