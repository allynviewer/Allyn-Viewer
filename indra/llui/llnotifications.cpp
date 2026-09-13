/**
* @file llnotifications.cpp
* @brief Non-UI queue manager for keeping a prioritized list of notifications
*
* $LicenseInfo:firstyear=2008&license=viewergpl$
* 
* Copyright (c) 2008-2009, Linden Research, Inc.
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
#include "linden_common.h"
#include "llnotifications.h"
#include "llnotificationtemplate.h"
#include "lluictrlfactory.h"
#include "lldir.h"
#include "llsdserialize.h"
#include "lltrans.h"
#include "llnotifications.h"
#include "aialert.h"
#include "aistatemachine.h"
#include "../newview/hippogridmanager.h"
#include <algorithm>
#if LL_MSVC
#pragma warning( disable       : 4265 )
#endif
#include <boost/regex.hpp>
#define AILOCK_mItems mItems_wat mItems_w(mItems_sf); LLNotificationSet& mItems(*mItems_w)
#define AILOCK_const_mItems mItems_crat mItems_r(mItems_sf); LLNotificationSet const& mItems(*mItems_r)
const std::string NOTIFICATION_PERSIST_VERSION = "0.93";
class LLNotificationHistoryChannel : public LLNotificationChannel
{
	LOG_CLASS(LLNotificationHistoryChannel);
public:
	LLNotificationHistoryChannel(const std::string& filename) :
		LLNotificationChannel("History", "Visible", &historyFilter, LLNotificationComparators::orderByUUID()),
		mFileName(filename)
	{
		connectChanged(boost::bind(&LLNotificationHistoryChannel::historyHandler, this, _1));
		loadPersistentNotifications();
	}
private:
	bool historyHandler(const LLSD& payload)
	{
		std::string sigtype = payload["sigtype"];
		if (sigtype != "load" && sigtype !=  "delete")
		{
			savePersistentNotifications();
		}
		return false;
	}
	static bool historyFilter(LLNotificationPtr pNotification)
	{
		return pNotification->isPersistent() && !pNotification->isCancelled() && !pNotification->isRespondedTo() && !pNotification->isExpired();
	}
	void savePersistentNotifications()
	{
		if (mLoading)
		{
			return;
		}
		LL_INFOS() << "Saving open notifications to " << mFileName << LL_ENDL;
		llofstream notify_file(mFileName.c_str());
		if (!notify_file.is_open())
		{
			LL_WARNS() << "Failed to open " << mFileName << LL_ENDL;
			return;
		}
		LLSD output;
		output["version"] = NOTIFICATION_PERSIST_VERSION;
		LLSD& data = output["data"];
		AILOCK_mItems;
		static LLCachedControl<S32> maxPersistentNotificaitons("MaxPersistentNotifications");
		for (LLNotificationSet::iterator it = mItems.begin(); it != mItems.end(); ++it)
		{
			if (!LLNotificationTemplates::instance().templateExists((*it)->getName())) continue;
			LLNotificationTemplatePtr templatep = LLNotificationTemplates::instance().getTemplate((*it)->getName());
			if (!templatep->mPersist) continue;
			if ((*it)->isCancelled() || (*it)->isExpired() || (*it)->isRespondedTo()) continue;
			if (data.size() >= maxPersistentNotificaitons)
			{
				LL_WARNS() << "Too many persistent notifications."
					<< " Saved " << maxPersistentNotificaitons << " of " << mItems.size()
					<< " persistent notifications." << LL_ENDL;
				break;
			}
			data.append((*it)->asLLSD());
		}
		LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
		formatter->format(output, notify_file, LLSDFormatter::OPTIONS_PRETTY);
	}
	void loadPersistentNotifications()
	{
		if (mLoading)
		{
			return;
		}
		mLoading = true;
		LL_INFOS() << "Loading open notifications from " << mFileName << LL_ENDL;
		while (true)
		{
			llifstream notify_file(mFileName.c_str());
			if (!notify_file.is_open())
			{
				LL_WARNS() << "Failed to open " << mFileName << LL_ENDL;
				break;
			}
			LLSD input;
			LLPointer<LLSDParser> parser = new LLSDXMLParser();
			if (parser->parse(notify_file, input, LLSDSerialize::SIZE_UNLIMITED) < 0)
			{
				LL_WARNS() << "Failed to parse open notifications" << LL_ENDL;
				break;
			}
			if (input.isUndefined()) return;
			std::string version = input["version"];
			if (version != NOTIFICATION_PERSIST_VERSION)
			{
				LL_WARNS() << "Bad open notifications version: " << version << LL_ENDL;
				break;
			}
			LLSD& data = input["data"];
			if (data.isUndefined()) break;
			S32 processed_notifications = 0;
			static LLCachedControl<S32> maxPersistentNotificaitons("MaxPersistentNotifications");
			LLNotifications& instance = LLNotifications::instance();
			for (LLSD::array_const_iterator notification_it = data.beginArray();
				notification_it != data.endArray();
				++notification_it)
			{
				if (processed_notifications++ >= maxPersistentNotificaitons)
				{
					LL_WARNS() << "Too many persistent notifications."
						<< " Processed " << maxPersistentNotificaitons << " of " << data.size() << " persistent notifications." << LL_ENDL;
					break;
				}
				instance.add(LLNotificationPtr(new LLNotification(*notification_it)));
			}
			break;
		}
		mLoading = false;
		savePersistentNotifications();
	}
	void onDelete(LLNotificationPtr pNotification)
	{
		{
			AILOCK_mItems;
			mItems.erase(pNotification);
		}
		savePersistentNotifications();
	}
private:
	bool mLoading = false;
	std::string mFileName;
};
bool filterIgnoredNotifications(LLNotificationPtr notification)
{
	LLNotificationFormPtr form = notification->getForm();
	return !notification->getForm()->getIgnored();
}
bool handleIgnoredNotification(const LLSD& payload)
{
	if (payload["sigtype"].asString() == "add")
	{
		LLNotificationPtr pNotif = LLNotifications::instance().find(payload["id"].asUUID());
		if (!pNotif) return false;
		LLNotificationFormPtr form = pNotif->getForm();
		LLSD response;
		switch(form->getIgnoreType())
		{
		case LLNotificationForm::IGNORE_WITH_DEFAULT_RESPONSE:
			response = pNotif->getResponseTemplate(LLNotification::WITH_DEFAULT_BUTTON);
			break;
		case LLNotificationForm::IGNORE_WITH_LAST_RESPONSE:
			response = LLUI::sIgnoresGroup->getLLSD("Default" + pNotif->getName());
			break;
		case LLNotificationForm::IGNORE_SHOW_AGAIN:
			break;
		default:
			return false;
		}
		pNotif->setIgnored(true);
		pNotif->respond(response);
		return true;
	}
	return false;
}
namespace LLNotificationFilters
{
	bool includeEverything(LLNotificationPtr p)
	{
		return true;
	}
};
LLNotificationForm::LLNotificationForm()
:	mFormData(LLSD::emptyArray()),
	mIgnore(IGNORE_NO)
{
}
LLNotificationForm::LLNotificationForm(const std::string& name, const LLXMLNodePtr xml_node)
:	mFormData(LLSD::emptyArray()),
	mIgnore(IGNORE_NO),
	mInvertSetting(false)
{
	if (!xml_node->hasName("form"))
	{
		LL_WARNS() << "Bad xml node for form: " << xml_node->getName() << LL_ENDL;
	}
	LLXMLNodePtr child = xml_node->getFirstChild();
	while(child)
	{
		child = LLNotificationTemplates::instance().checkForXMLTemplate(child);
		LLSD item_entry;
		std::string element_name = child->getName()->mString;
		if (element_name == "ignore")
		{
			bool save_option = false;
			child->getAttribute_bool("save_option", save_option);
			if (!save_option)
			{
				mIgnore = IGNORE_WITH_DEFAULT_RESPONSE;
			}
			else
			{
				mIgnore = IGNORE_WITH_LAST_RESPONSE;
				LLUI::sIgnoresGroup->declareLLSD(std::string("Default") + name, "", std::string("Default response for notification " + name));
			}
			child->getAttributeString("text", mIgnoreMsg);
			mIgnoreSetting = LLUI::sIgnoresGroup->addWarning(name);
		}
		else
		{
			item_entry["type"] = element_name;
			const LLXMLAttribList::iterator attrib_end = child->mAttributes.end();
			for(LLXMLAttribList::iterator attrib_it = child->mAttributes.begin();
				attrib_it != attrib_end;
				++attrib_it)
			{
				item_entry[std::string(attrib_it->second->getName()->mString)] = attrib_it->second->getValue();
			}
			item_entry["value"] = child->getTextContents();
			mFormData.append(item_entry);
		}
		child = child->getNextSibling();
	}
}
LLNotificationForm::LLNotificationForm(const LLSD& sd)
	: mIgnore(IGNORE_NO)
{
	if (sd.isArray())
	{
		mFormData = sd;
	}
	else
	{
		if (!sd.isUndefined())
			LL_WARNS() << "Invalid form data " << sd << LL_ENDL;
		mFormData = LLSD::emptyArray();
	}
}
LLSD LLNotificationForm::asLLSD() const
{
	return mFormData;
}
LLSD LLNotificationForm::getElement(const std::string& element_name)
{
	for (LLSD::array_const_iterator it = mFormData.beginArray();
		it != mFormData.endArray();
		++it)
	{
		if ((*it)["name"].asString() == element_name) return (*it);
	}
	return LLSD();
}
bool LLNotificationForm::hasElement(const std::string& element_name)
{
	for (LLSD::array_const_iterator it = mFormData.beginArray();
		it != mFormData.endArray();
		++it)
	{
		if ((*it)["name"].asString() == element_name) return true;
	}
	return false;
}
void LLNotificationForm::addElement(const std::string& type, const std::string& name, const LLSD& value)
{
	LLSD element;
	element["type"] = type;
	element["name"] = name;
	element["text"] = name;
	element["value"] = value;
	element["index"] = mFormData.size();
	mFormData.append(element);
}
void LLNotificationForm::append(const LLSD& sub_form)
{
	if (sub_form.isArray())
	{
		for (LLSD::array_const_iterator it = sub_form.beginArray();
			it != sub_form.endArray();
			++it)
		{
			mFormData.append(*it);
		}
	}
}
void LLNotificationForm::formatElements(const LLSD& substitutions)
{
	for (LLSD::array_iterator it = mFormData.beginArray();
		it != mFormData.endArray();
		++it)
	{
		if ((*it).has("text"))
		{
			std::string text = (*it)["text"].asString();
			LLStringUtil::format(text, substitutions);
			(*it)["text"] = text;
		}
		if ((*it)["type"].asString() == "text" && (*it).has("value"))
		{
			std::string value = (*it)["value"].asString();
			LLStringUtil::format(value, substitutions);
			(*it)["value"] = value;
		}
	}
}
std::string LLNotificationForm::getDefaultOption()
{
	for (LLSD::array_const_iterator it = mFormData.beginArray();
		it != mFormData.endArray();
		++it)
	{
		if ((*it)["default"]) return (*it)["name"].asString();
	}
	return "";
}
LLControlVariablePtr LLNotificationForm::getIgnoreSetting()
{
	return mIgnoreSetting;
}
bool LLNotificationForm::getIgnored()
{
	bool show = true;
	if (mIgnore != LLNotificationForm::IGNORE_NO
		&& mIgnoreSetting)
	{
		show = mIgnoreSetting->getValue().asBoolean();
		if (mInvertSetting) show = !show;
	}
	return !show;
}
void LLNotificationForm::setIgnored(bool ignored)
{
	if (mIgnoreSetting)
	{
		if (mInvertSetting) ignored = !ignored;
		mIgnoreSetting->setValue(!ignored);
	}
}
LLNotificationTemplate::LLNotificationTemplate() :
	mExpireSeconds(0),
	mExpireOption(-1),
	mURLOption(-1),
	mUnique(false),
	mPriority(NOTIFICATION_PRIORITY_NORMAL)
{
	mForm = LLNotificationFormPtr(new LLNotificationForm());
}
LLNotification::LLNotification(const LLNotification::Params& p) :
	mTimestamp(p.timestamp),
	mSubstitutions(p.substitutions),
	mPayload(p.payload),
	mExpiresAt(F64SecondsImplicit()),
	mResponseFunctorName(p.functor_name),
	mTemporaryResponder(p.mTemporaryResponder),
	mRespondedTo(false),
	mPriority(p.priority),
	mCancelled(false),
	mIgnored(false)
{
	mId.generate();
	init(p.name, p.form_elements);
}
LLNotification::LLNotification(const LLSD& sd) :
	mTemporaryResponder(false),
	mRespondedTo(false),
	mCancelled(false),
	mIgnored(false)
{
	mId.generate();
	mSubstitutions = sd["substitutions"];
	mPayload = sd["payload"];
	mTimestamp = sd["time"];
	mExpiresAt = sd["expiry"];
	mPriority = (ENotificationPriority)sd["priority"].asInteger();
	mResponseFunctorName = sd["responseFunctor"].asString();
	std::string templatename = sd["name"].asString();
	init(templatename, LLSD());
	mForm = LLNotificationFormPtr(new LLNotificationForm(sd["form"]));
}
LLSD LLNotification::asLLSD()
{
	LLSD output;
	output["id"] = mId;
	output["name"] = mTemplatep->mName;
	output["form"] = getForm()->asLLSD();
	output["substitutions"] = mSubstitutions;
	output["payload"] = mPayload;
	output["time"] = mTimestamp;
	output["expiry"] = mExpiresAt;
	output["priority"] = (S32)mPriority;
	output["responseFunctor"] = mResponseFunctorName;
	return output;
}
void LLNotification::update()
{
	LLNotifications::instance().update(shared_from_this());
}
void LLNotification::updateFrom(LLNotificationPtr other)
{
	if (mTemplatep != other->mTemplatep) return;
	mPayload = other->mPayload;
	mSubstitutions = other->mSubstitutions;
	mTimestamp = other->mTimestamp;
	mExpiresAt = other->mExpiresAt;
	mCancelled = other->mCancelled;
	mIgnored = other->mIgnored;
	mPriority = other->mPriority;
	mForm = other->mForm;
	mResponseFunctorName = other->mResponseFunctorName;
	mRespondedTo = other->mRespondedTo;
	mTemporaryResponder = other->mTemporaryResponder;
	update();
}
const LLNotificationFormPtr LLNotification::getForm()
{
	return mForm;
}
void LLNotification::cancel()
{
	mCancelled = true;
}
LLSD LLNotification::getResponseTemplate(EResponseTemplateType type)
{
	LLSD response = LLSD::emptyMap();
	for (S32 element_idx = 0;
		element_idx < mForm->getNumElements();
		++element_idx)
	{
		LLSD element = mForm->getElement(element_idx);
		if (element.has("name"))
		{
			response[element["name"].asString()] = element["value"];
		}
		if ((type == WITH_DEFAULT_BUTTON)
			&& element["default"].asBoolean())
		{
			response[element["name"].asString()] = true;
		}
	}
	return response;
}
S32 LLNotification::getSelectedOption(const LLSD& notification, const LLSD& response)
{
	LLNotificationForm form(notification["form"]);
	for (S32 element_idx = 0;
		element_idx < form.getNumElements();
		++element_idx)
	{
		LLSD element = form.getElement(element_idx);
		if (element["type"].asString() == "button"
			&& response[element["name"].asString()].asBoolean())
		{
			return element["index"].asInteger();
		}
	}
	return -1;
}
std::string LLNotification::getSelectedOptionName(const LLSD& response)
{
	for (LLSD::map_const_iterator response_it = response.beginMap();
		response_it != response.endMap();
		++response_it)
	{
		if (response_it->second.isBoolean() && response_it->second.asBoolean())
		{
			return response_it->first;
		}
	}
	return "";
}
void LLNotification::respond(const LLSD& response)
{
	mRespondedTo = true;
	LLNotificationFunctorRegistry::ResponseFunctor functor =
		LLNotificationFunctorRegistry::instance().getFunctor(mResponseFunctorName);
	functor(asLLSD(), response);
	if (mTemporaryResponder)
	{
		LLNotificationFunctorRegistry::instance().unregisterFunctor(mResponseFunctorName);
		mResponseFunctorName = "";
		mTemporaryResponder = false;
	}
	if (mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO)
	{
		mForm->setIgnored(mIgnored);
		if (mIgnored && mForm->getIgnoreType() == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE)
		{
			LLUI::sIgnoresGroup->setLLSD("Default" + getName(), response);
		}
	}
	update();
}
const std::string& LLNotification::getName() const
{
	return mTemplatep->mName;
}
const std::string& LLNotification::getIcon() const
{
	return mTemplatep->mIcon;
}
bool LLNotification::isPersistent() const
{
	return mTemplatep->mPersist;
}
std::string LLNotification::getType() const
{
	return (mTemplatep ? mTemplatep->mType : "");
}
S32 LLNotification::getURLOption() const
{
	return (mTemplatep ? mTemplatep->mURLOption : -1);
}
bool LLNotification::hasUniquenessConstraints() const
{
	return (mTemplatep ? mTemplatep->mUnique : false);
}
void LLNotification::setIgnored(bool ignore)
{
	mIgnored = ignore;
}
void LLNotification::setResponseFunctor(std::string const &responseFunctorName)
{
	if (mTemporaryResponder)
		LLNotificationFunctorRegistry::instance().unregisterFunctor(mResponseFunctorName);
	mResponseFunctorName = responseFunctorName;
	mTemporaryResponder = false;
}
bool LLNotification::isEquivalentTo(LLNotificationPtr that) const
{
	if (this->mTemplatep->mName != that->mTemplatep->mName)
	{
		return false;
	}
	if (this->mTemplatep->mUnique)
	{
		const LLSD& these_substitutions = this->getSubstitutions();
		const LLSD& those_substitutions = that->getSubstitutions();
		const LLSD& this_payload = this->getPayload();
		const LLSD& that_payload = that->getPayload();
		for (std::vector<std::string>::const_iterator it = mTemplatep->mUniqueContext.begin(), end_it = mTemplatep->mUniqueContext.end();
			it != end_it;
			++it)
		{
			if (these_substitutions.get(*it).asString() != those_substitutions.get(*it).asString()
				|| this_payload.get(*it).asString() != that_payload.get(*it).asString())
			{
				return false;
			}
		}
		return true;
	}
	return false;
}
void LLNotification::init(const std::string& template_name, const LLSD& form_elements)
{
	mTemplatep = LLNotificationTemplates::instance().getTemplate(template_name);
	if (!mTemplatep) return;
	const LLStringUtil::format_map_t& default_args = LLTrans::getDefaultArgs();
	for (LLStringUtil::format_map_t::const_iterator iter = default_args.begin();
		 iter != default_args.end(); ++iter)
	{
		mSubstitutions[iter->first] = iter->second;
	}
	mSubstitutions["_URL"] = getURL();
	mSubstitutions["_NAME"] = template_name;
	mForm = LLNotificationFormPtr(new LLNotificationForm(*mTemplatep->mForm));
	mForm->append(form_elements);
	mForm->formatElements(mSubstitutions);
	mIgnored = mForm->getIgnored();
	LLDate rightnow = LLDate::now();
	if (mTemplatep->mExpireSeconds)
	{
		mExpiresAt = LLDate(rightnow.secondsSinceEpoch() + mTemplatep->mExpireSeconds);
	}
	if (mPriority == NOTIFICATION_PRIORITY_UNSPECIFIED)
	{
		mPriority = mTemplatep->mPriority;
	}
}
std::string LLNotification::summarize() const
{
	std::string s = "Notification(";
	s += getName();
	s += ") : ";
	s += mTemplatep ? mTemplatep->mMessage : "";
	return s;
}
std::string LLNotification::getMessage() const
{
	if (!mTemplatep)
		return std::string();
	std::string message = mTemplatep->mMessage;
	LLStringUtil::format(message, mSubstitutions);
	return message;
}
std::string LLNotification::getLabel() const
{
	if(!mTemplatep)
		return std::string();
	std::string label = mTemplatep->mLabel;
	LLStringUtil::format(label, mSubstitutions);
	return label;
}
bool LLNotification::hasLabel() const
{
	return !mTemplatep->mLabel.empty();
}
std::string LLNotification::getURL() const
{
	if (!mTemplatep)
		return std::string();
	std::string url = mTemplatep->mURL;
	LLStringUtil::format(url, mSubstitutions);
	return (mTemplatep ? url : "");
}
void LLNotificationChannelBase::connectChanged(const LLStandardSignal::slot_type& slot)
{
	AILOCK_mItems;
	for (LLNotificationSet::iterator it = mItems.begin(); it != mItems.end(); ++it)
	{
		slot(LLSD().with("sigtype", "load").with("id", (*it)->id()));
	}
	mChanged.connect(slot);
}
void LLNotificationChannelBase::connectPassedFilter(const LLStandardSignal::slot_type& slot)
{
	mPassedFilter.connect(slot);
}
void LLNotificationChannelBase::connectFailedFilter(const LLStandardSignal::slot_type& slot)
{
	mFailedFilter.connect(slot);
}
bool LLNotificationChannelBase::updateItem(const LLSD& payload)
{
	LLNotificationPtr pNotification	 = LLNotifications::instance().find(payload["id"]);
	if (!pNotification)
		return false;
	return updateItem(payload, pNotification);
}
bool LLNotificationChannelBase::updateItem(const LLSD& payload, LLNotificationPtr pNotification)
{
	llassert(AIThreadID::in_main_thread());
	std::string cmd = payload["sigtype"];
	AILOCK_mItems;
	LLNotificationSet::iterator foundItem = mItems.find(pNotification);
	bool wasFound = (foundItem != mItems.end());
	bool passesFilter = mFilter(pNotification);
	bool abortProcessing = false;
	if (passesFilter)
	{
		abortProcessing = mPassedFilter(payload);
	}
	else
	{
		abortProcessing = mFailedFilter(payload);
	}
	if (abortProcessing)
	{
		return true;
	}
	if (cmd == "load")
	{
		assert(!wasFound);
		if (passesFilter)
		{
			mItems.insert(pNotification);
			abortProcessing = mChanged(payload);
			onLoad(pNotification);
		}
	}
	else if (cmd == "change")
	{
		if (passesFilter)
		{
			if (wasFound)
			{
				abortProcessing = mChanged(payload);
				onChange(pNotification);
			}
			else
			{
				mItems.insert(pNotification);
				LLSD newpayload = payload;
				newpayload["sigtype"] = "add";
				abortProcessing = mChanged(newpayload);
				onChange(pNotification);
			}
		}
		else
		{
			if (wasFound)
			{
				mItems.erase(pNotification);
				LLSD newpayload = payload;
				newpayload["sigtype"] = "delete";
				abortProcessing = mChanged(newpayload);
				onChange(pNotification);
			}
		}
	}
	else if (cmd == "add")
	{
		assert(!wasFound);
		if (passesFilter)
		{
			mItems.insert(pNotification);
			abortProcessing = mChanged(payload);
			onAdd(pNotification);
		}
	}
	else if (cmd == "delete")
	{
		if (wasFound)
		{
			onDelete(pNotification);
			abortProcessing = mChanged(payload);
			mItems.erase(pNotification);
		}
	}
	return abortProcessing;
}
LLNotificationChannelPtr LLNotificationChannel::buildChannel(const std::string& name,
															 const std::string& parent,
															 LLNotificationFilter filter,
															 LLNotificationComparator comparator)
{
	new LLNotificationChannel(name, parent, filter, comparator);
	return LLNotifications::instance().getChannel(name);
}
LLNotificationChannel::LLNotificationChannel(const std::string& name,
											 const std::string& parent,
											 LLNotificationFilter filter,
											 LLNotificationComparator comparator) :
LLNotificationChannelBase(filter, comparator),
mName(name),
mParent(parent)
{
	LLNotifications::instance().addChannel(LLNotificationChannelPtr(this));
	if (parent.empty())
	{
		LLNotifications::instance().connectChanged(
			boost::bind(&LLNotificationChannelBase::updateItem, this, _1));
	}
	else
	{
		LLNotificationChannelPtr p = LLNotifications::instance().getChannel(parent);
		p->connectChanged(boost::bind(&LLNotificationChannelBase::updateItem, this, _1));
	}
}
void LLNotificationChannel::setComparator(LLNotificationComparator comparator)
{
	mComparator = comparator;
	LLNotificationSet s2(mComparator);
	AILOCK_mItems;
	s2.insert(mItems.begin(), mItems.end());
	mItems.swap(s2);
	mChanged(LLSD().with("sigtype", "sort"));
}
bool LLNotificationChannel::isEmpty() const
{
	AILOCK_const_mItems;
	return mItems.empty();
}
LLNotificationChannel::Iterator LLNotificationChannel::begin()
{
	AILOCK_const_mItems;
	return mItems.begin();
}
LLNotificationChannel::Iterator LLNotificationChannel::end()
{
	AILOCK_const_mItems;
	return mItems.end();
}
std::string LLNotificationChannel::summarize()
{
	std::string s("Channel '");
	s += mName;
	s += "'\n  ";
	for (LLNotificationChannel::Iterator it = begin(); it != end(); ++it)
	{
		s += (*it)->summarize();
		s += "\n  ";
	}
	return s;
}
LLNotifications::LLNotifications() : LLNotificationChannelBase(LLNotificationFilters::includeEverything,
															   LLNotificationComparators::orderByUUID())
{
}
bool LLNotifications::expirationFilter(LLNotificationPtr pNotification)
{
	return pNotification->isCancelled() || pNotification->isRespondedTo();
}
bool LLNotifications::expirationHandler(const LLSD& payload)
{
	if (payload["sigtype"].asString() != "delete")
	{
		cancel(find(payload["id"]));
		return true;
	}
	return false;
}
bool LLNotifications::uniqueFilter(LLNotificationPtr pNotif)
{
	if (!pNotif->hasUniquenessConstraints())
	{
		return true;
	}
	for (LLNotificationMap::iterator existing_it = mUniqueNotifications.find(pNotif->getName());
		existing_it != mUniqueNotifications.end();
		++existing_it)
	{
		LLNotificationPtr existing_notification = existing_it->second;
		if (pNotif != existing_notification
			&& pNotif->isEquivalentTo(existing_notification))
		{
			return false;
		}
	}
	return true;
}
bool LLNotifications::uniqueHandler(const LLSD& payload)
{
	std::string cmd = payload["sigtype"];
	LLNotificationPtr pNotif = LLNotifications::instance().find(payload["id"].asUUID());
	if (pNotif && pNotif->hasUniquenessConstraints())
	{
		if (cmd == "add")
		{
			mUniqueNotifications.insert(std::make_pair(pNotif->getName(), pNotif));
		}
		else if (cmd == "delete")
		{
			mUniqueNotifications.erase(pNotif->getName());
		}
	}
	return false;
}
bool LLNotifications::failedUniquenessTest(const LLSD& payload)
{
	LLNotificationPtr pNotif = LLNotifications::instance().find(payload["id"].asUUID());
	if (!pNotif || !pNotif->hasUniquenessConstraints())
	{
		return false;
	}
	for (LLNotificationMap::iterator existing_it = mUniqueNotifications.find(pNotif->getName());
		existing_it != mUniqueNotifications.end();
		++existing_it)
	{
		LLNotificationPtr existing_notification = existing_it->second;
		if (pNotif != existing_notification
			&& pNotif->isEquivalentTo(existing_notification))
		{
			existing_notification->updateFrom(pNotif);
			pNotif->cancel();
		}
	}
	return false;
}
void LLNotifications::addChannel(LLNotificationChannelPtr pChan)
{
	mChannels[pChan->getName()] = pChan;
}
LLNotificationChannelPtr LLNotifications::getChannel(const std::string& channelName)
{
	ChannelMap::iterator p = mChannels.find(channelName);
	if(p == mChannels.end())
	{
		LL_ERRS() << "Did not find channel named " << channelName << LL_ENDL;
		return LLNotificationChannelPtr();
	}
	return p->second;
}
void LLNotificationTemplates::initSingleton()
{
	loadTemplates();
}
void LLNotifications::initSingleton()
{
	loadNotifications();
}
void LLNotifications::createDefaultChannels()
{
	LLNotificationChannel::buildChannel("Expiration", "",
		boost::bind(&LLNotifications::expirationFilter, this, _1));
	LLNotificationChannel::buildChannel("Unexpired", "",
		!boost::bind(&LLNotifications::expirationFilter, this, _1));
	LLNotificationChannel::buildChannel("Unique", "Unexpired",
		boost::bind(&LLNotifications::uniqueFilter, this, _1));
	LLNotificationChannel::buildChannel("Ignore", "Unique",
		filterIgnoredNotifications);
	LLNotificationChannel::buildChannel("Visible", "Ignore",
		&LLNotificationFilters::includeEverything);
	LLNotifications::instance().getChannel("Expiration")->
		connectChanged(boost::bind(&LLNotifications::expirationHandler, this, _1));
	LLNotifications::instance().getChannel("Unique")->
		connectChanged(boost::bind(&LLNotifications::uniqueHandler, this, _1));
	LLNotifications::instance().getChannel("Unique")->
		connectFailedFilter(boost::bind(&LLNotifications::failedUniquenessTest, this, _1));
	LLNotifications::instance().getChannel("Ignore")->
		connectFailedFilter(&handleIgnoredNotification);
}
void LLNotifications::onLoginCompleted()
{
	std::string notifications_log_file = gDirUtilp->getExpandedFilename ( LL_PATH_PER_SL_ACCOUNT, "singu_open_notifications_" + gHippoGridManager->getCurrentGrid()->getGridName() + ".xml");
	new LLNotificationHistoryChannel(notifications_log_file );
}
static std::string sStringSkipNextTime("Skip this dialog next time");
static std::string sStringAlwaysChoose("Always choose this option");
bool LLNotificationTemplates::addTemplate(const std::string &name,
								  LLNotificationTemplatePtr theTemplate)
{
	if (mTemplates.count(name))
	{
		LL_WARNS() << "LLNotifications -- attempted to add template '" << name << "' twice." << LL_ENDL;
		return false;
	}
	mTemplates[name] = theTemplate;
	return true;
}
LLNotificationTemplatePtr LLNotificationTemplates::getTemplate(const std::string& name)
{
	if (mTemplates.count(name))
	{
		return mTemplates[name];
	}
	else
	{
		return mTemplates["MissingAlert"];
	}
}
bool LLNotificationTemplates::templateExists(const std::string& name)
{
	return (mTemplates.count(name) != 0);
}
void LLNotificationTemplates::clearTemplates()
{
	mTemplates.clear();
}
void LLNotifications::forceResponse(const LLNotification::Params& params, S32 option)
{
	LLNotificationPtr temp_notify(new LLNotification(params));
	LLSD response = temp_notify->getResponseTemplate();
	LLSD selected_item = temp_notify->getForm()->getElement(option);
	if (selected_item.isUndefined())
	{
		LL_WARNS() << "Invalid option" << option << " for notification " << (std::string)params.name << LL_ENDL;
		return;
	}
	response[selected_item["name"].asString()] = true;
	temp_notify->respond(response);
}
LLNotificationTemplates::TemplateNames LLNotificationTemplates::getTemplateNames() const
{
	TemplateNames names;
	for (TemplateMap::const_iterator it = mTemplates.begin(); it != mTemplates.end(); ++it)
	{
		names.push_back(it->first);
	}
	return names;
}
typedef std::map<std::string, std::string> StringMap;
void replaceSubstitutionStrings(LLXMLNodePtr node, StringMap& replacements)
{
	for (LLXMLAttribList::iterator it=node->mAttributes.begin();
		 it != node->mAttributes.end(); ++it)
	{
		std::string value = it->second->getValue();
		if (value[0] == '$')
		{
			value.erase(0, 1);
			std::string replacement;
			StringMap::const_iterator found = replacements.find(value);
			if (found != replacements.end())
			{
				replacement = found->second;
				it->second->setValue(replacement);
			}
			else
			{
				LL_WARNS() << "replaceSubstitutionStrings FAILURE: could not find replacement \"" << value << "\"." << LL_ENDL;
			}
		}
	}
	for (LLXMLNodePtr child = node->getFirstChild();
		 child.notNull(); child = child->getNextSibling())
	{
		replaceSubstitutionStrings(child, replacements);
	}
}
LLXMLNodePtr LLNotificationTemplates::checkForXMLTemplate(LLXMLNodePtr item)
{
	if (item->hasName("usetemplate"))
	{
		std::string replacementName;
		if (item->getAttributeString("name", replacementName))
		{
			StringMap replacements;
			for (LLXMLAttribList::const_iterator it=item->mAttributes.begin();
				 it != item->mAttributes.end(); ++it)
			{
				replacements[it->second->getName()->mString] = it->second->getValue();
			}
			if (mXmlTemplates.count(replacementName))
			{
				item=LLXMLNode::replaceNode(item, mXmlTemplates[replacementName]);
				replaceSubstitutionStrings(item, replacements);
			}
			else
			{
				LL_WARNS() << "XML template lookup failure on '" << replacementName << "' " << LL_ENDL;
			}
		}
	}
	return item;
}
bool LLNotificationTemplates::loadTemplates()
{
	LL_INFOS() << "Reading notifications template" << LL_ENDL;
	std::vector<std::string> search_paths =
		gDirUtilp->findSkinnedFilenames(LLDir::XUI, "notifications.xml", LLDir::ALL_SKINS);
	std::string base_filename = search_paths.front();
	LLXMLNodePtr root;
	BOOL success  = LLXMLNode::getLayeredXMLNode(root, search_paths);
	if (!success || root.isNull() || !root->hasName( "notifications" ))
	{
		LL_ERRS() << "Problem reading XML from UI Notifications file: " << base_filename << LL_ENDL;
		return false;
	}
	clearTemplates();
	for (LLXMLNodePtr item = root->getFirstChild();
		 item.notNull(); item = item->getNextSibling())
	{
		if (item->hasName("global"))
		{
			std::string global_name;
			if (item->getAttributeString("name", global_name))
			{
				mGlobalStrings[global_name] = item->getTextContents();
			}
			continue;
		}
		if (item->hasName("template"))
		{
			std::string name;
			item->getAttributeString("name", name);
			LLXMLNodePtr ptr = item->getFirstChild();
			mXmlTemplates[name] = ptr;
			continue;
		}
		if (!item->hasName("notification"))
		{
            LL_WARNS() << "Unexpected entity " << item->getName()->mString <<
                       " found in notifications.xml [language=]" << LLUI::getLanguage() << LL_ENDL;
			continue;
		}
	}
	return true;
}
bool LLNotifications::loadNotifications()
{
	LL_INFOS() << "Reading notifications template" << LL_ENDL;
	std::vector<std::string> search_paths =
		gDirUtilp->findSkinnedFilenames(LLDir::XUI, "notifications.xml", LLDir::ALL_SKINS);
	std::string base_filename = search_paths.front();
	LLXMLNodePtr root;
	BOOL success  = LLXMLNode::getLayeredXMLNode(root, search_paths);
	if (!success || root.isNull() || !root->hasName( "notifications" ))
	{
		LL_ERRS() << "Problem reading XML from UI Notifications file: " << base_filename << LL_ENDL;
		return false;
	}
	for (LLXMLNodePtr item = root->getFirstChild();
		 item.notNull(); item = item->getNextSibling())
	{
		item = LLNotificationTemplates::instance().checkForXMLTemplate(item);
		if (!item->hasName("notification"))
		  continue;
		LLNotificationTemplatePtr pTemplate(new LLNotificationTemplate());
		if (!item->getAttributeString("name", pTemplate->mName))
		{
			LL_WARNS() << "Unable to parse notification with no name" << LL_ENDL;
			continue;
		}
		pTemplate->mMessage = item->getTextContents();
		pTemplate->mDefaultFunctor = pTemplate->mName;
		item->getAttributeString("type", pTemplate->mType);
		item->getAttributeString("icon", pTemplate->mIcon);
		item->getAttributeString("label", pTemplate->mLabel);
		item->getAttributeU32("duration", pTemplate->mExpireSeconds);
		item->getAttributeU32("expireOption", pTemplate->mExpireOption);
		std::string priority;
		item->getAttributeString("priority", priority);
		pTemplate->mPriority = NOTIFICATION_PRIORITY_NORMAL;
		if (!priority.empty())
		{
			if (priority == "low")      pTemplate->mPriority = NOTIFICATION_PRIORITY_LOW;
			if (priority == "normal")   pTemplate->mPriority = NOTIFICATION_PRIORITY_NORMAL;
			if (priority == "high")     pTemplate->mPriority = NOTIFICATION_PRIORITY_HIGH;
			if (priority == "critical") pTemplate->mPriority = NOTIFICATION_PRIORITY_CRITICAL;
		}
		item->getAttributeString("functor", pTemplate->mDefaultFunctor);
		BOOL persist = false;
		item->getAttributeBOOL("persist", persist);
		pTemplate->mPersist = persist;
		std::string sound;
		item->getAttributeString("sound", sound);
		if (!sound.empty())
		{
			pTemplate->mSoundEffect = LLUUID(LLUI::sConfigGroup->findString(sound.c_str()));
		}
		for (LLXMLNodePtr child = item->getFirstChild();
			 !child.isNull(); child = child->getNextSibling())
		{
			child = LLNotificationTemplates::instance().checkForXMLTemplate(child);
			if (child->hasName("url"))
			{
				pTemplate->mURL = child->getTextContents();
				child->getAttributeU32("option", pTemplate->mURLOption);
			}
            if (child->hasName("unique"))
            {
                pTemplate->mUnique = true;
                for (LLXMLNodePtr formitem = child->getFirstChild();
                     !formitem.isNull(); formitem = formitem->getNextSibling())
                {
                    if (formitem->hasName("context"))
                    {
                        std::string key;
                        formitem->getAttributeString("key", key);
                        pTemplate->mUniqueContext.push_back(key);
                    }
                    else
                    {
                        LL_WARNS() << "'unique' has unrecognized subelement "
                        << formitem->getName()->mString << LL_ENDL;
                    }
                }
            }
			if (child->hasName("form"))
			{
                pTemplate->mForm = LLNotificationFormPtr(new LLNotificationForm(pTemplate->mName, child));
			}
		}
		LLNotificationTemplates::instance().addTemplate(pTemplate->mName, pTemplate);
	}
	return true;
}
LLNotificationPtr LLNotifications::add(const std::string& name,
										const LLSD& substitutions,
										const LLSD& payload)
{
	return add(LLNotification::Params(name).substitutions(substitutions).payload(payload));
}
LLNotificationPtr LLNotifications::add(const std::string& name,
										const LLSD& substitutions,
										const LLSD& payload,
										const std::string& functor_name)
{
	return add(LLNotification::Params(name).substitutions(substitutions).payload(payload).functor_name(functor_name));
}
LLNotificationPtr LLNotifications::add(const std::string& name,
										const LLSD& substitutions,
										const LLSD& payload,
										LLNotificationFunctorRegistry::ResponseFunctor functor)
{
	return add(LLNotification::Params(name).substitutions(substitutions).payload(payload).functor(functor));
}
LLNotificationPtr LLNotifications::add(const LLNotification::Params& p)
{
	LLNotificationPtr pNotif(new LLNotification(p));
	add(pNotif);
	return pNotif;
}
namespace AIAlert { std::string text(Error const& error, int suppress_mask = 0); }
LLNotificationPtr LLNotifications::add(AIAlert::Error const& error, int type, unsigned int suppress_mask)
{
	LLSD substitutions = LLSD::emptyMap();
	substitutions["[PAYLOAD]"] = AIAlert::text(error, suppress_mask);
	return add(LLNotification::Params((type == AIAlert::modal || error.is_modal()) ? "AIAlertModal" : "AIAlert").substitutions(substitutions));
}
struct UpdateItem
{
  char const* sigtype;
  LLNotificationPtr pNotif;
  UpdateItem(char const* st, LLNotificationPtr const& np) : sigtype(st), pNotif(np) { }
  void doit(void) const;
};
void UpdateItem::doit(void) const
{
  LLNotifications::getInstance()->updateItem(LLSD().with("sigtype", sigtype).with("id", pNotif->id()), pNotif);
  if (!strcmp(sigtype, "delete"))
  {
	pNotif->cancel();
  }
}
class UpdateItemSM : public AIStateMachine
{
  protected:
	typedef AIStateMachine direct_base_type;
	enum update_item_state_type {
	  UpdateItem_idle = direct_base_type::max_state,
	  UpdateItem_doit
	};
  public:
	static state_type const max_state = UpdateItem_doit + 1;
  public:
	UpdateItemSM(void) : AIStateMachine(CWD_ONLY(true)) { }
	static void add(UpdateItem const& ui);
	const char* getName() const { return "UpdateItemSM"; }
  private:
	static UpdateItemSM* sSelf;
	typedef std::deque<UpdateItem> updateQueue_type;
	AIThreadSafeSimpleDC<updateQueue_type> mUpdateQueue;
	typedef AIAccess<updateQueue_type> mUpdateQueue_wat;
	typedef AIAccess<updateQueue_type> mUpdateQueue_rat;
	typedef AIAccessConst<updateQueue_type> mUpdateQueue_crat;
  protected:
    ~UpdateItemSM() { }
  protected:
    void initialize_impl(void) { set_state(UpdateItem_idle); }
    void multiplex_impl(state_type run_state);
    void abort_impl(void) { }
    void finish_impl(void) { }
    char const* state_str_impl(state_type run_state) const;
};
UpdateItemSM* UpdateItemSM::sSelf;
void UpdateItemSM::add(UpdateItem const& ui)
{
  if (!sSelf)
  {
	sSelf = new UpdateItemSM;
	sSelf->run(NULL, 0, false, true, &gMainThreadEngine);
  }
  if (AIThreadID::in_main_thread())
  {
	ui.doit();
	return;
  }
  mUpdateQueue_wat mUpdateQueue_w(sSelf->mUpdateQueue);
  mUpdateQueue_w->push_back(ui);
  sSelf->advance_state(UpdateItem_doit);
}
char const* UpdateItemSM::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(UpdateItem_idle);
    AI_CASE_RETURN(UpdateItem_doit);
  }
  llassert(false);
  return "UNKNOWN STATE";
}
void UpdateItemSM::multiplex_impl(state_type run_state)
{
  switch(run_state)
  {
	case UpdateItem_idle:
	  idle();
	  break;
	case UpdateItem_doit:
	{
	  mUpdateQueue_wat mUpdateQueue_w(sSelf->mUpdateQueue);
	  while (!mUpdateQueue_w->empty())
	  {
		UpdateItem const& ui(mUpdateQueue_w->front());
		ui.doit();
		mUpdateQueue_w->pop_front();
	  }
	  set_state(UpdateItem_idle);
	  break;
	}
  }
}
void LLNotifications::add(const LLNotificationPtr pNotif)
{
	if (pNotif == NULL) return;
	AILOCK_mItems;
	LLNotificationSet::iterator it=mItems.find(pNotif);
	if (it != mItems.end())
	{
		LL_ERRS() << "Notification added a second time to the master notification channel." << LL_ENDL;
	}
	UpdateItemSM::add(UpdateItem("add", pNotif));
}
void LLNotifications::cancel(LLNotificationPtr pNotif)
{
	if (pNotif == NULL || pNotif->isCancelled()) return;
	AILOCK_mItems;
	LLNotificationSet::iterator it=mItems.find(pNotif);
	if (it == mItems.end())
	{
		LL_ERRS() << "Attempted to delete nonexistent notification " << pNotif->getName() << LL_ENDL;
	}
	UpdateItemSM::add(UpdateItem("delete", pNotif));
}
void LLNotifications::update(const LLNotificationPtr pNotif)
{
	AILOCK_mItems;
	LLNotificationSet::iterator it=mItems.find(pNotif);
	if (it != mItems.end())
	{
		UpdateItemSM::add(UpdateItem("change", pNotif));
	}
}
LLNotificationPtr LLNotifications::find(LLUUID const& uuid)
{
	LLNotificationPtr target = LLNotificationPtr(new LLNotification(uuid));
	AILOCK_mItems;
	LLNotificationSet::iterator it=mItems.find(target);
	if (it == mItems.end())
	{
		LL_WARNS() << "Tried to dereference uuid '" << uuid << "' as a notification key but didn't find it." << LL_ENDL;
		return LLNotificationPtr((LLNotification*)NULL);
	}
	else
	{
		return *it;
	}
}
void LLNotifications::forEachNotification(NotificationProcess process)
{
	AILOCK_mItems;
	std::for_each(mItems.begin(), mItems.end(), process);
}
std::string LLNotificationTemplates::getGlobalString(const std::string& key) const
{
	GlobalStringMap::const_iterator it = mGlobalStrings.find(key);
	if (it != mGlobalStrings.end())
	{
		return it->second;
	}
	else
	{
		return key;
	}
}
std::ostream& operator<<(std::ostream& s, const LLNotification& notification)
{
	s << notification.summarize();
	return s;
}
