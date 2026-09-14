/**
 * @file llfloaternotranslate.cpp
 * @brief Floater listing residents excluded from chat/IM translation
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "llfloaternotranslate.h"
#include "llchataitranslate.h"
#include "llnamelistctrl.h"
#include "lluictrlfactory.h"
LLFloaterNoTranslate::LLFloaterNoTranslate(const LLSD&)
:	LLFloater(std::string("no translate floater"))
,	mList(NULL)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_no_translate.xml");
}
LLFloaterNoTranslate::~LLFloaterNoTranslate()
{
}
BOOL LLFloaterNoTranslate::postBuild()
{
	mList = getChild<LLNameListCtrl>("no_translate_list");
	mList->setCommitOnSelectionChange(TRUE);
	childSetCommitCallback("no_translate_list", boost::bind(&LLFloaterNoTranslate::updateButtons, this));
	childSetAction("reactivate_btn", boost::bind(&LLFloaterNoTranslate::onReactivate, this));
	refreshList();
	return TRUE;
}
void LLFloaterNoTranslate::onOpen()
{
	refreshList();
}
void LLFloaterNoTranslate::onClose(bool app_quitting)
{
	app_quitting ? destroy() : setVisible(FALSE);
}
void LLFloaterNoTranslate::refreshIfOpen()
{
	if (LLFloaterNoTranslate* floater = findInstance())
		floater->refreshList();
}
void LLFloaterNoTranslate::refreshList()
{
	if (!mList)
		return;
	uuid_vec_t selected = mList->getSelectedIDs();
	S32 scrollpos = mList->getScrollPos();
	mList->deleteAllItems();
	const LLSD agents = LLChatAITranslate::instance().getNoTranslateAgents();
	if (agents.isMap())
	{
		for (LLSD::map_const_iterator it = agents.beginMap(); it != agents.endMap(); ++it)
		{
			LLUUID id(it->first);
			if (id.isNull())
				continue;
			LLNameListCtrl::NameItem element;
			element.value = id;
			element.name = it->second.asString();
			element.target = LLNameListItem::INDIVIDUAL;
			mList->addNameItemRow(element);
		}
		mList->updateSort();
	}
	mList->selectMultiple(selected);
	mList->setScrollPos(scrollpos);
	updateButtons();
}
void LLFloaterNoTranslate::updateButtons()
{
	getChildView("reactivate_btn")->setEnabled(mList && mList->getFirstSelected());
}
void LLFloaterNoTranslate::onReactivate()
{
	if (!mList)
		return;
	uuid_vec_t ids = mList->getSelectedIDs();
	if (ids.empty())
		return;
	LLChatAITranslate::instance().removeAgentsNoTranslate(ids);
}
