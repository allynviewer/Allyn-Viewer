/**
* @file llnotifications.h
* @brief Non-UI manager and support for keeping a prioritized list of notifications
* @author Q (with assistance from Richard and Coco)
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
#ifndef LL_LLNOTIFICATIONS_H
#define LL_LLNOTIFICATIONS_H
#include <string>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <iomanip>
#include <sstream>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/type_traits.hpp>
#include <boost/signals2.hpp>
#include <boost/range.hpp>
#include "llsd.h"
#include "llinstancetracker.h"
#include "llavatarname.h"
#include "llevents.h"
#include "llfunctorregistry.h"
#include "llinitparam.h"
#include "llui.h"
#include "llxmlnode.h"
#include "llnotificationptr.h"
#include "llnotificationcontext.h"
#include "aithreadsafe.h"
namespace AIAlert { class Error; }
typedef enum e_notification_priority
{
	NOTIFICATION_PRIORITY_UNSPECIFIED,
	NOTIFICATION_PRIORITY_LOW,
	NOTIFICATION_PRIORITY_NORMAL,
	NOTIFICATION_PRIORITY_HIGH,
	NOTIFICATION_PRIORITY_CRITICAL
} ENotificationPriority;
typedef boost::function<void (const LLSD&, const LLSD&)> LLNotificationResponder;
typedef LLFunctorRegistry<LLNotificationResponder> LLNotificationFunctorRegistry;
typedef LLFunctorRegistration<LLNotificationResponder> LLNotificationFunctorRegistration;
class LLNotificationForm
{
	LOG_CLASS(LLNotificationForm);
public:
	struct FormElementBase : public LLInitParam::Block<FormElementBase>
	{
		Optional<std::string>	name;
		Optional<bool>			enabled;
		FormElementBase();
	};
	struct FormIgnore : public LLInitParam::Block<FormIgnore, FormElementBase>
	{
		Optional<std::string>	text;
		Optional<bool>			save_option;
		Optional<std::string>	control;
		Optional<bool>			invert_control;
		FormIgnore();
	};
	struct FormButton : public LLInitParam::Block<FormButton, FormElementBase>
	{
		Mandatory<S32>			index;
		Mandatory<std::string>	text;
		Optional<std::string>	ignore;
		Optional<bool>			is_default;
		Mandatory<std::string>	type;
		FormButton();
	};
	struct FormInput : public LLInitParam::Block<FormInput, FormElementBase>
	{
		Mandatory<std::string>	type;
		Optional<S32>			width;
		Optional<S32>			max_length_chars;
		Optional<std::string>	text;
		Optional<std::string>	value;
		FormInput();
	};
	struct FormElement : public LLInitParam::ChoiceBlock<FormElement>
	{
		Alternative<FormButton> button;
		Alternative<FormInput>	input;
		FormElement();
	};
	struct FormElements : public LLInitParam::Block<FormElements>
	{
		Multiple<FormElement> elements;
		FormElements();
	};
	struct Params : public LLInitParam::Block<Params>
	{
		Optional<std::string>	name;
		Optional<FormIgnore>	ignore;
		Optional<FormElements>	form_elements;
		Params();
	};
	typedef enum e_ignore_type
	{
		IGNORE_NO,
		IGNORE_WITH_DEFAULT_RESPONSE,
		IGNORE_WITH_LAST_RESPONSE,
		IGNORE_SHOW_AGAIN
	} EIgnoreType;
	LLNotificationForm();
	LLNotificationForm(const LLSD& sd);
	LLNotificationForm(const std::string& name, const LLXMLNodePtr xml_node);
	LLSD asLLSD() const;
	S32 getNumElements() { return mFormData.size(); }
	LLSD getElement(S32 index) { return mFormData.get(index); }
	LLSD getElement(const std::string& element_name);
	bool hasElement(const std::string& element_name);
	void addElement(const std::string& type, const std::string& name, const LLSD& value = LLSD());
	void formatElements(const LLSD& substitutions);
	void append(const LLSD& sub_form);
	std::string getDefaultOption();
	LLPointer<class LLControlVariable> getIgnoreSetting();
	bool getIgnored();
	void setIgnored(bool ignored);
	EIgnoreType getIgnoreType() { return mIgnore; }
	std::string getIgnoreMessage() { return mIgnoreMsg; }
private:
	LLSD	mFormData;
	EIgnoreType mIgnore;
	std::string							mIgnoreMsg;
	LLPointer<class LLControlVariable>	mIgnoreSetting;
	bool								mInvertSetting;
};
typedef boost::shared_ptr<LLNotificationForm> LLNotificationFormPtr;
struct LLNotificationTemplate;
typedef boost::shared_ptr<LLNotificationTemplate> LLNotificationTemplatePtr;
class LLNotification  :
	boost::noncopyable,
	public boost::enable_shared_from_this<LLNotification>
{
LOG_CLASS(LLNotification);
friend class LLNotifications;
friend struct UpdateItem;
public:
	class Params : public LLParamBlock<Params>
	{
		friend class LLNotification;
	public:
		Params(const std::string& _name)
			:	name(_name),
				mTemporaryResponder(false),
				functor_name(_name),
				priority(NOTIFICATION_PRIORITY_UNSPECIFIED),
				timestamp(LLDate::now())
		{
		}
		Params& functor(LLNotificationFunctorRegistry::ResponseFunctor f)
		{
			functor_name = LLUUID::generateNewID().asString();
			LLNotificationFunctorRegistry::instance().registerFunctor(functor_name, f);
			mTemporaryResponder = true;
			return *this;
		}
		LLMandatoryParam<std::string>					name;
		LLOptionalParam<LLSD>							substitutions;
		LLOptionalParam<LLSD>							payload;
		LLOptionalParam<ENotificationPriority>			priority;
		LLOptionalParam<LLSD>							form_elements;
		LLOptionalParam<LLDate>							timestamp;
		LLOptionalParam<LLNotificationContext*>			context;
		LLOptionalParam<std::string>					functor_name;
	private:
		bool					mTemporaryResponder;
	};
private:
	LLUUID mId;
	LLSD mPayload;
	LLSD mSubstitutions;
	LLDate mTimestamp;
	LLDate mExpiresAt;
	bool mCancelled;
	bool mRespondedTo;
	bool mIgnored;
	ENotificationPriority mPriority;
	LLNotificationFormPtr mForm;
	LLNotificationTemplatePtr mTemplatep;
	 std::string mResponseFunctorName;
	bool mTemporaryResponder;
	void init(const std::string& template_name, const LLSD& form_elements);
	LLNotification(const Params& p);
 LLNotification(LLUUID uuid) : mId(uuid), mCancelled(false), mRespondedTo(false), mIgnored(false), mPriority(NOTIFICATION_PRIORITY_UNSPECIFIED), mTemporaryResponder(false) {}
	void cancel();
public:
	LLNotification(const LLSD& sd);
	void setResponseFunctor(std::string const &responseFunctorName);
	typedef enum e_response_template_type
	{
		WITHOUT_DEFAULT_BUTTON,
		WITH_DEFAULT_BUTTON
	} EResponseTemplateType;
	LLSD getResponseTemplate(EResponseTemplateType type = WITHOUT_DEFAULT_BUTTON);
	static S32 getSelectedOption(const LLSD& notification, const LLSD& response);
	static std::string getSelectedOptionName(const LLSD& notification);
	LLSD asLLSD();
	void respond(const LLSD& sd);
	void setIgnored(bool ignore);
	bool isCancelled() const
	{
		return mCancelled;
	}
	bool isRespondedTo() const
	{
		return mRespondedTo;
	}
	bool isIgnored() const
	{
		return mIgnored;
	}
	const std::string& getName() const;
	const std::string& getIcon() const;
	bool isPersistent() const;
	const LLUUID& id() const
	{
		return mId;
	}
	const LLSD& getPayload() const
	{
		return mPayload;
	}
	const LLSD& getSubstitutions() const
	{
		return mSubstitutions;
	}
	const LLDate& getDate() const
	{
		return mTimestamp;
	}
	bool hasLabel() const;
	std::string getType() const;
	std::string getMessage() const;
	std::string getLabel() const;
	std::string getURL() const;
	S32 getURLOption() const;
	const LLNotificationFormPtr getForm();
	const LLDate getExpiration() const
	{
		return mExpiresAt;
	}
	ENotificationPriority getPriority() const
	{
		return mPriority;
	}
	const LLUUID getID() const
	{
		return mId;
	}
	bool operator<(const LLNotification& rhs) const
	{
		return mId < rhs.mId;
	}
	bool operator==(const LLNotification& rhs) const
	{
		return mId == rhs.mId;
	}
	bool operator!=(const LLNotification& rhs) const
	{
		return !operator==(rhs);
	}
	bool isSameObjectAs(const LLNotification* rhs) const
	{
		return this == rhs;
	}
	void update();
	void updateFrom(LLNotificationPtr other);
	bool isEquivalentTo(LLNotificationPtr that) const;
	bool isExpired() const
	{
		if (mExpiresAt.secondsSinceEpoch() == 0)
		{
			return false;
		}
		LLDate rightnow = LLDate::now();
		return rightnow > mExpiresAt;
	}
	std::string summarize() const;
	bool hasUniquenessConstraints() const;
	virtual ~LLNotification() {}
};
std::ostream& operator<<(std::ostream& s, const LLNotification& notification);
namespace LLNotificationFilters
{
	bool includeEverything(LLNotificationPtr p);
	typedef enum e_comparison
	{
		EQUAL,
		LESS,
		GREATER,
		LESS_EQUAL,
		GREATER_EQUAL
	} EComparison;
	template<typename T>
	struct filterBy
	{
		typedef boost::function<T (LLNotificationPtr)>	field_t;
		typedef typename boost::remove_reference<T>::type		value_t;
		filterBy(field_t field, value_t value, EComparison comparison = EQUAL)
			:	mField(field),
				mFilterValue(value),
				mComparison(comparison)
		{
		}
		bool operator()(LLNotificationPtr p)
		{
			switch(mComparison)
			{
			case EQUAL:
				return mField(p) == mFilterValue;
			case LESS:
				return mField(p) < mFilterValue;
			case GREATER:
				return mField(p) > mFilterValue;
			case LESS_EQUAL:
				return mField(p) <= mFilterValue;
			case GREATER_EQUAL:
				return mField(p) >= mFilterValue;
			default:
				return false;
			}
		}
		field_t mField;
		value_t	mFilterValue;
		EComparison mComparison;
	};
};
namespace LLNotificationComparators
{
	typedef enum e_direction { ORDER_DECREASING, ORDER_INCREASING } EDirection;
	template<typename T>
	struct orderBy
	{
		typedef boost::function<T (LLNotificationPtr)> field_t;
			orderBy(field_t field, EDirection direction = ORDER_INCREASING) : mField(field), mDirection(direction) {}
		bool operator()(LLNotificationPtr lhs, LLNotificationPtr rhs)
		{
			if (mDirection == ORDER_DECREASING)
			{
				return mField(lhs) > mField(rhs);
			}
			else
			{
				return mField(lhs) < mField(rhs);
			}
		}
		field_t mField;
		EDirection mDirection;
	};
	struct orderByUUID : public orderBy<const LLUUID&>
	{
		orderByUUID(EDirection direction = ORDER_INCREASING) : orderBy<const LLUUID&>(&LLNotification::id, direction) {}
	};
	struct orderByDate : public orderBy<const LLDate&>
	{
		orderByDate(EDirection direction = ORDER_INCREASING) : orderBy<const LLDate&>(&LLNotification::getDate, direction) {}
	};
};
typedef boost::function<bool (LLNotificationPtr)> LLNotificationFilter;
typedef boost::function<bool (LLNotificationPtr, LLNotificationPtr)> LLNotificationComparator;
typedef std::set<LLNotificationPtr, LLNotificationComparator> LLNotificationSet;
typedef std::multimap<std::string, LLNotificationPtr> LLNotificationMap;
class LLNotificationChannelBase :
	public boost::signals2::trackable
{
	LOG_CLASS(LLNotificationChannelBase);
	friend struct UpdateItem;
public:
	LLNotificationChannelBase(LLNotificationFilter filter, LLNotificationComparator comp) :
		mFilter(filter), mItems_sf(comp)
	{}
	virtual ~LLNotificationChannelBase() {}
	virtual void connectChanged(const LLStandardSignal::slot_type& slot);
	virtual void connectPassedFilter(const LLStandardSignal::slot_type& slot);
	virtual void connectFailedFilter(const LLStandardSignal::slot_type& slot);
	bool updateItem(const LLSD& payload);
	const LLNotificationFilter& getFilter() { return mFilter; }
protected:
	AIThreadSafeSimpleDC<LLNotificationSet> mItems_sf;
	typedef AIAccess<LLNotificationSet> mItems_wat;
	typedef AIAccessConst<LLNotificationSet> mItems_crat;
	LLStandardSignal mChanged;
	LLStandardSignal mPassedFilter;
	LLStandardSignal mFailedFilter;
	virtual void onLoad(LLNotificationPtr p) {}
	virtual void onAdd(LLNotificationPtr p) {}
	virtual void onDelete(LLNotificationPtr p) {}
	virtual void onChange(LLNotificationPtr p) {}
	bool updateItem(const LLSD& payload, LLNotificationPtr pNotification);
	LLNotificationFilter mFilter;
};
class LLNotificationChannel;
typedef boost::shared_ptr<LLNotificationChannel> LLNotificationChannelPtr;
class LLNotificationChannel :
	boost::noncopyable,
	public LLNotificationChannelBase
{
	LOG_CLASS(LLNotificationChannel);
public:
	virtual ~LLNotificationChannel() {}
	typedef LLNotificationSet::iterator Iterator;
	std::string getName() const { return mName; }
	std::string getParentChannelName() { return mParent; }
	bool isEmpty() const;
	Iterator begin();
	Iterator end();
	void setComparator(LLNotificationComparator comparator);
	std::string summarize();
	static LLNotificationChannelPtr buildChannel(const std::string& name, const std::string& parent,
						LLNotificationFilter filter=LLNotificationFilters::includeEverything,
						LLNotificationComparator comparator=LLNotificationComparators::orderByUUID());
protected:
	LLNotificationChannel(const std::string& name, const std::string& parent,
						  LLNotificationFilter filter, LLNotificationComparator comparator);
private:
	std::string mName;
	std::string mParent;
	LLNotificationComparator mComparator;
};
class LLNotificationTemplates :
	public LLSingleton<LLNotificationTemplates>
{
	LOG_CLASS(LLNotificationTemplates);
	friend class LLSingleton<LLNotificationTemplates>;
	typedef char LLNotifications;
public:
	bool loadTemplates();
	LLXMLNodePtr checkForXMLTemplate(LLXMLNodePtr item);
	LLNotificationTemplatePtr getTemplate(const std::string& name);
	typedef std::vector<std::string> TemplateNames;
	TemplateNames getTemplateNames() const;
	typedef std::map<std::string, LLNotificationTemplatePtr> TemplateMap;
	TemplateMap::const_iterator templatesBegin() { return mTemplates.begin(); }
	TemplateMap::const_iterator templatesEnd() { return mTemplates.end(); }
	bool templateExists(const std::string& name);
	void clearTemplates();
	bool addTemplate(const std::string& name, LLNotificationTemplatePtr theTemplate);
	std::string getGlobalString(const std::string& key) const;
private:
	LLNotificationTemplates() { }
	void initSingleton();
	TemplateMap mTemplates;
	typedef std::map<std::string, LLXMLNodePtr> XMLTemplateMap;
	XMLTemplateMap mXmlTemplates;
	typedef std::map<std::string, std::string> GlobalStringMap;
	GlobalStringMap mGlobalStrings;
};
class LLNotifications :
	public LLSingleton<LLNotifications>,
	public LLNotificationChannelBase
{
	LOG_CLASS(LLNotifications);
	friend class LLSingleton<LLNotifications>;
public:
	bool loadNotifications();
	void createDefaultChannels();
	void onLoginCompleted();
	LLNotificationPtr add(const std::string& name,
						const LLSD& substitutions = LLSD(),
						const LLSD& payload = LLSD());
	LLNotificationPtr add(const std::string& name,
						const LLSD& substitutions,
						const LLSD& payload,
						const std::string& functor_name);
	LLNotificationPtr add(const std::string& name,
						const LLSD& substitutions,
						const LLSD& payload,
						LLNotificationFunctorRegistry::ResponseFunctor functor);
	LLNotificationPtr add(AIAlert::Error const& error, int type, unsigned int suppress_mask);
	LLNotificationPtr add(const LLNotification::Params& p);
	void forceResponse(const LLNotification::Params& params, S32 option);
	typedef std::map<std::string, LLNotificationChannelPtr> ChannelMap;
	ChannelMap mChannels;
	void addChannel(LLNotificationChannelPtr pChan);
	LLNotificationChannelPtr getChannel(const std::string& channelName);
	void add(const LLNotificationPtr pNotif);
	void cancel(LLNotificationPtr pNotif);
	void update(const LLNotificationPtr pNotif);
	LLNotificationPtr find(LLUUID const& uuid);
	typedef boost::function<void (LLNotificationPtr)> NotificationProcess;
	void forEachNotification(NotificationProcess process);
private:
	LLNotifications();
	void initSingleton();
	void loadPersistentNotifications();
	bool expirationFilter(LLNotificationPtr pNotification);
	bool expirationHandler(const LLSD& payload);
	bool uniqueFilter(LLNotificationPtr pNotification);
	bool uniqueHandler(const LLSD& payload);
	bool failedUniquenessTest(const LLSD& payload);
	LLNotificationChannelPtr pHistoryChannel;
	LLNotificationChannelPtr pExpirationChannel;
	LLNotificationMap mUniqueNotifications;
};
#endif
