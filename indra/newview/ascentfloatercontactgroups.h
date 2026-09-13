/** 
 * @file ascentfloatercontactgroups.h
 * @Author Charley Levenque
 * Allows the user to assign friends to contact groups for advanced sorting.
 *
 * Created Sept 6th 2010
 * 
 * ALL SOURCE CODE IS PROVIDED "AS IS." THE CREATOR MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 *
 * "use a free softwares" - Richard Stallman, LUNIX Operations System
*/
#ifndef ASCENT_CONTACT_GROUPS
#define ASCENT_CONTACT_GROUPS
#include "llfloater.h"
#include "llsdserialize.h"
class LLScrollListCtrl;
class ASFloaterContactGroups : public LLFloater
{
public:
    ASFloaterContactGroups();
    virtual ~ASFloaterContactGroups();
    static void show(const uuid_vec_t& ids);
	void populateGroupList();
	void populateActiveGroupList(LLUUID to_add);
	void populateFriendList();
	void addContactMember(std::string contact_grp, LLUUID to_add);
	void createContactGroup(std::string contact_grp);
	void deleteContactGroup(std::string contact_grp);
	static void onBtnAdd(void* userdata);
	static void onBtnRemove(void* userdata);
	static void onBtnCreate(void* userdata);
	static void onBtnDelete(void* userdata);
private:
    static ASFloaterContactGroups* sInstance;
	static uuid_vec_t mSelectedUUIDs;
	static LLSD mContactGroupData;
};
#endif
