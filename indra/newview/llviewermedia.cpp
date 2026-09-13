/**
 * @file llviewermedia.cpp
 * @brief Client interface to the media engine
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
#include "llviewermedia.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h"
#include "llaudioengine.h"
#include "llcallbacklist.h"
#include "lldir.h"
#include "lldiriterator.h"
#include "llevent.h"
#include "aifilepicker.h"
#include "llfloaterdestinations.h"
#include "llfloaterwebcontent.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"
#include "llmarketplacefunctions.h"
#include "llmediaentry.h"
#include "llmenugl.h"
#include "llmimetypes.h"
#include "llmutelist.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpanelprofile.h"
#include "llparcel.h"
#include "llpluginclassmedia.h"
#include "llurldispatcher.h"
#include "lluuid.h"
#include "llvieweraudio.h"
#include "llviewermediafocus.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llvovolume.h"
#include "llwebprofile.h"
#include "llwindow.h"
#include "llvieweraudio.h"
#include "llhttpclient.h"
#include "llstartup.h"
std::string getProfileURL(const std::string& agent_name);
const char* LLViewerMedia::AUTO_PLAY_MEDIA_SETTING = "ParcelMediaAutoPlayEnable";
const char* LLViewerMedia::AUTO_PLAY_PRIM_MEDIA_SETTING = "PrimMediaAutoPlayEnable";
const char* LLViewerMedia::SHOW_MEDIA_ON_OTHERS_SETTING = "MediaShowOnOthers";
const char* LLViewerMedia::SHOW_MEDIA_WITHIN_PARCEL_SETTING = "MediaShowWithinParcel";
const char* LLViewerMedia::SHOW_MEDIA_OUTSIDE_PARCEL_SETTING = "MediaShowOutsideParcel";
LLViewerMediaEventEmitter::~LLViewerMediaEventEmitter()
{
	observerListType::iterator iter = mObservers.begin();
	while( iter != mObservers.end() )
	{
		LLViewerMediaObserver *self = *iter;
		iter++;
		remObserver(self);
	}
}
bool LLViewerMediaEventEmitter::addObserver( LLViewerMediaObserver* observer )
{
	if ( ! observer )
		return false;
	if ( std::find( mObservers.begin(), mObservers.end(), observer ) != mObservers.end() )
		return false;
	mObservers.push_back( observer );
	observer->mEmitters.push_back( this );
	return true;
}
bool LLViewerMediaEventEmitter::remObserver( LLViewerMediaObserver* observer )
{
	if ( ! observer )
		return false;
	mObservers.remove( observer );
	observer->mEmitters.remove(this);
	return true;
}
void LLViewerMediaEventEmitter::emitEvent( LLPluginClassMedia* media, LLViewerMediaObserver::EMediaEvent event )
{
	observerListType::iterator iter = mObservers.begin();
	while( iter != mObservers.end() )
	{
		LLViewerMediaObserver *self = *iter;
		++iter;
		self->handleMediaEvent( media, event );
	}
}
LLViewerMediaObserver::~LLViewerMediaObserver()
{
	std::list<LLViewerMediaEventEmitter *>::iterator iter = mEmitters.begin();
	while( iter != mEmitters.end() )
	{
		LLViewerMediaEventEmitter *self = *iter;
		iter++;
		self->remObserver( this );
	}
}
class LLMimeDiscoveryResponder : public LLHTTPClient::ResponderHeadersOnly
{
LOG_CLASS(LLMimeDiscoveryResponder);
public:
	LLMimeDiscoveryResponder( viewer_media_t media_impl)
		: mMediaImpl(media_impl),
		  mInitialized(false)
	{
		if(mMediaImpl->mMimeProbe)
		{
			LL_ERRS() << "impl already has an outstanding responder" << LL_ENDL;
		}
		mMediaImpl->mMimeProbe = this;
	}
	~LLMimeDiscoveryResponder() { disconnectOwner(); }
private:
	void completedHeaders()
	{
		if (!isGoodStatus(mStatus))
		{
			LL_WARNS() << dumpResponse()
					<< " [headers:" << getResponseHeaders() << "]" << LL_ENDL;
		}
		std::string media_type;
		std::string::size_type idx1 = media_type.find_first_of(";");
		std::string mime_type = media_type.substr(0, idx1);
		LL_DEBUGS() << "status is " << getStatus() << ", media type \"" << media_type << "\"" << LL_ENDL;
		{
			if(mime_type.empty())
			{
				mime_type = "text/html";
			}
		}
		LLViewerMediaImpl *impl = mMediaImpl;
		if(impl && !mInitialized && ! mime_type.empty())
		{
			if(impl->initializeMedia(mime_type))
			{
				mInitialized = true;
				impl->loadURI();
				disconnectOwner();
			}
		}
	}
public:
	char const* getName(void) const { return "LLMimeDiscoveryResponder"; }
	void cancelRequest()
	{
		disconnectOwner();
	}
private:
	void disconnectOwner()
	{
		if(mMediaImpl)
		{
			if(mMediaImpl->mMimeProbe != this)
			{
				LL_ERRS() << "internal error: mMediaImpl->mMimeProbe != this" << LL_ENDL;
			}
			mMediaImpl->mMimeProbe = nullptr;
		}
		mMediaImpl = nullptr;
	}
	public:
		LLViewerMediaImpl *mMediaImpl;
		bool mInitialized;
};
class LLViewerMediaOpenIDResponder : public LLHTTPClient::ResponderWithCompleted
{
LOG_CLASS(LLViewerMediaOpenIDResponder);
public:
	LLViewerMediaOpenIDResponder( )
	{
	}
	~LLViewerMediaOpenIDResponder()
	{
	}
	void completedRaw(
		const LLChannelDescriptors& channels,
		const LLIOPipe::buffer_ptr_t& buffer)
	{
		LL_DEBUGS("MediaAuth") << dumpResponse()
				<< " [headers:" << getResponseHeaders() << "]" << LL_ENDL;
		std::string cookie;
		getResponseHeaders().getFirstValue("set-cookie", cookie);
		LLViewerMedia::openIDCookieResponse(cookie);
	}
	char const* getName(void) const { return "LLViewerMediaOpenIDResponder"; }
	bool needsHeaders(void) const { return true; }
};
class LLViewerMediaWebProfileResponder : public LLHTTPClient::ResponderWithCompleted
{
LOG_CLASS(LLViewerMediaWebProfileResponder);
public:
	LLViewerMediaWebProfileResponder(std::string host)
	{
		mHost = host;
	}
	~LLViewerMediaWebProfileResponder()
	{
	}
	void completedRaw(
		const LLChannelDescriptors& channels,
		const LLIOPipe::buffer_ptr_t& buffer)
	{
		LL_WARNS("MediaAuth") << dumpResponse()
				<< " [headers:" << getResponseHeaders() << "]" << LL_ENDL;
		AIHTTPReceivedHeaders stripped_content = getResponseHeaders();
		LL_WARNS("MediaAuth") << stripped_content << LL_ENDL;
		AIHTTPReceivedHeaders::range_type cookies;
		if (mReceivedHeaders.getValues("set-cookie", cookies))
		{
			for (AIHTTPReceivedHeaders::iterator_type cookie = cookies.first; cookie != cookies.second; ++cookie)
			{
				if (cookie->second.substr(0, cookie->second.find('=')) == "_my_secondlife_session")
				{
					std::string auth_cookie = cookie->second.substr(0, cookie->second.find(";"));
					LLWebProfile::setAuthCookie(auth_cookie);
					break;
				}
			}
		}
	}
	char const* getName() const { return "LLViewerMediaWebProfileResponder"; }
	bool needsHeaders() const { return true; }
	std::string mHost;
};
LLURL LLViewerMedia::sOpenIDURL;
std::string LLViewerMedia::sOpenIDCookie;
LLPluginClassMedia* LLViewerMedia::sSpareBrowserMediaSource = nullptr;
static LLViewerMedia::impl_list sViewerMediaImplList;
static LLViewerMedia::impl_id_map sViewerMediaTextureIDMap;
static LLTimer sMediaCreateTimer;
static const F32 LLVIEWERMEDIA_CREATE_DELAY = 1.0f;
static F32 sGlobalVolume = 1.0f;
static bool sForceUpdate = false;
static LLUUID sOnlyAudibleTextureID = LLUUID::null;
static F64 sLowestLoadableImplInterest = 0.0f;
static bool sAnyMediaShowing = false;
static boost::signals2::connection sTeleportFinishConnection;
static void add_media_impl(LLViewerMediaImpl* media)
{
	sViewerMediaImplList.push_back(media);
}
static void remove_media_impl(LLViewerMediaImpl* media)
{
	LLViewerMedia::impl_list::iterator iter = sViewerMediaImplList.begin();
	LLViewerMedia::impl_list::iterator end = sViewerMediaImplList.end();
	for(; iter != end; iter++)
	{
		if(media == *iter)
		{
			sViewerMediaImplList.erase(iter);
			return;
		}
	}
}
class LLViewerMediaMuteListObserver : public LLMuteListObserver
{
	void onChange()  { LLViewerMedia::muteListChanged();}
};
static LLViewerMediaMuteListObserver sViewerMediaMuteListObserver;
static bool sViewerMediaMuteListObserverInitialized = false;
viewer_media_t LLViewerMedia::newMediaImpl(
											 const LLUUID& texture_id,
											 S32 media_width,
											 S32 media_height,
											 U8 media_auto_scale,
											 U8 media_loop)
{
	LLViewerMediaImpl* media_impl = getMediaImplFromTextureID(texture_id);
	if(media_impl == NULL || texture_id.isNull())
	{
		media_impl = new LLViewerMediaImpl(texture_id, media_width, media_height, media_auto_scale, media_loop);
	}
	else
	{
		media_impl->unload();
		media_impl->setTextureID(texture_id);
		media_impl->mMediaWidth = media_width;
		media_impl->mMediaHeight = media_height;
		media_impl->mMediaAutoScale = media_auto_scale;
		media_impl->mMediaLoop = media_loop;
	}
	media_impl->setPageZoomFactor(media_impl->mZoomFactor);
	return media_impl;
}
viewer_media_t LLViewerMedia::updateMediaImpl(LLMediaEntry* media_entry, const std::string& previous_url, bool update_from_self)
{
	viewer_media_t media_impl = getMediaImplFromTextureID(media_entry->getMediaID());
	LL_DEBUGS() << "called, current URL is \"" << media_entry->getCurrentURL()
			<< "\", previous URL is \"" << previous_url
			<< "\", update_from_self is " << (update_from_self?"true":"false")
			<< LL_ENDL;
	bool was_loaded = false;
	bool needs_navigate = false;
	if(media_impl)
	{
		was_loaded = media_impl->hasMedia();
		media_impl->setHomeURL(media_entry->getHomeURL());
		media_impl->mMediaAutoScale = media_entry->getAutoScale();
		media_impl->mMediaLoop = media_entry->getAutoLoop();
		media_impl->mMediaWidth = media_entry->getWidthPixels();
		media_impl->mMediaHeight = media_entry->getHeightPixels();
		media_impl->mMediaAutoPlay = media_entry->getAutoPlay();
		media_impl->mMediaEntryURL = media_entry->getCurrentURL();
		LLPluginClassMedia* plugin = media_impl->getMediaPlugin();
		if (plugin)
		{
			plugin->setAutoScale(media_impl->mMediaAutoScale);
			plugin->setLoop(media_impl->mMediaLoop);
			plugin->setSize(media_entry->getWidthPixels(), media_entry->getHeightPixels());
		}
		bool url_changed = (media_impl->mMediaEntryURL != previous_url);
		if(media_impl->mMediaEntryURL.empty())
		{
			if(url_changed)
			{
				media_impl->unload();
				LL_DEBUGS() << "Unloading media instance (new current URL is empty)." << LL_ENDL;
			}
		}
		else
		{
			bool auto_play = media_impl->isAutoPlayable();
			if((was_loaded || auto_play) && !update_from_self)
			{
				needs_navigate = url_changed;
			}
			LL_DEBUGS() << "was_loaded is " << (was_loaded?"true":"false")
					<< ", auto_play is " << (auto_play?"true":"false")
					<< ", needs_navigate is " << (needs_navigate?"true":"false") << LL_ENDL;
		}
	}
	else
	{
		media_impl = newMediaImpl(
			media_entry->getMediaID(),
			media_entry->getWidthPixels(),
			media_entry->getHeightPixels(),
			media_entry->getAutoScale(),
			media_entry->getAutoLoop());
		media_impl->setHomeURL(media_entry->getHomeURL());
		media_impl->mMediaAutoPlay = media_entry->getAutoPlay();
		media_impl->mMediaEntryURL = media_entry->getCurrentURL();
		if(media_impl->isAutoPlayable())
		{
			needs_navigate = true;
		}
	}
	if(media_impl)
	{
		if(needs_navigate)
		{
			media_impl->navigateTo(media_impl->mMediaEntryURL, "", true, true);
			LL_DEBUGS() << "navigating to URL " << media_impl->mMediaEntryURL << LL_ENDL;
		}
		else if(!media_impl->mMediaURL.empty() && (media_impl->mMediaURL != media_impl->mMediaEntryURL))
		{
			media_impl->mMediaURL = media_impl->mMediaEntryURL;
			media_impl->mNavigateServerRequest = true;
			LL_DEBUGS() << "updating URL in the media impl to " << media_impl->mMediaEntryURL << LL_ENDL;
		}
	}
	return media_impl;
}
LLViewerMediaImpl* LLViewerMedia::getMediaImplFromTextureID(const LLUUID& texture_id)
{
	LLViewerMediaImpl* result = NULL;
	impl_id_map::iterator iter = sViewerMediaTextureIDMap.find(texture_id);
	if(iter != sViewerMediaTextureIDMap.end())
	{
		result = iter->second;
	}
	return result;
}
std::string LLViewerMedia::getCurrentUserAgent()
{
	std::ostringstream codec;
	codec << "SecondLife/";
	codec << "C64 Basic V2";
	return codec.str();
}
void LLViewerMedia::updateBrowserUserAgent()
{
	std::string user_agent = getCurrentUserAgent();
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		LLPluginClassMedia* plugin = pimpl->getMediaPlugin();
		if(plugin && plugin->pluginSupportsMediaBrowser())
		{
			plugin->setBrowserUserAgent(user_agent);
		}
	}
}
bool LLViewerMedia::handleSkinCurrentChanged(const LLSD& )
{
	updateBrowserUserAgent();
	return true;
}
bool LLViewerMedia::textureHasMedia(const LLUUID& texture_id)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if(pimpl->getMediaTextureID() == texture_id)
		{
			return true;
		}
	}
	return false;
}
void LLViewerMedia::setVolume(F32 volume)
{
	if(volume != sGlobalVolume || sForceUpdate)
	{
		sGlobalVolume = volume;
		impl_list::iterator iter = sViewerMediaImplList.begin();
		impl_list::iterator end = sViewerMediaImplList.end();
		for(; iter != end; iter++)
		{
			LLViewerMediaImpl* pimpl = *iter;
			pimpl->updateVolume();
		}
		sForceUpdate = false;
	}
}
F32 LLViewerMedia::getVolume()
{
	return sGlobalVolume;
}
void LLViewerMedia::muteListChanged()
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		pimpl->mNeedsMuteCheck = true;
	}
}
bool LLViewerMedia::isInterestingEnough(const LLVOVolume *object, const F64 &object_interest)
{
	bool result = false;
	if (NULL == object)
	{
		result = false;
	}
	else if (LLViewerMediaFocus::getInstance()->getFocusedObjectID() == object->getID())
	{
		result = true;
	}
	else if (LLSelectMgr::getInstance()->getSelection()->contains(const_cast<LLVOVolume*>(object)))
	{
		result = true;
	}
	else
	{
		LL_DEBUGS() << "object interest = " << object_interest << ", lowest loadable = " << sLowestLoadableImplInterest << LL_ENDL;
		if(object_interest >= sLowestLoadableImplInterest)
			result = true;
	}
	return result;
}
LLViewerMedia::impl_list &LLViewerMedia::getPriorityList()
{
	return sViewerMediaImplList;
}
bool LLViewerMedia::priorityComparitor(const LLViewerMediaImpl* i1, const LLViewerMediaImpl* i2)
{
	if(i1->isForcedUnloaded() && !i2->isForcedUnloaded())
	{
		return false;
	}
	else if(i2->isForcedUnloaded() && !i1->isForcedUnloaded())
	{
		return true;
	}
	else if(i1->hasFocus())
	{
		return true;
	}
	else if(i2->hasFocus())
	{
		return false;
	}
	else if(i1->isParcelMedia())
	{
		return true;
	}
	else if(i2->isParcelMedia())
	{
		return false;
	}
	else if(i1->getUsedInUI() && !i2->getUsedInUI())
	{
		return true;
	}
	else if(i2->getUsedInUI() && !i1->getUsedInUI())
	{
		return false;
	}
	else if(i1->isPlayable() && !i2->isPlayable())
	{
		return true;
	}
	else if(!i1->isPlayable() && i2->isPlayable())
	{
		return false;
	}
	else if(i1->getInterest() == i2->getInterest())
	{
		return (i1->getProximityDistance() < i2->getProximityDistance());
	}
	else
	{
		return (i1->getInterest() > i2->getInterest());
	}
}
static bool proximity_comparitor(const LLViewerMediaImpl* i1, const LLViewerMediaImpl* i2)
{
	if(i1->getProximityDistance() < i2->getProximityDistance())
	{
		return true;
	}
	else if(i1->getProximityDistance() > i2->getProximityDistance())
	{
		return false;
	}
	else
	{
		return (i1 < i2);
	}
}
static LLTrace::BlockTimerStatHandle FTM_MEDIA_UPDATE("Update Media");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_SPARE_IDLE("Spare Idle");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_UPDATE_INTEREST("Update/Interest");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_SORT("Sort");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_SORT2("Sort 2");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_MISC("Misc");
void LLViewerMedia::updateMedia(void *dummy_arg)
{
	LL_RECORD_BLOCK_TIME(FTM_MEDIA_UPDATE);
	static LLCachedControl<bool> pluginUseReadThread(gSavedSettings, "PluginUseReadThread");
	LLPluginProcessParent::setUseReadThread(pluginUseReadThread);
	createSpareBrowserMediaSource();
	sAnyMediaShowing = false;
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	{
		LL_RECORD_BLOCK_TIME(FTM_MEDIA_UPDATE_INTEREST);
		for(; iter != end;)
		{
			LLViewerMediaImpl* pimpl = *iter++;
			pimpl->update();
			pimpl->calculateInterest();
		}
	}
	if(sSpareBrowserMediaSource)
	{
		LL_RECORD_BLOCK_TIME(FTM_MEDIA_SPARE_IDLE);
		sSpareBrowserMediaSource->idle();
	}
	{
		LL_RECORD_BLOCK_TIME(FTM_MEDIA_SORT);
		sViewerMediaImplList.sort(priorityComparitor);
	}
	iter = sViewerMediaImplList.begin();
	end = sViewerMediaImplList.end();
	F64 total_cpu = 0.0f;
	int impl_count_total = 0;
	int impl_count_interest_low = 0;
	int impl_count_interest_normal = 0;
	std::vector<LLViewerMediaImpl*> proximity_order;
	static LLCachedControl<bool> inworld_media_enabled(gSavedSettings, "AudioStreamingMedia");
	static LLCachedControl<bool> inworld_audio_enabled(gSavedSettings, "AudioStreamingMusic");
	static LLCachedControl<U32> max_instances(gSavedSettings, "PluginInstancesTotal");
	static LLCachedControl<U32> max_normal(gSavedSettings, "PluginInstancesNormal");
	static LLCachedControl<U32> max_low(gSavedSettings, "PluginInstancesLow");
	static LLCachedControl<F32> max_cpu(gSavedSettings, "PluginInstancesCPULimit");
	bool check_cpu_usage = (max_cpu != 0.0f);
	LLViewerMediaImpl* lowest_interest_loadable = NULL;
	{
		LL_RECORD_BLOCK_TIME(FTM_MEDIA_MISC);
		for(; iter != end; iter++)
		{
			LLViewerMediaImpl* pimpl = *iter;
			LLViewerMediaImpl::EPriority new_priority = LLViewerMediaImpl::PRIORITY_NORMAL;
			if(pimpl->isForcedUnloaded() || (impl_count_total >= (int)max_instances))
			{
				new_priority = LLViewerMediaImpl::PRIORITY_UNLOADED;
			}
			else if(!pimpl->getVisible())
			{
				new_priority = LLViewerMediaImpl::PRIORITY_HIDDEN;
			}
			else if(pimpl->hasFocus())
			{
				new_priority = LLViewerMediaImpl::PRIORITY_HIGH;
				impl_count_interest_normal++;
			}
			else if(pimpl->getUsedInUI())
			{
				new_priority = LLViewerMediaImpl::PRIORITY_NORMAL;
				impl_count_interest_normal++;
			}
			else if(pimpl->isParcelMedia())
			{
				new_priority = LLViewerMediaImpl::PRIORITY_NORMAL;
				impl_count_interest_normal++;
			}
			else
			{
				bool media_is_small = false;
				F64 approximate_interest = pimpl->getApproximateTextureInterest();
				if(approximate_interest == 0.0f)
				{
					media_is_small = true;
				}
				else if(pimpl->getInterest() < (approximate_interest / 4))
				{
					media_is_small = true;
				}
				if(pimpl->getInterest() == 0.0f)
				{
					new_priority = LLViewerMediaImpl::PRIORITY_HIDDEN;
				}
				else if(check_cpu_usage && (total_cpu > max_cpu))
				{
					new_priority = LLViewerMediaImpl::PRIORITY_SLIDESHOW;
				}
				else if((impl_count_interest_normal < (int)max_normal) && !media_is_small)
				{
					new_priority = LLViewerMediaImpl::PRIORITY_NORMAL;
					impl_count_interest_normal++;
				}
				else if (impl_count_interest_low + impl_count_interest_normal < (int)max_low + (int)max_normal)
				{
					new_priority = LLViewerMediaImpl::PRIORITY_LOW;
					impl_count_interest_low++;
					{
						F32 approximate_interest_dimension = (F32) sqrt(pimpl->getInterest());
						pimpl->setLowPrioritySizeLimit(ll_round(approximate_interest_dimension));
					}
				}
				else
				{
					new_priority = LLViewerMediaImpl::PRIORITY_SLIDESHOW;
				}
			}
			if(!pimpl->getUsedInUI() && (new_priority != LLViewerMediaImpl::PRIORITY_UNLOADED))
			{
				lowest_interest_loadable = pimpl;
				impl_count_total++;
			}
			if (!gViewerWindow->getActive()
				&& new_priority > LLViewerMediaImpl::PRIORITY_HIDDEN)
			{
				new_priority = LLViewerMediaImpl::PRIORITY_HIDDEN;
			}
			else if (!gFocusMgr.getAppHasFocus()
					 && new_priority > LLViewerMediaImpl::PRIORITY_LOW)
			{
				new_priority = LLViewerMediaImpl::PRIORITY_LOW;
			}
			if(!inworld_media_enabled)
			{
				if(!pimpl->getUsedInUI())
				{
					new_priority = LLViewerMediaImpl::PRIORITY_UNLOADED;
				}
			}
			if( !inworld_audio_enabled)
			{
				if(LLViewerMedia::isParcelAudioPlaying() && gAudiop && LLViewerMedia::hasParcelAudio())
				{
					gAudiop->stopInternetStream();
				}
			}
			pimpl->setPriority(new_priority);
			if(pimpl->getUsedInUI())
			{
				pimpl->mProximity = -1;
			}
			else
			{
				proximity_order.push_back(pimpl);
			}
			total_cpu += pimpl->getCPUUsage();
			if (!pimpl->getUsedInUI() && pimpl->hasMedia())
			{
				sAnyMediaShowing = true;
			}
		}
	}
	sLowestLoadableImplInterest	= 0.0f;
	if(lowest_interest_loadable && (impl_count_total >= (int)max_instances))
	{
		LLVOVolume *object = lowest_interest_loadable->getSomeObject();
		if(object)
		{
			sLowestLoadableImplInterest = object->getPixelArea();
		}
	}
	static LLCachedControl<bool> mediaPerformanceManager(gSavedSettings, "MediaPerformanceManagerDebug");
	if(mediaPerformanceManager)
	{
	}
	else
	{
		LL_RECORD_BLOCK_TIME(FTM_MEDIA_SORT2);
		std::stable_sort(proximity_order.begin(), proximity_order.end(), proximity_comparitor);
	}
	for(int i = 0; i < (int)proximity_order.size(); i++)
	{
		proximity_order[i]->mProximity = i;
	}
	LL_DEBUGS("PluginPriority") << "Total reported CPU usage is " << total_cpu << LL_ENDL;
}
bool LLViewerMedia::isAnyMediaShowing()
{
	return sAnyMediaShowing;
}
void LLViewerMedia::setAllMediaEnabled(bool val)
{
	gSavedSettings.setBOOL("MediaTentativeAutoPlay", val);
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if (!pimpl->getUsedInUI())
		{
			pimpl->setDisabled(!val);
		}
	}
	if (val)
	{
		if (!LLViewerMedia::isParcelMediaPlaying() && LLViewerMedia::hasParcelMedia())
		{
			LLViewerParcelMedia::play(LLViewerParcelMgr::getInstance()->getAgentParcel());
		}
		if (gSavedSettings.getBOOL("AudioStreamingMusic") &&
			!LLViewerMedia::isParcelAudioPlaying() &&
			gAudiop &&
			LLViewerMedia::hasParcelAudio())
		{
			if (LLAudioEngine::AUDIO_PAUSED == gAudiop->isInternetStreamPlaying())
			{
				gAudiop->pauseInternetStream(false);
			}
			else
			{
				LLViewerParcelMedia::playStreamingMusic(LLViewerParcelMgr::getInstance()->getAgentParcel());
			}
		}
	}
	else {
		LLViewerParcelMedia::stop();
		if (gAudiop)
		{
			gAudiop->stopInternetStream();
		}
	}
}
bool LLViewerMedia::isParcelMediaPlaying()
{
	return (LLViewerMedia::hasParcelMedia() && LLViewerParcelMedia::getParcelMedia() && LLViewerParcelMedia::getParcelMedia()->hasMedia());
}
bool LLViewerMedia::isParcelAudioPlaying()
{
	return (LLViewerMedia::hasParcelAudio() && gAudiop && LLAudioEngine::AUDIO_PLAYING == gAudiop->isInternetStreamPlaying());
}
void LLViewerMedia::onAuthSubmit(const LLSD& notification, const LLSD& response)
{
	LLViewerMediaImpl *impl = LLViewerMedia::getMediaImplFromTextureID(notification["payload"]["media_id"]);
	if(impl)
	{
		LLPluginClassMedia* media = impl->getMediaPlugin();
		if(media)
		{
			if (response["ok"])
			{
				media->sendAuthResponse(true, response["username"], response["password"]);
			}
			else
			{
				media->sendAuthResponse(false, "", "");
			}
		}
	}
}
void LLViewerMedia::clearAllCookies()
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		LLPluginClassMedia* plugin = pimpl->getMediaPlugin();
		if(plugin)
		{
			plugin->clear_cookies();
		}
	}
	setOpenIDCookie();
}
void LLViewerMedia::clearAllCaches()
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		pimpl->clearCache();
	}
}
void LLViewerMedia::setCookiesEnabled(bool enabled)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		LLPluginClassMedia* plugin = pimpl->getMediaPlugin();
		if(plugin)
		{
			plugin->cookies_enabled(enabled);
		}
	}
}
void LLViewerMedia::setProxyConfig(bool enable, const std::string &host, int port)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		LLPluginClassMedia* plugin = pimpl->getMediaPlugin();
		if(plugin)
		{
		}
	}
}
AIHTTPHeaders LLViewerMedia::getHeaders()
{
	AIHTTPHeaders headers;
	headers.addHeader("Accept", "*/*");
	headers.addHeader("Content-Type", "application/xml");
	headers.addHeader("Cookie", sOpenIDCookie);
	headers.addHeader("User-Agent", getCurrentUserAgent());
	return headers;
}
bool LLViewerMedia::parseRawCookie(const std::string raw_cookie, std::string& name, std::string& value, std::string& path, bool& httponly, bool& secure)
{
	std::size_t name_pos = raw_cookie.find_first_of('=');
	if (name_pos != std::string::npos)
	{
		name = raw_cookie.substr(0, name_pos);
		std::size_t value_pos = raw_cookie.find_first_of(';', name_pos);
		if (value_pos != std::string::npos)
		{
			value = raw_cookie.substr(name_pos + 1, value_pos - name_pos - 1);
			path = "/";
			httponly = true;
			secure = true;
			return true;
		}
	}
	return false;
}
void LLViewerMedia::setOpenIDCookie()
{
	if(!sOpenIDCookie.empty())
	{
		if (gSavedSettings.getString("WebProfileURL").empty()) return;
		std::string profileUrl = getProfileURL("");
        getOpenIDCookieCoro(profileUrl);
	}
}
void LLViewerMedia::getOpenIDCookieCoro(std::string url)
{
	std::string authority = sOpenIDURL.mAuthority;
	std::string::size_type hostStart = authority.find('@');
	if(hostStart == std::string::npos)
	{
		hostStart = 0;
	}
	else
	{
		++hostStart;
	}
	std::string::size_type hostEnd = authority.rfind(':');
	if((hostEnd == std::string::npos) || (hostEnd < hostStart))
	{
		hostEnd = authority.size();
	}
	if (url.length())
	{
		LLMediaCtrl* media_instance = LLFloaterDestinations::getInstance()->getChild<LLMediaCtrl>("destination_guide_contents");
		if (media_instance)
		{
			std::string cookie_host = authority.substr(hostStart, hostEnd - hostStart);
			std::string cookie_name = "";
			std::string cookie_value = "";
			std::string cookie_path = "";
			bool httponly = true;
			bool secure = true;
			if (parseRawCookie(sOpenIDCookie, cookie_name, cookie_value, cookie_path, httponly, secure) &&
                media_instance->getMediaPlugin())
			{
                std::string cefUrl(std::string(sOpenIDURL.mURI) + "://" + std::string(sOpenIDURL.mAuthority));
				media_instance->getMediaPlugin()->setCookie(cefUrl, cookie_name, cookie_value, cookie_host, cookie_path, httponly, secure);
			}
		}
	}
	AIHTTPHeaders headers;
	headers.addHeader("Accept", "*/*");
	headers.addHeader("Cookie", sOpenIDCookie);
	headers.addHeader("User-Agent", getCurrentUserAgent());
	LLURL raw_profile_url(url.data());
	LL_DEBUGS("MediaAuth") << "Requesting " << url << LL_ENDL;
	LL_DEBUGS("MediaAuth") << "sOpenIDCookie = [" << sOpenIDCookie << "]" << LL_ENDL;
	LLHTTPClient::get(url,
		new LLViewerMediaWebProfileResponder(raw_profile_url.getAuthority()),
		headers);
}
void LLViewerMedia::openIDSetup(const std::string &openidUrl, const std::string &openidToken)
{
	LL_DEBUGS("MediaAuth") << "url = \"" << openidUrl << "\", token = \"" << openidToken << "\"" << LL_ENDL;
	sOpenIDURL.init(openidUrl.c_str());
	sOpenIDCookie.clear();
	AIHTTPHeaders headers;
	headers.addHeader("Accept", "*/*");
	headers.addHeader("Content-Type", "application/x-www-form-urlencoded");
	size_t size = openidToken.size();
	U8* data = new U8[size];
	memcpy(data, openidToken.data(), size);
	LLHTTPClient::postRaw(
		openidUrl,
		data,
		size,
		new LLViewerMediaOpenIDResponder(),
		headers);
}
void LLViewerMedia::openIDCookieResponse(const std::string &cookie)
{
	LL_DEBUGS("MediaAuth") << "Cookie received: \"" << cookie << "\"" << LL_ENDL;
	sOpenIDCookie += cookie;
	setOpenIDCookie();
}
void LLViewerMedia::proxyWindowOpened(const std::string &target, const std::string &uuid)
{
	if(uuid.empty())
		return;
	for (impl_list::iterator iter = sViewerMediaImplList.begin(); iter != sViewerMediaImplList.end(); iter++)
	{
		LLPluginClassMedia* plugin = (*iter)->getMediaPlugin();
		if(plugin && plugin->pluginSupportsMediaBrowser())
		{
			plugin->proxyWindowOpened(target, uuid);
		}
	}
}
void LLViewerMedia::proxyWindowClosed(const std::string &uuid)
{
	if(uuid.empty())
		return;
	for (impl_list::iterator iter = sViewerMediaImplList.begin(); iter != sViewerMediaImplList.end(); iter++)
	{
		LLPluginClassMedia* plugin = (*iter)->getMediaPlugin();
		if(plugin && plugin->pluginSupportsMediaBrowser())
		{
			plugin->proxyWindowClosed(uuid);
		}
	}
}
void LLViewerMedia::createSpareBrowserMediaSource()
{
	static bool failedLoading = false;
	if (failedLoading) return;
	if (!gSavedSettings.getBOOL("MediaEnableSpareBrowser"))
	{
		return;
	}
	if (!sSpareBrowserMediaSource && !gSavedSettings.getBOOL("PluginAttachDebuggerToPlugins"))
	{
		sSpareBrowserMediaSource = LLViewerMediaImpl::newSourceFromMediaType("text/html", nullptr, 0, 0, 1.0);
		if (!sSpareBrowserMediaSource) failedLoading = true;
	}
}
LLPluginClassMedia* LLViewerMedia::getSpareBrowserMediaSource()
{
	LLPluginClassMedia* result = sSpareBrowserMediaSource;
	sSpareBrowserMediaSource = nullptr;
	return result;
};
bool LLViewerMedia::hasInWorldMedia()
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if (!pimpl->getUsedInUI() && !pimpl->isParcelMedia())
		{
			return true;
		}
	}
	return false;
}
bool LLViewerMedia::hasParcelMedia()
{
	return !LLViewerParcelMedia::getURL().empty();
}
bool LLViewerMedia::hasParcelAudio()
{
	return !LLViewerMedia::getParcelAudioURL().empty();
}
std::string LLViewerMedia::getParcelAudioURL()
{
	return LLViewerParcelMgr::getInstance()->getAgentParcel()->getMusicURL();
}
void LLViewerMedia::initClass()
{
	gIdleCallbacks.addFunction(LLViewerMedia::updateMedia, nullptr);
	sTeleportFinishConnection = LLViewerParcelMgr::getInstance()->
		setTeleportFinishedCallback(boost::bind(&LLViewerMedia::onTeleportFinished));
}
void LLViewerMedia::cleanupClass()
{
	gIdleCallbacks.deleteFunction(LLViewerMedia::updateMedia, nullptr);
	sTeleportFinishConnection.disconnect();
	if (sSpareBrowserMediaSource != nullptr)
	{
		delete sSpareBrowserMediaSource;
		sSpareBrowserMediaSource = nullptr;
	}
}
void LLViewerMedia::onTeleportFinished()
{
	gSavedSettings.setBOOL("MediaTentativeAutoPlay", true);
	LLViewerMediaImpl::sMimeTypesFailed.clear();
}
void LLViewerMedia::setOnlyAudibleMediaTextureID(const LLUUID& texture_id)
{
	sOnlyAudibleTextureID = texture_id;
	sForceUpdate = true;
}
std::vector<std::string> LLViewerMediaImpl::sMimeTypesFailed;
LLViewerMediaImpl::LLViewerMediaImpl(	  const LLUUID& texture_id,
										  S32 media_width,
										  S32 media_height,
										  U8 media_auto_scale,
										  U8 media_loop)
:
	mZoomFactor(1.0),
	mMovieImageHasMips(false),
	mMediaWidth(media_width),
	mMediaHeight(media_height),
	mMediaAutoScale(media_auto_scale),
	mMediaLoop(media_loop),
	mNeedsNewTexture(true),
	mTextureUsedWidth(0),
	mTextureUsedHeight(0),
	mSuspendUpdates(false),
	mVisible(true),
	mLastSetCursor( UI_CURSOR_ARROW ),
	mMediaNavState( MEDIANAVSTATE_NONE ),
	mInterest(0.0f),
	mUsedInUI(false),
	mHasFocus(false),
	mPriority(PRIORITY_UNLOADED),
	mNavigateRediscoverType(false),
	mNavigateServerRequest(false),
	mMediaSourceFailed(false),
	mRequestedVolume(1.0f),
	mPreviousVolume(1.0f),
	mIsMuted(false),
	mNeedsMuteCheck(false),
	mPreviousMediaState(MEDIA_NONE),
	mPreviousMediaTime(0.0f),
	mIsDisabled(false),
	mIsParcelMedia(false),
	mProximity(-1),
	mProximityDistance(0.0f),
	mMediaAutoPlay(false),
	mInNearbyMediaList(false),
	mClearCache(false),
	mBackgroundColor(LLColor4::white),
	mNavigateSuspended(false),
	mNavigateSuspendedDeferred(false),
	mTrustedBrowser(false),
    mCleanBrowser(false),
	mIsUpdated(false),
	mMimeProbe(nullptr)
{
	if(!sViewerMediaMuteListObserverInitialized)
	{
		LLMuteList::getInstance()->addObserver(&sViewerMediaMuteListObserver);
		sViewerMediaMuteListObserverInitialized = true;
	}
	add_media_impl(this);
	setTextureID(texture_id);
	LLViewerMediaTexture* media_tex = LLViewerTextureManager::getMediaTexture(mTextureId);
	if(media_tex)
	{
		media_tex->setMediaImpl();
	}
}
LLViewerMediaImpl::~LLViewerMediaImpl()
{
	destroyMediaSource();
	LLViewerMediaTexture::removeMediaImplFromTexture(mTextureId) ;
	setTextureID();
	remove_media_impl(this);
}
void LLViewerMediaImpl::emitEvent(LLPluginClassMedia* plugin, LLViewerMediaObserver::EMediaEvent event)
{
	LLViewerMediaEventEmitter::emitEvent(plugin, event);
	std::list< LLVOVolume* >::iterator iter = mObjectList.begin() ;
	while(iter != mObjectList.end())
	{
		LLVOVolume *self = *iter;
		++iter;
		self->mediaEvent(this, plugin, event);
	}
}
bool LLViewerMediaImpl::initializeMedia(const std::string& mime_type)
{
	bool mimeTypeChanged = (mMimeType != mime_type);
	bool pluginChanged = (LLMIMETypes::implType(mCurrentMimeType) != LLMIMETypes::implType(mime_type));
	if(!mPluginBase || pluginChanged)
	{
		(void)initializePlugin(mime_type);
	}
	else if(mimeTypeChanged)
	{
		mMimeType = mime_type;
	}
	return (mPluginBase != nullptr);
}
void LLViewerMediaImpl::createMediaSource()
{
	if(mPriority == PRIORITY_UNLOADED)
	{
		return;
	}
	if(! mMediaURL.empty())
	{
		navigateInternal();
	}
	else if(! mMimeType.empty())
	{
		if (!initializeMedia(mMimeType))
		{
			LL_WARNS("Media") << "Failed to initialize media for mime type " << mMimeType << LL_ENDL;
		}
	}
}
void LLViewerMediaImpl::destroyMediaSource()
{
	mNeedsNewTexture = true;
	LLViewerMediaTexture* oldImage = LLViewerTextureManager::findMediaTexture( mTextureId );
	if (oldImage)
	{
		oldImage->setPlaying(FALSE) ;
	}
	cancelMimeTypeProbe();
	if(mPluginBase)
	{
		mPluginBase->setDeleteOK(true) ;
		destroyPlugin();
	}
}
void LLViewerMediaImpl::setMediaType(const std::string& media_type)
{
	mMimeType = media_type;
}
LLPluginClassMedia* LLViewerMediaImpl::newSourceFromMediaType(std::string media_type, LLPluginClassMediaOwner *owner , S32 default_width, S32 default_height, F64 zoom_factor, const std::string target, bool clean_browser)
{
	std::string plugin_basename = LLMIMETypes::implType(media_type);
	LLPluginClassMedia* media_source = nullptr;
	if ((plugin_basename == "media_plugin_cef") &&
        !gSavedSettings.getBOOL("PluginAttachDebuggerToPlugins") && !clean_browser)
	{
		media_source = LLViewerMedia::getSpareBrowserMediaSource();
		if(media_source)
		{
			media_source->setOwner(owner);
			media_source->setTarget(target);
			media_source->setSize(default_width, default_height);
			media_source->setZoomFactor(zoom_factor);
			media_source->set_page_zoom_factor(zoom_factor);
			return media_source;
		}
	}
	if(plugin_basename.empty())
	{
		LL_WARNS_ONCE("Media") << "Couldn't find plugin for media type " << media_type << LL_ENDL;
	}
	else
	{
		std::string launcher_name = gDirUtilp->getLLPluginLauncher();
		std::string plugin_name = gDirUtilp->getLLPluginFilename(plugin_basename);
		std::string user_data_path_cache = gDirUtilp->getCacheDir(false);
		user_data_path_cache += gDirUtilp->getDirDelimiter();
		std::string user_data_path_cookies = gDirUtilp->getOSUserAppDir();
		user_data_path_cookies += gDirUtilp->getDirDelimiter();
		std::string user_data_path_cef_log;
		std::string linden_user_dir = gDirUtilp->getLindenUserDir();
		if ( ! linden_user_dir.empty() )
		{
			user_data_path_cookies = linden_user_dir;
			user_data_path_cookies += gDirUtilp->getDirDelimiter();
		};
		llstat s;
		if(LLFile::stat(launcher_name, &s))
		{
			LL_WARNS_ONCE("Media") << "Couldn't find launcher at " << launcher_name << LL_ENDL;
		}
		else if(LLFile::stat(plugin_name, &s))
		{
			LL_WARNS_ONCE("Media") << "Couldn't find plugin at " << plugin_name << LL_ENDL;
		}
		else
		{
			media_source = new LLPluginClassMedia(owner);
			media_source->proxy_setup(gSavedSettings.getBOOL("BrowserProxyEnabled"), gSavedSettings.getS32("BrowserProxyType"),
				gSavedSettings.getString("BrowserProxyAddress"), gSavedSettings.getS32("BrowserProxyPort"),
				gSavedSettings.getString("BrowserProxyUsername"), gSavedSettings.getString("BrowserProxyPassword"));
			media_source->setSize(default_width, default_height);
			media_source->setUserDataPath(user_data_path_cache, user_data_path_cookies, user_data_path_cef_log);
			media_source->setLanguageCode(LLUI::getLanguage());
			media_source->setZoomFactor(zoom_factor);
			bool cookies_enabled = gSavedSettings.getBOOL( "CookiesEnabled" );
			media_source->cookies_enabled( cookies_enabled || clean_browser);
			bool plugins_enabled = gSavedSettings.getBOOL( "BrowserPluginsEnabled" );
			media_source->setPluginsEnabled( plugins_enabled  || clean_browser);
			bool javascript_enabled = gSavedSettings.getBOOL( "BrowserJavascriptEnabled" );
			media_source->setJavascriptEnabled( javascript_enabled || clean_browser);
			bool media_plugin_debugging_enabled = gSavedSettings.getBOOL("MediaPluginDebugging");
			media_source->enableMediaPluginDebugging( media_plugin_debugging_enabled  || clean_browser);
			media_source->setBrowserUserAgent(LLViewerMedia::getCurrentUserAgent());
			media_source->setTarget(target);
			const std::string plugin_dir = gDirUtilp->getLLPluginDir();
			if (media_source->init(launcher_name, plugin_dir, plugin_name, gSavedSettings.getBOOL("PluginAttachDebuggerToPlugins")))
			{
				return media_source;
			}
			else
			{
				LL_WARNS("Media") << "Failed to init plugin.  Destroying." << LL_ENDL;
				delete media_source;
			}
		}
	}
	LL_WARNS_ONCE("Plugin") << "plugin initialization failed for mime type: " << media_type << LL_ENDL;
	if(gAgent.isInitialized())
	{
	    if (std::find(sMimeTypesFailed.begin(), sMimeTypesFailed.end(), media_type) == sMimeTypesFailed.end())
	    {
			LLSD args;
			args["MIME_TYPE"] = media_type;
			LLNotificationsUtil::add("NoPlugin", args);
	        sMimeTypesFailed.push_back(media_type);
	    }
	}
	return nullptr;
}
bool LLViewerMediaImpl::initializePlugin(const std::string& media_type)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		mMediaWidth = mMediaSource->getSetWidth();
		mMediaHeight = mMediaSource->getSetHeight();
		mZoomFactor = mMediaSource->getZoomFactor();
	}
	destroyMediaSource();
	mMimeType = media_type;
	if(mPriority == PRIORITY_UNLOADED)
	{
		LL_DEBUGS("PluginPriority") << this << "Not loading (PRIORITY_UNLOADED)" << LL_ENDL;
		return false;
	}
	mMediaSourceFailed = false;
	mCurrentMimeType = mMimeType;
	LLPluginClassMedia* media_source = newSourceFromMediaType(mMimeType, this, mMediaWidth, mMediaHeight, mZoomFactor, mTarget, mCleanBrowser);
	if (media_source)
	{
		media_source->setDisableTimeout(gSavedSettings.getBOOL("DebugPluginDisableTimeout"));
		media_source->setLoop(mMediaLoop);
		media_source->setAutoScale(mMediaAutoScale);
		media_source->setBrowserUserAgent(LLViewerMedia::getCurrentUserAgent());
		media_source->focus(mHasFocus);
		media_source->setBackgroundColor(mBackgroundColor);
		if(gSavedSettings.getBOOL("BrowserIgnoreSSLCertErrors"))
		{
			media_source->ignore_ssl_cert_errors(true);
		}
		std::string ca_path = gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS, "ca-bundle.crt" );
		media_source->addCertificateFilePath( ca_path );
		media_source->proxy_setup(gSavedSettings.getBOOL("BrowserProxyEnabled"), gSavedSettings.getS32("BrowserProxyType"), gSavedSettings.getString("BrowserProxyAddress"), gSavedSettings.getS32("BrowserProxyPort"),
			gSavedSettings.getString("BrowserProxyUsername"), gSavedSettings.getString("BrowserProxyPassword"));
		if(mClearCache)
		{
			mClearCache = false;
			media_source->clear_cache();
		}
		mPluginBase = media_source;
		mPluginBase->setDeleteOK(false) ;
		updateVolume();
		return true;
	}
	mMediaSourceFailed = true;
	return false;
}
void LLViewerMediaImpl::loadURI()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		LLStringUtil::trim( mMediaURL );
		std::string uri = LLURI::escape(mMediaURL,
										"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
										"0123456789"
										"$-_.+"
										"!*'(),"
										"{}|\\^~[]`"
										"<>#%"
										";/?:@&=",
										false);
		{
			LLURI u(uri);
			std::string sanitized_uri = (u.query().empty() ? uri : u.scheme() + "://" + u.authority() + u.path());
			LL_INFOS() << "Asking media source to load URI: " << sanitized_uri << LL_ENDL;
		}
		mMediaSource->loadURI( uri );
		if(mPreviousMediaTime != 0.0f)
		{
			seek(mPreviousMediaTime);
		}
		if(mPreviousMediaState == MEDIA_PLAYING)
		{
			start();
		}
		else if(mPreviousMediaState == MEDIA_PAUSED)
		{
			pause();
		}
		else
		{
			start();
		}
	}
}
void LLViewerMediaImpl::setSize(int width, int height)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	mMediaWidth = width;
	mMediaHeight = height;
	if (mMediaSource)
	{
		mMediaSource->setSize(width, height);
	}
}
void LLViewerMediaImpl::showNotification(LLNotificationPtr notify)
{
	mNotification = notify;
}
void LLViewerMediaImpl::hideNotification()
{
	mNotification.reset();
}
void LLViewerMediaImpl::play()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource == nullptr)
	{
	 	if(!initializeMedia(mMimeType))
		{
			return;
		}
		loadURI();
	}
	start();
}
void LLViewerMediaImpl::stop()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		mMediaSource->stop();
	}
}
void LLViewerMediaImpl::pause()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		mMediaSource->pause();
	}
	else
	{
		mPreviousMediaState = MEDIA_PAUSED;
	}
}
void LLViewerMediaImpl::start()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		mMediaSource->start();
	}
	else
	{
		mPreviousMediaState = MEDIA_PLAYING;
	}
}
void LLViewerMediaImpl::seek(F32 time)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		mMediaSource->seek(time);
	}
	else
	{
		mPreviousMediaTime = time;
	}
}
void LLViewerMediaImpl::skipBack(F32 step_scale)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		if(mMediaSource->pluginSupportsMediaTime())
		{
			F64 back_step = mMediaSource->getCurrentTime() - (mMediaSource->getDuration()*step_scale);
			if(back_step < 0.0)
			{
				back_step = 0.0;
			}
			mMediaSource->seek(back_step);
		}
	}
}
void LLViewerMediaImpl::skipForward(F32 step_scale)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		if(mMediaSource->pluginSupportsMediaTime())
		{
			F64 forward_step = mMediaSource->getCurrentTime() + (mMediaSource->getDuration()*step_scale);
			if(forward_step > mMediaSource->getDuration())
			{
				forward_step = mMediaSource->getDuration();
			}
			mMediaSource->seek(forward_step);
		}
	}
}
void LLViewerMediaImpl::setVolume(F32 volume)
{
	mRequestedVolume = volume;
	updateVolume();
}
void LLViewerMediaImpl::setMute(bool mute)
{
	if (mute)
	{
		mPreviousVolume = mRequestedVolume;
		setVolume(0.0);
	}
	else
	{
		setVolume(mPreviousVolume);
	}
}
void LLViewerMediaImpl::updateVolume()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		F32 volume = mRequestedVolume * LLViewerMedia::getVolume();
		if (mProximityCamera > 0)
		{
			static LLCachedControl<F32> sMediaRollOffMax(gSavedSettings, "MediaRollOffMax", 30.f);
			static LLCachedControl<F32> sMediaRollOffMin(gSavedSettings, "MediaRollOffMin", 5.f);
			static LLCachedControl<F32> sMediaRollOffRate(gSavedSettings, "MediaRollOffRate", 0.125f);
			if (mProximityCamera > sMediaRollOffMax)
			{
				volume = 0;
			}
			else if (mProximityCamera > sMediaRollOffMin)
			{
				F64 adjusted_distance = mProximityCamera - sMediaRollOffMin;
				F64 attenuation = 1.0 + (sMediaRollOffRate * adjusted_distance);
				attenuation = 1.0 / (attenuation * attenuation);
				volume = volume * llmin(1.0, attenuation);
			}
		}
		if (sOnlyAudibleTextureID == LLUUID::null || sOnlyAudibleTextureID == mTextureId)
		{
			mMediaSource->setVolume(volume);
		}
		else
		{
			mMediaSource->setVolume(0.0f);
		}
	}
}
F32 LLViewerMediaImpl::getVolume()
{
	return mRequestedVolume;
}
void LLViewerMediaImpl::focus(bool focus)
{
	mHasFocus = focus;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
	{
		mMediaSource->focus(focus);
		if (focus)
		{
		}
	}
}
bool LLViewerMediaImpl::hasFocus() const
{
	return mHasFocus;
}
std::string LLViewerMediaImpl::getCurrentMediaURL()
{
	if(!mCurrentMediaURL.empty())
	{
		return mCurrentMediaURL;
	}
	return mMediaURL;
}
void LLViewerMediaImpl::clearCache()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		mMediaSource->clear_cache();
	}
	else
	{
		mClearCache = true;
	}
}
void LLViewerMediaImpl::setPageZoomFactor( double factor )
{
	LLPluginClassMedia* media_source = getMediaPlugin();
	if (media_source && factor == mZoomFactor)
	{
		return;
	}
	mZoomFactor = factor;
	if (media_source)
	{
		media_source->set_page_zoom_factor(factor);
	}
}
void LLViewerMediaImpl::mouseDown(S32 x, S32 y, MASK mask, S32 button)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOWN, button, x, y, mask);
	}
}
void LLViewerMediaImpl::mouseUp(S32 x, S32 y, MASK mask, S32 button)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, button, x, y, mask);
	}
}
void LLViewerMediaImpl::mouseMove(S32 x, S32 y, MASK mask)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_MOVE, 0, x, y, mask);
	}
}
void LLViewerMediaImpl::scaleTextureCoords(const LLVector2& texture_coords, S32 *x, S32 *y)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	F32 texture_x = texture_coords.mV[VX];
	F32 texture_y = texture_coords.mV[VY];
	texture_x = fmodf(texture_x, 1.0f);
	if(texture_x < 0.0f)
		texture_x = 1.0 + texture_x;
	texture_y = fmodf(texture_y, 1.0f);
	if(texture_y < 0.0f)
		texture_y = 1.0 + texture_y;
	*x = ll_round(texture_x * mMediaSource->getTextureWidth());
	*y = ll_round((1.0f - texture_y) * mMediaSource->getTextureHeight());
	*y -= (mMediaSource->getTextureHeight() - mMediaSource->getHeight());
}
void LLViewerMediaImpl::mouseDown(const LLVector2& texture_coords, MASK mask, S32 button)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		S32 x, y;
		scaleTextureCoords(texture_coords, &x, &y);
		mouseDown(x, y, mask, button);
	}
}
void LLViewerMediaImpl::mouseUp(const LLVector2& texture_coords, MASK mask, S32 button)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		S32 x, y;
		scaleTextureCoords(texture_coords, &x, &y);
		mouseUp(x, y, mask, button);
	}
}
void LLViewerMediaImpl::mouseMove(const LLVector2& texture_coords, MASK mask)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		S32 x, y;
		scaleTextureCoords(texture_coords, &x, &y);
		mouseMove(x, y, mask);
	}
}
void LLViewerMediaImpl::mouseDoubleClick(const LLVector2& texture_coords, MASK mask)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
    if (mMediaSource)
    {
        S32 x, y;
        scaleTextureCoords(texture_coords, &x, &y);
        mouseDoubleClick(x, y, mask);
    }
}
void LLViewerMediaImpl::mouseDoubleClick(S32 x, S32 y, MASK mask, S32 button)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOUBLE_CLICK, button, x, y, mask);
	}
}
void LLViewerMediaImpl::scrollWheel(S32 x, S32 y, S32 scroll_x, S32 scroll_y, MASK mask)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->scrollEvent(x, y, scroll_x, scroll_y, mask);
	}
}
void LLViewerMediaImpl::onMouseCaptureLost()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, 0, mLastMouseX, mLastMouseY, 0);
	}
}
BOOL LLViewerMediaImpl::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if(hasMouseCapture())
	{
		gFocusMgr.setMouseCapture( FALSE );
	}
	return TRUE;
}
void LLViewerMediaImpl::updateJavascriptObject()
{
	static LLFrameTimer timer ;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if ( mMediaSource )
	{
		static LLCachedControl<bool> enable(gSavedSettings, "BrowserEnableJSObject", false);
		if(!enable)
		{
			return ;
		}
		if(timer.getElapsedTimeF32() < 1.0f)
		{
			return ;
		}
		timer.reset() ;
		mMediaSource->jsEnableObject( enable );
		bool logged_in = (LLStartUp::getStartupState() >= STATE_STARTED);
		if ( logged_in )
		{
			LLVector3 agent_pos = gAgent.getPositionAgent();
			double x = agent_pos.mV[ VX ];
			double y = agent_pos.mV[ VY ];
			double z = agent_pos.mV[ VZ ];
			mMediaSource->jsAgentLocationEvent( x, y, z );
			LLVector3d agent_pos_global = gAgent.getLastPositionGlobal();
			double global_x = agent_pos_global.mdV[ VX ];
			double global_y = agent_pos_global.mdV[ VY ];
			double global_z = agent_pos_global.mdV[ VZ ];
			mMediaSource->jsAgentGlobalLocationEvent( global_x, global_y, global_z );
			double rotation = atan2( gAgent.getAtAxis().mV[VX], gAgent.getAtAxis().mV[VY] );
			double angle = rotation * RAD_TO_DEG;
			if ( angle < 0.0f ) angle = 360.0f + angle;
			mMediaSource->jsAgentOrientationEvent( angle );
			std::string region_name("");
			LLViewerRegion* region = gAgent.getRegion();
			if ( region )
			{
				region_name = region->getName();
			};
			mMediaSource->jsAgentRegionEvent( region_name );
		}
		mMediaSource->jsAgentLanguageEvent( LLUI::getLanguage() );
		if ( gAgent.prefersAdult() )
			mMediaSource->jsAgentMaturityEvent( "GMA" );
		else
		if ( gAgent.prefersMature() )
			mMediaSource->jsAgentMaturityEvent( "GM" );
		else
		if ( gAgent.prefersPG() )
			mMediaSource->jsAgentMaturityEvent( "G" );
	}
}
const std::string& LLViewerMediaImpl::getName() const
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
	{
		return mMediaSource->getMediaName();
	}
	return LLStringUtil::null;
};
void LLViewerMediaImpl::navigateBack()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
	{
		mMediaSource->browse_back();
	}
}
void LLViewerMediaImpl::navigateForward()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
	{
		mMediaSource->browse_forward();
	}
}
void LLViewerMediaImpl::navigateReload()
{
	navigateTo(getCurrentMediaURL(), "", true, false);
}
void LLViewerMediaImpl::navigateHome()
{
	bool rediscover_mimetype = mHomeMimeType.empty();
	navigateTo(mHomeURL, mHomeMimeType, rediscover_mimetype, false);
}
void LLViewerMediaImpl::unload()
{
	destroyMediaSource();
	resetPreviousMediaState();
	mMediaURL.clear();
	mMimeType.clear();
	mCurrentMediaURL.clear();
	mCurrentMimeType.clear();
}
void LLViewerMediaImpl::navigateTo(const std::string& url, const std::string& mime_type,  bool rediscover_type, bool server_request, bool clean_browser)
{
	if (url.empty())
	{
		LL_WARNS() << "Calling LLViewerMediaImpl::navigateTo with empty url" << LL_ENDL;
		return;
	}
	cancelMimeTypeProbe();
	if(mMediaURL != url)
	{
		resetPreviousMediaState();
	}
	mMediaURL = url;
	mMimeType = mime_type;
    mCleanBrowser = clean_browser;
	mCurrentMediaURL.clear();
	mNavigateRediscoverType = rediscover_type;
	mNavigateServerRequest = server_request;
	mMediaSourceFailed = false;
	if(mPriority == PRIORITY_UNLOADED)
	{
		{
			LLURI u(url);
			std::string sanitized_url = (u.query().empty() ? url : u.scheme() + "://" + u.authority() + u.path());
			LL_INFOS() << "NOT LOADING media id= " << mTextureId << " url=" << sanitized_url << ", mime_type=" << mime_type << LL_ENDL;
		}
		LL_DEBUGS("PluginPriority") << this << "Not loading (PRIORITY_UNLOADED)" << LL_ENDL;
		return;
	}
	navigateInternal();
}
void LLViewerMediaImpl::navigateInternal()
{
	{
		LLURI u(mMediaURL);
		std::string sanitized_url = (u.query().empty() ? mMediaURL : u.scheme() + "://" + u.authority() + u.path());
		LL_INFOS() << "media id= " << mTextureId << " url=" << sanitized_url << ", mime_type=" << mMimeType << LL_ENDL;
	}
	if (mMediaURL.empty())
	{
		LL_WARNS() << "Calling LLViewerMediaImpl::navigateInternal() with empty mMediaURL" << LL_ENDL;
		return;
	}
	if(mNavigateSuspended)
	{
		LL_WARNS() << "Deferring navigate." << LL_ENDL;
		mNavigateSuspendedDeferred = true;
		return;
	}
	if(mMimeProbe != nullptr)
	{
		LL_WARNS() << "MIME type probe already in progress -- bailing out." << LL_ENDL;
		return;
	}
	if(mNavigateServerRequest)
	{
		setNavState(MEDIANAVSTATE_SERVER_SENT);
	}
	else
	{
		setNavState(MEDIANAVSTATE_NONE);
	}
	if(!mMimeType.empty() && (mMimeType != LLMIMETypes::getDefaultMimeType()))
	{
		std::string plugin_basename = LLMIMETypes::implType(mMimeType);
		if(!plugin_basename.empty())
		{
			mNavigateRediscoverType = false;
		}
	}
	if(mNavigateRediscoverType)
	{
		LLURI uri(mMediaURL);
		std::string scheme = uri.scheme();
		if(scheme.empty() || "http" == scheme || "https" == scheme)
		{
			AIHTTPHeaders headers;
			headers.addHeader("Accept", "*/*");
			headers.addHeader("Cookie", "");
			LLHTTPClient::getHeaderOnly( mMediaURL, new LLMimeDiscoveryResponder(this), headers);
		}
		else if("data" == scheme || "file" == scheme || "about" == scheme)
		{
			if ("blank" != uri.hostName())
			{
				if(initializeMedia("text/html"))
				{
					loadURI();
				}
			}
		}
		else
		{
			if(initializeMedia(scheme))
			{
				loadURI();
			}
		}
	}
	else if(initializeMedia(mMimeType))
	{
		loadURI();
	}
	else
	{
		LL_WARNS("Media") << "Couldn't navigate to: " << mMediaURL << " as there is no media type for: " << mMimeType << LL_ENDL;
	}
}
void LLViewerMediaImpl::navigateStop()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		mMediaSource->browse_stop();
	}
}
bool LLViewerMediaImpl::handleKeyHere(KEY key, MASK mask)
{
	bool result = false;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
	{
		switch (mask)
		{
		case MASK_CONTROL:
		{
			result = true;
			switch(key)
			{
			case KEY_LEFT: case KEY_RIGHT: case KEY_HOME: case KEY_END: break;
			case 'C': mMediaSource->copy(); break;
			case 'V': mMediaSource->paste(); break;
			case 'X': mMediaSource->cut(); break;
			case '=': setPageZoomFactor(mZoomFactor + .1); break;
			case '-': setPageZoomFactor(mZoomFactor - .1);  break;
			case '0': setPageZoomFactor(1.0);  break;
			default: result = false; break;
			}
			break;
		}
		case MASK_SHIFT|MASK_CONTROL:
			if (key == 'I')
			{
				mMediaSource->showWebInspector(true);
				result = true;
			}
			break;
		}
		extern LLMenuBarGL* gLoginMenuBarView;
		result = result || (gLoginMenuBarView && gLoginMenuBarView->getVisible() && gLoginMenuBarView->handleAcceleratorKey(key, mask));
		if(!result)
		{
			LLSD native_key_data = gViewerWindow->getWindow()->getNativeKeyData();
			result = mMediaSource->keyEvent(LLPluginClassMedia::KEY_EVENT_DOWN, key, mask, native_key_data);
		}
	}
	return result;
}
bool LLViewerMediaImpl::handleKeyUpHere(KEY key, MASK mask)
{
	bool result = false;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
	{
		if (MASK_CONTROL & mask && key != KEY_LEFT && key != KEY_RIGHT && key != KEY_HOME && key != KEY_END)
		{
			result = true;
		}
		if (!result)
		{
			LLSD native_key_data = gViewerWindow->getWindow()->getNativeKeyData();
			result = mMediaSource->keyEvent(LLPluginClassMedia::KEY_EVENT_UP, key, mask, native_key_data);
		}
	}
	return result;
}
bool LLViewerMediaImpl::handleUnicodeCharHere(llwchar uni_char)
{
	bool result = false;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
	{
		if (uni_char >= 32
			&& uni_char != 127)
		{
			LLSD native_key_data = gViewerWindow->getWindow()->getNativeKeyData();
			mMediaSource->textInput(wstring_to_utf8str(LLWString(1, uni_char)), gKeyboard->currentMask(FALSE), native_key_data);
		}
	}
	return result;
}
bool LLViewerMediaImpl::canNavigateForward()
{
	BOOL result = FALSE;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
	{
		result = mMediaSource->getHistoryForwardAvailable();
	}
	return result;
}
bool LLViewerMediaImpl::canNavigateBack()
{
	BOOL result = FALSE;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
	{
		result = mMediaSource->getHistoryBackAvailable();
	}
	return result;
}
static LLTrace::BlockTimerStatHandle FTM_MEDIA_DO_UPDATE("Do Update");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_GET_DATA("Get Data");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_SET_SUBIMAGE("Set Subimage");
void LLViewerMediaImpl::update()
{
	LL_RECORD_BLOCK_TIME(FTM_MEDIA_DO_UPDATE);
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource == nullptr)
	{
		if(mPriority == PRIORITY_UNLOADED)
		{
		}
		else if(mPriority <= PRIORITY_SLIDESHOW)
		{
		}
		else if(mMimeProbe != nullptr)
		{
		}
		else
		{
			if(sMediaCreateTimer.hasExpired())
			{
				LL_DEBUGS("PluginPriority") << this << ": creating media based on timer expiration" << LL_ENDL;
				createMediaSource();
				sMediaCreateTimer.setTimerExpirySec(LLVIEWERMEDIA_CREATE_DELAY);
			}
			else
			{
				LL_DEBUGS("PluginPriority") << this << ": NOT creating media (waiting on timer)" << LL_ENDL;
			}
		}
	}
	else
	{
		updateVolume();
		updateJavascriptObject();
	}
	if(mMediaSource == nullptr)
	{
		return;
	}
	setNavigateSuspended(true);
	mMediaSource->idle();
	setNavigateSuspended(false);
	mMediaSource = getMediaPlugin();
	if(mMediaSource == nullptr)
	{
		return;
	}
	if(mMediaSource->isPluginExited())
	{
		resetPreviousMediaState();
		destroyMediaSource();
		return;
	}
	if(!mMediaSource->textureValid())
	{
		return;
	}
	if(mSuspendUpdates || !mVisible)
	{
		return;
	}
	LLViewerMediaTexture* placeholder_image = updatePlaceholderImage();
	if(placeholder_image)
	{
		LLRect dirty_rect;
		placeholder_image->setPlaying(TRUE);
		if(mMediaSource->getDirty(&dirty_rect))
		{
			S32 x_pos = llmax(dirty_rect.mLeft, 0);
			S32 y_pos = llmax(dirty_rect.mBottom, 0);
			S32 width = llmin(dirty_rect.mRight, placeholder_image->getWidth()) - x_pos;
			S32 height = llmin(dirty_rect.mTop, placeholder_image->getHeight()) - y_pos;
			if(width > 0 && height > 0)
			{
				U8* data = nullptr;
				{
					LL_RECORD_BLOCK_TIME(FTM_MEDIA_GET_DATA);
					data = mMediaSource->getBitsData();
				}
				if(data != NULL)
				{
				data += ( x_pos * mMediaSource->getTextureDepth() * mMediaSource->getBitsWidth() );
				data += ( y_pos * mMediaSource->getTextureDepth() );
				{
					LL_RECORD_BLOCK_TIME(FTM_MEDIA_SET_SUBIMAGE);
					placeholder_image->setSubImage(
							data,
							mMediaSource->getBitsWidth(),
							mMediaSource->getBitsHeight(),
							x_pos,
							y_pos,
							width,
							height,
							TRUE);
					}
				}
			}
			mMediaSource->resetDirty();
		}
	}
}
void LLViewerMediaImpl::updateImagesMediaStreams()
{
}
LLViewerMediaTexture* LLViewerMediaImpl::updatePlaceholderImage()
{
	if(mTextureId.isNull())
	{
		return nullptr;
	}
	LLViewerMediaTexture* placeholder_image = LLViewerTextureManager::getMediaTexture( mTextureId );
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mNeedsNewTexture
		|| placeholder_image->getUseMipMaps()
		|| (placeholder_image->getWidth() != mMediaSource->getTextureWidth())
		|| (placeholder_image->getHeight() != mMediaSource->getTextureHeight())
		|| (mTextureUsedWidth != mMediaSource->getWidth())
		|| (mTextureUsedHeight != mMediaSource->getHeight())
		)
	{
		LL_DEBUGS("Media") << "initializing media placeholder" << LL_ENDL;
		LL_DEBUGS("Media") << "movie image id " << mTextureId << LL_ENDL;
		int texture_width = mMediaSource->getTextureWidth();
		int texture_height = mMediaSource->getTextureHeight();
		int texture_depth = mMediaSource->getTextureDepth();
		placeholder_image->destroyGLTexture();
		placeholder_image->reinit(FALSE);
		LLPointer<LLImageRaw> raw = new LLImageRaw(texture_width, texture_height, texture_depth);
		raw->clear(int(mBackgroundColor.mV[VX] * 255.0f), int(mBackgroundColor.mV[VY] * 255.0f), int(mBackgroundColor.mV[VZ] * 255.0f), 0xff);
		int discard_level = 0;
		placeholder_image->setExplicitFormat(mMediaSource->getTextureFormatInternal(),
											 mMediaSource->getTextureFormatPrimary(),
											 mMediaSource->getTextureFormatType(),
											 mMediaSource->getTextureFormatSwapBytes());
		placeholder_image->createGLTexture(discard_level, raw);
		mNeedsNewTexture = false;
		mTextureUsedWidth = mMediaSource->getWidth();
		mTextureUsedHeight = mMediaSource->getHeight();
	}
	return placeholder_image;
}
LLUUID LLViewerMediaImpl::getMediaTextureID() const
{
	return mTextureId;
}
void LLViewerMediaImpl::setVisible(bool visible)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	mVisible = visible;
	if(mVisible)
	{
		if(mMediaSource && mMediaSource->isPluginExited())
		{
			destroyMediaSource();
			mMediaSource = NULL;
		}
		if(!mMediaSource)
		{
			createMediaSource();
		}
	}
}
void LLViewerMediaImpl::mouseCapture()
{
	gFocusMgr.setMouseCapture(this);
}
void LLViewerMediaImpl::scaleMouse(S32 *mouse_x, S32 *mouse_y)
{
#if 0
	S32 media_width, media_height;
	S32 texture_width, texture_height;
	getMediaSize( &media_width, &media_height );
	getTextureSize( &texture_width, &texture_height );
	S32 y_delta = texture_height - media_height;
	*mouse_y -= y_delta;
#endif
}
bool LLViewerMediaImpl::isMediaTimeBased()
{
	bool result = false;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		result = mMediaSource->pluginSupportsMediaTime();
	}
	return result;
}
bool LLViewerMediaImpl::isMediaPlaying()
{
	bool result = false;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		EMediaStatus status = mMediaSource->getStatus();
		if(status == MEDIA_PLAYING || status == MEDIA_LOADING)
			result = true;
	}
	return result;
}
bool LLViewerMediaImpl::isMediaPaused()
{
	bool result = false;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		if(mMediaSource->getStatus() == MEDIA_PAUSED)
			result = true;
	}
	return result;
}
bool LLViewerMediaImpl::hasMedia() const
{
	return mPluginBase != NULL;
}
void LLViewerMediaImpl::resetPreviousMediaState()
{
	mPreviousMediaState = MEDIA_NONE;
	mPreviousMediaTime = 0.0f;
}
void LLViewerMediaImpl::setDisabled(bool disabled, bool forcePlayOnEnable)
{
	if(mIsDisabled != disabled)
	{
		mIsDisabled = disabled;
		if(mIsDisabled)
		{
			unload();
		}
		else
		{
			if(isAutoPlayable() || forcePlayOnEnable)
			{
				navigateTo(mMediaEntryURL, "", true, true);
			}
		}
	}
};
bool LLViewerMediaImpl::isForcedUnloaded() const
{
	if(mIsMuted || mMediaSourceFailed || mIsDisabled)
	{
		return true;
	}
	if (!shouldShowBasedOnClass())
	{
		return true;
	}
	return false;
}
bool LLViewerMediaImpl::isPlayable() const
{
	if(isForcedUnloaded())
	{
		return false;
	}
	if(hasMedia())
	{
		return true;
	}
	if(!mMediaURL.empty())
	{
		return true;
	}
	return false;
}
static void handle_pick_file_request_continued(LLPluginClassMedia* plugin, AIFilePicker* filepicker)
{
	plugin->sendPickFileResponse(filepicker->hasFilename() ? filepicker->getFilenames() : std::vector<std::string>());
}
void LLViewerMediaImpl::handleMediaEvent(LLPluginClassMedia* plugin, LLPluginClassMediaOwner::EMediaEvent event)
{
	bool pass_through = true;
	switch(event)
	{
		case MEDIA_EVENT_CLICK_LINK_NOFOLLOW:
		{
			LL_DEBUGS("Media") << "MEDIA_EVENT_CLICK_LINK_NOFOLLOW, uri is: " << plugin->getClickURL() << LL_ENDL;
			std::string url = plugin->getClickURL();
			std::string nav_type = plugin->getClickNavType();
			LLURLDispatcher::dispatch(url, nav_type, nullptr, mTrustedBrowser);
		}
		break;
		case MEDIA_EVENT_CLICK_LINK_HREF:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLICK_LINK_HREF, target is \"" << plugin->getClickTarget() << "\", uri is " << plugin->getClickURL() << LL_ENDL;
		};
		break;
		case MEDIA_EVENT_PLUGIN_FAILED_LAUNCH:
		{
			mMediaSourceFailed = true;
			resetPreviousMediaState();
			LLSD args;
			args["PLUGIN"] = LLMIMETypes::implType(mCurrentMimeType);
			LLNotificationsUtil::add("MediaPluginFailed", args);
		}
		break;
		case MEDIA_EVENT_PLUGIN_FAILED:
		{
			mMediaSourceFailed = true;
			resetPreviousMediaState();
			LLSD args;
			args["PLUGIN"] = LLMIMETypes::implType(mCurrentMimeType);
		}
		break;
		case MEDIA_EVENT_CURSOR_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CURSOR_CHANGED, new cursor is " << plugin->getCursorName() << LL_ENDL;
			std::string cursor = plugin->getCursorName();
			if(cursor == "arrow")
				mLastSetCursor = UI_CURSOR_ARROW;
			else if(cursor == "ibeam")
				mLastSetCursor = UI_CURSOR_IBEAM;
			else if(cursor == "splith")
				mLastSetCursor = UI_CURSOR_SIZEWE;
			else if(cursor == "splitv")
				mLastSetCursor = UI_CURSOR_SIZENS;
			else if(cursor == "hand")
				mLastSetCursor = UI_CURSOR_HAND;
			else
				mLastSetCursor = UI_CURSOR_ARROW;
		}
		break;
		case LLViewerMediaObserver::MEDIA_EVENT_FILE_DOWNLOAD:
		{
			LLNotificationsUtil::add("MediaFileDownloadUnsupported");
		}
		break;
		case LLViewerMediaObserver::MEDIA_EVENT_NAVIGATE_BEGIN:
		{
			LL_DEBUGS("Media") << "MEDIA_EVENT_NAVIGATE_BEGIN, uri is: " << plugin->getNavigateURI() << LL_ENDL;
			hideNotification();
			if(getNavState() == MEDIANAVSTATE_SERVER_SENT)
			{
				setNavState(MEDIANAVSTATE_SERVER_BEGUN);
			}
			else
			{
				setNavState(MEDIANAVSTATE_BEGUN);
			}
		}
		break;
		case LLViewerMediaObserver::MEDIA_EVENT_NAVIGATE_COMPLETE:
		{
			LL_DEBUGS("Media") << "MEDIA_EVENT_NAVIGATE_COMPLETE, uri is: " << plugin->getNavigateURI() << LL_ENDL;
			std::string url = plugin->getNavigateURI();
			if(getNavState() == MEDIANAVSTATE_BEGUN)
			{
				if(mCurrentMediaURL == url)
				{
					setNavState(MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS);
				}
				else
				{
					mCurrentMediaURL = url;
					setNavState(MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED);
				}
			}
			else if(getNavState() == MEDIANAVSTATE_SERVER_BEGUN)
			{
				mCurrentMediaURL = url;
				setNavState(MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED);
			}
			else
			{
			}
		}
		break;
		case LLViewerMediaObserver::MEDIA_EVENT_LOCATION_CHANGED:
		{
			LL_DEBUGS("Media") << "MEDIA_EVENT_LOCATION_CHANGED, uri is: " << plugin->getLocation() << LL_ENDL;
			std::string url = plugin->getLocation();
			if(getNavState() == MEDIANAVSTATE_BEGUN)
			{
				if(mCurrentMediaURL == url)
				{
					setNavState(MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS);
				}
				else
				{
					mCurrentMediaURL = url;
					setNavState(MEDIANAVSTATE_FIRST_LOCATION_CHANGED);
				}
			}
			else if(getNavState() == MEDIANAVSTATE_SERVER_BEGUN)
			{
				mCurrentMediaURL = url;
				setNavState(MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED);
			}
			else
			{
				setNavState(MEDIANAVSTATE_NONE);
			}
		}
		break;
		case LLViewerMediaObserver::MEDIA_EVENT_PICK_FILE_REQUEST:
		{
			AIFilePicker* filepicker = AIFilePicker::create();
			filepicker->open(FFLOAD_ALL, "", "openfile", true);
			filepicker->run(boost::bind(&handle_pick_file_request_continued, plugin, filepicker));
		}
		break;
		case LLViewerMediaObserver::MEDIA_EVENT_AUTH_REQUEST:
		{
			LLNotification::Params auth_request_params("AuthRequest");
			LLSD args;
			LLURL raw_url( plugin->getAuthURL().c_str() );
			args["HOST_NAME"] = raw_url.getAuthority();
			args["REALM"] = plugin->getAuthRealm();
			auth_request_params.substitutions = args;
			auth_request_params.payload = LLSD().with("media_id", mTextureId);
			auth_request_params.functor(boost::bind(&LLViewerMedia::onAuthSubmit, _1, _2));
			LLNotifications::instance().add(auth_request_params);
		};
		break;
		case LLViewerMediaObserver::MEDIA_EVENT_CLOSE_REQUEST:
		{
			std::string uuid = plugin->getClickUUID();
			LL_INFOS() << "MEDIA_EVENT_CLOSE_REQUEST for uuid " << uuid << LL_ENDL;
			if(uuid.empty())
			{
			}
			else
			{
				pass_through = false;
				LLFloaterWebContent::closeRequest(uuid);
			}
		}
		break;
		case LLViewerMediaObserver::MEDIA_EVENT_GEOMETRY_CHANGE:
		{
			std::string uuid = plugin->getClickUUID();
			LL_INFOS() << "MEDIA_EVENT_GEOMETRY_CHANGE for uuid " << uuid << LL_ENDL;
			if(uuid.empty())
			{
			}
			else
			{
				pass_through = false;
				LLFloaterWebContent::geometryChanged(uuid, plugin->getGeometryX(), plugin->getGeometryY(), plugin->getGeometryWidth(), plugin->getGeometryHeight());
			}
		}
		break;
		case MEDIA_EVENT_DEBUG_MESSAGE:
		{
			std::string level = plugin->getDebugMessageLevel();
			if (level == "debug")
			{
				LL_DEBUGS("Media") << plugin->getDebugMessageText() << LL_ENDL;
			}
			else if (level == "info")
			{
				LL_INFOS("Media") << plugin->getDebugMessageText() << LL_ENDL;
			}
			else if (level == "warn")
			{
				LL_WARNS("Media") << plugin->getDebugMessageText() << LL_ENDL;
			}
			else if (level == "error")
			{
				LL_ERRS("Media") << plugin->getDebugMessageText() << LL_ENDL;
			}
			else
			{
				LL_INFOS("Media") << plugin->getDebugMessageText() << LL_ENDL;
			}
		};
		break;
		default:
		break;
	}
	if(pass_through)
	{
		emitEvent(plugin, event);
	}
}
void
LLViewerMediaImpl::undo()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		mMediaSource->undo();
}
BOOL
LLViewerMediaImpl::canUndo() const
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		return mMediaSource->canUndo();
	else
		return FALSE;
}
void
LLViewerMediaImpl::redo()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		mMediaSource->redo();
}
BOOL
LLViewerMediaImpl::canRedo() const
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		return mMediaSource->canRedo();
	else
		return FALSE;
}
void
LLViewerMediaImpl::cut()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		mMediaSource->cut();
}
BOOL
LLViewerMediaImpl::canCut() const
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		return mMediaSource->canCut();
	else
		return FALSE;
}
void
LLViewerMediaImpl::copy() const
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		mMediaSource->copy();
}
BOOL
LLViewerMediaImpl::canCopy() const
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		return mMediaSource->canCopy();
	else
		return FALSE;
}
void
LLViewerMediaImpl::paste()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		mMediaSource->paste();
}
BOOL
LLViewerMediaImpl::canPaste() const
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		return mMediaSource->canPaste();
	else
		return FALSE;
}
void
LLViewerMediaImpl::doDelete()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		mMediaSource->doDelete();
}
BOOL
LLViewerMediaImpl::canDoDelete() const
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		return mMediaSource->canDoDelete();
	else
		return FALSE;
}
void
LLViewerMediaImpl::selectAll()
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		mMediaSource->selectAll();
}
BOOL
LLViewerMediaImpl::canSelectAll() const
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if (mMediaSource)
		return mMediaSource->canSelectAll();
	else
		return FALSE;
}
void LLViewerMediaImpl::setUpdated(BOOL updated)
{
	mIsUpdated = updated ;
}
BOOL LLViewerMediaImpl::isUpdated()
{
	return mIsUpdated ;
}
static LLTrace::BlockTimerStatHandle FTM_MEDIA_CALCULATE_INTEREST("Calculate Interest");
void LLViewerMediaImpl::calculateInterest()
{
	LL_RECORD_BLOCK_TIME(FTM_MEDIA_CALCULATE_INTEREST);
	LLViewerMediaTexture* texture = LLViewerTextureManager::findMediaTexture( mTextureId );
	if(texture != nullptr)
	{
		mInterest = texture->getMaxVirtualSize();
	}
	else
	{
		mInterest = 0.0f;
	}
	mProximityDistance = 0.0f;
	mProximityCamera = 0.0f;
	if(!mObjectList.empty())
	{
		std::list< LLVOVolume* >::iterator iter = mObjectList.begin() ;
		LLVOVolume* objp = *iter ;
		llassert_always(objp != NULL) ;
		if(!objp->isHUDAttachment())
		{
			LLVector3d obj_global = objp->getPositionGlobal() ;
			LLVector3d agent_global = gAgent.getPositionGlobal() ;
			LLVector3d global_delta = agent_global - obj_global ;
			mProximityDistance = global_delta.magVecSquared();
			LLVector3d camera_delta = gAgentCamera.getCameraPositionGlobal() - obj_global;
			mProximityCamera = camera_delta.magVec();
		}
	}
	if(mNeedsMuteCheck)
	{
		mIsMuted = false;
		std::list< LLVOVolume* >::iterator iter = mObjectList.begin() ;
		for(; iter != mObjectList.end() ; ++iter)
		{
			LLVOVolume *obj = *iter;
			llassert(obj);
			if (!obj) continue;
			if(LLMuteList::getInstance() &&
			   LLMuteList::getInstance()->isMuted(obj->getID()))
			{
				mIsMuted = true;
			}
			else
			{
				if (LLSelectMgr::getInstance())
				{
					LLPermissions* obj_perm = LLSelectMgr::getInstance()->findObjectPermissions(obj);
					if(obj_perm)
					{
						if(LLMuteList::getInstance() &&
						   LLMuteList::getInstance()->isMuted(obj_perm->getOwner()))
							mIsMuted = true;
					}
				}
			}
		}
		mNeedsMuteCheck = false;
	}
}
F64 LLViewerMediaImpl::getApproximateTextureInterest()
{
	F64 result = 0.0f;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		result = mMediaSource->getFullWidth();
		result *= mMediaSource->getFullHeight();
	}
	else
	{
		result = mMediaWidth;
		result *= mMediaHeight;
	}
	return result;
}
void LLViewerMediaImpl::setUsedInUI(bool used_in_ui)
{
	mUsedInUI = used_in_ui;
	if(mUsedInUI && (mPriority == PRIORITY_UNLOADED))
	{
		if(getVisible())
		{
			setPriority(PRIORITY_NORMAL);
		}
		else
		{
			setPriority(PRIORITY_HIDDEN);
		}
		createMediaSource();
	}
};
void LLViewerMediaImpl::setBackgroundColor(LLColor4 color)
{
	mBackgroundColor = color;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		mMediaSource->setBackgroundColor(mBackgroundColor);
	}
};
F64 LLViewerMediaImpl::getCPUUsage() const
{
	F64 result = 0.0f;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		result = mMediaSource->getCPUUsage();
	}
	return result;
}
char const* PRIORITYToString(LLViewerMediaImpl::EPriority priority)
{
	switch(priority)
	{
	case LLViewerMediaImpl::PRIORITY_UNLOADED:	return "unloaded";
	case LLViewerMediaImpl::PRIORITY_HIDDEN:	return "hidden";
	case LLViewerMediaImpl::PRIORITY_SLIDESHOW: return "slideshow";
	case LLViewerMediaImpl::PRIORITY_LOW:		return "low";
	case LLViewerMediaImpl::PRIORITY_NORMAL:	return "normal";
	case LLViewerMediaImpl::PRIORITY_HIGH:		return "high";
	default:									return "UNKNOWN";
	}
}
void LLViewerMediaImpl::setPriority(EPriority priority)
{
	if(mPriority != priority)
	{
		LL_DEBUGS("PluginPriority")
			<< "changing priority of media id " << mTextureId
			<< " from " << ::PRIORITYToString(mPriority)
			<< " to " << ::PRIORITYToString(priority)
			<< LL_ENDL;
	}
	mPriority = priority;
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(priority == PRIORITY_UNLOADED)
	{
		if(mMediaSource)
		{
			mPreviousMediaState = mMediaSource->getStatus();
			mPreviousMediaTime = mMediaSource->getCurrentTime();
			destroyMediaSource();
			mMediaSource = NULL;
		}
	}
	if(mMediaSource)
	{
		if(mPriority >= PRIORITY_LOW)
			mMediaSource->setPriority((LLPluginClassBasic::EPriority)((U32)mPriority-((U32)PRIORITY_LOW-1)));
		else
			mMediaSource->setPriority(LLPluginClassBasic::PRIORITY_SLEEP);
	}
}
void LLViewerMediaImpl::setLowPrioritySizeLimit(int size)
{
	LLPluginClassMedia* mMediaSource = getMediaPlugin();
	if(mMediaSource)
	{
		mMediaSource->setLowPrioritySizeLimit(size);
	}
}
void LLViewerMediaImpl::setNavState(EMediaNavState state)
{
	mMediaNavState = state;
	switch (state)
	{
		case MEDIANAVSTATE_NONE: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_NONE" << LL_ENDL; break;
		case MEDIANAVSTATE_BEGUN: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_BEGUN" << LL_ENDL; break;
		case MEDIANAVSTATE_FIRST_LOCATION_CHANGED: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_FIRST_LOCATION_CHANGED" << LL_ENDL; break;
		case MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS" << LL_ENDL; break;
		case MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED" << LL_ENDL; break;
		case MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS" << LL_ENDL; break;
		case MEDIANAVSTATE_SERVER_SENT: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_SERVER_SENT" << LL_ENDL; break;
		case MEDIANAVSTATE_SERVER_BEGUN: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_SERVER_BEGUN" << LL_ENDL; break;
		case MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED" << LL_ENDL; break;
		case MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED" << LL_ENDL; break;
	}
}
void LLViewerMediaImpl::setNavigateSuspended(bool suspend)
{
	if(mNavigateSuspended != suspend)
	{
		mNavigateSuspended = suspend;
		if(!suspend)
		{
			if(mNavigateSuspendedDeferred)
			{
				mNavigateSuspendedDeferred = false;
				navigateInternal();
			}
		}
	}
}
void LLViewerMediaImpl::cancelMimeTypeProbe()
{
	if(mMimeProbe)
	{
		mMimeProbe->cancelRequest();
		if (mMimeProbe)
		{
			LL_ERRS() << "internal error: mMimeProbe is not nullptr after cancelling request." << LL_ENDL;
		}
	}
}
void LLViewerMediaImpl::addObject(LLVOVolume* obj)
{
	std::list< LLVOVolume* >::iterator iter = mObjectList.begin() ;
	for(; iter != mObjectList.end() ; ++iter)
	{
		if(*iter == obj)
		{
			return ;
		}
	}
	mObjectList.push_back(obj) ;
	mNeedsMuteCheck = true;
}
void LLViewerMediaImpl::removeObject(LLVOVolume* obj)
{
	mObjectList.remove(obj) ;
	mNeedsMuteCheck = true;
}
const std::list< LLVOVolume* >* LLViewerMediaImpl::getObjectList() const
{
	return &mObjectList ;
}
LLVOVolume *LLViewerMediaImpl::getSomeObject()
{
	LLVOVolume *result = nullptr;
	std::list< LLVOVolume* >::iterator iter = mObjectList.begin() ;
	if(iter != mObjectList.end())
	{
		result = *iter;
	}
	return result;
}
void LLViewerMediaImpl::setTextureID(LLUUID id)
{
	if(id != mTextureId)
	{
		if(mTextureId.notNull())
		{
			sViewerMediaTextureIDMap.erase(mTextureId);
		}
		if(id.notNull())
		{
			sViewerMediaTextureIDMap.insert(LLViewerMedia::impl_id_map::value_type(id, this));
		}
		mTextureId = id;
	}
}
bool LLViewerMediaImpl::isAutoPlayable() const
{
	static const LLCachedControl<bool> media_tentative_auto_play("MediaTentativeAutoPlay",false);
	static const LLCachedControl<bool> auto_play_parcel_media(LLViewerMedia::AUTO_PLAY_MEDIA_SETTING,false);
	static const LLCachedControl<bool> auto_play_prim_media(LLViewerMedia::AUTO_PLAY_PRIM_MEDIA_SETTING,false);
	return mMediaAutoPlay && media_tentative_auto_play &&
		(getUsedInUI()
		|| (isParcelMedia() && auto_play_parcel_media)
		|| auto_play_prim_media);
}
bool LLViewerMediaImpl::shouldShowBasedOnClass() const
{
	if (getUsedInUI() || isParcelMedia()) return true;
	bool attached_to_another_avatar = isAttachedToAnotherAvatar();
	bool inside_parcel = isInAgentParcel();
	if (attached_to_another_avatar)
	{
		static LLCachedControl<bool> show_media_on_others(gSavedSettings, LLViewerMedia::SHOW_MEDIA_ON_OTHERS_SETTING, false);
		return show_media_on_others;
	}
	if (inside_parcel)
	{
		static LLCachedControl<bool> show_media_within_parcel(gSavedSettings, LLViewerMedia::SHOW_MEDIA_WITHIN_PARCEL_SETTING, true);
		return show_media_within_parcel;
	}
	else
	{
		static LLCachedControl<bool> show_media_outside_parcel(gSavedSettings, LLViewerMedia::SHOW_MEDIA_OUTSIDE_PARCEL_SETTING, true);
		return show_media_outside_parcel;
	}
}
bool LLViewerMediaImpl::isAttachedToAnotherAvatar() const
{
	bool result = false;
	std::list< LLVOVolume* >::const_iterator iter = mObjectList.begin();
	std::list< LLVOVolume* >::const_iterator end = mObjectList.end();
	for ( ; iter != end; iter++)
	{
		if (isObjectAttachedToAnotherAvatar(*iter))
		{
			result = true;
			break;
		}
	}
	return result;
}
bool LLViewerMediaImpl::isObjectAttachedToAnotherAvatar(LLVOVolume *obj)
{
	bool result = false;
	LLXform *xform = obj;
	while (nullptr != xform)
	{
		LLViewerObject *object = dynamic_cast<LLViewerObject*> (xform);
		if (nullptr != object)
		{
			LLVOAvatar *avatar = object->asAvatar();
			if ((nullptr != avatar) && (avatar != gAgentAvatarp))
			{
				result = true;
				break;
			}
		}
		xform = xform->getParent();
	}
	return result;
}
bool LLViewerMediaImpl::isInAgentParcel() const
{
	bool result = false;
	std::list< LLVOVolume* >::const_iterator iter = mObjectList.begin();
	std::list< LLVOVolume* >::const_iterator end = mObjectList.end();
	for ( ; iter != end; iter++)
	{
		LLVOVolume *object = *iter;
		if (LLViewerMediaImpl::isObjectInAgentParcel(object))
		{
			result = true;
			break;
		}
	}
	return result;
}
LLNotificationPtr LLViewerMediaImpl::getCurrentNotification() const
{
	return mNotification;
}
bool LLViewerMediaImpl::isObjectInAgentParcel(LLVOVolume *obj)
{
	return (LLViewerParcelMgr::getInstance()->inAgentParcel(obj->getPositionGlobal()));
}
