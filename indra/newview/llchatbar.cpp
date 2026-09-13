/** 
 * @file llchatbar.cpp
 * @brief LLChatBar class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "llchatbar.h"
#include "imageids.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llparcel.h"
#include "llstring.h"
#include "message.h"
#include "llfocusmgr.h"
#include "llagent.h"
#include "llautoreplace.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llcommandhandler.h"
#include "llviewercontrol.h"
#include "llfloaterchat.h"
#include "llchataitranslate.h"
#include "llgesturemgr.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llstatusbar.h"
#include "lltextbox.h"
#include "lluiconstants.h"
#include "llviewergesture.h"
#include "llviewermenu.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llframetimer.h"
#include "llresmgr.h"
#include "llworld.h"
#include "llinventorymodel.h"
#include "llmultigesture.h"
#include "llui.h"
#include "llviewermenu.h"
#include "lluictrlfactory.h"
#include "chatbar_as_cmdline.h"
#include "rlvcommon.h"
#include "rlvactions.h"
#include "rlvhandler.h"
#if SHY_MOD
#include "llvoavatarself.h"
#include "shcommandhandler.h"
#endif
const F32 AGENT_TYPING_TIMEOUT = 5.f;
LLChatBar *gChatBar = NULL;
void toggleChatHistory();
void send_chat_from_viewer(std::string utf8_out_text, EChatType type, S32 channel);
void really_send_chat_from_viewer(const std::string& utf8_out_text, EChatType type, S32 channel);
class LLChatBarGestureObserver : public LLGestureManagerObserver
{
public:
	LLChatBarGestureObserver(LLChatBar* chat_barp) : mChatBar(chat_barp){}
	virtual ~LLChatBarGestureObserver() = default;
	void changed() override { mChatBar->refreshGestures(); }
private:
	LLChatBar* mChatBar;
};
LLChatBar::LLChatBar()
:	LLPanel(),
	mInputEditor(nullptr),
	mGestureLabelTimer(),
	mLastSpecialChatChannel(0),
	mIsBuilt(FALSE),
	mGestureCombo(nullptr),
	mObserver(nullptr)
{
	setIsChrome(TRUE);
	#if !LL_RELEASE_FOR_DOWNLOAD
	childDisplayNotFound();
#endif
}
LLChatBar::~LLChatBar()
{
	LLGestureMgr::instance().removeObserver(mObserver);
	delete mObserver;
	mObserver = nullptr;
}
BOOL LLChatBar::postBuild()
{
	if (LLUICtrl* history_ctrl = findChild<LLUICtrl>("History"))
		history_ctrl->setCommitCallback(boost::bind(&toggleChatHistory));
	if (LLUICtrl* say_ctrl = getChild<LLUICtrl>("Say"))
		say_ctrl->setCommitCallback(boost::bind(&LLChatBar::onClickSay, this, _1));
	if (LLComboBox* gesture_combo = findChild<LLComboBox>("Gesture"))
		setGestureCombo(gesture_combo);
	mInputEditor = findChild<LLLineEditor>("Chat Editor");
	if (mInputEditor)
	{
		mInputEditor->setAutoreplaceCallback(boost::bind(&LLAutoReplace::autoreplaceCallback, LLAutoReplace::getInstance(), _1, _2, _3, _4, _5));
		mInputEditor->setKeystrokeCallback(boost::bind(&LLChatBar::onInputEditorKeystroke,this));
		mInputEditor->setFocusLostCallback(boost::bind(&LLChatBar::onInputEditorFocusLost));
		mInputEditor->setFocusReceivedCallback(boost::bind(&LLChatBar::onInputEditorGainFocus));
		mInputEditor->setCommitOnFocusLost( FALSE );
		mInputEditor->setRevertOnEsc( FALSE );
		mInputEditor->setIgnoreTab(TRUE);
		mInputEditor->setPassDelete(TRUE);
		mInputEditor->setReplaceNewlinesWithSpaces(FALSE);
		mInputEditor->setEnableLineHistory(TRUE);
	}
	mIsBuilt = TRUE;
	return TRUE;
}
BOOL LLChatBar::handleKeyHere( KEY key, MASK mask )
{
	BOOL handled = FALSE;
	if( KEY_RETURN == key )
	{
		if (mask == MASK_CONTROL)
		{
			sendChat(CHAT_TYPE_SHOUT);
			handled = TRUE;
		}
		else if (mask == MASK_SHIFT)
		{
			sendChat(CHAT_TYPE_WHISPER);
			handled = TRUE;
		}
		else if (mask == MASK_NONE)
		{
			sendChat( CHAT_TYPE_NORMAL );
			handled = TRUE;
		}
	}
	else if (KEY_ESCAPE == key && mask == MASK_NONE && gChatBar == this)
	{
		stopChat();
		handled = TRUE;
	}
	return handled;
}
void LLChatBar::onFocusLost()
{
}
void LLChatBar::refresh()
{
	const F32 SHOW_GESTURE_NAME_TIME = 2.f;
	if (mGestureLabelTimer.getStarted() && mGestureLabelTimer.getElapsedTimeF32() > SHOW_GESTURE_NAME_TIME)
	{
		LLCtrlListInterface* gestures = mGestureCombo ? mGestureCombo->getListInterface() : NULL;
		if (gestures) gestures->selectFirstItem();
		mGestureLabelTimer.stop();
	}
	if ((gAgent.getTypingTime() > AGENT_TYPING_TIMEOUT) && (gAgent.getRenderState() & AGENT_STATE_TYPING))
	{
		gAgent.stopTyping();
	}
	if (LLUICtrl* history_ctrl = findChild<LLUICtrl>("History"))
		history_ctrl->setValue(LLFloaterChat::instanceVisible());
	if (LLUICtrl* say_ctrl = getChild<LLUICtrl>("Say"))
		say_ctrl->setEnabled(mInputEditor->getText().size() > 0);
}
void LLChatBar::refreshGestures()
{
	if (mGestureCombo)
	{
		std::string cur_gesture = mGestureCombo->getValue().asString();
		mGestureCombo->selectFirstItem();
		mGestureCombo->clearRows();
		std::map <std::string, BOOL> unique;
		const LLGestureMgr::item_map_t& active_gestures = LLGestureMgr::instance().getActiveGestures();
		for (const auto& active_gesture : active_gestures)
		{
			LLMultiGesture* gesture = active_gesture.second;
			if (gesture)
			{
				if (!gesture->mTrigger.empty())
				{
					unique[gesture->mTrigger] = TRUE;
				}
			}
		}
		for (auto& it2 : unique)
		{
			mGestureCombo->addSimpleElement(it2.first);
		}
		mGestureCombo->sortByName();
		mGestureCombo->addSeparator(ADD_TOP);
		mGestureCombo->addSimpleElement(getString("gesture_label"), ADD_TOP);
		if (!cur_gesture.empty())
		{
			mGestureCombo->selectByValue(LLSD(cur_gesture));
		}
		else
		{
			mGestureCombo->selectFirstItem();
		}
	}
}
void LLChatBar::setKeyboardFocus(BOOL focus)
{
	if (focus)
	{
		if (mInputEditor)
		{
			mInputEditor->setFocus(TRUE);
			mInputEditor->selectAll();
		}
	}
	else if (gFocusMgr.childHasKeyboardFocus(this))
	{
		if (mInputEditor)
		{
			mInputEditor->deselect();
		}
		setFocus(FALSE);
	}
}
void LLChatBar::setIgnoreArrowKeys(BOOL b)
{
	if (mInputEditor)
	{
		mInputEditor->setIgnoreArrowKeys(b);
	}
}
BOOL LLChatBar::inputEditorHasFocus() const
{
	return mInputEditor && mInputEditor->hasFocus();
}
std::string LLChatBar::getCurrentChat() const
{
	return mInputEditor ? mInputEditor->getText() : LLStringUtil::null;
}
void LLChatBar::setGestureCombo(LLComboBox* combo)
{
	mGestureCombo = combo;
	if (mGestureCombo)
	{
		mGestureCombo->setCommitCallback(boost::bind(&LLChatBar::onCommitGesture, this, _1));
		mObserver = new LLChatBarGestureObserver(this);
		LLGestureMgr::instance().addObserver(mObserver);
		refreshGestures();
	}
}
LLWString LLChatBar::stripChannelNumber(const LLWString &mesg, S32* channel)
{
	if (mesg[0] == '/'
		&& mesg[1] == '/')
	{
		*channel = mLastSpecialChatChannel;
		return mesg.substr(2, mesg.length() - 2);
	}
	else if (mesg[0] == '/'
			 && mesg[1]
			 && ( LLStringOps::isDigit(mesg[1])
				|| mesg[1] == '-' ))
	{
		S32 pos = 0;
		if(mesg[1] == '-')
			pos++;
		LLWString channel_string;
		llwchar c;
		do
		{
			c = mesg[pos+1];
			channel_string.push_back(c);
			pos++;
		}
		while(c && pos < 64 && LLStringOps::isDigit(c));
		while(c && iswspace(c))
		{
			c = mesg[pos+1];
			pos++;
		}
		mLastSpecialChatChannel = strtol(wstring_to_utf8str(channel_string).c_str(), nullptr, 10);
		if(mesg[1] == '-')
			mLastSpecialChatChannel = -mLastSpecialChatChannel;
		*channel = mLastSpecialChatChannel;
		return mesg.substr(pos, mesg.length() - pos);
	}
	else
	{
		*channel = 0;
		return mesg;
	}
}
bool convert_roleplay_text(std::string& utf8text)
{
	bool action(true);
	if (utf8text.find("/ME'") == 0 || utf8text.find("/ME ") == 0)
	{
		utf8text.replace(1, 2, "me");
	}
	else if (gSavedSettings.getBOOL("AscentAllowMUpose") && utf8text.length() > 3 && utf8text.find(":") == 0)
	{
		if (utf8text[1] == '\'')
			utf8text.replace(0, 1, "/me");
		else if (isalpha(utf8text[1]))
			utf8text.replace(0, 1, "/me ");
		else
			action = false;
	}
	else if (utf8text.find("/me'") != 0 && utf8text.find("/me ") != 0)
	{
		action = false;
	}
	if (gSavedSettings.getBOOL("AscentAutoCloseOOC") && (utf8text.length() > 3))
	{
		const U32 pos(action ? 4 : 0);
		if (utf8text.find("((") == pos && utf8text.find("))") == std::string::npos)
		{
			if (*utf8text.rbegin() == ')')
				utf8text += " ";
			utf8text += "))";
		}
		else if (utf8text.find("[[") == pos && utf8text.find("]]") == std::string::npos)
		{
			if (*utf8text.rbegin() == ']')
				utf8text += " ";
			utf8text += "]]";
		}
		else if (utf8text.find("((") == std::string::npos && utf8text.find("))") == (utf8text.length() - 2))
		{
			if (utf8text.at(pos) == '(')
				utf8text.insert(pos, " ");
			utf8text.insert(pos, "((");
		}
		else if (utf8text.find("[[") == std::string::npos && utf8text.find("]]") == (utf8text.length() - 2))
		{
			if (utf8text.at(pos) == '[')
				utf8text.insert(pos, " ");
			utf8text.insert(pos, "[[");
		}
	}
	return action;
}
void LLChatBar::sendChat( EChatType type )
{
	if (mInputEditor)
	{
		LLWString text = mInputEditor->getConvertedText();
		if (!text.empty())
		{
			if (mInputEditor) mInputEditor->updateHistory();
			S32 channel = 0;
			stripChannelNumber(text, &channel);
			std::string utf8text = wstring_to_utf8str(text);
			std::string utf8_revised_text;
			if (0 == channel)
			{
				convert_roleplay_text(utf8text);
				LLGestureMgr::instance().triggerAndReviseString(utf8text, &utf8_revised_text);
			}
			else
			{
				utf8_revised_text = utf8text;
			}
			utf8_revised_text = utf8str_trim(utf8_revised_text);
			EChatType nType(type == CHAT_TYPE_OOC ? CHAT_TYPE_NORMAL : type);
			if (!utf8_revised_text.empty() && cmd_line_chat(utf8_revised_text, nType))
			{
#if SHY_MOD
				if(!SHCommandHandler::handleCommand(true, utf8_revised_text, gAgentID, (LLViewerObject*)gAgentAvatarp))
#endif
				sendChatFromViewer(utf8_revised_text, nType, TRUE);
			}
		}
	}
	childSetValue("Chat Editor", LLStringUtil::null);
	gAgent.stopTyping();
	if (gChatBar == this && gSavedSettings.getBOOL("CloseChatOnReturn"))
	{
		stopChat();
	}
}
void LLChatBar::startChat(const char* line)
{
	if (!gChatBar || !gChatBar->getParent())
	{
		return;
	}
	gChatBar->getParent()->setVisible(TRUE);
	gChatBar->setKeyboardFocus(TRUE);
	gSavedSettings.setBOOL("ChatVisible", TRUE);
	if (gChatBar->mInputEditor)
	{
		if (line)
		{
			std::string line_string(line);
			gChatBar->mInputEditor->setText(line_string);
		}
		gChatBar->mInputEditor->setCursorToEnd();
	}
}
void LLChatBar::stopChat()
{
	if (gChatBar)
		gChatBar->setKeyboardFocus(FALSE);
	gKeyboard->resetKeys();
	gKeyboard->resetMaskKeys();
	gAgent.stopTyping();
	if (gChatBar && gChatBar->getParent())
		gChatBar->getParent()->setVisible(FALSE);
	gSavedSettings.setBOOL("ChatVisible", FALSE);
}
void LLChatBar::onInputEditorKeystroke()
{
	LLWString raw_text;
	if (mInputEditor) raw_text = mInputEditor->getWText();
	LLWStringUtil::trimHead(raw_text);
	S32 length = raw_text.length();
	if ( (length > 0) && (raw_text[0] != '/') && (raw_text[0] != ':') && (!RlvActions::hasBehaviour(RLV_BHVR_REDIRCHAT)) )
	{
		gAgent.startTyping();
	}
	else
	{
		gAgent.stopTyping();
	}
	KEY key = gKeyboard->currentKey();
	if (length > 1
		&& raw_text[0] == '/'
		&& key < KEY_SPECIAL)
	{
		std::string utf8_trigger = wstring_to_utf8str(raw_text);
		std::string utf8_out_str(utf8_trigger);
		if (LLGestureMgr::instance().matchPrefix(utf8_trigger, &utf8_out_str))
		{
			if (mInputEditor)
			{
				std::string rest_of_match = utf8_out_str.substr(utf8_trigger.size());
				mInputEditor->setText(utf8_trigger + rest_of_match);
				S32 outlength = mInputEditor->getLength();
				mInputEditor->setSelection(length, outlength);
			}
		}
	}
}
void LLChatBar::onInputEditorFocusLost()
{
	gAgent.stopTyping();
}
void LLChatBar::onInputEditorGainFocus()
{
	if (gSavedSettings.getBOOL("LiruLegacyScrollToEnd"))
		LLFloaterChat::setHistoryCursorAndScrollToEnd();
}
void LLChatBar::onClickSay( LLUICtrl* ctrl )
{
	std::string cmd = ctrl->getValue().asString();
	e_chat_type chat_type = CHAT_TYPE_NORMAL;
	if (cmd == "shout")
	{
		chat_type = CHAT_TYPE_SHOUT;
	}
	else if (cmd == "whisper")
	{
		chat_type = CHAT_TYPE_WHISPER;
	}
	sendChat(chat_type);
}
void LLChatBar::sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate)
{
	sendChatFromViewer(utf8str_to_wstring(utf8text), type, animate);
}
void LLChatBar::sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate)
{
	S32 channel = gSavedSettings.getS32("AlchemyNearbyChatChannel");
	LLWString out_text = stripChannelNumber(wtext, &channel);
	std::string utf8_out_text = wstring_to_utf8str(out_text);
	std::string utf8_text = wstring_to_utf8str(wtext);
	utf8_text = utf8str_trim(utf8_text);
	if (!utf8_text.empty())
	{
		utf8_text = utf8str_truncate(utf8_text, MAX_STRING - 1);
	}
	if ( (0 == channel) && (RlvActions::isRlvEnabled()) )
	{
		type = RlvActions::checkChatVolume(type);
		animate &= !RlvActions::hasBehaviour( (!RlvUtil::isEmote(utf8_text)) ? RLV_BHVR_REDIRCHAT : RLV_BHVR_REDIREMOTE );
	}
	LLCachedControl<bool> disable_chat_animation("SGDisableChatAnimation");
	if (animate && (channel == 0) && !disable_chat_animation)
	{
		if (type == CHAT_TYPE_WHISPER)
		{
			LL_DEBUGS() << "You whisper " << utf8_text << LL_ENDL;
			gAgent.sendAnimationRequest(ANIM_AGENT_WHISPER, ANIM_REQUEST_START);
		}
		else if (type == CHAT_TYPE_NORMAL)
		{
			LL_DEBUGS() << "You say " << utf8_text << LL_ENDL;
			gAgent.sendAnimationRequest(ANIM_AGENT_TALK, ANIM_REQUEST_START);
		}
		else if (type == CHAT_TYPE_SHOUT)
		{
			LL_DEBUGS() << "You shout " << utf8_text << LL_ENDL;
			gAgent.sendAnimationRequest(ANIM_AGENT_SHOUT, ANIM_REQUEST_START);
		}
		else
		{
			LL_INFOS() << "send_chat_from_viewer() - invalid volume" << LL_ENDL;
			return;
		}
	}
	else
	{
		if (type != CHAT_TYPE_START && type != CHAT_TYPE_STOP)
		{
			LL_DEBUGS() << "Channel chat: " << utf8_text << LL_ENDL;
		}
	}
	send_chat_from_viewer(utf8_out_text, type, channel);
}
void send_chat_from_viewer(std::string utf8_out_text, EChatType type, S32 channel)
{
	if ( (RlvActions::isRlvEnabled()) && ( (CHAT_TYPE_WHISPER == type) || (CHAT_TYPE_NORMAL == type) || (CHAT_TYPE_SHOUT == type) ) )
	{
		if (0 == channel)
		{
			type = RlvActions::checkChatVolume(type);
			if ( ( (gRlvHandler.hasBehaviour(RLV_BHVR_REDIRCHAT) || (gRlvHandler.hasBehaviour(RLV_BHVR_REDIREMOTE)) ) &&
				 (gRlvHandler.redirectChatOrEmote(utf8_out_text)) ) )
			{
				return;
			}
			if (gRlvHandler.hasBehaviour(RLV_BHVR_SENDCHAT))
				gRlvHandler.filterChat(utf8_out_text, true);
		}
		else
		{
			if (!RlvActions::canSendChannel(channel))
				return;
			if (CHAT_CHANNEL_DEBUG == channel)
			{
				bool fIsEmote = RlvUtil::isEmote(utf8_out_text);
				if ( (gRlvHandler.hasBehaviour(RLV_BHVR_SENDCHAT)) ||
					 ((!fIsEmote) && (gRlvHandler.hasBehaviour(RLV_BHVR_REDIRCHAT))) ||
					 ((fIsEmote) && (gRlvHandler.hasBehaviour(RLV_BHVR_REDIREMOTE))) )
				{
					return;
				}
			}
		}
	}
	if (LLChatAITranslate::instance().maybeDeferOutgoingNearby(utf8_out_text, (U8)type, channel))
	{
		return;
	}
	U32 split = MAX_MSG_BUF_SIZE - 1;
	U32 pos = 0;
	U32 total = utf8_out_text.length();
	if (total == 0)
	{
		really_send_chat_from_viewer(utf8_out_text, type, channel);
	}
	while (pos < total)
	{
		U32 next_split = split;
		if (pos + next_split > total)
		{
			next_split = total - pos;
		}
		else
		{
			while (U8(utf8_out_text[pos + next_split]) != 0x20
				&& U8(utf8_out_text[pos + next_split]) != 0x21
				&& U8(utf8_out_text[pos + next_split]) != 0x2C
				&& U8(utf8_out_text[pos + next_split]) != 0x2E
				&& U8(utf8_out_text[pos + next_split]) != 0x3F
				&& next_split > 0)
			{
				--next_split;
			}
			if (next_split == 0)
			{
				next_split = split;
				LL_WARNS("Splitting") << "utf-8 couldn't be split correctly" << LL_ENDL;
			}
			else
			{
				++next_split;
			}
		}
		std::string send = utf8_out_text.substr(pos, next_split);
		pos += next_split;
		really_send_chat_from_viewer(send, type, channel);
	}
}
void really_send_chat_from_viewer(const std::string& utf8_out_text, EChatType type, S32 channel)
{
	LLMessageSystem* msg = gMessageSystem;
	if (channel >= 0)
	{
		msg->newMessageFast(_PREHASH_ChatFromViewer);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ChatData);
		msg->addStringFast(_PREHASH_Message, utf8_out_text);
		msg->addU8Fast(_PREHASH_Type, type);
		msg->addS32("Channel", channel);
	}
	else
	{
		msg->newMessageFast(_PREHASH_ScriptDialogReply);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Data);
		msg->addUUIDFast(_PREHASH_ObjectID, gAgent.getID());
		msg->addS32("ChatChannel", channel);
		msg->addS32Fast(_PREHASH_ButtonIndex, 0);
		msg->addStringFast(_PREHASH_ButtonLabel, utf8_out_text);
	}
	gAgent.sendReliableMessage();
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_CHAT_COUNT);
}
void LLChatBar::onCommitGesture(LLUICtrl* ctrl)
{
	LLCtrlListInterface* gestures = mGestureCombo ? mGestureCombo->getListInterface() : NULL;
	if (gestures)
	{
		S32 index = gestures->getFirstSelectedIndex();
		if (index == 0)
		{
			return;
		}
		const std::string& trigger = gestures->getSelectedValue().asString();
		std::string text(trigger);
		std::string revised_text;
		LLGestureMgr::instance().triggerAndReviseString(text, &revised_text);
		revised_text = utf8str_trim(revised_text);
		if (!revised_text.empty())
		{
			sendChatFromViewer(revised_text, CHAT_TYPE_NORMAL, FALSE);
		}
	}
	mGestureLabelTimer.start();
	if (mGestureCombo != nullptr)
	{
		mGestureCombo->setFocus(FALSE);
	}
}
void toggleChatHistory()
{
	LLFloaterChat::toggleInstance(LLSD());
}
class LLChatCommandHandler final : public LLCommandHandler
{
public:
	LLChatCommandHandler() : LLCommandHandler("chat", UNTRUSTED_BLOCK) { }
	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLMediaCtrl* web) override
	{
		bool retval = false;
		if (tokens.size() < 2)
		{
			retval = false;
		}
		else
		{
			S32 channel = tokens[0].asInteger();
			{
				retval = true;
				std::string unescaped_mesg (LLURI::unescape(tokens[1].asString()));
				send_chat_from_viewer(unescaped_mesg, CHAT_TYPE_NORMAL, channel);
			}
		}
		return retval;
	}
};
LLChatCommandHandler gChatHandler;
