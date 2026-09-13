#ifndef LL_LLFLOATERBLACKLIST_H
#define LL_LLFLOATERBLACKLIST_H
#include "llfloater.h"
class AIFilePicker;
class LLFloaterBlacklist : LLFloater
{
public:
	LLFloaterBlacklist();
	~LLFloaterBlacklist();
	static void show();
	static void toggle();
	static BOOL visible();
	BOOL postBuild();
	void refresh();
	static LLFloaterBlacklist* getInstance() { return sInstance; };
	static void addEntry(LLUUID key, LLSD data);
	static boost::unordered_map<LLUUID,LLSD> blacklist_entries;
	static uuid_vec_t blacklist_textures;
	static uuid_vec_t blacklist_objects;
	static void loadFromSave();
protected:
	LLUUID mSelectID;
private:
	static LLFloaterBlacklist* sInstance;
	static void updateBlacklists();
	static void saveToDisk();
	static void onClickAdd(void* user_data);
	static void onClickClear(void* user_data);
	static void onClickSave(void* user_data);
	static void onClickSave_continued(AIFilePicker* filepicker);
	static void onClickLoad(void* user_data);
	static void onClickLoad_continued(AIFilePicker* filepicker);
	static void onClickRerender(void* user_data);
	static void onClickCopyUUID(void* user_data);
	static void onClickRemove(void* user_data);
};
#endif
