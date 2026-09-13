/** 
 * @file llpanelnearbymedia.cpp
 * @brief Management interface for muting and controlling nearby media
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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
#include "llpanelnearbymedia.h"
#include "llaudioengine.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llresizebar.h"
#include "llresizehandle.h"
#include "llscrolllistcolumn.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llslider.h"
#include "llsliderctrl.h"
#include "llagent.h"
#include "llagentui.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "llviewermedia.h"
#include "llviewerparcelmedia.h"
#include "llviewerregion.h"
#include "llviewermediafocus.h"
#include "llviewerparcelmgr.h"
#include "llparcel.h"
#include "llpluginclassmedia.h"
#include "llvovolume.h"
#include "llstatusbar.h"
#include "llsdutil.h"
#include "llvieweraudio.h"
#include "lluictrlfactory.h"
#include "llfloaterpreference.h"
#include "lltabcontainer.h"
#include <stringize.h>
#include "lloverlaybar.h"
#include "llmediaremotectrl.h"
#include "llchatbar.h"
#include "lltoolbar.h"
extern LLControlGroup gSavedSettings;
static const LLUUID PARCEL_MEDIA_LIST_ITEM_UUID = LLUUID("CAB5920F-E484-4233-8621-384CF373A321");
static const LLUUID PARCEL_AUDIO_LIST_ITEM_UUID = LLUUID("DF4B020D-8A24-4B95-AB5D-CA970D694822");
LLPanelNearByMedia::LLPanelNearByMedia(bool standalone_panel)
	: mNearbyMediaPanel(nullptr)
	, mMediaList(nullptr)
	, mEnableAllCtrl(nullptr)
	, mDisableAllCtrl(nullptr)
	, mShowCtrl(nullptr)
	, mStopCtrl(nullptr)
	, mPlayCtrl(nullptr)
	, mPauseCtrl(nullptr)
	, mMuteCtrl(nullptr)
	, mVolumeSliderCtrl(nullptr)
	, mZoomCtrl(nullptr)
	, mUnzoomCtrl(nullptr)
	, mVolumeSlider(nullptr)
	, mMuteBtn(nullptr)
	, mDebugInfoVisible(false)
	, mParcelMediaItem(nullptr)
	, mParcelAudioItem(nullptr)
	, mStandalonePanel(standalone_panel)
{
	mHoverTimer.stop();
	mParcelAudioAutoStart = gSavedSettings.getBOOL("MediaTentativeAutoPlay");
	mCommitCallbackRegistrar.add("MediaListCtrl.EnableAll",		boost::bind(&LLPanelNearByMedia::onClickEnableAll, this));
	mCommitCallbackRegistrar.add("MediaListCtrl.DisableAll",		boost::bind(&LLPanelNearByMedia::onClickDisableAll, this));
	mCommitCallbackRegistrar.add("MediaListCtrl.GoMediaPrefs", boost::bind(&LLPanelNearByMedia::onAdvancedButtonClick, this));
	mCommitCallbackRegistrar.add("MediaListCtrl.MoreLess", boost::bind(&LLPanelNearByMedia::onMoreLess, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Stop",		boost::bind(&LLPanelNearByMedia::onClickSelectedMediaStop, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Play",		boost::bind(&LLPanelNearByMedia::onClickSelectedMediaPlay, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Pause",		boost::bind(&LLPanelNearByMedia::onClickSelectedMediaPause, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Mute",		boost::bind(&LLPanelNearByMedia::onClickSelectedMediaMute, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Volume",	boost::bind(&LLPanelNearByMedia::onCommitSelectedMediaVolume, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Zoom",		boost::bind(&LLPanelNearByMedia::onClickSelectedMediaZoom, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Unzoom",	boost::bind(&LLPanelNearByMedia::onClickSelectedMediaUnzoom, this));
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_nearby_media.xml");
}
LLPanelNearByMedia::~LLPanelNearByMedia()
{
	LLViewerMedia::impl_list impls = LLViewerMedia::getPriorityList();
	LLViewerMedia::impl_list::iterator priority_iter;
	for(priority_iter = impls.begin(); priority_iter != impls.end(); priority_iter++)
	{
		(*priority_iter)->setInNearbyMediaList(false);
	}
}
BOOL LLPanelNearByMedia::postBuild()
{
	LLPanel::postBuild();
	if(mStandalonePanel)
	{
	const S32 RESIZE_BAR_THICKNESS = 6;
	LLResizeBar::Params p;
	p.rect = LLRect(0, RESIZE_BAR_THICKNESS, getRect().getWidth(), 0);
	p.name = "resizebar_bottom";
	p.min_size = getRect().getHeight();
	p.side = LLResizeBar::BOTTOM;
	p.resizing_view = this;
	addChild( LLUICtrlFactory::create<LLResizeBar>(p) );
	p.rect = LLRect( 0, getRect().getHeight(), RESIZE_BAR_THICKNESS, 0);
	p.name = "resizebar_left";
	p.min_size = getRect().getWidth();
	p.side = LLResizeBar::LEFT;
	addChild( LLUICtrlFactory::create<LLResizeBar>(p) );
	LLResizeHandle::Params resize_handle_p;
	resize_handle_p.rect = LLRect( 0, RESIZE_HANDLE_HEIGHT, RESIZE_HANDLE_WIDTH, 0 );
	resize_handle_p.mouse_opaque(false);
	resize_handle_p.min_width(getRect().getWidth());
	resize_handle_p.min_height(getRect().getHeight());
	resize_handle_p.corner(LLResizeHandle::LEFT_BOTTOM);
	addChild(LLUICtrlFactory::create<LLResizeHandle>(resize_handle_p));
	}
	mNearbyMediaPanel = getChild<LLUICtrl>("nearby_media_panel");
	mMediaList = getChild<LLScrollListCtrl>("media_list");
	mEnableAllCtrl = getChild<LLUICtrl>("all_nearby_media_enable_btn");
	mDisableAllCtrl = getChild<LLUICtrl>("all_nearby_media_disable_btn");
	mShowCtrl = getChild<LLComboBox>("show_combo");
	mStopCtrl = getChild<LLUICtrl>("stop");
	mPlayCtrl = getChild<LLUICtrl>("play");
	mPauseCtrl = getChild<LLUICtrl>("pause");
	mMuteCtrl = getChild<LLUICtrl>("mute");
	mVolumeSliderCtrl = getChild<LLUICtrl>("volume_slider_ctrl");
	mZoomCtrl = getChild<LLUICtrl>("zoom");
	mUnzoomCtrl = getChild<LLUICtrl>("unzoom");
	mVolumeSlider = getChild<LLSlider>("volume_slider");
	mMuteBtn = getChild<LLButton>("mute_btn");
	mEmptyNameString = getString("empty_item_text");
	mParcelMediaName = getString("parcel_media_name");
	mParcelAudioName = getString("parcel_audio_name");
	mPlayingString = getString("playing_suffix");
	mMediaList->setDoubleClickCallback(onZoomMedia, this);
	mMediaList->sortByColumnIndex(PROXIMITY_COLUMN, TRUE);
	mMediaList->sortByColumnIndex(VISIBILITY_COLUMN, FALSE);
	refreshList();
	updateControls();
	updateColumns();
	LLView* minimized_controls = getChildView("minimized_controls");
	mMoreRect = getRect();
	mLessRect = getRect();
	mLessRect.mBottom = minimized_controls->getRect().mBottom;
	getChild<LLUICtrl>("more_btn")->setVisible(false);
	onMoreLess();
	return TRUE;
}
void LLPanelNearByMedia::handleMediaAutoPlayChanged(const LLSD& newvalue)
{
	mParcelAudioAutoStart = gSavedSettings.getBOOL(LLViewerMedia::AUTO_PLAY_MEDIA_SETTING) &&
							gSavedSettings.getBOOL("MediaTentativeAutoPlay");
}
void LLPanelNearByMedia::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mHoverTimer.stop();
	LLPanel::onMouseEnter(x,y,mask);
}
void LLPanelNearByMedia::onMouseLeave(S32 x, S32 y, MASK mask)
{
	if(mStandalonePanel)
		mHoverTimer.start();
	LLPanel::onMouseLeave(x,y,mask);
}
void LLPanelNearByMedia::onTopLost()
{
	setVisible(FALSE);
}
void LLPanelNearByMedia::handleVisibilityChange ( BOOL new_visibility )
{
	if (mStandalonePanel && new_visibility)
	{
		mHoverTimer.start();
	}
	else
	{
		mHoverTimer.stop();
	}
}
void LLPanelNearByMedia::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);
	LLButton* more_btn = findChild<LLButton>("more_btn");
	if (!mStandalonePanel || more_btn && more_btn->getValue().asBoolean())
	{
		mMoreRect = getRect();
	}
}
const F32 AUTO_CLOSE_FADE_TIME_START= 4.0f;
const F32 AUTO_CLOSE_FADE_TIME_END = 5.0f;
void LLPanelNearByMedia::draw()
{
	LLRect screen_rect = calcScreenRect();
	if (screen_rect.mBottom < 0)
	{
		LLRect new_rect = getRect();
		new_rect.mBottom += 0 - screen_rect.mBottom;
		setShape(new_rect);
	}
	refreshList();
	updateControls();
	F32 alpha = mHoverTimer.getStarted()
		? clamp_rescale(mHoverTimer.getElapsedTimeF32(), AUTO_CLOSE_FADE_TIME_START, AUTO_CLOSE_FADE_TIME_END, 1.f, 0.f)
		: 1.0f;
	LLViewDrawContext context(alpha);
	LLPanel::draw();
	if (alpha == 0.f)
	{
		setVisible(false);
	}
}
BOOL LLPanelNearByMedia::handleHover(S32 x, S32 y, MASK mask)
{
	LLPanel::handleHover(x, y, mask);
	LLViewerMediaFocus::getInstance()->clearHover();
	return true;
}
bool LLPanelNearByMedia::getParcelAudioAutoStart()
{
	return mParcelAudioAutoStart;
}
LLScrollListItem* LLPanelNearByMedia::addListItem(const LLUUID &id)
{
	if (NULL == mMediaList) return NULL;
	LLSD row;
	row["id"] = id;
	LLSD &columns = row["columns"];
	columns[CHECKBOX_COLUMN]["name"]="media_checkbox_ctrl";
	columns[CHECKBOX_COLUMN]["column"] = "media_checkbox_ctrl";
	columns[CHECKBOX_COLUMN]["type"] = "checkbox";
	{
	columns[PROXIMITY_COLUMN]["name"]="media_proximity";
	columns[PROXIMITY_COLUMN]["column"] = "media_proximity";
	columns[PROXIMITY_COLUMN]["value"] = "";
		columns[VISIBILITY_COLUMN]["name"]="media_visibility";
		columns[VISIBILITY_COLUMN]["column"] = "media_visibility";
		columns[VISIBILITY_COLUMN]["value"] = "";
		columns[CLASS_COLUMN]["name"]="media_class";
		columns[CLASS_COLUMN]["column"] = "media_class";
		columns[CLASS_COLUMN]["type"] = "text";
		columns[CLASS_COLUMN]["value"] = "";
	}
	columns[NAME_COLUMN]["name"] = "media_name";
	columns[NAME_COLUMN]["column"] = "media_name";
	columns[NAME_COLUMN]["type"] = "text";
	columns[NAME_COLUMN]["value"] = "";
	{
		columns[DEBUG_COLUMN]["name"] = "media_debug";
		columns[DEBUG_COLUMN]["column"] = "media_debug";
		columns[DEBUG_COLUMN]["type"] = "text";
		columns[DEBUG_COLUMN]["value"] = "";
	}
	LLScrollListItem* new_item = mMediaList->addElement(row);
	if (NULL != new_item)
	{
		LLScrollListCheck* scroll_list_check = dynamic_cast<LLScrollListCheck*>(new_item->getColumn(CHECKBOX_COLUMN));
		if (scroll_list_check)
		{
			LLCheckBoxCtrl *check = scroll_list_check->getCheckBox();
			check->setCommitCallback(boost::bind(&LLPanelNearByMedia::onCheckItem, this, _1, id));
		}
	}
	return new_item;
}
extern char const* PRIORITYToString(LLViewerMediaImpl::EPriority priority);
void LLPanelNearByMedia::updateListItem(LLScrollListItem* item, LLViewerMediaImpl* impl)
{
	std::string item_name;
	std::string item_tooltip;
	std::string debug_str;
	LLPanelNearByMedia::MediaClass media_class = MEDIA_CLASS_ALL;
	getNameAndUrlHelper(impl, item_name, item_tooltip, mEmptyNameString);
	if (impl->hasFocus())
	{
		media_class = MEDIA_CLASS_FOCUSED;
	}
	else if (impl->isAttachedToAnotherAvatar())
	{
		media_class = MEDIA_CLASS_ON_OTHERS;
	}
	else if (!impl->isInAgentParcel())
	{
		media_class = MEDIA_CLASS_OUTSIDE_PARCEL;
	}
	else {
		media_class = MEDIA_CLASS_WITHIN_PARCEL;
	}
	if(mDebugInfoVisible)
	{
		debug_str += llformat("%g/", (float)impl->getInterest());
		debug_str += llformat("%g/", (F32) sqrt(impl->getProximityDistance()));
		debug_str += llformat("%g/", (float)(NULL == impl->getSomeObject()) ? 0.0 : impl->getSomeObject()->getPixelArea());
		debug_str += PRIORITYToString(impl->getPriority());
		if(impl->hasMedia())
		{
			debug_str += '@';
		}
		else if(impl->isPlayable())
		{
			debug_str += '+';
		}
		else if(impl->isForcedUnloaded())
		{
			debug_str += '!';
		}
	}
	updateListItem(item,
				   item_name,
				   item_tooltip,
				   impl->getProximity(),
				   impl->isMediaDisabled(),
				   impl->hasMedia(),
				   impl->isMediaTimeBased() &&	impl->isMediaPlaying(),
				   media_class,
				   debug_str);
}
void LLPanelNearByMedia::updateListItem(LLScrollListItem* item,
										  const std::string &item_name,
										  const std::string &item_tooltip,
										  S32 proximity,
										  bool is_disabled,
										  bool has_media,
										  bool is_time_based_and_playing,
										  LLPanelNearByMedia::MediaClass media_class,
										  const std::string &debug_str)
{
	LLScrollListCell* cell = item->getColumn(PROXIMITY_COLUMN);
	if(cell)
	{
		std::string proximity_string = STRINGIZE(proximity);
		std::string old_proximity_string = cell->getValue().asString();
		if(proximity_string != old_proximity_string)
		{
			cell->setValue(proximity_string);
			mMediaList->setNeedsSort(true);
		}
	}
	cell = item->getColumn(CHECKBOX_COLUMN);
	if(cell)
	{
		cell->setValue(!is_disabled);
	}
	cell = item->getColumn(VISIBILITY_COLUMN);
	if(cell)
	{
		S32 old_visibility = cell->getValue();
		S32 new_visibility =
			item->getUUID() == PARCEL_MEDIA_LIST_ITEM_UUID ? 3
			: item->getUUID() == PARCEL_AUDIO_LIST_ITEM_UUID ? 2
			: (has_media) ? 1
			: ((is_disabled) ? 0
			: -1);
		cell->setValue(STRINGIZE(new_visibility));
		if (new_visibility != old_visibility)
		{
			mMediaList->setNeedsSort(true);
		}
	}
	cell = item->getColumn(NAME_COLUMN);
	if(cell)
	{
		std::string name = item_name;
		std::string old_name = cell->getValue().asString();
		if (has_media)
		{
			name += " " + mPlayingString;
		}
		if (name != old_name)
		{
			cell->setValue(name);
		}
		cell->setToolTip(item_tooltip);
		U8 font_style = LLFontGL::NORMAL;
		LLColor4 cell_color = LLUI::sColorsGroup->getColor("DefaultListText");
		if (mDebugInfoVisible)
		{
			switch (media_class) {
				case MEDIA_CLASS_FOCUSED:
					cell_color = LLColor4::yellow;
					break;
				case MEDIA_CLASS_ON_OTHERS:
					cell_color = LLColor4::red;
					break;
				case MEDIA_CLASS_OUTSIDE_PARCEL:
					cell_color = LLColor4::orange;
					break;
				case MEDIA_CLASS_WITHIN_PARCEL:
				default:
					break;
			}
		}
		if (is_disabled)
		{
			if (mDebugInfoVisible)
			{
				font_style |= LLFontGL::ITALIC;
				cell_color = LLUI::sColorsGroup->getColor("DefaultListText");
			}
			else {
				cell_color.setAlpha(0.25);
			}
		}
		else if (!has_media)
		{
			cell_color.setAlpha(0.25);
		}
		else if (is_time_based_and_playing)
		{
			if (mDebugInfoVisible) font_style |= LLFontGL::BOLD;
		}
		cell->setColor(cell_color);
		LLScrollListText *text_cell = dynamic_cast<LLScrollListText*> (cell);
		if (text_cell)
		{
			text_cell->setFontStyle(font_style);
		}
	}
	cell = item->getColumn(CLASS_COLUMN);
	if(cell)
	{
		cell->setValue(STRINGIZE(media_class));
	}
	if(mDebugInfoVisible)
	{
		cell = item->getColumn(DEBUG_COLUMN);
		if(cell)
		{
			cell->setValue(debug_str);
		}
	}
}
void LLPanelNearByMedia::removeListItem(const LLUUID &id)
{
	if (NULL == mMediaList) return;
	mMediaList->deleteSingleItem(mMediaList->getItemIndex(id));
}
void LLPanelNearByMedia::refreshParcelItems()
{
	const LLSD &choice_llsd = mShowCtrl->getSelectedValue();
	MediaClass choice = (MediaClass)choice_llsd.asInteger();
	bool should_include = (choice == MEDIA_CLASS_ALL || choice == MEDIA_CLASS_WITHIN_PARCEL);
	if (gSavedSettings.getBOOL("AudioStreamingMedia") &&should_include && LLViewerMedia::hasParcelMedia())
	{
		if (NULL == mParcelMediaItem)
		{
			mParcelMediaItem = addListItem(PARCEL_MEDIA_LIST_ITEM_UUID);
			mMediaList->setNeedsSort(true);
		}
	}
	else {
		if (NULL != mParcelMediaItem) {
			removeListItem(PARCEL_MEDIA_LIST_ITEM_UUID);
			mParcelMediaItem = NULL;
			mMediaList->setNeedsSort(true);
		}
	}
	if (NULL != mParcelMediaItem)
	{
		std::string name, url, tooltip;
		getNameAndUrlHelper(LLViewerParcelMedia::getParcelMedia(), name, url, "");
		if (name.empty() || name == url)
		{
			tooltip = url;
		}
		else
		{
			tooltip = name + " : " + url;
		}
		LLViewerMediaImpl *impl = LLViewerParcelMedia::getParcelMedia();
		updateListItem(mParcelMediaItem,
					   mParcelMediaName,
					   tooltip,
					   -2,
					   impl == NULL || impl->isMediaDisabled(),
					   impl != NULL && !LLViewerParcelMedia::getURL().empty(),
					   impl != NULL && impl->isMediaTimeBased() &&	impl->isMediaPlaying(),
					   MEDIA_CLASS_ALL,
					   "parcel media");
	}
	if (should_include && LLViewerMedia::hasParcelAudio() && gSavedSettings.getBOOL("AudioStreamingMusic"))
	{
		if (NULL == mParcelAudioItem)
		{
			mParcelAudioItem = addListItem(PARCEL_AUDIO_LIST_ITEM_UUID);
			mMediaList->setNeedsSort(true);
		}
	}
	else {
		if (NULL != mParcelAudioItem) {
			removeListItem(PARCEL_AUDIO_LIST_ITEM_UUID);
			mParcelAudioItem = NULL;
			mMediaList->setNeedsSort(true);
		}
	}
	if (NULL != mParcelAudioItem)
	{
		bool is_playing = LLViewerMedia::isParcelAudioPlaying();
		std::string url;
        url = LLViewerMedia::getParcelAudioURL();
		updateListItem(mParcelAudioItem,
					   mParcelAudioName,
					   url,
					   -1,
					   (!is_playing),
					   is_playing,
					   is_playing,
					   MEDIA_CLASS_ALL,
					   "parcel audio");
	}
}
void LLPanelNearByMedia::refreshList()
{
	bool all_items_deleted = false;
	if(!mMediaList)
	{
		return;
	}
	bool debug_info_visible = gSavedSettings.getBOOL("MediaPerformanceManagerDebug");
	if(debug_info_visible != mDebugInfoVisible)
	{
		mDebugInfoVisible = debug_info_visible;
		mMediaList->deleteAllItems();
		mParcelAudioItem = NULL;
		mParcelMediaItem = NULL;
		all_items_deleted = true;
		updateColumns();
	}
	refreshParcelItems();
	LLViewerMedia::impl_list impls = LLViewerMedia::getPriorityList();
	LLViewerMedia::impl_list::iterator priority_iter;
	U32 enabled_count = 0;
	U32 disabled_count = 0;
	for(priority_iter = impls.begin(); priority_iter != impls.end(); priority_iter++)
	{
		LLViewerMediaImpl *impl = *priority_iter;
		if(all_items_deleted)
		{
			impl->setInNearbyMediaList(false);
		}
		if (!impl->isParcelMedia())
		{
			LLUUID media_id = impl->getMediaTextureID();
			S32 proximity = impl->getProximity();
			if (proximity < 0 || !shouldShow(impl))
			{
				if (impl->getInNearbyMediaList())
				{
					removeListItem(media_id);
					impl->setInNearbyMediaList(false);
				}
			}
			else
			{
				if (!impl->getInNearbyMediaList())
				{
					addListItem(media_id);
					impl->setInNearbyMediaList(true);
				}
			}
			if (impl->isMediaDisabled())
			{
				disabled_count++;
			}
			else {
				enabled_count++;
		}
	}
	}
	mDisableAllCtrl->setEnabled((gSavedSettings.getBOOL("AudioStreamingMusic") ||
		                         gSavedSettings.getBOOL("AudioStreamingMedia")) &&
								(LLViewerMedia::isAnyMediaShowing() ||
								 LLViewerMedia::isParcelMediaPlaying() ||
								 LLViewerMedia::isParcelAudioPlaying()));
	mEnableAllCtrl->setEnabled( (gSavedSettings.getBOOL("AudioStreamingMusic") ||
								gSavedSettings.getBOOL("AudioStreamingMedia")) &&
							   (disabled_count > 0 ||
								(LLViewerMedia::hasParcelMedia() && ! LLViewerMedia::isParcelMediaPlaying()) ||
								(LLViewerMedia::hasParcelAudio() && ! LLViewerMedia::isParcelAudioPlaying())));
	std::vector<LLScrollListItem*> items = mMediaList->getAllData();
	for (std::vector<LLScrollListItem*>::iterator item_it = items.begin();
		item_it != items.end();
		++item_it)
	{
		LLScrollListItem* item = (*item_it);
		LLUUID row_id = item->getUUID();
		if (row_id != PARCEL_MEDIA_LIST_ITEM_UUID &&
			row_id != PARCEL_AUDIO_LIST_ITEM_UUID)
		{
			LLViewerMediaImpl* impl = LLViewerMedia::getMediaImplFromTextureID(row_id);
			if(impl)
			{
				updateListItem(item, impl);
			}
			else
			{
				removeListItem(row_id);
			}
		}
	}
	LLUUID media_target = LLViewerMediaFocus::getInstance()->getControlsMediaID();
	if(media_target.notNull())
	{
		mMediaList->selectByID(media_target);
	}
}
void LLPanelNearByMedia::updateColumns()
{
	if (!mDebugInfoVisible)
	{
		if (mMediaList->getColumn(CHECKBOX_COLUMN)) mMediaList->getColumn(VISIBILITY_COLUMN)->setWidth(-1);
		if (mMediaList->getColumn(VISIBILITY_COLUMN)) mMediaList->getColumn(VISIBILITY_COLUMN)->setWidth(-1);
		if (mMediaList->getColumn(PROXIMITY_COLUMN)) mMediaList->getColumn(PROXIMITY_COLUMN)->setWidth(-1);
		if (mMediaList->getColumn(CLASS_COLUMN)) mMediaList->getColumn(CLASS_COLUMN)->setWidth(-1);
		if (mMediaList->getColumn(DEBUG_COLUMN)) mMediaList->getColumn(DEBUG_COLUMN)->setWidth(-1);
	}
	else {
		if (mMediaList->getColumn(CHECKBOX_COLUMN)) mMediaList->getColumn(VISIBILITY_COLUMN)->setWidth(20);
		if (mMediaList->getColumn(VISIBILITY_COLUMN)) mMediaList->getColumn(VISIBILITY_COLUMN)->setWidth(20);
		if (mMediaList->getColumn(PROXIMITY_COLUMN)) mMediaList->getColumn(PROXIMITY_COLUMN)->setWidth(30);
		if (mMediaList->getColumn(CLASS_COLUMN)) mMediaList->getColumn(CLASS_COLUMN)->setWidth(20);
		if (mMediaList->getColumn(DEBUG_COLUMN)) mMediaList->getColumn(DEBUG_COLUMN)->setWidth(200);
	}
}
void LLPanelNearByMedia::onClickEnableAll()
{
	LLViewerMedia::setAllMediaEnabled(true);
}
void LLPanelNearByMedia::onClickDisableAll()
{
	LLViewerMedia::setAllMediaEnabled(false);
}
void LLPanelNearByMedia::onClickEnableParcelMedia()
{
	if ( ! LLViewerMedia::isParcelMediaPlaying() )
	{
		LLViewerParcelMedia::play(LLViewerParcelMgr::getInstance()->getAgentParcel());
	}
}
void LLPanelNearByMedia::onClickDisableParcelMedia()
{
	LLViewerParcelMedia::stop();
}
void LLPanelNearByMedia::onCheckItem(LLUICtrl* ctrl, const LLUUID &row_id)
{
	LLCheckBoxCtrl* check = static_cast<LLCheckBoxCtrl*>(ctrl);
	setDisabled(row_id, ! check->getValue());
}
bool LLPanelNearByMedia::setDisabled(const LLUUID &row_id, bool disabled)
{
	if (row_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		if (disabled)
		{
			onClickParcelAudioStop();
		}
		else
		{
			onClickParcelAudioPlay();
		}
		return true;
	}
	else if (row_id == PARCEL_MEDIA_LIST_ITEM_UUID)
	{
		if (disabled)
		{
			onClickDisableParcelMedia();
		}
		else
		{
			onClickEnableParcelMedia();
		}
		return true;
	}
	else {
		LLViewerMediaImpl* impl = LLViewerMedia::getMediaImplFromTextureID(row_id);
		if(impl)
		{
			impl->setDisabled(disabled, true);
			return true;
		}
	}
	return false;
}
void LLPanelNearByMedia::onZoomMedia(void* user_data)
{
	LLPanelNearByMedia* panelp = (LLPanelNearByMedia*)user_data;
	LLUUID media_id = panelp->mMediaList->getValue().asUUID();
	LLViewerMediaFocus::getInstance()->focusZoomOnMedia(media_id);
}
void LLPanelNearByMedia::onClickParcelMediaPlay()
{
	LLViewerParcelMedia::play(LLViewerParcelMgr::getInstance()->getAgentParcel());
}
void LLPanelNearByMedia::onClickParcelMediaStop()
{
	if (LLViewerParcelMedia::getParcelMedia())
	{
		LLViewerParcelMedia::getParcelMedia()->stop();
	}
}
void LLPanelNearByMedia::onClickParcelMediaPause()
{
	LLViewerParcelMedia::pause();
}
void LLPanelNearByMedia::onClickParcelAudioPlay()
{
	mParcelAudioAutoStart = true;
	if (!gAudiop)
		return;
	if (LLAudioEngine::AUDIO_PAUSED == gAudiop->isInternetStreamPlaying())
	{
		gAudiop->pauseInternetStream(false);
	}
	else
	{
		LLViewerParcelMedia::playStreamingMusic(LLViewerParcelMgr::getInstance()->getAgentParcel());
	}
}
void LLPanelNearByMedia::onClickParcelAudioStop()
{
	mParcelAudioAutoStart = false;
	if (!gAudiop)
		return;
	gAudiop->stopInternetStream();
}
void LLPanelNearByMedia::onClickParcelAudioPause()
{
	if (!gAudiop)
		return;
	gAudiop->pauseInternetStream(true);
}
bool LLPanelNearByMedia::shouldShow(LLViewerMediaImpl* impl)
{
	const LLSD &choice_llsd = mShowCtrl->getSelectedValue();
	MediaClass choice = (MediaClass)choice_llsd.asInteger();
	switch (choice)
	{
		case MEDIA_CLASS_ALL:
			return true;
			break;
		case MEDIA_CLASS_WITHIN_PARCEL:
			return impl->isInAgentParcel();
			break;
		case MEDIA_CLASS_OUTSIDE_PARCEL:
			return ! impl->isInAgentParcel();
			break;
		case MEDIA_CLASS_ON_OTHERS:
			return impl->isAttachedToAnotherAvatar();
			break;
		default:
			break;
	}
	return true;
}
void LLPanelNearByMedia::onAdvancedButtonClick()
{
	LLFloaterPreference::show(NULL);
	LLFloaterPreference* prefsfloater = LLFloaterPreference::sInstance;
	if (prefsfloater)
	{
		LLTabContainer* tabcontainer = prefsfloater->getChild<LLTabContainer>("pref core");
		if (tabcontainer)
		{
			tabcontainer->selectTabByName("Media panel");
		}
	}
}
void LLPanelNearByMedia::onMoreLess()
{
	bool is_more = !mStandalonePanel || getChild<LLButton>("more_btn")->getToggleState();
	mNearbyMediaPanel->setVisible(is_more);
	if(mStandalonePanel)
		getChildView("resizebar_bottom")->setEnabled(is_more);
	LLRect new_rect = is_more ? mMoreRect : mLessRect;
	new_rect.translate(getRect().mRight - new_rect.mRight, getRect().mTop - new_rect.mTop);
	setShape(new_rect);
	getChild<LLUICtrl>("more_btn")->setVisible(mStandalonePanel);
}
void LLPanelNearByMedia::updateControls()
{
	LLUUID selected_media_id = mMediaList->getValue().asUUID();
	if (selected_media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		if (!LLViewerMedia::hasParcelAudio() || !gSavedSettings.getBOOL("AudioStreamingMusic"))
		{
			showDisabledControls();
		}
		else {
			showTimeBasedControls(LLViewerMedia::isParcelAudioPlaying(),
							  false,
							  false,
							  gSavedSettings.getBOOL("MuteMusic"),
							  gSavedSettings.getF32("AudioLevelMusic") );
		}
	}
	else if (selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
	{
		if (!LLViewerMedia::hasParcelMedia() || !gSavedSettings.getBOOL("AudioStreamingMedia"))
		{
			showDisabledControls();
		}
		else {
			LLViewerMediaImpl* impl = LLViewerParcelMedia::getParcelMedia();
			if (NULL == impl)
			{
				showBasicControls(false, false, false, false, 0);
			}
			else if (impl->isMediaTimeBased())
			{
				showTimeBasedControls(impl->isMediaPlaying(),
									  false,
									  false,
									  impl->getVolume() == 0.0,
									  impl->getVolume() );
			}
			else {
				showBasicControls(LLViewerMedia::isParcelMediaPlaying(),
							      false,
								  false,
								  impl->getVolume() == 0.0,
								  impl->getVolume());
			}
		}
	}
	else {
		LLViewerMediaImpl* impl = LLViewerMedia::getMediaImplFromTextureID(selected_media_id);
		if (NULL == impl || !gSavedSettings.getBOOL("AudioStreamingMedia"))
		{
			showDisabledControls();
		}
		else {
			if (impl->isMediaTimeBased())
			{
				showTimeBasedControls(impl->isMediaPlaying(),
									  ! impl->isParcelMedia(),
									  LLViewerMediaFocus::getInstance()->isZoomed(),
									  impl->getVolume() == 0.0,
									  impl->getVolume());
			}
			else {
				showBasicControls(!impl->isMediaDisabled(),
								  ! impl->isParcelMedia(),
								  LLViewerMediaFocus::getInstance()->isZoomed(),
								  impl->getVolume() == 0.0,
								  impl->getVolume());
			}
		}
	}
}
void LLPanelNearByMedia::showBasicControls(bool playing, bool include_zoom, bool is_zoomed, bool muted, F32 volume)
{
	mStopCtrl->setVisible(playing);
	mPlayCtrl->setVisible(!playing);
	mPauseCtrl->setVisible(false);
	mVolumeSliderCtrl->setVisible(true);
	mMuteCtrl->setVisible(true);
	mMuteBtn->setValue(muted);
	mVolumeSlider->setValue(volume);
	mZoomCtrl->setVisible(include_zoom && !is_zoomed);
	mUnzoomCtrl->setVisible(include_zoom && is_zoomed);
	mStopCtrl->setEnabled(true);
	mZoomCtrl->setEnabled(true);
}
void LLPanelNearByMedia::showTimeBasedControls(bool playing, bool include_zoom, bool is_zoomed, bool muted, F32 volume)
{
	mStopCtrl->setVisible(true);
	mPlayCtrl->setVisible(!playing);
	mPauseCtrl->setVisible(playing);
	mMuteCtrl->setVisible(true);
	mVolumeSliderCtrl->setVisible(true);
	mZoomCtrl->setVisible(include_zoom);
	mZoomCtrl->setVisible(include_zoom && !is_zoomed);
	mUnzoomCtrl->setVisible(include_zoom && is_zoomed);
	mStopCtrl->setEnabled(true);
	mZoomCtrl->setEnabled(true);
	mMuteBtn->setValue(muted);
	mVolumeSlider->setValue(volume);
}
void LLPanelNearByMedia::showDisabledControls()
{
	mStopCtrl->setVisible(true);
	mPlayCtrl->setVisible(false);
	mPauseCtrl->setVisible(false);
	mMuteCtrl->setVisible(false);
	mVolumeSliderCtrl->setVisible(false);
	mZoomCtrl->setVisible(true);
	mUnzoomCtrl->setVisible(false);
	mStopCtrl->setEnabled(false);
	mZoomCtrl->setEnabled(false);
}
void LLPanelNearByMedia::onClickSelectedMediaStop()
{
	setDisabled(mMediaList->getValue().asUUID(), true);
}
void LLPanelNearByMedia::onClickSelectedMediaPlay()
{
	LLUUID selected_media_id = mMediaList->getValue().asUUID();
	setDisabled(selected_media_id, false);
	if (selected_media_id != PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		LLViewerMediaImpl *impl = (selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID) ?
			((LLViewerMediaImpl*)LLViewerParcelMedia::getParcelMedia()) : LLViewerMedia::getMediaImplFromTextureID(selected_media_id);
		if (NULL != impl)
		{
			if (impl->isMediaTimeBased() && impl->isMediaPaused())
			{
				impl->play();
				return;
			}
			else if (impl->isParcelMedia())
			{
				LLViewerParcelMedia::play(LLViewerParcelMgr::getInstance()->getAgentParcel());
			}
		}
	}
}
void LLPanelNearByMedia::onClickSelectedMediaPause()
{
	LLUUID selected_media_id = mMediaList->getValue().asUUID();
	if (selected_media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		onClickParcelAudioPause();
	}
	else if (selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
	{
		onClickParcelMediaPause();
	}
	else {
		LLViewerMediaImpl* impl = LLViewerMedia::getMediaImplFromTextureID(selected_media_id);
		if (NULL != impl && impl->isMediaTimeBased() && impl->isMediaPlaying())
		{
			impl->pause();
		}
	}
}
void LLPanelNearByMedia::onClickSelectedMediaMute()
{
	LLUUID selected_media_id = mMediaList->getValue().asUUID();
	if (selected_media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		gSavedSettings.setBOOL("MuteMusic", mMuteBtn->getValue());
	}
	else {
		LLViewerMediaImpl* impl = (selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID) ?
			((LLViewerMediaImpl*)LLViewerParcelMedia::getParcelMedia()) : LLViewerMedia::getMediaImplFromTextureID(selected_media_id);
		if (NULL != impl)
		{
			F32 volume = impl->getVolume();
			if(volume > 0.0)
			{
				impl->setVolume(0.0);
			}
			else if (mVolumeSlider->getValueF32() == 0.0)
			{
				impl->setVolume(1.0);
				mVolumeSlider->setValue(1.0);
			}
			else
			{
				impl->setVolume(mVolumeSlider->getValueF32());
			}
		}
	}
}
void LLPanelNearByMedia::onCommitSelectedMediaVolume()
{
	LLUUID selected_media_id = mMediaList->getValue().asUUID();
	if (selected_media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		F32 vol = mVolumeSlider->getValueF32();
		gSavedSettings.setF32("AudioLevelMusic", vol);
	}
	else {
		LLViewerMediaImpl* impl = (selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID) ?
			((LLViewerMediaImpl*)LLViewerParcelMedia::getParcelMedia()) : LLViewerMedia::getMediaImplFromTextureID(selected_media_id);
		if (NULL != impl)
		{
			impl->setVolume(mVolumeSlider->getValueF32());
		}
	}
}
void LLPanelNearByMedia::onClickSelectedMediaZoom()
{
	LLUUID selected_media_id = mMediaList->getValue().asUUID();
	if (selected_media_id == PARCEL_AUDIO_LIST_ITEM_UUID || selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
		return;
	LLViewerMediaFocus::getInstance()->focusZoomOnMedia(selected_media_id);
}
void LLPanelNearByMedia::onClickSelectedMediaUnzoom()
{
	LLViewerMediaFocus::getInstance()->unZoom();
}
void LLPanelNearByMedia::getNameAndUrlHelper(LLViewerMediaImpl* impl, std::string& name, std::string & url, const std::string &defaultName)
{
	if (NULL == impl) return;
	name = impl->getName();
	url = impl->getCurrentMediaURL();
	if (url.empty())
	{
		url = impl->getMediaEntryURL();
	}
	if (url.empty())
	{
		url = impl->getHomeURL();
	}
	if (name.empty())
	{
		name = url;
	}
	if (name.empty())
	{
		name = defaultName;
	}
}
void* createNearbyMediaPanel(void* userdata)
{
	return new LLPanelNearByMedia(false);
}
LLFloaterNearbyMedia::LLFloaterNearbyMedia() : mPanel(NULL)
{
	mFactoryMap["nearby_media"] = LLCallbackMap(createNearbyMediaPanel, this);
	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_nearby_media.xml",&mFactoryMap,false);
}
BOOL LLFloaterNearbyMedia::postBuild()
{
	mPanel = getChild<LLPanelNearByMedia>("nearby_media",false,false);
	return LLFloater::postBuild();
}
void LLFloaterNearbyMedia::updateClass()
{
	if(gSavedSettings.getBOOL("ShowNearbyMediaFloater"))
		LLFloaterNearbyMedia::getInstance()->open();
	else if(LLFloaterNearbyMedia::instanceExists())
		LLFloaterNearbyMedia::getInstance()->close();
}
void LLFloaterNearbyMedia::onClose(bool app_quitting)
{
	if(!app_quitting)
		gSavedSettings.setBOOL("ShowNearbyMediaFloater", false);
	LLFloater::onClose(app_quitting);
}
void LLFloaterNearbyMedia::onOpen()
{
	LLRect rect = gSavedSettings.getRect("FloaterNearbyMediaRect");
	if(!rect.isEmpty())
	{
		setRectControl("FloaterNearbyMediaRect");
		applyRectControl();
	}
	else
	{
		const LLRect media_rect = gOverlayBar->mMediaRemote->calcScreenRect();
		setOrigin(media_rect.mLeft - getRect().getWidth(), media_rect.mBottom + gOverlayBar->mChatBar->getRect().getHeight());
	}
	gSavedSettings.setBOOL("ShowNearbyMediaFloater", true);
}
void LLFloaterNearbyMedia::handleReshape(const LLRect& new_rect, bool by_user)
{
	if(by_user && getRectControl().empty())
	{
		setRectControl("FloaterNearbyMediaRect");
	}
	LLFloater::handleReshape(new_rect, by_user);
}
