/** 
 * @File llvoavatar.cpp
 * @brief Implementation of LLVOAvatar class which is a derivation of LLViewerObject
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llvoavatar.h"
#include <stdio.h>
#include <ctype.h>
#include "llaudioengine.h"
#include "llperlin.h"
#include "raytrace.h"
#include "llagent.h"
#include "llagentbenefits.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llanimationstates.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llavatarpropertiesprocessor.h"
#include "llcontrolavatar.h"
#include "llphysicsmotion.h"
#include "llviewercontrol.h"
#include "lldrawpoolavatar.h"
#include "lldriverparam.h"
#include "llpolyskeletaldistortion.h"
#include "lleditingmotion.h"
#include "llemote.h"
#include "llfirstuse.h"
#include "llfloaterchat.h"
#include "llfloaterinventory.h"
#include "llheadrotmotion.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llhudnametag.h"
#include "llhudtext.h"
#include "llkeyframefallmotion.h"
#include "llkeyframestandmotion.h"
#include "llkeyframewalkmotion.h"
#include "llmanipscale.h"
#include "llmeshrepository.h"
#include "llmutelist.h"
#include "llnotificationsutil.h"
#include "llquantize.h"
#include "llrand.h"
#include "llregionhandle.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsprite.h"
#include "lltargetingmotion.h"
#include "lltoolmorph.h"
#include "llviewercamera.h"
#include "llviewergenericmessage.h"
#include "llviewercontrol.h"
#include "llviewertexlayer.h"
#include "llviewertexturelist.h"
#include "llviewermedia.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewershadermgr.h"
#include "llviewerstats.h"
#include "llviewerwearable.h"
#include "llvoavatarself.h"
#include "llvovolume.h"
#include "llworld.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llsky.h"
#include "llanimstatelabels.h"
#include "lltrans.h"
#include "llappearancemgr.h"
#include "llgesturemgr.h"
#include "llvoiceclient.h"
#include "llvoicevisualizer.h"
#include "llsdserialize.h"
#include "aosystem.h"
#include "llcachename.h"
#include "llsdutil.h"
#include "llskinningutil.h"
#include "llfloaterexploreanimations.h"
#include "aixmllindengenepool.h"
#include "llavatarname.h"
#include "../lscript/lscript_byteformat.h"
#include "hippogridmanager.h"
#include "llrendersphere.h"
#include "rlvhandler.h"
#include <boost/algorithm/string/replace.hpp>
extern U32 JOINT_COUNT_REQUIRED_FOR_FULLRIG;
const F32 MAX_HOVER_Z = 2.0;
const F32 MIN_HOVER_Z = -2.0;
const F32 MIN_ATTACHMENT_COMPLEXITY = 0.f;
const F32 DEFAULT_MAX_ATTACHMENT_COMPLEXITY = 1.0e6f;
using namespace LLAvatarAppearanceDefines;
const LLUUID ANIM_AGENT_BODY_NOISE_ID = LLUUID("9aa8b0a6-0c6f-9518-c7c3-4f41f2c001ad");
const LLUUID ANIM_AGENT_BREATHE_ROT_ID	= LLUUID("4c5a103e-b830-2f1c-16bc-224aa0ad5bc8");
const LLUUID ANIM_AGENT_EDITING_ID	= LLUUID("2a8eba1d-a7f8-5596-d44a-b4977bf8c8bb");
const LLUUID ANIM_AGENT_EYE_ID	= LLUUID("5c780ea8-1cd1-c463-a128-48c023f6fbea");
const LLUUID ANIM_AGENT_FLY_ADJUST_ID = LLUUID("db95561f-f1b0-9f9a-7224-b12f71af126e");
const LLUUID ANIM_AGENT_HAND_MOTION_ID	= LLUUID("ce986325-0ba7-6e6e-cc24-b17c4b795578");
const LLUUID ANIM_AGENT_HEAD_ROT_ID = LLUUID("e6e8d1dd-e643-fff7-b238-c6b4b056a68d");
const LLUUID ANIM_AGENT_PELVIS_FIX_ID = LLUUID("0c5dd2a2-514d-8893-d44d-05beffad208b");
const LLUUID ANIM_AGENT_TARGET_ID = LLUUID("0e4896cb-fba4-926c-f355-8720189d5b55");
const LLUUID ANIM_AGENT_WALK_ADJUST_ID	= LLUUID("829bc85b-02fc-ec41-be2e-74cc6dd7215d");
const LLUUID ANIM_AGENT_PHYSICS_MOTION_ID = LLUUID("7360e029-3cb8-ebc4-863e-212df440d987");
static LLUUID const* lookup[] = {
	&ANIM_AGENT_BODY_NOISE_ID,
	&ANIM_AGENT_BREATHE_ROT_ID,
	&ANIM_AGENT_PHYSICS_MOTION_ID,
	&ANIM_AGENT_EDITING_ID,
	&ANIM_AGENT_EYE_ID,
	&ANIM_AGENT_FLY_ADJUST_ID,
	&ANIM_AGENT_HAND_MOTION_ID,
	&ANIM_AGENT_HEAD_ROT_ID,
	&ANIM_AGENT_PELVIS_FIX_ID,
	&ANIM_AGENT_TARGET_ID,
	&ANIM_AGENT_WALK_ADJUST_ID
};
LLUUID const& mask2ID(U32 bit)
{
	int const lookupsize = sizeof(lookup) / sizeof(LLUUID const*);
	int i = lookupsize - 1;
	U32 mask = 1 << i;
	for(;;)
	{
	  if (bit == mask)
	  {
		return *lookup[i];
	  }
	  --i;
	  mask >>= 1;
	  llassert_always(i >= 0);
	}
}
#ifdef CWDEBUG
static char const* strlookup[] = {
	"ANIM_AGENT_BODY_NOISE",
	"ANIM_AGENT_BREATHE_ROT",
	"ANIM_AGENT_PHYSICS_MOTION",
	"ANIM_AGENT_EDITING",
	"ANIM_AGENT_EYE",
	"ANIM_AGENT_FLY_ADJUST",
	"ANIM_AGENT_HAND_MOTION",
	"ANIM_AGENT_HEAD_ROT",
	"ANIM_AGENT_PELVIS_FIX",
	"ANIM_AGENT_TARGET",
	"ANIM_AGENT_WALK_ADJUST"
};
char const* mask2str(U32 bit)
{
	int const lookupsize = sizeof(lookup) / sizeof(LLUUID const*);
	int i = lookupsize - 1;
	U32 mask = 1 << i;
	do
	{
	  if (bit == mask)
	  {
		return strlookup[i];
	  }
	  --i;
	  mask >>= 1;
	}
	while(i >= 0);
	return "<unknown>";
}
#endif
void LLVOAvatar::startMotion(U32 bit, F32 time_offset)
{
	if (!isMotionActive(bit))
	{
		mMotionController.disable_syncing();
		startMotion(mask2ID(bit), time_offset);
		mMotionController.enable_syncing();
	}
}
void LLVOAvatar::stopMotion(U32 bit, BOOL stop_immediate)
{
	if (isMotionActive(bit))
	{
		stopMotion(mask2ID(bit), stop_immediate);
	}
}
const S32 MIN_PIXEL_AREA_FOR_COMPOSITE = 1024;
const F32 SHADOW_OFFSET_AMT = 0.03f;
const F32 DELTA_TIME_MIN = 0.01f;
const F32 DELTA_TIME_MAX = 0.2f;
const F32 PELVIS_LAG_FLYING		= 0.22f;
const F32 PELVIS_LAG_WALKING	= 0.4f;
const F32 PELVIS_LAG_MOUSELOOK = 0.15f;
const F32 MOUSELOOK_PELVIS_FOLLOW_FACTOR = 0.5f;
const F32 PELVIS_LAG_WHEN_FOLLOW_CAM_IS_ON = 0.0001f;
const F32 PELVIS_ROT_THRESHOLD_SLOW = 60.0f;
const F32 PELVIS_ROT_THRESHOLD_FAST = 2.0f;
const F32 TORSO_NOISE_AMOUNT = 1.0f;
const F32 TORSO_NOISE_SPEED = 0.2f;
const F32 BREATHE_ROT_MOTION_STRENGTH = 0.05f;
const F32 BREATHE_SCALE_MOTION_STRENGTH = 0.005f;
const F32 MIN_SHADOW_HEIGHT = 0.f;
const F32 MAX_SHADOW_HEIGHT = 0.3f;
const S32 MIN_REQUIRED_PIXEL_AREA_BODY_NOISE = 10000;
const S32 MIN_REQUIRED_PIXEL_AREA_BREATHE = 10000;
const S32 MIN_REQUIRED_PIXEL_AREA_PELVIS_FIX = 40;
const S32 TEX_IMAGE_SIZE_SELF = 512;
const S32 TEX_IMAGE_AREA_SELF = TEX_IMAGE_SIZE_SELF * TEX_IMAGE_SIZE_SELF;
const S32 TEX_IMAGE_SIZE_OTHER = 512 / 4;
const F32 HEAD_MOVEMENT_AVG_TIME = 0.9f;
const S32 MORPH_MASK_REQUESTED_DISCARD = 0;
const F32 MAX_STANDOFF_FROM_ORIGIN = 3;
const F32 MAX_STANDOFF_DISTANCE_CHANGE = 32;
const S32 SWITCH_TO_BAKED_DISCARD = 5;
const F32 FOOT_COLLIDE_FUDGE = 0.04f;
const F32 HOVER_EFFECT_MAX_SPEED = 3.f;
const F32 HOVER_EFFECT_STRENGTH = 0.f;
const F32 UNDERWATER_EFFECT_STRENGTH = 0.1f;
const F32 UNDERWATER_FREQUENCY_DAMP = 0.33f;
const F32 APPEARANCE_MORPH_TIME = 0.65f;
const F32 TIME_BEFORE_MESH_CLEANUP = 5.f;
const S32 AVATAR_RELEASE_THRESHOLD = 10;
const F32 FOOT_GROUND_COLLISION_TOLERANCE = 0.25f;
const F32 AVATAR_LOD_TWEAK_RANGE = 0.7f;
const S32 MAX_BUBBLE_CHAT_LENGTH = DB_CHAT_MSG_STR_LEN;
const S32 MAX_BUBBLE_CHAT_UTTERANCES = 12;
const F32 CHAT_FADE_TIME = 8.0;
const F32 BUBBLE_CHAT_TIME = CHAT_FADE_TIME * 3.f;
const F32 NAMETAG_UPDATE_THRESHOLD = 0.3f;
const F32 NAMETAG_VERTICAL_SCREEN_OFFSET = 25.f;
const F32 NAMETAG_VERT_OFFSET_WEIGHT = 0.17f;
const U32 LLVOAvatar::VISUAL_COMPLEXITY_UNKNOWN = 0;
const F64 HUD_OVERSIZED_TEXTURE_DATA_SIZE = 1024 * 1024;
#define SLOW_ATTACHMENT_LIST 0
enum ERenderName
{
	RENDER_NAME_NEVER,
	RENDER_NAME_FADE,
	RENDER_NAME_ALWAYS
};
std::string get_sequential_numbered_file_name(const std::string& prefix,
											  const std::string& suffix);
struct LLTextureMaskData
{
	LLTextureMaskData( const LLUUID& id ) :
		mAvatarID(id),
		mLastDiscardLevel(S32_MAX)
	{}
	LLUUID				mAvatarID;
	S32					mLastDiscardLevel;
};
struct LLAppearanceMessageContents: public LLRefCount
{
	LLAppearanceMessageContents():
		mAppearanceVersion(-1),
		mParamAppearanceVersion(-1),
		mCOFVersion(LLViewerInventoryCategory::VERSION_UNKNOWN)
	{
	}
	LLTEContents mTEContents;
	S32 mAppearanceVersion;
	S32 mParamAppearanceVersion;
	S32 mCOFVersion;
	std::vector<F32> mParamWeights;
	std::vector<LLVisualParam*> mParams;
	LLVector3 mHoverOffset;
	bool mHoverOffsetWasSet;
};
class LLBodyNoiseMotion :
	public AIMaskedMotion
{
public:
	LLBodyNoiseMotion(LLUUID const& id, LLMotionController* controller)
		: AIMaskedMotion(id, controller, ANIM_AGENT_BODY_NOISE)
	{
		mName = "body_noise";
		mTorsoState = new LLJointState;
	}
	virtual ~LLBodyNoiseMotion() { }
public:
	static LLMotion* create(LLUUID const& id, LLMotionController* controller) { return new LLBodyNoiseMotion(id, controller); }
public:
	virtual BOOL getLoop() { return TRUE; }
	virtual F32 getDuration() { return 0.0; }
	virtual F32 getEaseInDuration() { return 0.0; }
	virtual F32 getEaseOutDuration() { return 0.0; }
	virtual LLJoint::JointPriority getPriority() { return LLJoint::HIGH_PRIORITY; }
	virtual LLMotionBlendType getBlendType() { return ADDITIVE_BLEND; }
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_BODY_NOISE; }
	virtual LLMotionInitStatus onInitialize(LLCharacter *character)
	{
		if( !mTorsoState->setJoint( character->getJoint("mTorso") ))
		{
			return STATUS_FAILURE;
		}
		mTorsoState->setUsage(LLJointState::ROT);
		addJointState( mTorsoState );
		return STATUS_SUCCESS;
	}
	virtual BOOL onUpdate(F32 time, U8* joint_mask)
	{
		F32 noiseX = LLPerlinNoise::noise(LLVector2(time*TORSO_NOISE_SPEED, 0));
		F32 noiseY = LLPerlinNoise::noise(LLVector2(0, time*TORSO_NOISE_SPEED));
		F32 rx = TORSO_NOISE_AMOUNT * DEG_TO_RAD * noiseX / 0.42f;
		F32 ry = TORSO_NOISE_AMOUNT * DEG_TO_RAD * noiseY / 0.42f;
		LLQuaternion tQn;
		tQn.setQuat( rx, ry, 0.0f );
		mTorsoState->setRotation( tQn );
		return TRUE;
	}
private:
	LLPointer<LLJointState> mTorsoState;
};
class LLBreatheMotionRot :
	public AIMaskedMotion
{
public:
	LLBreatheMotionRot(LLUUID const& id, LLMotionController* controller) :
		AIMaskedMotion(id, controller, ANIM_AGENT_BREATHE_ROT),
		mBreatheRate(1.f),
		mCharacter(NULL)
	{
		mName = "breathe_rot";
		mChestState = new LLJointState;
	}
	virtual ~LLBreatheMotionRot() {}
public:
	static LLMotion* create(LLUUID const& id, LLMotionController* controller) { return new LLBreatheMotionRot(id, controller); }
public:
	virtual BOOL getLoop() { return TRUE; }
	virtual F32 getDuration() { return 0.0; }
	virtual F32 getEaseInDuration() { return 0.0; }
	virtual F32 getEaseOutDuration() { return 0.0; }
	virtual LLJoint::JointPriority getPriority() { return LLJoint::MEDIUM_PRIORITY; }
	virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_BREATHE; }
	virtual LLMotionInitStatus onInitialize(LLCharacter *character)
	{
		mCharacter = character;
		bool success = true;
		if ( !mChestState->setJoint( character->getJoint( "mChest" ) ) ) { success = false; }
		if ( success )
		{
			mChestState->setUsage(LLJointState::ROT);
			addJointState( mChestState );
		}
		if ( success )
		{
			return STATUS_SUCCESS;
		}
		else
		{
			return STATUS_FAILURE;
		}
	}
	virtual BOOL onUpdate(F32 time, U8* joint_mask)
	{
		mBreatheRate = 1.f;
		F32 breathe_amt = (sinf(mBreatheRate * time) * BREATHE_ROT_MOTION_STRENGTH);
		mChestState->setRotation(LLQuaternion(breathe_amt, LLVector3(0.f, 1.f, 0.f)));
		return TRUE;
	}
private:
	LLPointer<LLJointState> mChestState;
	F32					mBreatheRate;
	LLCharacter*		mCharacter;
};
class LLPelvisFixMotion :
	public AIMaskedMotion
{
public:
	LLPelvisFixMotion(LLUUID const& id, LLMotionController* controller)
		: AIMaskedMotion(id, controller, ANIM_AGENT_PELVIS_FIX), mCharacter(NULL)
	{
		mName = "pelvis_fix";
		mPelvisState = new LLJointState;
	}
	virtual ~LLPelvisFixMotion() { }
public:
	static LLMotion* create(LLUUID const& id, LLMotionController* controller) { return new LLPelvisFixMotion(id, controller); }
public:
	virtual BOOL getLoop() { return TRUE; }
	virtual F32 getDuration() { return 0.0; }
	virtual F32 getEaseInDuration() { return 0.5f; }
	virtual F32 getEaseOutDuration() { return 0.5f; }
	virtual LLJoint::JointPriority getPriority() { return LLJoint::LOW_PRIORITY; }
	virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_PELVIS_FIX; }
	virtual LLMotionInitStatus onInitialize(LLCharacter *character)
	{
		mCharacter = character;
		if (!mPelvisState->setJoint( character->getJoint("mPelvis")))
		{
			return STATUS_FAILURE;
		}
		mPelvisState->setUsage(LLJointState::POS);
		addJointState( mPelvisState );
		return STATUS_SUCCESS;
	}
	virtual BOOL onUpdate(F32 time, U8* joint_mask)
	{
		mPelvisState->setPosition(LLVector3::zero);
		return TRUE;
	}
private:
	LLPointer<LLJointState> mPelvisState;
	LLCharacter*		mCharacter;
};
SHClientTagMgr::SHClientTagMgr()
{
	gSavedSettings.getControl("AscentShowFriendsTag")->getSignal()->connect(boost::bind(&LLVOAvatar::invalidateNameTags));
	gSavedSettings.getControl("AscentUseStatusColors")->getSignal()->connect(boost::bind(&LLVOAvatar::invalidateNameTags));
	gSavedSettings.getControl("AscentShowSelfTagColor")->getSignal()->connect(boost::bind(&LLVOAvatar::invalidateNameTags));
	gSavedSettings.getControl("AscentShowOthersTagColor")->getSignal()->connect(boost::bind(&LLVOAvatar::invalidateNameTags));
	gSavedSettings.getControl("AscentLindenColor")->getSignal()->connect(boost::bind(&LLVOAvatar::invalidateNameTags));
	gSavedSettings.getControl("AscentEstateOwnerColor")->getSignal()->connect(boost::bind(&LLVOAvatar::invalidateNameTags));
	gSavedSettings.getControl("AscentFriendColor")->getSignal()->connect(boost::bind(&LLVOAvatar::invalidateNameTags));
	gSavedSettings.getControl("AscentMutedColor")->getSignal()->connect(boost::bind(&LLVOAvatar::invalidateNameTags));
	gSavedSettings.getControl("SLBDisplayClientTagOnNewLine")->getSignal()->connect(boost::bind(&LLVOAvatar::invalidateNameTags));
	gSavedSettings.getControl("AscentUseCustomTag")->getSignal()->connect(boost::bind(&SHClientTagMgr::updateAgentAvatarTag, this));
	gSavedSettings.getControl("AscentCustomTagColor")->getSignal()->connect(boost::bind(&SHClientTagMgr::updateAgentAvatarTag, this));
	gSavedSettings.getControl("AscentCustomTagLabel")->getSignal()->connect(boost::bind(&SHClientTagMgr::updateAgentAvatarTag, this));
	gSavedSettings.getControl("AscentShowSelfTag")->getSignal()->connect(boost::bind(&LLVOAvatar::invalidateNameTags));
	if(!getIsEnabled())
		return;
	if (gSavedSettings.getBOOL("AscentUpdateTagsOnLoad"))
		fetchDefinitions();
	parseDefinitions();
	gSavedSettings.getControl("AscentShowOthersTag")->getSignal()->connect(boost::bind(&LLVOAvatar::invalidateNameTags));
	gSavedSettings.getControl("AscentReportClientUUID")->getSignal()->connect(boost::bind(&SHClientTagMgr::updateAgentAvatarTag, this));
	gSavedSettings.getControl("AscentBroadcastTag")->getSignal()->connect(boost::bind(&LLAgent::sendAgentSetAppearance, &gAgent));
	gSavedSettings.getControl("AscentReportClientUUID")->getSignal()->connect(boost::bind(&LLAgent::sendAgentSetAppearance, &gAgent));
}
bool SHClientTagMgr::getIsEnabled() const
{
	return !gHippoGridManager->getConnectedGrid()->isSecondLife();
}
bool SHClientTagMgr::fetchDefinitions() const
{
	std::string client_list_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "client_tags_sg1.xml");
	std::string client_list_url = gSavedSettings.getString("ClientDefinitionsURL");
	LLSD response = LLHTTPClient::blockingGet(client_list_url);
	if(response.has("body"))
	{
		const LLSD &client_list = response["body"];
		if(client_list.has("isComplete"))
		{
			llofstream export_file;
			export_file.open(client_list_filename);
			LLSDSerialize::toPrettyXML(client_list, export_file);
			export_file.close();
			return true;
		}
	}
	return false;
}
bool SHClientTagMgr::parseDefinitions()
{
	std::string client_list_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "client_tags_sg1.xml");
	if(!LLFile::isfile(client_list_filename))
	{
		client_list_filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "client_tags_sg1.xml");
	}
	if(LLFile::isfile(client_list_filename))
	{
		LLSD client_list;
		llifstream importer(client_list_filename);
		if(importer.is_open())
		{
			LLSDSerialize::fromXMLDocument(client_list, importer);
			importer.close();
			if(client_list.has("isComplete"))
			{
				mClientDefinitions.clear();
				for (LLSD::map_const_iterator it = client_list.beginMap(); it != client_list.endMap(); ++it)
				{
					LL_INFOS() << it->first << LL_ENDL;
					LLUUID id;
					if(id.set(it->first,false))
					{
						LLSD info = it->second;
						info["id"] = id;
						F32 mult = info["multiple"].asReal();
						if(mult != 0.f)
						{
							static const LLCachedControl<LLColor4>	avatar_name_color(gColors,"AvatarNameColor",LLColor4(LLColor4U(251, 175, 93, 255)) );
							LLColor4 color(info["color"]);
							color += avatar_name_color;
							color *= 1.f/(mult+1.f);
							info.insert("color", color.getValue());
						}
						LL_INFOS() << info["name"] << " : " << info["id"] << LL_ENDL;
						mClientDefinitions[id]=info;
						LL_INFOS() << mClientDefinitions.size() << LL_ENDL;
					}
				}
				if(getIsEnabled())
				{
					for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin(); iter != LLCharacter::sInstances.end(); ++iter)
					{
						LLVOAvatar* pAvatar = dynamic_cast<LLVOAvatar*>(*iter);
						if(pAvatar)
							updateAvatarTag(pAvatar);
					}
				}
				return true;
			}
		}
	}
	return false;
}
void SHClientTagMgr::updateAgentAvatarTag()
{
	if(isAgentAvatarValid())
		updateAvatarTag(gAgentAvatarp);
}
const LLSD SHClientTagMgr::generateClientTag(const LLVOAvatar* pAvatar) const
{
	static const LLCachedControl<LLColor4>		avatar_name_color(gColors,"AvatarNameColor",LLColor4(LLColor4U(251, 175, 93, 255)) );
	LLUUID id;
	if (pAvatar->isSelf())
	{
		static const LLCachedControl<bool>			ascent_use_custom_tag("AscentUseCustomTag", false);
		static const LLCachedControl<LLColor4>		ascent_custom_tag_color("AscentCustomTagColor", LLColor4(.5f,1.f,.25f,1.f));
		static const LLCachedControl<std::string>	ascent_custom_tag_label("AscentCustomTagLabel","custom");
		static const LLCachedControl<std::string>	ascent_report_client_uuid("AscentReportClientUUID","f25263b7-6167-4f34-a4ef-af65213b2e39");
		if (ascent_use_custom_tag)
		{
			LLSD info;
			info.insert("name", ascent_custom_tag_label.get());
			info.insert("color", ascent_custom_tag_color.get().getValue());
			return info;
		}
		else if(!getIsEnabled())
		{
			return LLSD();
		}
		else
		{
			id.set(ascent_report_client_uuid,false);
		}
	}
	else
	{
		if(!getIsEnabled())
			return LLSD();
		LLTextureEntry* pTextureEntry = pAvatar->getTE(TEX_HEAD_BODYPAINT);
		if (!pTextureEntry)
			return LLSD();
			id = pTextureEntry->getID();
		if(pAvatar->isFullyLoaded() && pTextureEntry->getGlow() > 0.0)
		{
			U32 tag_len = strnlen((const char*)&id.mData[0], UUID_BYTES);
			std::string client((const char*)&id.mData[0], tag_len);
			LLStringFn::replace_ascii_controlchars(client, LL_UNKNOWN_CHAR);
			LLSD info;
			info.insert("name", client);
			info.insert("color", pTextureEntry->getColor().getValue());
			return info;
		}
		if(pAvatar->getTEImage(TEX_HEAD_BODYPAINT)->getID() == IMG_DEFAULT_AVATAR)
		{
			for(S32 te = TEX_UPPER_SHIRT; te <= TEX_SKIRT; ++te)
			{
				LLWearableType::EType wearable = LLAvatarAppearanceDictionary::getTEWearableType((LLAvatarAppearanceDefines::ETextureIndex)te);
				if(	wearable != LLWearableType::WT_INVALID &&
					wearable != LLWearableType::WT_ALPHA &&
					wearable != LLWearableType::WT_TATTOO &&
					pAvatar->getTEImage(te)->getID() != IMG_DEFAULT_AVATAR)
				{
					static LLUUID grey_id("4934f1bf-3b1f-cf4f-dbdf-a72550d05bc6");
					LLSD info;
					if(	pAvatar->getTEImage(TEX_EYES_IRIS)->getID()			== grey_id &&
						pAvatar->getTEImage(TEX_UPPER_BODYPAINT)->getID()	== grey_id &&
						pAvatar->getTEImage(TEX_LOWER_BODYPAINT)->getID()	== grey_id )
					{
						info.insert("name", "?");
						info.insert("color", avatar_name_color.get().getValue());
					}
					return info;
				}
			}
			LLSD info;
			info.insert("name", "Viewer 2.0");
			info.insert("color", LLColor4::white.getValue());
			return info;
		}
		else if(pAvatar->getTEImage(TEX_HEAD_BODYPAINT)->isMissingAsset())
		{
			LLSD info;
			info.insert("name", "Unknown");
			info.insert("color", LLColor4(0.5f, 0.0f, 0.0f).getValue());
			return info;
		}
	}
	if(!mClientDefinitions.empty())
	{
		std::map<LLUUID, LLSD>::const_iterator it = mClientDefinitions.find(id);
		if(it != mClientDefinitions.end())
		{
			return it->second;
		}
	}
	return LLSD();
}
void SHClientTagMgr::updateAvatarTag(LLVOAvatar* pAvatar)
{
	LLUUID id = pAvatar->getID();
	std::map<LLUUID, LLSD>::iterator it = mAvatarTags.find(id);
	LLSD new_tag = generateClientTag(pAvatar);
	LLSD old_tag = (it != mAvatarTags.end()) ? it->second : LLSD();
	bool dirty = old_tag.size() != new_tag.size() || !llsd_equals(new_tag,old_tag);
	if(dirty)
	{
		if(new_tag.isUndefined())
			mAvatarTags.erase(id);
		else
			mAvatarTags[id]=new_tag;
		pAvatar->clearNameTag();
	}
}
const std::string SHClientTagMgr::getClientName(const LLVOAvatar* pAvatar, bool is_friend) const
{
	static LLCachedControl<bool> ascent_show_friends_tag("AscentShowFriendsTag", false);
	static LLCachedControl<bool> ascent_show_self_tag("AscentShowSelfTag", false);
	static LLCachedControl<bool> ascent_show_others_tag("AscentShowOthersTag", false);
	if(is_friend && ascent_show_friends_tag)
		return "Friend";
	else
	{
		if((!pAvatar->isSelf() && ascent_show_others_tag) ||
			(pAvatar->isSelf() && ascent_show_self_tag))
		{
			LLSD tag;
			std::map<LLUUID, LLSD>::const_iterator it = mAvatarTags.find(pAvatar->getID());
			if(it != mAvatarTags.end())
			{
				tag = it->second.get("name");
			}
			return tag.asString();
		}
		else
			return std::string();
	}
}
const LLUUID SHClientTagMgr::getClientID(const LLVOAvatar* pAvatar) const
{
	LLSD tag;
	std::map<LLUUID, LLSD>::const_iterator it = mAvatarTags.find(pAvatar->getID());
	if(it != mAvatarTags.end())
	{
		tag = it->second.get("id");
	}
	return tag.asUUID();
}
bool getColorFor(const LLUUID& id, LLViewerRegion* parent_estate, LLColor4& color, bool name_restricted = false)
{
	static const LLCachedControl<LLColor4> ascent_linden_color("AscentLindenColor");
	static const LLCachedControl<LLColor4> ascent_estate_owner_color("AscentEstateOwnerColor" );
	static const LLCachedControl<LLColor4> ascent_friend_color("AscentFriendColor");
	static const LLCachedControl<LLColor4> ascent_muted_color("AscentMutedColor");
	if (LLMuteList::getInstance()->isLinden(id))
		color = ascent_linden_color;
	else if (parent_estate && parent_estate->isAlive() && id.notNull() && id == parent_estate->getOwner())
		color = ascent_estate_owner_color;
	else if (!name_restricted && LLAvatarTracker::instance().isBuddy(id))
		color = ascent_friend_color;
	else if (LLMuteList::getInstance()->isMuted(id))
		color = ascent_muted_color;
	else
		return false;
	return true;
}
bool mm_getMarkerColor(const LLUUID&, LLColor4&);
bool getCustomColor(const LLUUID& id, LLColor4& color, LLViewerRegion* parent_estate)
{
	return mm_getMarkerColor(id, color) || getColorFor(id, parent_estate, color);
}
bool getCustomColorRLV(const LLUUID& id, LLColor4& color, LLViewerRegion* parent_estate, bool name_restricted)
{
	return mm_getMarkerColor(id, color) || getColorFor(id, parent_estate, color, name_restricted);
}
bool SHClientTagMgr::getClientColor(const LLVOAvatar* pAvatar, bool check_status, LLColor4& color) const
{
	const LLUUID& id(pAvatar->getID());
	if (mm_getMarkerColor(id, color)) return true;
	static const LLCachedControl<bool> ascent_use_status_colors("AscentUseStatusColors",true);
	static const LLCachedControl<bool> ascent_show_self_tag_color("AscentShowSelfTagColor");
	static const LLCachedControl<bool> ascent_show_others_tag_color("AscentShowOthersTagColor");
	if (check_status && ascent_use_status_colors && !pAvatar->isSelf())
	{
		if (getColorFor(id, pAvatar->getRegion(), color, false))
			return true;
	}
	std::map<LLUUID, LLSD>::const_iterator it;
	LLSD tag;
	if(
		(pAvatar->isSelf() ? ascent_show_self_tag_color : ascent_show_others_tag_color) &&
		 (it = mAvatarTags.find(id))!=mAvatarTags.end() &&
		 (tag = it->second.get("color")).isDefined())
	{
		color.setValue(tag);
		return true;
	}
	return false;
}
void SHClientTagMgr::clearAvatarTag(const LLVOAvatar* pAvatar)
{
	mAvatarTags.erase(pAvatar->getID());
}
LLAvatarAppearanceDictionary *LLVOAvatar::sAvatarDictionary = NULL;
S32 LLVOAvatar::sFreezeCounter = 0;
U32 LLVOAvatar::sMaxVisible = 50;
F32 LLVOAvatar::sRenderDistance = 256.f;
S32	LLVOAvatar::sNumVisibleAvatars = 0;
S32	LLVOAvatar::sNumLODChangesThisFrame = 0;
const LLUUID LLVOAvatar::sStepSoundOnLand("e8af4a28-aa83-4310-a7c4-c047e15ea0df");
const LLUUID LLVOAvatar::sStepSounds[LL_MCODE_END] =
{
	SND_STONE_RUBBER,
	SND_METAL_RUBBER,
	SND_GLASS_RUBBER,
	SND_WOOD_RUBBER,
	SND_FLESH_RUBBER,
	SND_RUBBER_PLASTIC,
	SND_RUBBER_RUBBER
};
S32 LLVOAvatar::sRenderName = RENDER_NAME_ALWAYS;
bool LLVOAvatar::sRenderGroupTitles = true;
S32 LLVOAvatar::sNumVisibleChatBubbles = 0;
BOOL LLVOAvatar::sDebugInvisible = false;
bool LLVOAvatar::sShowAttachmentPoints = false;
BOOL LLVOAvatar::sShowAnimationDebug = false;
bool LLVOAvatar::sShowFootPlane = false;
bool LLVOAvatar::sVisibleInFirstPerson = false;
F32 LLVOAvatar::sLODFactor = 1.f;
F32 LLVOAvatar::sPhysicsLODFactor = 1.f;
bool LLVOAvatar::sUseImpostors = false;
BOOL LLVOAvatar::sJointDebug = false;
F32 LLVOAvatar::sUnbakedTime = 0.f;
F32 LLVOAvatar::sUnbakedUpdateTime = 0.f;
F32 LLVOAvatar::sGreyTime = 0.f;
F32 LLVOAvatar::sGreyUpdateTime = 0.f;
BOOL LLVOAvatar::sDebugAvatarRotation = false;
static F32 calc_bouncy_animation(F32 x);
LLVOAvatar::LLVOAvatar(const LLUUID& id,
					   const LLPCode pcode,
					   LLViewerRegion* regionp) :
	LLAvatarAppearance(&gAgentWearables),
	LLViewerObject(id, pcode, regionp),
	mSpecialRenderMode(0),
	mAttachmentSurfaceArea(0.f),
	mAttachmentVisibleTriangleCount(0),
	mAttachmentEstTriangleCount(0.f),
	mReportedVisualComplexity(VISUAL_COMPLEXITY_UNKNOWN),
	mTurning(FALSE),
	mFreezeTimeLangolier(false),
	mFreezeTimeDead(false),
	mLastSkeletonSerialNum( 0 ),
	mIsSitting(FALSE),
	mTimeVisible(),
	mTyping(FALSE),
	mMeshValid(FALSE),
	mVisible(FALSE),
	mLastImpostorUpdateFrameTime(0.f),
	mWindFreq(0.f),
	mRipplePhase( 0.f ),
	mBelowWater(FALSE),
	mLastAppearanceBlendTime(0.f),
	mAppearanceAnimating(FALSE),
	mNameString(),
	mTitle(),
	mNameAway(false),
	mNameBusy(false),
	mNameMute(false),
	mNameAppearance(false),
	mNameFriend(false),
	mNameAlpha(0.f),
	mRenderGroupTitles(sRenderGroupTitles),
	mNameCloud(false),
	mFirstTEMessageReceived( FALSE ),
	mFirstAppearanceMessageReceived( FALSE ),
	mCulled( FALSE ),
	mVisibilityRank(0),
	mNeedsSkin(FALSE),
	mLastSkinTime(0.f),
	mUpdatePeriod(1),
	mVisualComplexityStale(true),
	mFirstFullyVisible(TRUE),
	mFullyLoaded(FALSE),
	mPreviousFullyLoaded(FALSE),
	mFullyLoadedInitialized(FALSE),
	mVisualComplexity(0),
	mSupportsAlphaLayers(FALSE),
	mLoadedCallbacksPaused(FALSE),
	mLastRezzedStatus(-1),
	mIsEditingAppearance(FALSE),
	mUseLocalAppearance(FALSE),
	mUseServerBakes(FALSE),
	mLastUpdateRequestCOFVersion(-1),
	mLastUpdateReceivedCOFVersion(-1),
	mIsControlAvatar(false),
	mIsUIAvatar(false),
	mEnableDefaultMotions(true),
	mHasPhysicsParameters( false ),
	mIdleMinute(0),
	mCCSChatTextOverride(false)
{
	mAttachedObjectsVector.reserve(38);
	static LLCachedControl<bool> const freeze_time("FreezeTime", false);
	mFreezeTimeLangolier = freeze_time;
	setHoverOffset(LLVector3(0.0, 0.0, 0.0));
	const bool needsSendToSim = false;
	mVoiceVisualizer = ( LLVoiceVisualizer *)LLHUDManager::getInstance()->createViewerEffect( LLHUDObject::LL_HUD_EFFECT_VOICE_VISUALIZER, needsSendToSim );
	LL_DEBUGS("Avatar","Message") << "LLVOAvatar Constructor (0x" << this << ") id:" << mID << LL_ENDL;
	mPelvisp = NULL;
	mDirtyMesh = 2;
	mMeshTexturesDirty = FALSE;
	mHeadp = NULL;
	mSpeed = 0.f;
	setAnimationData("Speed", &mSpeed);
	mNeedsImpostorUpdate = TRUE;
	mNeedsAnimUpdate = TRUE;
	mNeedsExtentUpdate = true;
	mImpostorDistance = 0;
	mImpostorPixelArea = 0;
	setNumTEs(TEX_NUM_INDICES);
	mbCanSelect = TRUE;
	mSignaledAnimations.clear();
	mPlayingAnimations.clear();
	mWasOnGroundLeft = FALSE;
	mWasOnGroundRight = FALSE;
	mTimeLast = 0.0f;
	mSpeedAccum = 0.0f;
	mRippleTimeLast = 0.f;
	mInAir = FALSE;
	mStepOnLand = TRUE;
	mStepMaterial = 0;
	mLipSyncActive = false;
	mOohMorph      = NULL;
	mAahMorph      = NULL;
	mCurrentGesticulationLevel = 0;
	mRuthTimer.reset();
	mRuthDebugTimer.reset();
	mDebugExistenceTimer.reset();
	mLastAppearanceMessageTimer.reset();
}
std::string LLVOAvatar::avString() const
{
	if (isControlAvatar())
	{
		return getFullname();
	}
	else
	{
		std::string viz_string = LLVOAvatar::rezStatusToString(getRezzedStatus());
		return " Avatar '" + getFullname() + "' " + viz_string + " ";
	}
}
void LLVOAvatar::debugAvatarRezTime(std::string notification_name, std::string comment)
{
	LL_INFOS("Avatar") << "REZTIME: [ " << (U32)mDebugExistenceTimer.getElapsedTimeF32()
					   << "sec ]"
					   << avString()
					   << "RuthTimer " << (U32)mRuthDebugTimer.getElapsedTimeF32()
					   << " Notification " << notification_name
					   << " : " << comment
					   << LL_ENDL;
	if (gSavedSettings.getBOOL("DebugAvatarRezTime"))
	{
		LLSD args;
		args["EXISTENCE"] = llformat("%d",(U32)mDebugExistenceTimer.getElapsedTimeF32());
		args["TIME"] = llformat("%d",(U32)mRuthDebugTimer.getElapsedTimeF32());
		args["NAME"] = getFullname();
		LLNotificationsUtil::add(notification_name,args);
	}
}
LLVOAvatar::~LLVOAvatar()
{
	if (!LLApp::isQuitting())
	{
		if (!mFullyLoaded)
		{
			debugAvatarRezTime("AvatarRezLeftCloudNotification","left after ruth seconds as cloud");
		}
		else
		{
			debugAvatarRezTime("AvatarRezLeftNotification","left sometime after declouding");
		}
	}
	LL_DEBUGS("Avatar") << "LLVOAvatar Destructor (0x" << this << ") id:" << mID << LL_ENDL;
	std::for_each(mAttachmentPoints.begin(), mAttachmentPoints.end(), DeletePairedPointer());
	mDead = TRUE;
	mAnimationSources.clear();
	LLLoadedCallbackEntry::cleanUpCallbackList(&mCallbackTextureList) ;
	getPhases().clearPhases();
	SHClientTagMgr::instance().clearAvatarTag(this);
	LL_DEBUGS() << "LLVOAvatar Destructor end" << LL_ENDL;
}
void LLVOAvatar::markDead()
{
	static const LLCachedControl<bool> freeze_time("FreezeTime", false);
	if (freeze_time && !mFreezeTimeLangolier)
	{
		mFreezeTimeDead = true;
		return;
	}
	if (mNameText)
	{
		mNameText->markDead();
		mNameText = NULL;
		sNumVisibleChatBubbles--;
	}
	mVoiceVisualizer->markDead();
	LLLoadedCallbackEntry::cleanUpCallbackList(&mCallbackTextureList) ;
	if(!isDead())
		logPendingPhases();
	LLViewerObject::markDead();
}
BOOL LLVOAvatar::isFullyBaked()
{
	if (mIsDummy) return TRUE;
	if (getNumTEs() == 0) return FALSE;
	for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
	{
		if (!isTextureDefined(mBakedTextureDatas[i].mTextureIndex)
			&& ( (i != BAKED_SKIRT) || isWearingWearableType(LLWearableType::WT_SKIRT) )
			&& (i != BAKED_LEFT_ARM) && (i != BAKED_LEFT_LEG) && (i != BAKED_AUX1) && (i != BAKED_AUX2) && (i != BAKED_AUX3) )
		{
			return FALSE;
		}
	}
	return TRUE;
}
BOOL LLVOAvatar::isFullyTextured() const
{
	for (U32 i = 0; i < mMeshLOD.size(); i++)
	{
		LLAvatarJoint* joint = mMeshLOD[i];
		if (i==MESH_ID_SKIRT && !isWearingWearableType(LLWearableType::WT_SKIRT))
		{
			continue;
		}
		if (!joint)
		{
			continue;
		}
		avatar_joint_mesh_list_t::iterator meshIter = joint->mMeshParts.begin();
		if (meshIter != joint->mMeshParts.end())
		{
			LLAvatarJointMesh *mesh = (*meshIter);
			if (!mesh)
			{
				continue;
			}
			if (mesh->hasGLTexture())
			{
				continue;
			}
			if (mesh->hasComposite())
			{
				continue;
			}
			return FALSE;
		}
	}
	return TRUE;
}
BOOL LLVOAvatar::hasGray() const
{
	return !getIsCloud() && !isFullyTextured();
}
S32 LLVOAvatar::getRezzedStatus() const
{
	if (getIsCloud()) return 0;
	if (isFullyTextured() && allBakedTexturesCompletelyDownloaded()) return 3;
	if (isFullyTextured()) return 2;
	llassert(hasGray());
	return 1;
}
void LLVOAvatar::deleteLayerSetCaches(bool clearAll)
{
	for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
	{
		if (mBakedTextureDatas[i].mTexLayerSet)
		{
			if ((i != BAKED_HAIR || isSelf()) && !clearAll)
			{
				mBakedTextureDatas[i].mTexLayerSet->deleteCaches();
			}
		}
		mBakedTextureDatas[i].mMaskTexName.reset();
	}
}
BOOL LLVOAvatar::areAllNearbyInstancesBaked(S32& grey_avatars)
{
	BOOL res = TRUE;
	grey_avatars = 0;
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		if( inst->isDead() )
		{
			continue;
		}
		else if( !inst->isFullyBaked() )
		{
			res = FALSE;
			if (inst->mHasGrey)
			{
				++grey_avatars;
			}
		}
	}
	return res;
}
void LLVOAvatar::getNearbyRezzedStats(std::vector<S32>& counts)
{
	counts.clear();
	counts.resize(4);
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		if (inst)
		{
			S32 rez_status = inst->getRezzedStatus();
			counts[rez_status]++;
		}
	}
}
std::string LLVOAvatar::rezStatusToString(S32 rez_status)
{
	if (rez_status==0) return "cloud";
	if (rez_status==1) return "gray";
	if (rez_status==2) return "downloading";
	if (rez_status==3) return "full";
	return "unknown";
}
void LLVOAvatar::dumpBakedStatus()
{
	LLVector3d camera_pos_global = gAgentCamera.getCameraPositionGlobal();
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		LL_INFOS() << "Avatar ";
		LLNameValue* firstname = inst->getNVPair("FirstName");
		LLNameValue* lastname = inst->getNVPair("LastName");
		if( firstname )
		{
			LL_CONT << firstname->getString();
		}
		if( lastname )
		{
			LL_CONT << " " << lastname->getString();
		}
		LL_CONT << " " << inst->mID;
		if( inst->isDead() )
		{
			LL_CONT << " DEAD ("<< inst->getNumRefs() << " refs)";
		}
		if( inst->isSelf() )
		{
			LL_CONT << " (self)";
		}
		F64 dist_to_camera = (inst->getPositionGlobal() - camera_pos_global).length();
		LL_CONT << " " << dist_to_camera << "m ";
		LL_CONT << " " << inst->mPixelArea << " pixels";
		if( inst->isVisible() )
		{
			LL_CONT << " (visible)";
		}
		else
		{
			LL_CONT << " (not visible)";
		}
		if( inst->isFullyBaked() )
		{
			LL_CONT << " Baked";
		}
		else
		{
			LL_CONT << " Unbaked (";
			for (LLAvatarAppearanceDictionary::BakedTextures::const_iterator iter = LLAvatarAppearanceDictionary::getInstance()->getBakedTextures().begin();
				 iter != LLAvatarAppearanceDictionary::getInstance()->getBakedTextures().end();
				 ++iter)
			{
				const LLAvatarAppearanceDictionary::BakedEntry *baked_dict = iter->second;
				const ETextureIndex index = baked_dict->mTextureIndex;
				if (!inst->isTextureDefined(index))
				{
					LL_CONT << " " << (LLAvatarAppearanceDictionary::getInstance()->getTexture(index) ? LLAvatarAppearanceDictionary::getInstance()->getTexture(index)->mName : "");
				}
			}
			LL_CONT << " ) " << inst->getUnbakedPixelAreaRank();
			if( inst->isCulled() )
			{
				LL_CONT << " culled";
			}
		}
		LL_CONT << LL_ENDL;
	}
}
void LLVOAvatar::restoreGL()
{
	if (!isAgentAvatarValid()) return;
	gAgentAvatarp->setCompositeUpdatesEnabled(TRUE);
	for (U32 i = 0; i < gAgentAvatarp->mBakedTextureDatas.size(); i++)
	{
		gAgentAvatarp->invalidateComposite(gAgentAvatarp->getTexLayerSet(i), FALSE);
	}
	gAgentAvatarp->updateMeshTextures();
}
void LLVOAvatar::destroyGL()
{
	deleteCachedImages();
	resetImpostors();
}
void LLVOAvatar::resetImpostors()
{
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* avatar = (LLVOAvatar*) *iter;
		avatar->mImpostor.release();
		avatar->mNeedsImpostorUpdate = TRUE;
	}
}
void LLVOAvatar::deleteCachedImages(bool clearAll)
{
	if (LLViewerTexLayerSet::sHasCaches)
	{
		LL_DEBUGS() << "Deleting layer set caches" << LL_ENDL;
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
			 iter != LLCharacter::sInstances.end(); ++iter)
		{
			LLVOAvatar* inst = (LLVOAvatar*) *iter;
			inst->deleteLayerSetCaches(clearAll);
		}
		LLViewerTexLayerSet::sHasCaches = FALSE;
	}
	LLTexLayerStaticImageList::getInstance()->deleteCachedImages();
}
void LLVOAvatar::initClass()
{
	gAnimLibrary.animStateSetString(ANIM_AGENT_BODY_NOISE_ID,"body_noise");
	gAnimLibrary.animStateSetString(ANIM_AGENT_BREATHE_ROT_ID,"breathe_rot");
	gAnimLibrary.animStateSetString(ANIM_AGENT_PHYSICS_MOTION_ID,"physics_motion");
	gAnimLibrary.animStateSetString(ANIM_AGENT_EDITING_ID,"editing");
	gAnimLibrary.animStateSetString(ANIM_AGENT_EYE_ID,"eye");
	gAnimLibrary.animStateSetString(ANIM_AGENT_FLY_ADJUST_ID,"fly_adjust");
	gAnimLibrary.animStateSetString(ANIM_AGENT_HAND_MOTION_ID,"hand_motion");
	gAnimLibrary.animStateSetString(ANIM_AGENT_HEAD_ROT_ID,"head_rot");
	gAnimLibrary.animStateSetString(ANIM_AGENT_PELVIS_FIX_ID,"pelvis_fix");
	gAnimLibrary.animStateSetString(ANIM_AGENT_TARGET_ID,"target");
	gAnimLibrary.animStateSetString(ANIM_AGENT_WALK_ADJUST_ID,"walk_adjust");
	SHClientTagMgr::instance();
	LLControlAvatar::sRegionChangedSlot = gAgent.addRegionChangedCallback(&LLControlAvatar::onRegionChanged);
}
void LLVOAvatar::cleanupClass()
{
}
void LLVOAvatar::initInstance()
{
	if (LLCharacter::sInstances.size() == 1)
	{
		LLKeyframeMotion::setVFS(gStaticVFS);
		registerMotion(ANIM_AGENT_DO_NOT_DISTURB,			LLNullMotion::create );
		registerMotion( ANIM_AGENT_CROUCH,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_CROUCHWALK,				LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_EXPRESS_AFRAID,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_ANGER,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_BORED,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_CRY,				LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_DISDAIN,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_EMBARRASSED,		LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_FROWN,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_KISS,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_LAUGH,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_OPEN_MOUTH,		LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_REPULSED,		LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_SAD,				LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_SHRUG,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_SMILE,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_SURPRISE,		LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_TONGUE_OUT,		LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_TOOTHSMILE,		LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_WINK,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_WORRY,			LLEmote::create );
		registerMotion( ANIM_AGENT_FEMALE_RUN_NEW,			LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_FEMALE_WALK,				LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_FEMALE_WALK_NEW,			LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_RUN,						LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_RUN_NEW,					LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_STAND,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STAND_1,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STAND_2,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STAND_3,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STAND_4,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STANDUP,					LLKeyframeFallMotion::create );
		registerMotion( ANIM_AGENT_TURNLEFT,				LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_TURNRIGHT,				LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_WALK,					LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_WALK_NEW,				LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_BODY_NOISE_ID,			LLBodyNoiseMotion::create );
		registerMotion( ANIM_AGENT_BREATHE_ROT_ID,			LLBreatheMotionRot::create );
		registerMotion( ANIM_AGENT_PHYSICS_MOTION_ID,		LLPhysicsMotionController::create );
		registerMotion( ANIM_AGENT_EDITING_ID,				LLEditingMotion::create	);
		registerMotion( ANIM_AGENT_EYE_ID,					LLEyeMotion::create	);
		registerMotion( ANIM_AGENT_FLY_ADJUST_ID,			LLFlyAdjustMotion::create );
		registerMotion( ANIM_AGENT_HAND_MOTION_ID,			LLHandMotion::create );
		registerMotion( ANIM_AGENT_HEAD_ROT_ID,				LLHeadRotMotion::create );
		registerMotion( ANIM_AGENT_PELVIS_FIX_ID,			LLPelvisFixMotion::create );
		registerMotion( ANIM_AGENT_SIT_FEMALE,				LLKeyframeMotion::create );
		registerMotion( ANIM_AGENT_TARGET_ID,				LLTargetingMotion::create );
		registerMotion( ANIM_AGENT_WALK_ADJUST_ID,			LLWalkAdjustMotion::create );
	}
	LLAvatarAppearance::initInstance();
	createMotion( ANIM_AGENT_CUSTOMIZE);
	createMotion( ANIM_AGENT_CUSTOMIZE_DONE);
	mVoiceVisualizer->setVoiceEnabled( LLVoiceClient::getInstance()->getVoiceEnabled( mID ) );
    mInitFlags |= 1<<1;
}
LLAvatarJoint* LLVOAvatar::createAvatarJoint()
{
	return new LLViewerJoint();
}
LLAvatarJoint* LLVOAvatar::createAvatarJoint(S32 joint_num)
{
	return new LLViewerJoint(joint_num);
}
LLAvatarJointMesh* LLVOAvatar::createAvatarJointMesh()
{
	return new LLViewerJointMesh();
}
LLTexLayerSet* LLVOAvatar::createTexLayerSet()
{
	return new LLViewerTexLayerSet(this);
}
const LLVector3 LLVOAvatar::getRenderPosition() const
{
	if (mDrawable.isNull() || mDrawable->getGeneration() < 0)
	{
		return getPositionAgent();
	}
	else if (isRoot())
	{
		F32 fixup;
		if ( hasPelvisFixup( fixup) )
		{
			LLVector3 pos = mDrawable->getPositionAgent();
			pos[VZ] += fixup;
			return pos;
		}
		else
		{
			return mDrawable->getPositionAgent();
		}
	}
	else
	{
		LLVector4a pos;
		pos.load3(getPosition().mV);
		mDrawable->getParent()->getRenderMatrix().affineTransform(pos,pos);
		return LLVector3(pos.getF32ptr());
	}
}
void LLVOAvatar::updateDrawable(BOOL force_damped)
{
	clearChanged(SHIFTED);
}
void LLVOAvatar::onShift(const LLVector4a& shift_vector)
{
	const LLVector3& shift = reinterpret_cast<const LLVector3&>(shift_vector);
	mLastAnimExtents[0] += shift;
	mLastAnimExtents[1] += shift;
}
void LLVOAvatar::updateSpatialExtents(LLVector4a& newMin, LLVector4a &newMax)
{
	if (mDrawable.isNull())
	{
		return;
	}
	if (mNeedsExtentUpdate)
	{
		calculateSpatialExtents(newMin,newMax);
		mLastAnimExtents[0].set(newMin.getF32ptr());
		mLastAnimExtents[1].set(newMax.getF32ptr());
		mLastAnimBasePos = mPelvisp->getWorldPosition();
		mNeedsExtentUpdate = false;
	}
	else
	{
		LLVector3 new_base_pos = mPelvisp->getWorldPosition();
		LLVector3 shift = new_base_pos-mLastAnimBasePos;
		mLastAnimExtents[0] += shift;
		mLastAnimExtents[1] += shift;
		mLastAnimBasePos = new_base_pos;
	}
	if (isImpostor() && !needsImpostorUpdate())
	{
		LLVector3 delta = getRenderPosition() -
			((LLVector3(mDrawable->getPositionGroup().getF32ptr())-mImpostorOffset));
		newMin.load3( (mLastAnimExtents[0] + delta).mV);
		newMax.load3( (mLastAnimExtents[1] + delta).mV);
	}
	else
	{
		newMin.load3(mLastAnimExtents[0].mV);
		newMax.load3(mLastAnimExtents[1].mV);
		LLVector4a pos_group;
		pos_group.setAdd(newMin,newMax);
		pos_group.mul(0.5f);
		mImpostorOffset = LLVector3(pos_group.getF32ptr())-getRenderPosition();
		mDrawable->setPositionGroup(pos_group);
	}
}
static LLTrace::BlockTimerStatHandle FTM_AVATAR_EXTENT_UPDATE("Av Upd Extent");
void LLVOAvatar::calculateSpatialExtents(LLVector4a& newMin, LLVector4a& newMax)
{
    LL_RECORD_BLOCK_TIME(FTM_AVATAR_EXTENT_UPDATE);
	static const LLCachedControl<bool> control_derender_huge_attachments("DerenderHugeAttachments", true);
	static const LLCachedControl<S32> box_detail("AvatarBoundingBoxComplexity");
    LLVector3 zero_pos;
	LLVector4a pos;
    if (dist_vec(zero_pos, mPelvisp->getWorldPosition())<0.001)
    {
        pos.load3(getRenderPosition().mV);
    }
    else
    {
        pos.load3(mPelvisp->getWorldPosition().mV);
    }
	newMin = pos;
	newMax = pos;
	if ((box_detail >= 1) && !isControlAvatar())
	{
		for (polymesh_map_t::iterator i = mPolyMeshes.begin(); i != mPolyMeshes.end(); ++i)
		{
			LLPolyMesh* mesh = i->second;
			for (const auto& joint : mesh->mJointRenderData)
			{
				static const LLVector4Logical mask = _mm_load_ps((F32*)&S_V4LOGICAL_MASK_TABLE[3 * 4]);
				LLVector4a trans;
				trans.setSelectWithMask(mask, _mm_setzero_ps(), joint->mWorldMatrix->getRow<3>());
				update_min_max(newMin, newMax, trans);
			}
		}
	}
	LLVector4a padding(0.25);
	newMin.sub(padding);
	newMax.add(padding);
	static std::vector<LLViewerObject*> removal;
	if (box_detail >= 2)
	{
		float max_attachment_span = get_default_max_prim_scale() * 5.0f;
#if SLOW_ATTACHMENT_LIST
		for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
			 iter != mAttachmentPoints.end();
			 ++iter)
		{
			LLViewerJointAttachment* attachment = iter->second;
			if (attachment->getValid())
			{
				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					 attachment_iter != attachment->mAttachedObjects.end();
					 ++attachment_iter)
				{
					const LLViewerObject* attached_object = (*attachment_iter);
#else
		for(auto& iter : mAttachedObjectsVector)
		{{{
					LLViewerJointAttachment* attachment = iter.second;
					if (!attachment->getValid())
						continue;
					const LLViewerObject* attached_object = iter.first;
#endif
					if (attached_object && !attached_object->isHUDAttachment())
					{
						const LLVOVolume *vol = dynamic_cast<const LLVOVolume*>(attached_object);
						if (vol && vol->isAnimatedObject())
						{
							LLControlAvatar *cav = vol->getControlAvatar();
							if (cav)
							{
								LLVector4a cav_min;
								cav_min.load3(cav->mLastAnimExtents[0].mV);
								LLVector4a cav_max;
								cav_max.load3(cav->mLastAnimExtents[1].mV);
								update_min_max(newMin,newMax,cav_min);
								update_min_max(newMin,newMax,cav_max);
								continue;
							}
						}
						if (vol && vol->isRiggedMesh())
						{
							continue;
						}
						LLDrawable* drawable = attached_object->mDrawable;
						if (drawable && !drawable->isState(LLDrawable::RIGGED))
						{
							LLSpatialBridge* bridge = drawable->getSpatialBridge();
							if (bridge)
							{
								const LLVector4a* ext = bridge->getSpatialExtents();
								LLVector4a distance;
								distance.setSub(ext[1], ext[0]);
								LLVector4a max_span(max_attachment_span);
								S32 lt = distance.lessThan(max_span).getGatheredBits() & 0x7;
								if (lt == 0x7)
								{
									update_min_max(newMin,newMax,ext[0]);
									update_min_max(newMin,newMax,ext[1]);
								}
								else if(control_derender_huge_attachments)
								{
									removal.push_back((LLViewerObject *)attached_object);
								}
							}
						}
					}
				}
			}
		}
	}
	if(removal.size() > 0)
	{
		for(std::vector<LLViewerObject*>::iterator removal_iter = removal.begin(); removal_iter != removal.end(); ++removal_iter)
		{
			LLViewerObject *object_to_remove = *removal_iter;
			gObjectList.killObject(object_to_remove);
		}
		removal.clear();
	}
    if (box_detail>=3 && gMeshRepo.meshRezEnabled())
    {
		updateRiggingInfo();
        for (S32 joint_num = 0; joint_num < LL_CHARACTER_MAX_ANIMATED_JOINTS; joint_num++)
        {
            LLJoint *joint = getJoint(joint_num);
            LLJointRiggingInfo *rig_info = NULL;
            if (joint_num < mJointRiggingInfoTab.size())
            {
                rig_info = &mJointRiggingInfoTab[joint_num];
            }
            if (joint && rig_info && rig_info->isRiggedTo())
            {
                LLViewerJointAttachment *as_joint_attach = dynamic_cast<LLViewerJointAttachment*>(joint);
                if (as_joint_attach && as_joint_attach->getIsHUDAttachment())
                {
                    continue;
                }
				const LLMatrix4a& mat = joint->getWorldMatrix();
                LLVector4a new_extents[2];
                matMulBoundBox(mat, rig_info->getRiggedExtents(), new_extents);
                update_min_max(newMin, newMax, new_extents[0]);
                update_min_max(newMin, newMax, new_extents[1]);
            }
        }
    }
    LLVector4a center, size;
    center.setAdd(newMin, newMax);
    center.mul(0.5f);
    size.setSub(newMax,newMin);
    size.mul(0.5f);
    mPixelArea = LLPipeline::calcPixelArea(center, size, *LLViewerCamera::getInstance());
}
void render_sphere_and_line(const LLVector3& begin_pos, const LLVector3& end_pos, F32 sphere_scale, const LLVector3& occ_color, const LLVector3& visible_color)
{
    LLGLDepthTest normal_depth(GL_TRUE);
    gGL.diffuseColor3f(visible_color[0], visible_color[1], visible_color[2]);
    gGL.begin(LLRender::LINES);
    gGL.vertex3fv(begin_pos.mV);
    gGL.vertex3fv(end_pos.mV);
    gGL.end();
    gGL.pushMatrix();
    gGL.scalef(sphere_scale, sphere_scale, sphere_scale);
    gSphere.renderGGL();
    gGL.popMatrix();
    LLGLDepthTest depth_under(GL_TRUE, GL_FALSE, GL_GREATER);
    gGL.diffuseColor3f(occ_color[0], occ_color[1], occ_color[2]);
    gGL.begin(LLRender::LINES);
    gGL.vertex3fv(begin_pos.mV);
    gGL.vertex3fv(end_pos.mV);
    gGL.end();
    gGL.pushMatrix();
    gGL.scalef(sphere_scale, sphere_scale, sphere_scale);
    gSphere.renderGGL();
    gGL.popMatrix();
}
void LLVOAvatar::renderCollisionVolumes()
{
	std::ostringstream ostr;
	LLGLDepthTest gls_depth(GL_FALSE);
	for (size_t i = 0; i < mCollisionVolumes.size(); i++)
	{
		ostr << mCollisionVolumes[i]->getName() << ", ";
		LLAvatarJointCollisionVolume& collision_volume = *mCollisionVolumes[i];
		collision_volume.updateWorldMatrix();
		gGL.pushMatrix();
		gGL.multMatrix( collision_volume.getXform()->getWorldMatrix() );
		LLVector3 begin_pos(0,0,0);
		LLVector3 end_pos(collision_volume.getEnd());
		static F32 sphere_scale = 1.0f;
		static F32 center_dot_scale = 0.05f;
        static LLVector3 BLUE(0.0f, 0.0f, 1.0f);
        static LLVector3 PASTEL_BLUE(0.5f, 0.5f, 1.0f);
        static LLVector3 RED(1.0f, 0.0f, 0.0f);
        static LLVector3 PASTEL_RED(1.0f, 0.5f, 0.5f);
        static LLVector3 WHITE(1.0f, 1.0f, 1.0f);
        LLVector3 cv_color_occluded;
        LLVector3 cv_color_visible;
        LLVector3 dot_color_occluded(WHITE);
        LLVector3 dot_color_visible(WHITE);
        if (isControlAvatar())
        {
            cv_color_occluded = RED;
            cv_color_visible = PASTEL_RED;
        }
        else
        {
            cv_color_occluded = BLUE;
            cv_color_visible = PASTEL_BLUE;
        }
        render_sphere_and_line(begin_pos, end_pos, sphere_scale, cv_color_occluded, cv_color_visible);
        render_sphere_and_line(begin_pos, end_pos, center_dot_scale, dot_color_occluded, dot_color_visible);
		gGL.popMatrix();
	}
	if (mNameText.notNull())
	{
		LLVector4a unused;
		mNameText->lineSegmentIntersect(unused, unused, unused, TRUE);
	}
	mDebugText.clear();
	addDebugText(ostr.str());
}
void LLVOAvatar::renderBones()
{
    LLGLEnable<GL_BLEND> blend;
	avatar_joint_list_t::iterator iter = mSkeleton.begin();
	avatar_joint_list_t::iterator end  = mSkeleton.end();
    static LLVector3 OVERRIDE_COLOR_OCCLUDED(1.0f, 0.0f, 0.0f);
    static LLVector3 OVERRIDE_COLOR_VISIBLE(0.5f, 0.5f, 0.5f);
    static LLVector3 RIGGED_COLOR_OCCLUDED(0.0f, 1.0f, 1.0f);
    static LLVector3 RIGGED_COLOR_VISIBLE(0.5f, 0.5f, 0.5f);
    static LLVector3 OTHER_COLOR_OCCLUDED(0.0f, 1.0f, 0.0f);
    static LLVector3 OTHER_COLOR_VISIBLE(0.5f, 0.5f, 0.5f);
    static F32 SPHERE_SCALEF = 0.001f;
	for (; iter != end; ++iter)
	{
		LLJoint* jointp = *iter;
		if (!jointp)
		{
			continue;
		}
		jointp->updateWorldMatrix();
        LLVector3 occ_color, visible_color;
        LLVector3 pos;
        LLUUID mesh_id;
        if (jointp->hasAttachmentPosOverride(pos,mesh_id))
        {
            occ_color = OVERRIDE_COLOR_OCCLUDED;
            visible_color = OVERRIDE_COLOR_VISIBLE;
        }
        else
        {
            if (jointIsRiggedTo(jointp))
            {
                occ_color = RIGGED_COLOR_OCCLUDED;
                visible_color = RIGGED_COLOR_VISIBLE;
            }
            else
            {
                occ_color = OTHER_COLOR_OCCLUDED;
                visible_color = OTHER_COLOR_VISIBLE;
            }
        }
        LLVector3 begin_pos(0,0,0);
        LLVector3 end_pos(jointp->getEnd());
        F32 sphere_scale = SPHERE_SCALEF;
		gGL.pushMatrix();
		gGL.multMatrix( jointp->getXform()->getWorldMatrix() );
        render_sphere_and_line(begin_pos, end_pos, sphere_scale, occ_color, visible_color);
		gGL.popMatrix();
	}
}
void LLVOAvatar::renderJoints()
{
	std::ostringstream ostr;
	std::ostringstream nullstr;
	for (joint_map_t::iterator iter = mJointMap.begin(); iter != mJointMap.end(); ++iter)
	{
		LLJoint* jointp = iter->second;
		if (!jointp)
		{
			nullstr << iter->first << " is NULL" << std::endl;
			continue;
		}
		ostr << jointp->getName() << ", ";
		jointp->updateWorldMatrix();
		gGL.pushMatrix();
		gGL.multMatrix(jointp->getXform()->getWorldMatrix());
		gGL.diffuseColor3f( 1.f, 0.f, 1.f );
		gGL.begin(LLRender::LINES);
		LLVector3 v[] =
		{
			LLVector3(1,0,0),
			LLVector3(-1,0,0),
			LLVector3(0,1,0),
			LLVector3(0,-1,0),
			LLVector3(0,0,-1),
			LLVector3(0,0,1),
		};
		gGL.vertex3fv(v[0].mV);
		gGL.vertex3fv(v[2].mV);
		gGL.vertex3fv(v[0].mV);
		gGL.vertex3fv(v[3].mV);
		gGL.vertex3fv(v[1].mV);
		gGL.vertex3fv(v[2].mV);
		gGL.vertex3fv(v[1].mV);
		gGL.vertex3fv(v[3].mV);
		gGL.vertex3fv(v[0].mV);
		gGL.vertex3fv(v[4].mV);
		gGL.vertex3fv(v[1].mV);
		gGL.vertex3fv(v[4].mV);
		gGL.vertex3fv(v[2].mV);
		gGL.vertex3fv(v[4].mV);
		gGL.vertex3fv(v[3].mV);
		gGL.vertex3fv(v[4].mV);
		gGL.vertex3fv(v[0].mV);
		gGL.vertex3fv(v[5].mV);
		gGL.vertex3fv(v[1].mV);
		gGL.vertex3fv(v[5].mV);
		gGL.vertex3fv(v[2].mV);
		gGL.vertex3fv(v[5].mV);
		gGL.vertex3fv(v[3].mV);
		gGL.vertex3fv(v[5].mV);
		gGL.end();
		gGL.popMatrix();
	}
	mDebugText.clear();
	addDebugText(ostr.str());
	addDebugText(nullstr.str());
}
BOOL LLVOAvatar::lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
									  S32 face,
									  BOOL pick_transparent,
									  BOOL pick_rigged,
									  S32* face_hit,
									  LLVector4a* intersection,
									  LLVector2* tex_coord,
									  LLVector4a* normal,
									  LLVector4a* tangent)
{
	if ((isSelf() && !gAgent.needsRenderAvatar()) || !LLPipeline::sPickAvatar)
	{
		return FALSE;
	}
	if (isControlAvatar())
	{
		return FALSE;
	}
	if (lineSegmentBoundingBox(start, end))
	{
		for (S32 i = 0; i < (S32)mCollisionVolumes.size(); ++i)
		{
			mCollisionVolumes[i]->updateWorldMatrix();
			const LLMatrix4a& mat = mCollisionVolumes[i]->getXform()->getWorldMatrix();
			LLMatrix4a inverse = mat;
			inverse.invert();
			LLMatrix4a norm_mat = inverse;
			norm_mat.transpose();
			LLVector4a p1, p2;
			inverse.affineTransform(start,p1);
			inverse.affineTransform(end,p2);
			LLVector3 position;
			LLVector3 norm;
			if (linesegment_sphere(LLVector3(p1.getF32ptr()), LLVector3(p2.getF32ptr()), LLVector3(0,0,0), 1.f, position, norm))
			{
				if (intersection)
				{
					LLVector4a res_pos;
					res_pos.load3(position.mV);
					mat.affineTransform(res_pos,res_pos);
					*intersection = res_pos;
				}
				if (normal)
				{
					LLVector4a res_norm;
					res_norm.load3(norm.mV);
					res_norm.normalize3fast();
					norm_mat.perspectiveTransform(res_norm,res_norm);
					*normal = res_norm;
				}
				return TRUE;
			}
		}
		if (isSelf())
		{
#if SLOW_ATTACHMENT_LIST
			for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
			 iter != mAttachmentPoints.end();
			 ++iter)
			{
				LLViewerJointAttachment* attachment = iter->second;
				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					 attachment_iter != attachment->mAttachedObjects.end();
					 ++attachment_iter)
				{
					LLViewerObject* attached_object = (*attachment_iter);
#else
			for(auto& iter : mAttachedObjectsVector)
			{{
					const LLViewerJointAttachment* attachment = iter.second;
					const LLViewerObject* attached_object = iter.first;
#endif
					if (attached_object && !attached_object->isDead() && attachment->getValid())
					{
						LLDrawable* drawable = attached_object->mDrawable;
						if (drawable->isState(LLDrawable::RIGGED))
						{
							gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_RIGGED, TRUE);
						}
					}
				}
			}
		}
	}
	LLVector4a position;
	if (mNameText.notNull() && mNameText->lineSegmentIntersect(start, end, position))
	{
		if (intersection)
		{
			*intersection = position;
		}
		return TRUE;
	}
	return FALSE;
}
LLViewerObject* LLVOAvatar::lineSegmentIntersectRiggedAttachments(const LLVector4a& start, const LLVector4a& end,
									  S32 face,
									  BOOL pick_transparent,
									  BOOL pick_rigged,
									  S32* face_hit,
									  LLVector4a* intersection,
									  LLVector2* tex_coord,
									  LLVector4a* normal,
									  LLVector4a* tangent)
{
	if (isSelf() && !gAgent.needsRenderAvatar())
	{
		return NULL;
	}
	LLViewerObject* hit = NULL;
	if (lineSegmentBoundingBox(start, end))
	{
		LLVector4a local_end = end;
		LLVector4a local_intersection;
#if SLOW_ATTACHMENT_LIST
		for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
			iter != mAttachmentPoints.end();
			++iter)
		{
			LLViewerJointAttachment* attachment = iter->second;
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					attachment_iter != attachment->mAttachedObjects.end();
					++attachment_iter)
			{
				LLViewerObject* attached_object = (*attachment_iter);
#else
		for(auto& iter : mAttachedObjectsVector)
		{{
				LLViewerObject* attached_object = iter.first;
#endif
				if (attached_object->lineSegmentIntersect(start, local_end, face, pick_transparent, pick_rigged, face_hit, &local_intersection, tex_coord, normal, tangent))
				{
					local_end = local_intersection;
					if (intersection)
					{
						*intersection = local_intersection;
					}
					hit = attached_object;
				}
			}
		}
	}
	return hit;
}
LLVOAvatar* LLVOAvatar::asAvatar()
{
	return this;
}
void LLVOAvatar::startDefaultMotions()
{
	startMotion( ANIM_AGENT_HEAD_ROT );
	startMotion( ANIM_AGENT_EYE );
	startMotion( ANIM_AGENT_BODY_NOISE );
	startMotion( ANIM_AGENT_BREATHE_ROT );
	startMotion( ANIM_AGENT_PHYSICS_MOTION );
	startMotion( ANIM_AGENT_HAND_MOTION );
	startMotion( ANIM_AGENT_PELVIS_FIX );
	processAnimationStateChanges();
}
void LLVOAvatar::buildCharacter()
{
	LLAvatarAppearance::buildCharacter();
	mIsBuilt = FALSE;
	updateHeadOffset();
	mOohMorph     = getVisualParam( "Lipsync_Ooh" );
	mAahMorph     = getVisualParam( "Lipsync_Aah" );
	if (!mOohMorph)
	{
		LL_WARNS() << "Missing 'Ooh' morph for lipsync, using fallback." << LL_ENDL;
		mOohMorph = getVisualParam( "Express_Kiss" );
	}
	if (!mAahMorph)
	{
		LL_WARNS() << "Missing 'Aah' morph for lipsync, using fallback." << LL_ENDL;
		mAahMorph = getVisualParam( "Express_Open_Mouth" );
	}
    if (mEnableDefaultMotions)
    {
        startDefaultMotions();
    }
	processAnimationStateChanges();
	mIsBuilt = TRUE;
	stop_glerror();
	mMeshValid = TRUE;
}
void LLVOAvatar::resetVisualParams()
{
	{
		LLAvatarXmlInfo::skeletal_distortion_info_list_t::iterator iter;
		for (iter = sAvatarXmlInfo->mSkeletalDistortionInfoList.begin();
			 iter != sAvatarXmlInfo->mSkeletalDistortionInfoList.end();
			 ++iter)
		{
			LLPolySkeletalDistortionInfo *info = (LLPolySkeletalDistortionInfo*)*iter;
			LLPolySkeletalDistortion *param = dynamic_cast<LLPolySkeletalDistortion*>(getVisualParam(info->getID()));
            *param = LLPolySkeletalDistortion(this);
            llassert(param);
			if (!param->setInfo(info))
			{
				llassert(false);
			}
		}
	}
	for (LLAvatarXmlInfo::driver_info_list_t::iterator iter = sAvatarXmlInfo->mDriverInfoList.begin();
		 iter != sAvatarXmlInfo->mDriverInfoList.end();
		 ++iter)
	{
		LLDriverParamInfo *info = *iter;
        LLDriverParam *param = dynamic_cast<LLDriverParam*>(getVisualParam(info->getID()));
        LLDriverParam::entry_list_t driven_list = param->getDrivenList();
        *param = LLDriverParam(this);
        llassert(param);
        if (!param->setInfo(info))
        {
            llassert(false);
        }
        param->setDrivenList(driven_list);
	}
}
void LLVOAvatar::resetSkeleton(bool reset_animations)
{
    LL_DEBUGS("Avatar") << avString() << " reset starts" << LL_ENDL;
    if (!isControlAvatar() && !mLastProcessedAppearance)
    {
        LL_WARNS() << "Can't reset avatar; no appearance message has been received yet." << LL_ENDL;
        return;
    }
    clearAttachmentOverrides();
	if( !buildSkeleton(sAvatarSkeletonInfo) )
    {
        LL_ERRS() << "Error resetting skeleton" << LL_ENDL;
	}
    resetVisualParams();
	if( !buildSkeleton(sAvatarSkeletonInfo) )
    {
        LL_ERRS() << "Error resetting skeleton" << LL_ENDL;
	}
    bool ignore_hud_joints = true;
    initAttachmentPoints(ignore_hud_joints);
    for (LLVisualParam *param = getFirstVisualParam();
         param;
         param = getNextVisualParam())
    {
        LLPolyMorphTarget *poly_morph = dynamic_cast<LLPolyMorphTarget*>(param);
        if (poly_morph)
        {
            F32 delta_weight = poly_morph->getLastWeight() - poly_morph->getDefaultWeight();
            poly_morph->applyVolumeChanges(delta_weight);
        }
    }
    if (mLastProcessedAppearance)
    {
        bool slam_params = true;
        applyParsedAppearanceMessage(*mLastProcessedAppearance, slam_params);
    }
    updateVisualParams();
    updateAttachmentOverrides();
    if (reset_animations)
    {
        if (isSelf())
        {
            gAgent.stopCurrentAnimations();
        }
        else
        {
            resetAnimations();
        }
    }
    LL_DEBUGS("Avatar") << avString() << " reset ends" << LL_ENDL;
}
void LLVOAvatar::releaseMeshData()
{
	if (sInstances.size() < AVATAR_RELEASE_THRESHOLD || isUIAvatar())
	{
		return;
	}
	for (avatar_joint_list_t::iterator iter = mMeshLOD.begin();
		 iter != mMeshLOD.end();
		 ++iter)
	{
		LLAvatarJoint* joint = (*iter);
		joint->setValid(FALSE, TRUE);
	}
	if (mDrawable.notNull())
	{
		LLFace* facep = mDrawable->getFace(0);
		if (facep)
		{
			facep->setSize(0, 0);
			for(S32 i = mNumInitFaces ; i < mDrawable->getNumFaces(); i++)
			{
				facep = mDrawable->getFace(i);
				if (facep)
				{
					facep->setSize(0, 0);
				}
			}
		}
	}
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (!attachment->getIsHUDAttachment())
		{
			attachment->setAttachmentVisibility(FALSE);
		}
	}
	mMeshValid = FALSE;
}
void LLVOAvatar::restoreMeshData()
{
	llassert(!isSelf());
    if (mDrawable.isNull())
    {
        return;
    }
	mMeshValid = TRUE;
	updateJointLODs();
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (!attachment->getIsHUDAttachment())
		{
			attachment->setAttachmentVisibility(TRUE);
		}
	}
	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, TRUE);
}
void LLVOAvatar::updateMeshData()
{
	if (mDrawable.notNull())
	{
		stop_glerror();
		S32 f_num = 0 ;
		const U32 VERTEX_NUMBER_THRESHOLD = 128 ;
		const S32 num_parts = mMeshLOD.size();
		for(S32 part_index = 0 ; part_index < num_parts ;)
		{
			S32 j = part_index ;
			U32 last_v_num = 0, num_vertices = 0 ;
			U32 last_i_num = 0, num_indices = 0 ;
			while(part_index < num_parts && num_vertices < VERTEX_NUMBER_THRESHOLD)
			{
				last_v_num = num_vertices ;
				last_i_num = num_indices ;
				LLViewerJoint* part_mesh = getViewerJoint(part_index++);
				if (part_mesh)
				{
					part_mesh->updateFaceSizes(num_vertices, num_indices, mAdjustedPixelArea);
				}
			}
			if(num_vertices < 1)
			{
				continue ;
			}
			if(last_v_num > 0)
			{
				num_vertices = last_v_num ;
				num_indices = last_i_num ;
				part_index-- ;
			}
			LLFace* facep = NULL;
			if(f_num < mDrawable->getNumFaces())
			{
				facep = mDrawable->getFace(f_num);
			}
			else
			{
				facep = mDrawable->getFace(0);
				if (facep)
				{
					facep = mDrawable->addFace(facep->getPool(), facep->getTexture()) ;
				}
			}
			if (!facep) continue;
			facep->setSize(num_vertices, num_indices);
			bool terse_update = false;
			facep->setGeomIndex(0);
			facep->setIndicesIndex(0);
			LLVertexBuffer* buff = facep->getVertexBuffer();
			if(!facep->getVertexBuffer())
			{
				buff = new LLVertexBufferAvatar();
				buff->allocateBuffer(num_vertices, num_indices, TRUE);
				facep->setVertexBuffer(buff);
			}
			else
			{
				if (buff->getNumIndices() == num_indices &&
					buff->getNumVerts() == num_vertices)
				{
					terse_update = true;
				}
				else
				{
					buff->resizeBuffer(num_vertices, num_indices);
				}
			}
			if (facep->getGeomIndex() > 0)
			{
				LL_ERRS() << "non-zero geom index: " << facep->getGeomIndex() << " in LLVOAvatar::restoreMeshData" << LL_ENDL;
			}
			for(S32 k = j ; k < part_index ; k++)
			{
				bool rigid = false;
				if (k == MESH_ID_EYEBALL_LEFT ||
					k == MESH_ID_EYEBALL_RIGHT)
				{
					rigid = true;
				}
				LLViewerJoint* mesh = getViewerJoint(k);
				if (mesh)
				{
					mesh->updateFaceData(facep, mAdjustedPixelArea, k == MESH_ID_HAIR, terse_update && !rigid);
				}
			}
			stop_glerror();
			buff->flush();
			if(!f_num)
			{
				f_num += mNumInitFaces ;
			}
			else
			{
				f_num++ ;
			}
		}
	}
}
U32 LLVOAvatar::processUpdateMessage(LLMessageSystem *mesgsys,
									 void **user_data,
									 U32 block_num, const EObjectUpdateType update_type,
									 LLDataPacker *dp)
{
	const BOOL has_name = !getNVPair("FirstName");
	U32 retval = LLViewerObject::processUpdateMessage(mesgsys, user_data, block_num, update_type, dp);
	if (has_name && getNVPair("FirstName"))
	{
		mDebugExistenceTimer.reset();
		debugAvatarRezTime("AvatarRezArrivedNotification","avatar arrived");
	}
	if(retval & LLViewerObject::INVALID_UPDATE)
	{
		if(isSelf())
		{
			gAgent.teleportViaLocation(gAgent.getPositionGlobal());
		}
	}
	return retval;
}
LLViewerFetchedTexture *LLVOAvatar::getBakedTextureImage(const U8 te, const LLUUID& uuid)
{
	LLViewerFetchedTexture *result = NULL;
	if (uuid == IMG_DEFAULT_AVATAR ||
		uuid == IMG_DEFAULT ||
		uuid == IMG_INVISIBLE)
	{
		result = gTextureList.findImage(uuid, TEX_LIST_STANDARD);
	}
	if (!result)
	{
		const std::string url = getImageURL(te,uuid);
		const LLGLTexture::EBoostLevel bake_boost = isSelf()
			? LLGLTexture::BOOST_AVATAR_BAKED_SELF
			: LLGLTexture::BOOST_AVATAR_BAKED;
		if (!url.empty())
		{
			LL_DEBUGS("Avatar") << avString() << "get server-bake image from URL " << url << LL_ENDL;
			result = LLViewerTextureManager::getFetchedTextureFromUrl(
				url, FTT_SERVER_BAKE, TRUE, bake_boost, LLViewerTexture::LOD_TEXTURE, 0, 0, uuid);
			if (result->isMissingAsset())
			{
				result->setIsMissingAsset(false);
			}
		}
		else
		{
			LL_DEBUGS("Avatar") << avString() << "from host " << uuid << LL_ENDL;
			LLHost host = getObjectHost();
			result = LLViewerTextureManager::getFetchedTexture(
				uuid, FTT_SERVER_BAKE, TRUE, bake_boost, LLViewerTexture::LOD_TEXTURE, 0, 0, host);
		}
		if (result)
		{
			result->setBoostLevel(bake_boost);
		}
	}
	return result;
}
S32 LLVOAvatar::setTETexture(const U8 te, const LLUUID& uuid)
{
	if (!isIndexBakedTexture((ETextureIndex)te))
	{
		return LLViewerObject::setTETexture(te, LLUUID::null);
	}
	LLViewerFetchedTexture *image = getBakedTextureImage(te,uuid);
	llassert(image);
	return setTETextureCore(te, image);
}
static LLFastTimer::DeclareTimer FTM_AVATAR_UPDATE("Avatar Update");
static LLFastTimer::DeclareTimer FTM_JOINT_UPDATE("Update Joints");
static LLFastTimer::DeclareTimer FTM_CHARACTER_UPDATE("Character Update");
static LLFastTimer::DeclareTimer FTM_BASE_UPDATE("Base Update");
static LLFastTimer::DeclareTimer FTM_MISC_UPDATE("Misc Update");
static LLFastTimer::DeclareTimer FTM_DETAIL_UPDATE("Detail Update");
void LLVOAvatar::dumpAnimationState()
{
	LL_INFOS() << "==============================================" << LL_ENDL;
	for (LLVOAvatar::AnimIterator it = mSignaledAnimations.begin(); it != mSignaledAnimations.end(); ++it)
	{
		LLUUID id = it->first;
		std::string playtag = "";
		if (mPlayingAnimations.find(id) != mPlayingAnimations.end())
		{
			playtag = "*";
		}
		LL_INFOS() << gAnimLibrary.animationName(id) << playtag << LL_ENDL;
	}
	for (LLVOAvatar::AnimIterator it = mPlayingAnimations.begin(); it != mPlayingAnimations.end(); ++it)
	{
		LLUUID id = it->first;
		bool is_signaled = mSignaledAnimations.find(id) != mSignaledAnimations.end();
		if (!is_signaled)
		{
			LL_INFOS() << gAnimLibrary.animationName(id) << "!S" << LL_ENDL;
		}
	}
}
void LLVOAvatar::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	LL_RECORD_BLOCK_TIME(FTM_AVATAR_UPDATE);
	if (isDead())
	{
		LL_INFOS() << "Warning!  Idle on dead avatar" << LL_ENDL;
		return;
	}
	if (!(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_AVATAR)))
	{
		return;
	}
	const S32 upd_freq = 4;
	if ((mLastAnimExtents[0]==LLVector3())||
		(mLastAnimExtents[1])==LLVector3())
	{
		mNeedsExtentUpdate = true;
	}
	else
	{
		mNeedsExtentUpdate = ((LLDrawable::getCurrentFrame()+mID.mData[0])%upd_freq==0);
	}
	checkTextureLoading() ;
	setPixelAreaAndAngle(gAgent);
	if(mDrawable.notNull() && !gNoRender)
	{
		LL_RECORD_BLOCK_TIME(FTM_JOINT_UPDATE);
		if (isSitting() && getParent())
		{
			LLViewerObject *root_object = (LLViewerObject*)getRoot();
			LLDrawable* drawablep = root_object->mDrawable;
			if (drawablep)
			{
				if (root_object->isSelected())
				{
					gPipeline.updateMoveNormalAsync(drawablep);
				}
				else
				{
					gPipeline.updateMoveDampedAsync(drawablep);
				}
			}
		}
		else
		{
			gPipeline.updateMoveDampedAsync(mDrawable);
		}
	}
	if (isSelf())
	{
		{
			LL_RECORD_BLOCK_TIME(FTM_BASE_UPDATE);
			LLViewerObject::idleUpdate(agent, world, time);
		}
		if (isAnyAnimationSignaled(AGENT_STAND_ANIMS, NUM_AGENT_STAND_ANIMS))
		{
			agent.fidget();
		}
	}
	else
	{
		LLQuaternion rotation = getRotation();
		{
			LL_RECORD_BLOCK_TIME(FTM_BASE_UPDATE);
			LLViewerObject::idleUpdate(agent, world, time);
		}
		setRotation(rotation);
	}
	lazyAttach();
	mLastRootPos = mRoot->getWorldPosition();
	bool detailed_update;
	{
		LL_RECORD_BLOCK_TIME(FTM_CHARACTER_UPDATE);
		detailed_update = updateCharacter(agent);
	}
	if (gNoRender)
	{
		return;
	}
	static LLUICachedControl<bool> visualizers_in_calls("ShowVoiceVisualizersInCalls", false);
	bool voice_enabled = (visualizers_in_calls || LLVoiceClient::getInstance()->inProximalChannel()) &&
						 LLVoiceClient::getInstance()->getVoiceEnabled(mID);
	LL_RECORD_BLOCK_TIME(FTM_MISC_UPDATE);
	idleUpdateVoiceVisualizer(voice_enabled);
	idleUpdateMisc(detailed_update);
	idleUpdateAppearanceAnimation();
	if (detailed_update)
	{
		LL_RECORD_BLOCK_TIME(FTM_DETAIL_UPDATE);
		idleUpdateLipSync(voice_enabled);
		idleUpdateLoadingEffect();
		idleUpdateBelowWater();
		idleUpdateWindEffect();
	}
	idleUpdateNameTag(mLastRootPos);
	idleUpdateRenderComplexity();
}
void LLVOAvatar::idleUpdateVoiceVisualizer(bool voice_enabled)
{
	bool render_visualizer = voice_enabled;
	if(isSelf())
	{
		if(gAgentCamera.cameraMouselook() || gSavedSettings.getBOOL("VoiceDisableMic"))
		{
			render_visualizer = false;
		}
	}
	else if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS) || (gRlvHandler.hasBehaviour(RLV_BHVR_CAMAVDIST) && (gAgent.getPosGlobalFromAgent(getCharacterPosition()) - gAgent.getPosGlobalFromAgent(gAgentAvatarp->getRenderPosition())).magVec() > gRlvHandler.camPole(RLV_BHVR_CAMAVDIST)))
	{
		render_visualizer = false;
	}
	mVoiceVisualizer->setVoiceEnabled(render_visualizer);
	if ( voice_enabled )
	{
		if( isSelf() )
		{
			int lastGesticulationLevel = mCurrentGesticulationLevel;
			mCurrentGesticulationLevel = mVoiceVisualizer->getCurrentGesticulationLevel();
			if ( lastGesticulationLevel != mCurrentGesticulationLevel )
			{
				if ( mCurrentGesticulationLevel != VOICE_GESTICULATION_LEVEL_OFF )
				{
					std::string gestureString = "unInitialized";
					if ( mCurrentGesticulationLevel == 0 )	{ gestureString = "/voicelevel1";	}
					else	if ( mCurrentGesticulationLevel == 1 )	{ gestureString = "/voicelevel2";	}
					else	if ( mCurrentGesticulationLevel == 2 )	{ gestureString = "/voicelevel3";	}
					else	{ LL_INFOS() << "oops - CurrentGesticulationLevel can be only 0, 1, or 2"  << LL_ENDL; }
					LLGestureMgr::instance().triggerAndReviseString( gestureString );
				}
			}
		}
		if (LLVoiceClient::getInstance()->getIsSpeaking( mID ))
		{
			if ( ! mVoiceVisualizer->getCurrentlySpeaking() )
			{
				mVoiceVisualizer->setStartSpeaking();
			}
			mVoiceVisualizer->setSpeakingAmplitude( LLVoiceClient::getInstance()->getCurrentPower( mID ) );
			if( isSelf() )
			{
				gAgent.clearAFK();
			}
			mIdleTimer.reset();
		}
		else
		{
			if ( mVoiceVisualizer->getCurrentlySpeaking() )
			{
				mVoiceVisualizer->setStopSpeaking();
				if ( mLipSyncActive )
				{
					if( mOohMorph ) mOohMorph->setWeight(mOohMorph->getMinWeight(), FALSE);
					if( mAahMorph ) mAahMorph->setWeight(mAahMorph->getMinWeight(), FALSE);
					mLipSyncActive = false;
					LLCharacter::updateVisualParams();
					dirtyMesh();
				}
			}
		}
		if ( isSitting() )
		{
			LLVector3 headOffset = LLVector3( 0.0f, 0.0f, mHeadOffset.mV[2] );
			mVoiceVisualizer->setVoiceSourceWorldPosition( mRoot->getWorldPosition() + headOffset );
		}
		else
		{
			LLVector3 tagPos = mRoot->getWorldPosition();
			tagPos[VZ] -= mPelvisToFoot;
			tagPos[VZ] += ( mBodySize[VZ] + 0.125f );
			mVoiceVisualizer->setVoiceSourceWorldPosition( tagPos );
		}
	}
}
static LLFastTimer::DeclareTimer FTM_ATTACHMENT_UPDATE("Update Attachments");
void LLVOAvatar::idleUpdateMisc(bool detailed_update)
{
	if (LLVOAvatar::sJointDebug)
	{
		LL_INFOS() << getFullname() << ": joint touches: " << LLJoint::sNumTouches << " updates: " << LLJoint::sNumUpdates << LL_ENDL;
	}
	LLJoint::sNumUpdates = 0;
	LLJoint::sNumTouches = 0;
	BOOL visible = isVisible() || mNeedsAnimUpdate;
	if (detailed_update || !sUseImpostors)
	{
		LL_RECORD_BLOCK_TIME(FTM_ATTACHMENT_UPDATE);
#if SLOW_ATTACHMENT_LIST
		for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
			 iter != mAttachmentPoints.end();
			 ++iter)
		{
			LLViewerJointAttachment* attachment = iter->second;
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
				 attachment_iter != attachment->mAttachedObjects.end();
				 ++attachment_iter)
			{
				LLViewerObject* attached_object = (*attachment_iter);
#else
		for(auto& iter : mAttachedObjectsVector)
		{{
				LLViewerJointAttachment* attachment = iter.second;
				LLViewerObject* attached_object = iter.first;
#endif
				BOOL visibleAttachment = visible || (attached_object &&
													 !(attached_object->mDrawable->getSpatialBridge() &&
													   attached_object->mDrawable->getSpatialBridge()->getRadius() < 2.0));
				if (visibleAttachment && attached_object && !attached_object->isDead() && attachment->getValid())
				{
					if (LLSelectMgr::getInstance()->getSelection()->getObjectCount() && LLSelectMgr::getInstance()->getSelection()->isAttachment())
					{
						gPipeline.updateMoveNormalAsync(attached_object->mDrawable);
					}
					else
					{
						gPipeline.updateMoveDampedAsync(attached_object->mDrawable);
					}
					LLSpatialBridge* bridge = attached_object->mDrawable->getSpatialBridge();
					if (bridge)
					{
						gPipeline.updateMoveNormalAsync(bridge);
					}
					attached_object->updateText();
				}
			}
		}
	}
	mNeedsAnimUpdate = FALSE;
	if (isImpostor() && !mNeedsImpostorUpdate)
	{
		LL_ALIGN_16(LLVector4a ext[2]);
		F32 distance;
		LLVector3 angle;
		getImpostorValues(ext, angle, distance);
		for (U32 i = 0; i < 3 && !mNeedsImpostorUpdate; i++)
		{
			F32 cur_angle = angle.mV[i];
			F32 old_angle = mImpostorAngle.mV[i];
			F32 angle_diff = fabsf(cur_angle-old_angle);
			if (angle_diff > F_PI/512.f*distance*mUpdatePeriod)
			{
				mNeedsImpostorUpdate = TRUE;
			}
		}
		if (detailed_update && !mNeedsImpostorUpdate)
		{
			F32 dist_diff = fabsf(distance-mImpostorDistance);
			if (dist_diff/mImpostorDistance > 0.1f)
			{
				mNeedsImpostorUpdate = TRUE;
			}
			else
			{
				ext[0].load3(mLastAnimExtents[0].mV);
                ext[1].load3(mLastAnimExtents[1].mV);
				LLVector4a diff;
				diff.setSub(ext[1], mImpostorExtents[1]);
				if (diff.getLength3().getF32() > 0.05f)
				{
					mNeedsImpostorUpdate = TRUE;
				}
				else
				{
					diff.setSub(ext[0], mImpostorExtents[0]);
					if (diff.getLength3().getF32() > 0.05f)
					{
						mNeedsImpostorUpdate = TRUE;
					}
				}
			}
		}
	}
	if (mDrawable.notNull())
	{
		mDrawable->movePartition();
		if (getParent() && ((LLViewerObject*)getParent())->mDrawable.notNull() && ((LLViewerObject*) getParent())->mDrawable->isActive())
		{
			gPipeline.markMoved(mDrawable, TRUE);
		}
	}
}
void LLVOAvatar::idleUpdateAppearanceAnimation()
{
	if (mAppearanceAnimating)
	{
		ESex avatar_sex = getSex();
		F32 appearance_anim_time = mAppearanceMorphTimer.getElapsedTimeF32();
		if (appearance_anim_time >= APPEARANCE_MORPH_TIME)
		{
			mAppearanceAnimating = FALSE;
			for (LLVisualParam *param = getFirstVisualParam();
				 param;
				 param = getNextVisualParam())
			{
				if (param->isTweakable())
				{
					param->stopAnimating(FALSE);
				}
			}
			updateVisualParams();
			if (isSelf())
			{
				gAgent.sendAgentSetAppearance();
			}
			mIdleTimer.reset();
		}
		else
		{
			F32 morph_amt = calcMorphAmount();
			LLVisualParam *param;
			if (!isSelf())
			{
				for (param = getFirstVisualParam();
					 param;
					 param = getNextVisualParam())
				{
					if (param->isTweakable())
					{
						param->animate(morph_amt, FALSE);
					}
				}
			}
			for (param = getFirstVisualParam();
				 param;
				 param = getNextVisualParam())
			{
				param->apply(avatar_sex);
			}
			mLastAppearanceBlendTime = appearance_anim_time;
		}
		dirtyMesh();
	}
}
F32 LLVOAvatar::calcMorphAmount()
{
	F32 appearance_anim_time = mAppearanceMorphTimer.getElapsedTimeF32();
	F32 blend_frac = calc_bouncy_animation(appearance_anim_time / APPEARANCE_MORPH_TIME);
	F32 last_blend_frac = calc_bouncy_animation(mLastAppearanceBlendTime / APPEARANCE_MORPH_TIME);
	F32 morph_amt;
	if (last_blend_frac == 1.f)
	{
		morph_amt = 1.f;
	}
	else
	{
		morph_amt = (blend_frac - last_blend_frac) / (1.f - last_blend_frac);
	}
	return morph_amt;
}
void LLVOAvatar::idleUpdateLipSync(bool voice_enabled)
{
	if ( voice_enabled && (LLVoiceClient::getInstance()->lipSyncEnabled()) && LLVoiceClient::getInstance()->getIsSpeaking( mID ) )
	{
		F32 ooh_morph_amount = 0.0f;
		F32 aah_morph_amount = 0.0f;
		mVoiceVisualizer->lipSyncOohAah( ooh_morph_amount, aah_morph_amount );
		if( mOohMorph )
		{
			F32 ooh_weight = mOohMorph->getMinWeight()
				+ ooh_morph_amount * (mOohMorph->getMaxWeight() - mOohMorph->getMinWeight());
			mOohMorph->setWeight( ooh_weight, FALSE );
		}
		if( mAahMorph )
		{
			F32 aah_weight = mAahMorph->getMinWeight()
				+ aah_morph_amount * (mAahMorph->getMaxWeight() - mAahMorph->getMinWeight());
			mAahMorph->setWeight( aah_weight, FALSE );
		}
		mLipSyncActive = true;
		LLCharacter::updateVisualParams();
		dirtyMesh();
		mIdleTimer.reset();
	}
}
void LLVOAvatar::idleUpdateLoadingEffect()
{
	if (updateIsFullyLoaded())
	{
		if (isFullyLoaded())
		{
			if (mFirstFullyVisible)
			{
				mFirstFullyVisible = FALSE;
				if (isSelf())
				{
					LL_INFOS("Avatar") << avString() << "self isFullyLoaded, mFirstFullyVisible" << LL_ENDL;
					LLAppearanceMgr::instance().onFirstFullyVisible();
				}
				else
				{
					LL_INFOS("Avatar") << avString() << "other isFullyLoaded, mFirstFullyVisible" << LL_ENDL;
				}
			}
			deleteParticleSource();
			updateLOD();
		}
		else
		{
			LLPartSysData particle_parameters;
			particle_parameters.mPartData.mMaxAge            = 4.f;
			particle_parameters.mPartData.mStartScale.mV[VX] = 0.8f;
			particle_parameters.mPartData.mStartScale.mV[VX] = 0.8f;
			particle_parameters.mPartData.mStartScale.mV[VY] = 1.0f;
			particle_parameters.mPartData.mEndScale.mV[VX]   = 0.02f;
			particle_parameters.mPartData.mEndScale.mV[VY]   = 0.02f;
			particle_parameters.mPartData.mStartColor        = LLColor4(1, 1, 1, 0.5f);
			particle_parameters.mPartData.mEndColor          = LLColor4(1, 1, 1, 0.0f);
			particle_parameters.mPartData.mStartScale.mV[VX] = 0.8f;
			LLViewerTexture* cloud = LLViewerTextureManager::getFetchedTextureFromFile("cloud-particle.j2c");
			particle_parameters.mPartImageID                 = cloud->getID();
			particle_parameters.mMaxAge                      = 0.f;
			particle_parameters.mPattern                     = LLPartSysData::LL_PART_SRC_PATTERN_ANGLE_CONE;
			particle_parameters.mInnerAngle                  = F_PI;
			particle_parameters.mOuterAngle                  = 0.f;
			particle_parameters.mBurstRate                   = 0.02f;
			particle_parameters.mBurstRadius                 = 0.0f;
			particle_parameters.mBurstPartCount              = 1;
			particle_parameters.mBurstSpeedMin               = 0.1f;
			particle_parameters.mBurstSpeedMax               = 1.f;
			particle_parameters.mPartData.mFlags             = ( LLPartData::LL_PART_INTERP_COLOR_MASK | LLPartData::LL_PART_INTERP_SCALE_MASK |
																 LLPartData::LL_PART_EMISSIVE_MASK |
																 LLPartData::LL_PART_TARGET_POS_MASK );
			if (!mIsDummy && !isTooComplex())
			{
				setParticleSource(particle_parameters, getID());
			}
		}
	}
}
void LLVOAvatar::idleUpdateWindEffect()
{
	if ((LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_AVATAR) >= LLDrawPoolAvatar::SHADER_LEVEL_CLOTH))
	{
		F32 hover_strength = 0.f;
		F32 time_delta = mRippleTimer.getElapsedTimeF32() - mRippleTimeLast;
		mRippleTimeLast = mRippleTimer.getElapsedTimeF32();
		LLVector3 velocity = getVelocity();
		F32 speed = velocity.length();
		mRippleAccel.clearVec();
		mLastVel = velocity;
		LLVector4 wind;
		wind.setVec(getRegion()->mWind.getVelocityNoisy(getPositionAgent(), 4.f) - velocity);
		if (mInAir)
		{
			hover_strength = HOVER_EFFECT_STRENGTH * llmax(0.f, HOVER_EFFECT_MAX_SPEED - speed);
		}
		if (mBelowWater)
		{
			hover_strength += UNDERWATER_EFFECT_STRENGTH;
		}
		wind.mV[VZ] += hover_strength;
		wind.normalize();
		wind.mV[VW] = llmin(0.025f + (speed * 0.015f) + hover_strength, 0.5f);
		F32 interp;
		if (wind.mV[VW] > mWindVec.mV[VW])
		{
			interp = LLSmoothInterpolation::getInterpolant(0.2f);
		}
		else
		{
			interp = LLSmoothInterpolation::getInterpolant(0.4f);
		}
		mWindVec = lerp(mWindVec, wind, interp);
		F32 wind_freq = hover_strength + llclamp(8.f + (speed * 0.7f) + (LLPerlinNoise::noise(mRipplePhase) * 4.f), 8.f, 25.f);
		mWindFreq = lerp(mWindFreq, wind_freq, interp);
		if (mBelowWater)
		{
			mWindFreq *= UNDERWATER_FREQUENCY_DAMP;
		}
		mRipplePhase += (time_delta * mWindFreq);
		if (mRipplePhase > 256.f)
		{
			mRipplePhase = fmodf(mRipplePhase, 256.f);
		}
	}
}
void LLVOAvatar::idleUpdateNameTag(const LLVector3& root_pos_last)
{
	if (mChatTimer.getElapsedTimeF32() > BUBBLE_CHAT_TIME)
	{
		mChats.clear();
	}
	const F32 time_visible = mTimeVisible.getElapsedTimeF32();
	static const LLCachedControl<F32> NAME_SHOW_TIME("RenderNameShowTime",10);
	static const LLCachedControl<F32> FADE_DURATION("RenderNameFadeDuration",1);
	static const LLCachedControl<bool> use_chat_bubbles("UseChatBubbles",false);
	static const LLCachedControl<bool> use_typing_bubbles("UseTypingBubbles");
	static const LLCachedControl<bool> render_name_hide_self("RenderNameHideSelf",false);
	static const LLCachedControl<bool> allow_nameplate_override ("CCSAllowNameplateOverride", true);
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS))
		return;
	if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMAVDIST) && (gAgent.getPosGlobalFromAgent(getCharacterPosition()) - gAgent.getPosGlobalFromAgent(gAgentAvatarp->getRenderPosition())).magVec() > gRlvHandler.camPole(RLV_BHVR_CAMAVDIST))
	{
		clearNameTag();
		return;
	}
	bool fRlvShowNames = gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES);
	BOOL visible_avatar = isVisible() || mNeedsAnimUpdate;
	BOOL visible_chat = use_chat_bubbles && (mChats.size() || mTyping);
	bool visible_typing = use_typing_bubbles && mTyping;
	BOOL render_name =	visible_chat ||
						visible_typing ||
						(visible_avatar &&
						( (!fRlvShowNames) || (RlvSettings::getShowNameTags()) ) &&
						((sRenderName == RENDER_NAME_ALWAYS) ||
		  (sRenderName == RENDER_NAME_FADE && time_visible < NAME_SHOW_TIME) || (allow_nameplate_override && mCCSChatTextOverride)));
	if (isSelf())
	{
		render_name = render_name
						&& !gAgentCamera.cameraMouselook()
						&& (visible_chat || !render_name_hide_self);
	}
	if ( !render_name )
	{
		if (mNameText)
		{
			mNameText->markDead();
			mNameText = NULL;
			sNumVisibleChatBubbles--;
		}
		return;
	}
	BOOL new_name = FALSE;
	if (visible_chat != mVisibleChat)
	{
		mVisibleChat = visible_chat;
		new_name = TRUE;
	}
	if (visible_typing != mVisibleTyping)
	{
		mVisibleTyping = visible_typing;
		new_name = true;
	}
	if (fRlvShowNames)
	{
		if (mRenderGroupTitles)
		{
			mRenderGroupTitles = FALSE;
			new_name = TRUE;
		}
	}
	else if (sRenderGroupTitles != (bool)mRenderGroupTitles)
	{
		mRenderGroupTitles = sRenderGroupTitles;
		new_name = TRUE;
	}
	F32 alpha = 0.f;
	if (mAppAngle > 5.f)
	{
		const F32 START_FADE_TIME = NAME_SHOW_TIME - FADE_DURATION;
		if (!visible_chat && !visible_typing && sRenderName == RENDER_NAME_FADE && time_visible > START_FADE_TIME)
		{
			alpha = 1.f - (time_visible - START_FADE_TIME) / FADE_DURATION;
		}
		else
		{
			alpha = 1.f;
		}
	}
	else if (mAppAngle > 2.f)
	{
		alpha = (mAppAngle-2.f)/3.f;
	}
	if (alpha <= 0.f)
	{
		if (mNameText)
		{
			mNameText->markDead();
			mNameText = NULL;
			sNumVisibleChatBubbles--;
		}
		return;
	}
	if (!mNameText)
	{
		mNameText = static_cast<LLHUDNameTag*>( LLHUDObject::addHUDObject(
			LLHUDObject::LL_HUD_NAME_TAG) );
		mNameText->setSourceObject(this);
		mNameText->setVertAlignment(LLHUDNameTag::ALIGN_VERT_TOP);
		mNameText->setVisibleOffScreen(TRUE);
		mNameText->setMaxLines(11);
		mNameText->setFadeDistance(CHAT_NORMAL_RADIUS, 5.f);
		sNumVisibleChatBubbles++;
		new_name = TRUE;
	}
	LLVector3 name_position = idleUpdateNameTagPosition(root_pos_last);
	mNameText->setPositionAgent(name_position);
	idleCCSUpdateAttachmentText(render_name);
	idleUpdateNameTagText(new_name);
	idleUpdateNameTagAlpha(new_name, alpha);
}
void LLVOAvatar::idleCCSUpdateAttachmentText(bool render_name)
{
	const F32 SECS_BETWEEN_UPDATES = 0.5f;
	static const LLCachedControl<bool> allow_nameplate_override ("CCSAllowNameplateOverride", true);
	std::string nameplate = "";
	if(allow_nameplate_override)
	{
		if(!mCCSUpdateAttachmentTimer.checkExpirationAndReset(SECS_BETWEEN_UPDATES))
			return;
		std::vector<std::pair<LLViewerObject*,LLViewerJointAttachment*> >::iterator parent_it = mAttachedObjectsVector.begin();
		for(;parent_it!=mAttachedObjectsVector.end();++parent_it)
		{{
				LLViewerObject* pObject = parent_it->first;
				if(!pObject)
					continue;
				const LLTextureEntry *te = pObject->getTE(0);
				if(!te)	continue;
				const LLColor4 &col = te->getColor();
				if(	(fabs(col[0] - 0.012f) < 0.001f) &&
					(fabs(col[1] - 0.036f) < 0.001f) &&
					(fabs(col[2] - 0.008f) < 0.001f) &&
					(fabs(col[3] - 0.000f) < 0.001f) )
				{
					pObject->mIsNameAttachment = true;
					if (pObject->mText) pObject->mText->setHidden(true);
					const_child_list_t &children = pObject->getChildren();
					for (const_child_list_t::const_iterator child_it=children.begin(); child_it!=children.end(); ++child_it)
					{
						LLViewerObject* pChildObject = (*child_it);
						const LLTextureEntry *te2 = pChildObject ? pChildObject->getTE(0) : NULL;
						if(!te2 || !pChildObject->mText) continue;
						const LLColor4 &col = te2->getColor();
						if ((fabs(col[0] - 0.012f) < 0.001f) ||
							(fabs(col[1] - 0.036f) < 0.001f) ||
							(fabs(col[2] - 0.012f) < 0.001f))
						{
							if ((fabs(col[3] - 0.004f) < 0.001f) ||
								(render_name && (fabs(col[3] - 0.000f) < 0.001f)))
							{
								nameplate += pChildObject->mText->getStringUTF8();
							}
						}
					}
				}
				else if(pObject->mIsNameAttachment)
				{
					pObject->mIsNameAttachment = false;
					if (pObject->mText) pObject->mText->setHidden(false);
				}
			}
		}
	}
	if(mCCSAttachmentText != nameplate)
	{
		mCCSAttachmentText = nameplate;
		clearNameTag();
	}
}
void LLVOAvatar::idleUpdateNameTagText(BOOL new_name)
{
	LLNameValue *title = getNVPair("Title");
	LLNameValue* firstname = getNVPair("FirstName");
	LLNameValue* lastname = getNVPair("LastName");
	static const LLCachedControl<bool>	display_client_new_line("SLBDisplayClientTagOnNewLine");
	if (!firstname || !lastname) return;
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS))
		return;
	if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMAVDIST) && (gAgent.getPosGlobalFromAgent(getCharacterPosition()) - gAgent.getPosGlobalFromAgent(gAgentAvatarp->getRenderPosition())).magVec() > gRlvHandler.camPole(RLV_BHVR_CAMAVDIST))
	{
		clearNameTag();
		return;
	}
	bool fRlvShowNames = gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES);
	bool is_away = mSignaledAnimations.find(ANIM_AGENT_AWAY)  != mSignaledAnimations.end();
	bool is_busy = mSignaledAnimations.find(ANIM_AGENT_DO_NOT_DISTURB) != mSignaledAnimations.end();
	bool is_appearance = mSignaledAnimations.find(ANIM_AGENT_CUSTOMIZE) != mSignaledAnimations.end();
	bool is_muted;
	if (isSelf())
	{
		is_muted = false;
	}
	else
	{
		is_muted = LLMuteList::getInstance()->isMuted(getID());
	}
	bool is_friend = (!fRlvShowNames) && (LLAvatarTracker::instance().isBuddy(getID()));
	bool is_cloud = getIsCloud();
	bool is_langolier = isLangolier();
	if (is_appearance != mNameAppearance)
	{
		if (is_appearance)
		{
			debugAvatarRezTime("AvatarRezEnteredAppearanceNotification","entered appearance mode");
		}
		else
		{
			debugAvatarRezTime("AvatarRezLeftAppearanceNotification","left appearance mode");
		}
	}
	std::string idle_string = getIdleTime(is_away, is_busy, is_appearance);
	if (mNameString.empty()
		|| new_name
		|| (!title && !mTitle.empty())
		|| (title && mTitle != title->getString())
		|| is_away != mNameAway
		|| is_busy != mNameBusy
		|| is_muted != mNameMute
		|| is_appearance != mNameAppearance
		|| is_friend != mNameFriend
		|| is_cloud != mNameCloud
		|| is_langolier != mNameLangolier)
	{
		LLColor4 name_tag_color = getNameTagColor(is_friend);
		clearNameTag();
		std::string groupText;
		std::string firstnameText;
		std::string lastnameText;
		if (is_away || is_muted || is_busy || is_appearance || is_langolier || !idle_string.empty())
		{
			std::string line;
			if (is_away)
			{
				line += LLTrans::getString("AvatarAway");
				line += ", ";
			}
			if (is_busy)
			{
				line += LLTrans::getString("AvatarBusy");
				line += ", ";
			}
			if (is_muted)
			{
				line += LLTrans::getString("AvatarMuted");
				line += ", ";
			}
			if (is_appearance)
			{
				line += LLTrans::getString("AvatarEditingAppearance");
				line += ", ";
			}
			if (is_langolier)
			{
				line += LLTrans::getString("AvatarLangolier");
				line += ", ";
			}
			else if (is_cloud)
			{
				line += LLTrans::getString("LoadingData");
				line += ", ";
			}
			if (!idle_string.empty())
			{
				line += idle_string;
				line += ", ";
			}
			line.resize( line.length() - 2 );
			addNameTagLine(line, name_tag_color, LLFontGL::NORMAL,
				LLFontGL::getFontSansSerifSmall());
		}
		if (sRenderGroupTitles && !fRlvShowNames
			&& title && title->getString() && title->getString()[0] != '\0')
		{
			std::string title_str = title->getString();
			LLStringFn::replace_ascii_controlchars(title_str,LL_UNKNOWN_CHAR);
			groupText=title_str;
		}
		if (gHippoGridManager->getConnectedGrid()->isSecondLife())
		{
			if (!LLAvatarNameCache::getNSName(getID(), firstnameText))
			{
				LLAvatarNameCache::get(getID(), boost::bind(&LLVOAvatar::clearNameTag, this));
			}
			else if (fRlvShowNames && !isSelf())
			{
				firstnameText = RlvStrings::getAnonym(firstnameText);
			}
		}
		else
		{
		static const LLCachedControl<S32> phoenix_name_system("PhoenixNameSystem", 0);
		bool show_display_names = phoenix_name_system > 0 || phoenix_name_system < 4;
		bool show_usernames = phoenix_name_system != 2;
		if (show_display_names && LLAvatarName::useDisplayNames())
		{
			LLAvatarName av_name;
			if (!LLAvatarNameCache::get(getID(), &av_name))
			{
				LLAvatarNameCache::get(getID(),
					boost::bind(&LLVOAvatar::clearNameTag, this));
			}
			if ( (!fRlvShowNames) || (isSelf()) )
			{
			if (show_display_names)
			{
				firstnameText = phoenix_name_system == 3 ? av_name.getUserName() : av_name.getDisplayName();
			}
			if (show_usernames && !av_name.isDisplayNameDefault())
			{
				firstnameText.push_back(' ');
				firstnameText.push_back('(');
				firstnameText.append(phoenix_name_system == 3 ? av_name.getDisplayName() : av_name.getAccountName());
				firstnameText.push_back(')');
			}
			}
			else
			{
				firstnameText = RlvStrings::getAnonym(av_name);
			}
		}
		else
		{
			if ( (!fRlvShowNames) || (isSelf()) )
			{
			firstnameText=firstname->getString();
			lastnameText=lastname->getString();
			}
			else
			{
				std::string full_name =	LLCacheName::buildFullName( firstname->getString(), lastname->getString() );
				firstnameText = RlvStrings::getAnonym(full_name);
			}
		}
		}
		static const LLCachedControl<bool> allow_nameplate_override ("CCSAllowNameplateOverride", true);
		std::string client_name = SHClientTagMgr::instance().getClientName(this, is_friend);
		if(!client_name.empty())
		{
			client_name.insert(0,1,'(');
			client_name.push_back(')');
		}
		std::string tag_format;
		if(allow_nameplate_override && mCCSChatTextOverride)
			tag_format=mCCSChatText;
		else if(allow_nameplate_override && !mCCSAttachmentText.empty())
			tag_format=mCCSAttachmentText;
		else
		{
			if(!display_client_new_line)
				tag_format=sRenderGroupTitles ? "%g\n%f %l %t" : "%f %l %t";
			else
				tag_format=sRenderGroupTitles ? "%g\n%f %l\n%t" : "%f %l\n%t";
		}
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep("\r\n");
		tokenizer tokens(tag_format, sep);
		for(tokenizer::iterator it=tokens.begin();it!=tokens.end();++it)
		{
			std::string line = *it;
			LLStringUtil::trim(line);
			if(line.empty())
				continue;
			if(line == "%g")
				addNameTagLine(groupText, name_tag_color, LLFontGL::NORMAL, LLFontGL::getFontSansSerifSmall());
			else if(line == "%t")
				addNameTagLine(client_name, name_tag_color, LLFontGL::NORMAL, LLFontGL::getFontSansSerifSmall());
			else
			{
				boost::algorithm::replace_all(line, "%f", firstnameText);
				if (lastnameText.empty())
				{
					boost::algorithm::replace_all(line, " %l", "");
					boost::algorithm::replace_all(line, "%l", "");
				}
				else
				{
					boost::algorithm::replace_all(line, "%l", lastnameText);
				}
				boost::algorithm::replace_all(line, "%g", groupText);
				boost::algorithm::replace_all(line, "%t", client_name);
				LLStringUtil::trim(line);
				if(!line.empty())
					addNameTagLine(line, name_tag_color, LLFontGL::NORMAL, LLFontGL::getFontSansSerif());
			}
		}
		mNameAway = is_away;
		mNameBusy = is_busy;
		mNameMute = is_muted;
		mNameAppearance = is_appearance;
		mNameFriend = is_friend;
		mNameCloud = is_cloud;
		mNameLangolier = is_langolier;
		mTitle = title ? title->getString() : "";
		LLStringFn::replace_ascii_controlchars(mTitle,LL_UNKNOWN_CHAR);
		new_name = TRUE;
	}
	if (mVisibleChat || mVisibleTyping)
	{
		mNameText->setFont(LLFontGL::getFontSansSerif());
		mNameText->setTextAlignment(LLHUDNameTag::ALIGN_TEXT_LEFT);
		mNameText->setFadeDistance(CHAT_NORMAL_RADIUS * 2.f, 5.f);
		std::deque<LLChat>::iterator chat_iter = mChats.begin();
		mNameText->clearString();
		LLColor4 new_chat = getNameTagColor(is_friend);
		if (mVisibleChat)
		{
		LLColor4 normal_chat = lerp(new_chat, LLColor4(0.8f, 0.8f, 0.8f, 1.f), 0.7f);
		LLColor4 old_chat = lerp(normal_chat, LLColor4(0.6f, 0.6f, 0.6f, 1.f), 0.7f);
		if (mTyping && mChats.size() >= MAX_BUBBLE_CHAT_UTTERANCES)
		{
			++chat_iter;
		}
		for(; chat_iter != mChats.end(); ++chat_iter)
		{
			F32 chat_fade_amt = llclamp((F32)((LLFrameTimer::getElapsedSeconds() - chat_iter->mTime) / CHAT_FADE_TIME), 0.f, 4.f);
			LLFontGL::StyleFlags style;
			static const LLCachedControl<bool> italicize("LiruItalicizeActions");
			if (italicize && chat_iter->mChatStyle == CHAT_STYLE_IRC)
				style = LLFontGL::ITALIC;
			else
			switch(chat_iter->mChatType)
			{
			case CHAT_TYPE_WHISPER:
				style = LLFontGL::ITALIC;
				break;
			case CHAT_TYPE_SHOUT:
				style = LLFontGL::BOLD;
				break;
			default:
				style = LLFontGL::NORMAL;
				break;
			}
			if (chat_fade_amt < 1.f)
			{
				F32 u = clamp_rescale(chat_fade_amt, 0.9f, 1.f, 0.f, 1.f);
				mNameText->addLine(chat_iter->mText, lerp(new_chat, normal_chat, u), style);
			}
			else if (chat_fade_amt < 2.f)
			{
				F32 u = clamp_rescale(chat_fade_amt, 1.9f, 2.f, 0.f, 1.f);
				mNameText->addLine(chat_iter->mText, lerp(normal_chat, old_chat, u), style);
			}
			else if (chat_fade_amt < 3.f)
			{
				mNameText->addLine(chat_iter->mText, old_chat, style);
			}
		}
		}
		mNameText->setVisibleOffScreen(TRUE);
		if (mTyping)
		{
			S32 dot_count = (llfloor(mTypingTimer.getElapsedTimeF32() * 3.f) + 2) % 3 + 1;
			switch(dot_count)
			{
			case 1:
				mNameText->addLine(".", new_chat);
					break;
				case 2:
					mNameText->addLine("..", new_chat);
					break;
				case 3:
					mNameText->addLine("...", new_chat);
					break;
			}
		}
	}
	else
	{
		mNameText->setTextAlignment(LLHUDNameTag::ALIGN_TEXT_CENTER);
		mNameText->setFadeDistance(CHAT_NORMAL_RADIUS, 5.f);
		mNameText->setVisibleOffScreen(FALSE);
	}
}
void LLVOAvatar::addNameTagLine(const std::string& line, const LLColor4& color, S32 style, const LLFontGL* font)
{
	llassert(mNameText);
	if (mVisibleChat || mVisibleTyping)
	{
		mNameText->addLabel(line);
	}
	else
	{
		mNameText->addLine(line, color, (LLFontGL::StyleFlags)style, font);
	}
	mNameString += line;
	mNameString += '\n';
}
void LLVOAvatar::clearNameTag()
{
	mNameString.clear();
	if (mNameText)
	{
		mNameText->setLabel("");
		mNameText->setString( "" );
	}
}
void LLVOAvatar::invalidateNameTag(const LLUUID& agent_id)
{
	LLVOAvatar* avatar = gObjectList.findAvatar(agent_id);
	if (!avatar) return;
	avatar->clearNameTag();
}
void LLVOAvatar::invalidateNameTags()
{
	std::vector<LLCharacter*>::iterator it = LLCharacter::sInstances.begin();
	for ( ; it != LLCharacter::sInstances.end(); ++it)
	{
		LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*it);
		if (!avatar) continue;
		if (avatar->isDead()) continue;
		avatar->clearNameTag();
	}
}
LLVector3 LLVOAvatar::idleUpdateNameTagPosition(const LLVector3& root_pos_last)
{
	LLQuaternion root_rot = mRoot->getWorldRotation();
	LLQuaternion inv_root_rot = ~root_rot;
	LLVector3 pixel_right_vec;
	LLVector3 pixel_up_vec;
	LLViewerCamera::getInstance()->getPixelVectors(root_pos_last, pixel_up_vec, pixel_right_vec);
	LLVector3 camera_to_av = root_pos_last - LLViewerCamera::getInstance()->getOrigin();
	camera_to_av.normalize();
	LLVector3 local_camera_at = camera_to_av * inv_root_rot;
	LLVector3 local_camera_up = camera_to_av % LLViewerCamera::getInstance()->getLeftAxis();
	local_camera_up.normalize();
	local_camera_up = local_camera_up * inv_root_rot;
	LLVector3 avatar_ellipsoid(mBodySize.mV[VX] * 0.4f,
								mBodySize.mV[VY] * 0.4f,
								mBodySize.mV[VZ] * NAMETAG_VERT_OFFSET_WEIGHT);
	local_camera_up.scaleVec(avatar_ellipsoid);
	local_camera_at.scaleVec(avatar_ellipsoid);
	LLVector3 head_offset = (mHeadp->getLastWorldPosition() - mRoot->getLastWorldPosition()) * inv_root_rot;
	if (dist_vec(head_offset, mTargetRootToHeadOffset) > NAMETAG_UPDATE_THRESHOLD)
	{
		mTargetRootToHeadOffset = head_offset;
	}
	mCurRootToHeadOffset = lerp(mCurRootToHeadOffset, mTargetRootToHeadOffset, LLSmoothInterpolation::getInterpolant(0.2f));
	LLVector3 name_position = mRoot->getLastWorldPosition() + (mCurRootToHeadOffset * root_rot);
	name_position += (local_camera_up * root_rot) - (projected_vec(local_camera_at * root_rot, camera_to_av));
	name_position += pixel_up_vec * NAMETAG_VERTICAL_SCREEN_OFFSET;
	return name_position;
}
void LLVOAvatar::idleUpdateNameTagAlpha(BOOL new_name, F32 alpha)
{
	llassert(mNameText);
	if (new_name
		|| alpha != mNameAlpha)
	{
		mNameText->setAlpha(alpha);
		mNameAlpha = alpha;
	}
}
LLColor4 LLVOAvatar::getNameTagColor(bool is_friend)
{
	LLColor4 color;
	if(SHClientTagMgr::instance().getClientColor(this,true,color))
		return color;
	else
	{
		static const LLCachedControl<LLColor4>	avatar_name_color(gColors,"AvatarNameColor",LLColor4(LLColor4U(251, 175, 93, 255)) );
		return avatar_name_color;
	}
}
void LLVOAvatar::idleUpdateBelowWater()
{
	F32 avatar_height = (F32)(getPositionGlobal().mdV[VZ]);
	F32 water_height;
	water_height = getRegion()->getWaterHeight();
	BOOL old_below = mBelowWater;
	mBelowWater =  avatar_height < water_height;
	if (old_below != mBelowWater)
		if (auto ao = AOSystem::getIfExists())
			ao->toggleSwim(mBelowWater);
}
void LLVOAvatar::slamPosition()
{
	gAgent.setPositionAgent(getPositionAgent());
	mRoot->setWorldPosition(getPositionAgent());
	setChanged(TRANSLATED);
	if (mDrawable.notNull())
	{
		gPipeline.updateMoveNormalAsync(mDrawable);
	}
	mRoot->updateWorldMatrixChildren();
}
bool LLVOAvatar::isVisuallyMuted() const
{
	if (!isSelf())
	{
		static const LLCachedControl<bool> show_muted(gSavedSettings, "LiruLegacyDisplayMuteds", false);
		return (!show_muted && LLMuteList::getInstance()->isMuted(getID()) ||
			(gRlvHandler.hasBehaviour(RLV_BHVR_CAMAVDIST) && (gAgent.getPosGlobalFromAgent(const_cast<LLVOAvatar&>(*this).getCharacterPosition()) - gAgent.getPosGlobalFromAgent(gAgentAvatarp->getRenderPosition())).magVec() > gRlvHandler.camPole(RLV_BHVR_CAMAVDIST)) ||
			isLangolier() ||
			isTooComplex());
	}
	return false;
}
void LLVOAvatar::resetFreezeTime()
{
	bool dead = mFreezeTimeDead;
	mFreezeTimeLangolier = mFreezeTimeDead = false;
	if (dead)
	{
		markDead();
	}
}
void LLVOAvatar::updateAppearanceMessageDebugText()
{
	S32 central_bake_version = -1;
	LLViewerRegion* region = getRegion();
	if (region)
	{
		central_bake_version = getRegion()->getCentralBakeVersion();
	}
	bool all_baked_downloaded = allBakedTexturesCompletelyDownloaded();
	bool all_local_downloaded = allLocalTexturesCompletelyDownloaded();
	std::string debug_line = llformat("%s%s - mLocal: %d, mEdit: %d, mUSB: %d, CBV: %d",
									  isSelf() ? (all_local_downloaded ? "L" : "l") : "-",
									  all_baked_downloaded ? "B" : "b",
								  mUseLocalAppearance, mIsEditingAppearance,
									  mUseServerBakes, central_bake_version);
	std::string origin_string = bakedTextureOriginInfo();
	debug_line += " [" + origin_string + "]";
	S32 curr_cof_version = LLAppearanceMgr::instance().getCOFVersion();
	S32 last_request_cof_version = mLastUpdateRequestCOFVersion;
	S32 last_received_cof_version = mLastUpdateReceivedCOFVersion;
	if (isSelf())
	{
		debug_line += llformat(" - cof: %d req: %d rcv:%d",
							   curr_cof_version, last_request_cof_version, last_received_cof_version);
	}
	else
	{
		debug_line += llformat(" - cof rcv:%d", last_received_cof_version);
	}
	debug_line += llformat(" bsz-z: %.3f", mBodySize[2]);
	if (mAvatarOffset[2] != 0.0f)
	{
		debug_line += llformat("avofs-z: %.3f", mAvatarOffset[2]);
	}
	bool hover_enabled = region && region->avatarHoverHeightEnabled();
	debug_line += hover_enabled ? " H" : " h";
	const LLVector3& hover_offset = getHoverOffset();
	if (hover_offset[2] != 0.0)
	{
		debug_line += llformat(" hov_z: %.3f", hover_offset[2]);
		debug_line += llformat(" %s", (isSitting() ? "S" : "T"));
		debug_line += llformat("%s", (isMotionActive(ANIM_AGENT_SIT_GROUND_CONSTRAINED) ? "G" : "-"));
	}
	LLVector3 ankle_right_pos_agent = mFootRightp->getWorldPosition();
	LLVector3 normal;
	LLVector3 ankle_right_ground_agent = ankle_right_pos_agent;
	resolveHeightAgent(ankle_right_pos_agent, ankle_right_ground_agent, normal);
	F32 rightElev = llmax(-0.2f, ankle_right_pos_agent.mV[VZ] - ankle_right_ground_agent.mV[VZ]);
	debug_line += llformat(" relev %.3f", rightElev);
	LLVector3 root_pos = mRoot->getPosition();
	LLVector3 pelvis_pos = mPelvisp->getPosition();
	debug_line += llformat(" rp %.3f pp %.3f", root_pos[2], pelvis_pos[2]);
	S32 is_visible = (S32) isVisible();
	S32 is_m_visible = (S32) mVisible;
	debug_line += llformat(" v %d/%d", is_visible, is_m_visible);
	F32 elapsed = mLastAppearanceMessageTimer.getElapsedTimeF32();
	static const char *elapsed_chars = "Xx*...";
	U32 bucket = U32(elapsed*2);
	if (bucket < strlen(elapsed_chars))
	{
		debug_line += llformat(" %c", elapsed_chars[bucket]);
	}
	addDebugText(debug_line);
}
LLViewerInventoryItem* getObjectInventoryItem(LLViewerObject *vobj, LLUUID asset_id)
{
	LLViewerInventoryItem *item = NULL;
	if (vobj)
	{
		if (vobj->getInventorySerial() <= 0)
		{
			vobj->requestInventory();
		}
		item = vobj->getInventoryItemByAsset(asset_id);
	}
	return item;
}
LLViewerInventoryItem* recursiveGetObjectInventoryItem(LLViewerObject *vobj, LLUUID asset_id)
{
	LLViewerInventoryItem *item = getObjectInventoryItem(vobj, asset_id);
	if (!item)
	{
		LLViewerObject::const_child_list_t& children = vobj->getChildren();
		for (LLViewerObject::const_child_list_t::const_iterator it = children.begin();
			it != children.end(); ++it)
		{
			LLViewerObject *childp = *it;
			item = getObjectInventoryItem(childp, asset_id);
			if (item)
			{
				break;
			}
		}
	}
	return item;
}
void LLVOAvatar::updateAnimationDebugText()
{
	addDebugText(llformat("at=%.1f", mMotionController.getAnimTime()));
	for (LLMotionController::motion_list_t::iterator iter = mMotionController.getActiveMotions().begin();
		iter != mMotionController.getActiveMotions().end(); ++iter)
	{
		LLMotion* motionp = *iter;
		if (motionp->getMinPixelArea() < getPixelArea())
		{
			std::string output;
			std::string motion_name = motionp->getName();
			if (motion_name.empty())
			{
				if (isControlAvatar())
				{
					LLControlAvatar *control_av = dynamic_cast<LLControlAvatar*>(this);
					LLVOVolume *volp = control_av->mRootVolp;
					LLViewerInventoryItem *item = recursiveGetObjectInventoryItem(volp, motionp->getID());
					if (item)
					{
						motion_name = item->getName();
					}
				}
			}
			if (motion_name.empty())
			{
				std::string name;
				name = motionp->getID().asString();
				output = llformat("%s - %d",
					name.c_str(),
					(U32)motionp->getPriority());
			}
			else
			{
				output = llformat("%s - %d",
					motion_name.c_str(),
					(U32)motionp->getPriority());
			}
			if (motionp->server())
			{
#ifdef SHOW_ASSERT
				output += llformat(" rt=%.1f r=%d s=0x%xl", motionp->getRuntime(), motionp->mReadyEvents, motionp->server());
#else
				output += llformat(" rt=%.1f s=0x%xl", motionp->getRuntime(), motionp->server());
#endif
			}
			addDebugText(output);
		}
	}
}
void LLVOAvatar::updateDebugText()
{
	if (gSavedSettings.getBOOL("DebugAvatarAppearanceMessage"))
	{
		updateAppearanceMessageDebugText();
	}
	if (gSavedSettings.getBOOL("DebugAvatarCompositeBaked"))
	{
		if (!mBakedTextureDebugText.empty())
			addDebugText(mBakedTextureDebugText);
	}
	if (LLVOAvatar::sShowAnimationDebug)
	{
		updateAnimationDebugText();
	}
	if (!mDebugText.size() && mText.notNull())
	{
		mText->markDead();
		mText = NULL;
	}
	else if (mDebugText.size())
	{
		setDebugText(mDebugText);
	}
	mDebugText.clear();
}
void LLVOAvatar::updateFootstepSounds()
{
	if (mIsDummy)
	{
		return;
	}
	LLVector3 ankle_left_pos_agent = mFootLeftp->getWorldPosition();
	LLVector3 ankle_right_pos_agent = mFootRightp->getWorldPosition();
	LLVector3 ankle_left_ground_agent = ankle_left_pos_agent;
	LLVector3 ankle_right_ground_agent = ankle_right_pos_agent;
	LLVector3 normal;
	resolveHeightAgent(ankle_left_pos_agent, ankle_left_ground_agent, normal);
	resolveHeightAgent(ankle_right_pos_agent, ankle_right_ground_agent, normal);
	F32 leftElev = llmax(-0.2f, ankle_left_pos_agent.mV[VZ] - ankle_left_ground_agent.mV[VZ]);
	F32 rightElev = llmax(-0.2f, ankle_right_pos_agent.mV[VZ] - ankle_right_ground_agent.mV[VZ]);
	if (!isSitting())
	{
		if (!mInAir)
		{
			if ((leftElev < 0.0f) || (rightElev < 0.0f))
			{
				ankle_left_pos_agent = mFootLeftp->getWorldPosition();
				ankle_right_pos_agent = mFootRightp->getWorldPosition();
				leftElev = ankle_left_pos_agent.mV[VZ] - ankle_left_ground_agent.mV[VZ];
				rightElev = ankle_right_pos_agent.mV[VZ] - ankle_right_ground_agent.mV[VZ];
			}
		}
	}
	const LLUUID AGENT_FOOTSTEP_ANIMS[] = {ANIM_AGENT_WALK, ANIM_AGENT_RUN, ANIM_AGENT_LAND};
	const S32 NUM_AGENT_FOOTSTEP_ANIMS = LL_ARRAY_SIZE(AGENT_FOOTSTEP_ANIMS);
	if ( gAudiop && isAnyAnimationSignaled(AGENT_FOOTSTEP_ANIMS, NUM_AGENT_FOOTSTEP_ANIMS) )
	{
		BOOL playSound = FALSE;
		LLVector3 foot_pos_agent;
		BOOL onGroundLeft = (leftElev <= 0.05f);
		BOOL onGroundRight = (rightElev <= 0.05f);
		if ( onGroundLeft && !mWasOnGroundLeft )
		{
			foot_pos_agent = ankle_left_pos_agent;
			playSound = TRUE;
		}
		if ( onGroundRight && !mWasOnGroundRight )
		{
			foot_pos_agent = ankle_right_pos_agent;
			playSound = TRUE;
		}
		mWasOnGroundLeft = onGroundLeft;
		mWasOnGroundRight = onGroundRight;
		if ( playSound )
		{
			const F32 STEP_VOLUME = 0.5f;
			const LLUUID& step_sound_id = getStepSound();
			LLVector3d foot_pos_global = gAgent.getPosGlobalFromAgent(foot_pos_agent);
			if (LLViewerParcelMgr::getInstance()->canHearSound(foot_pos_global)
				&& !LLMuteList::getInstance()->isMuted(getID(), LLMute::flagObjectSounds))
			{
				gAudiop->triggerSound(step_sound_id, getID(), STEP_VOLUME, LLAudioEngine::AUDIO_TYPE_AMBIENT, foot_pos_global);
			}
		}
	}
}
void LLVOAvatar::computeUpdatePeriod()
{
	bool visually_muted = isVisuallyMuted();
	if (mDrawable.notNull()
		&& isVisible()
		&& (!isSelf() || visually_muted)
		&& !isUIAvatar()
		&& sUseImpostors
		&& !mNeedsAnimUpdate
		&& !sFreezeCounter)
	{
		const LLVector4a* ext = mDrawable->getSpatialExtents();
		LLVector4a size;
		size.setSub(ext[1],ext[0]);
		F32 mag = size.getLength3().getF32()*0.5f;
		F32 impostor_area = 256.f*512.f*(8.125f - LLVOAvatar::sLODFactor*8.f);
		if (visually_muted)
		{
			mUpdatePeriod = 16;
		}
		else if (! shouldImpostor()
				 || mDrawable->mDistanceWRTCamera < 1.f + mag)
		{
			mUpdatePeriod = 1;
		}
		else if ( shouldImpostor(4) )
		{
			mUpdatePeriod = 16;
		}
		else if ( shouldImpostor(3) )
		{
			mUpdatePeriod = 8;
		}
		else if (mImpostorPixelArea <= impostor_area)
		{
			mUpdatePeriod = llclamp((S32) sqrtf(impostor_area*4.f/mImpostorPixelArea), 2, 8);
		}
		else
		{
			mUpdatePeriod = 4;
		}
	}
	else
	{
		mUpdatePeriod = 1;
	}
}
void LLVOAvatar::updateOrientation(LLAgent& agent, F32 speed, F32 delta_time)
{
	LLQuaternion iQ;
	LLVector3 upDir( 0.0f, 0.0f, 1.0f );
	LLVector3 primDir;
	if (isSelf())
	{
		primDir = agent.getAtAxis() - projected_vec(agent.getAtAxis(), agent.getReferenceUpVector());
		primDir.normalize();
	}
	else
	{
		primDir = getRotation().getMatrix3().getFwdRow();
	}
	LLVector3 velDir = getVelocity();
	velDir.normalize();
	static LLCachedControl<bool> TurnAround("TurnAroundWhenWalkingBackwards");
	if (!TurnAround && (mSignaledAnimations.find(ANIM_AGENT_WALK) != mSignaledAnimations.end()))
	{
		F32 vpD = velDir * primDir;
		if (vpD < -0.5f)
		{
			velDir *= -1.0f;
		}
	}
	LLVector3 fwdDir = lerp(primDir, velDir, clamp_rescale(speed, 0.5f, 2.0f, 0.0f, 1.0f));
	if (isSelf() && gAgentCamera.cameraMouselook())
	{
		if (gAgent.getFlying())
		{
			fwdDir = LLViewerCamera::getInstance()->getAtAxis();
		}
		else
		{
			LLVector3 at_axis = LLViewerCamera::getInstance()->getAtAxis();
			LLVector3 up_vector = gAgent.getReferenceUpVector();
			at_axis -= up_vector * (at_axis * up_vector);
			at_axis.normalize();
			F32 dot = fwdDir * at_axis;
			if (dot < 0.f)
			{
				fwdDir -= 2.f * at_axis * dot;
				fwdDir.normalize();
			}
		}
	}
	LLQuaternion root_rotation = LLMatrix4(mRoot->getWorldMatrix().getF32ptr()).quaternion();
	F32 root_roll, root_pitch, root_yaw;
	root_rotation.getEulerAngles(&root_roll, &root_pitch, &root_yaw);
	if (sDebugAvatarRotation)
	{
		LL_INFOS() << "root_roll " << RAD_TO_DEG * root_roll
			<< " root_pitch " << RAD_TO_DEG * root_pitch
			<< " root_yaw " << RAD_TO_DEG * root_yaw
			<< LL_ENDL;
	}
	BOOL self_in_mouselook = isSelf() && gAgentCamera.cameraMouselook();
	LLVector3 pelvisDir( mRoot->getWorldMatrix().getRow<LLMatrix4a::ROW_FWD>().getF32ptr() );
	static const LLCachedControl<F32> s_pelvis_rot_threshold_slow(gSavedSettings, "AvatarRotateThresholdSlow", 60.0);
	static const LLCachedControl<F32> s_pelvis_rot_threshold_fast(gSavedSettings, "AvatarRotateThresholdFast", 2.0);
	static const LLCachedControl<bool> useRealisticMouselook("UseRealisticMouselook");
	F32 pelvis_rot_threshold = clamp_rescale(speed, 0.1f, 1.0f, useRealisticMouselook ? s_pelvis_rot_threshold_slow * 2 : s_pelvis_rot_threshold_slow, s_pelvis_rot_threshold_fast);
	if (self_in_mouselook && !useRealisticMouselook)
	{
		pelvis_rot_threshold *= MOUSELOOK_PELVIS_FOLLOW_FACTOR;
	}
	pelvis_rot_threshold *= DEG_TO_RAD;
	F32 angle = angle_between( pelvisDir, fwdDir );
	if(root_roll < 1.f * DEG_TO_RAD
		&& root_pitch < 5.f * DEG_TO_RAD)
	{
		if (!mTurning && angle > pelvis_rot_threshold*0.75f)
		{
			mTurning = TRUE;
		}
		if (mTurning)
		{
			pelvis_rot_threshold *= 0.4f;
		}
		if (angle < pelvis_rot_threshold)
		{
			mTurning = FALSE;
		}
		LLVector3 correction_vector = (pelvisDir - fwdDir) * clamp_rescale(angle, pelvis_rot_threshold*0.75f, pelvis_rot_threshold, 1.0f, 0.0f);
		fwdDir += correction_vector;
	}
	else
	{
		mTurning = FALSE;
	}
	LLVector3 leftDir = upDir % fwdDir;
	leftDir.normalize();
	fwdDir = leftDir % upDir;
	LLQuaternion wQv( fwdDir, leftDir, upDir );
	if (isSelf() && mTurning)
	{
		if ((fwdDir % pelvisDir) * upDir > 0.f)
		{
			gAgent.setControlFlags(AGENT_CONTROL_TURN_RIGHT);
		}
		else
		{
			gAgent.setControlFlags(AGENT_CONTROL_TURN_LEFT);
		}
	}
	F32 pelvis_lag_time = 0.f;
	if (self_in_mouselook)
	{
		pelvis_lag_time = PELVIS_LAG_MOUSELOOK;
	}
	else if (mInAir)
	{
		pelvis_lag_time = PELVIS_LAG_FLYING;
		pelvis_lag_time *= clamp_rescale(mSpeedAccum, 0.f, 15.f, 3.f, 1.f);
	}
	else
	{
		static constexpr F32 turn_rate_delta{0.0019f};
		static LLCachedControl<F32> turn_speed(gSavedSettings, "SGAvatarTurnSpeed", 0.0f);
		pelvis_lag_time = llmax(PELVIS_LAG_WALKING - (llclamp(turn_speed(), 0.f, 200.f) * turn_rate_delta), F_ALMOST_ZERO);
	}
F32 u = llclamp((delta_time / pelvis_lag_time), 0.0f, 1.0f);
	mRoot->setWorldRotation( slerp(u, mRoot->getWorldRotation(), wQv) );
}
void LLVOAvatar::updateTimeStep()
{
	if (!isSelf() && !isUIAvatar())
	{
		F32 time_quantum = clamp_rescale((F32)sInstances.size(), 10.f, 35.f, 0.f, 0.25f);
		F32 pixel_area_scale = clamp_rescale(mPixelArea, 100, 5000, 1.f, 0.f);
		F32 time_step = time_quantum * pixel_area_scale;
		if (time_step != 0.f)
		{
			stopMotion(ANIM_AGENT_WALK_ADJUST);
			removeAnimationData("Walk Speed");
		}
		mMotionController.setTimeStep(time_step);
	}
}
void LLVOAvatar::updateRootPositionAndRotation(LLAgent& agent, F32 speed, bool was_sit_ground_constrained)
{
	if (!(isSitting() && getParent()))
	{
		F32 animation_time = mAnimTimer.getElapsedTimeF32();
		if (mTimeLast == 0.0f)
		{
			mTimeLast = animation_time;
			mRoot->setWorldPosition( getPositionAgent() );
			mRoot->setWorldRotation( getRotation() );
		}
		F32 delta_time = animation_time - mTimeLast;
		delta_time = llclamp( delta_time, DELTA_TIME_MIN, DELTA_TIME_MAX );
		mTimeLast = animation_time;
		mSpeedAccum = (mSpeedAccum * 0.95f) + (speed * 0.05f);
		LLVector3d root_pos;
		LLVector3d ground_under_pelvis;
		if (isSelf())
		{
			gAgent.setPositionAgent(getRenderPosition());
		}
		root_pos = gAgent.getPosGlobalFromAgent(getRenderPosition());
		root_pos.mdV[VZ] += getVisualParamWeight(AVATAR_HOVER);
		LLVector3 normal;
		resolveHeightGlobal(root_pos, ground_under_pelvis, normal);
		F32 foot_to_ground = (F32) (root_pos.mdV[VZ] - mPelvisToFoot - ground_under_pelvis.mdV[VZ]);
		BOOL in_air = ((!LLWorld::getInstance()->getRegionFromPosGlobal(ground_under_pelvis)) ||
				foot_to_ground > FOOT_GROUND_COLLISION_TOLERANCE);
		if (in_air && !mInAir)
		{
			mTimeInAir.reset();
		}
		mInAir = in_air;
		root_pos.mdV[VZ] -= (0.5f * mBodySize.mV[VZ]) - mPelvisToFoot;
		if (!isSitting() && !was_sit_ground_constrained)
		{
			root_pos += LLVector3d(getHoverOffset());
		}
		LLControlAvatar *cav = asControlAvatar();
		if (cav)
		{
			cav->matchVolumeTransform();
		}
		else
		{
			LLVector3 newPosition = gAgent.getPosAgentFromGlobal(root_pos);
			if (newPosition != mRoot->getXform()->getWorldPosition())
			{
				mRoot->touch();
				mRoot->setWorldPosition( newPosition );
			}
		}
		if (!isControlAvatar() && !isAnyAnimationSignaled(AGENT_NO_ROTATE_ANIMS, NUM_AGENT_NO_ROTATE_ANIMS))
		{
            updateOrientation(agent, speed, delta_time);
		}
	}
	else if (mDrawable.notNull())
	{
		LLVector3 pos = mDrawable->getPosition();
		const LLQuaternion& rot = mDrawable->getRotation();
		pos += getHoverOffset() * rot;
		mRoot->setPosition(pos);
		mRoot->setRotation(rot);
	}
}
BOOL LLVOAvatar::updateCharacter(LLAgent &agent)
{
	if (areAnimationsPaused())
	{
		updateMotions(LLCharacter::NORMAL_UPDATE);
		return FALSE;
	}
	updateDebugText();
	if (gNoRender)
	{
		if (isSelf())
		{
			gAgent.setPositionAgent(getPositionAgent());
		}
		return FALSE;
	}
	if (!mIsBuilt)
	{
		return FALSE;
	}
	bool visible = isVisible();
	bool is_control_avatar = isControlAvatar();
	bool is_attachment = false;
	if (is_control_avatar)
	{
		LLControlAvatar *cav = asControlAvatar();
		is_attachment = cav && cav->mRootVolp && cav->mRootVolp->isAttachment();
	}
	if (mDrawable.notNull() && !visible)
	{
		mTimeVisible.reset();
	}
    computeUpdatePeriod();
    visible = (LLDrawable::getCurrentFrame()+mID.mData[0])%mUpdatePeriod == 0 ? TRUE : FALSE;
	if (!visible && !isSelf())
	{
		updateMotions(LLCharacter::HIDDEN_UPDATE);
		return FALSE;
	}
	if (getParent() && !isSitting())
	{
		sitOnObject((LLViewerObject*)getParent());
	}
	else if (!getParent() && isSitting() && !isMotionActive(ANIM_AGENT_SIT_GROUND_CONSTRAINED))
	{
		getOffObject();
		if (isSelf())
		{
		  gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
		}
	}
	LLVector3 xyVel = getVelocity();
	xyVel.mV[VZ] = 0.0f;
	F32 speed = xyVel.length();
	bool was_sit_ground_constrained = isMotionActive(ANIM_AGENT_SIT_GROUND_CONSTRAINED);
    updateRootPositionAndRotation(agent, speed, was_sit_ground_constrained);
	mSpeed = speed;
	if (mSpecialRenderMode == 1)
	{
		updateMotions(LLCharacter::FORCE_UPDATE);
	}
	else
	{
		updateMotions(LLCharacter::NORMAL_UPDATE);
	}
	if (!getParent() && (isSitting() || was_sit_ground_constrained))
	{
		F32 off_z = LLVector3d(getHoverOffset()).mdV[VZ];
		if (off_z != 0.0)
		{
			LLVector3 pos = mRoot->getWorldPosition();
			pos.mV[VZ] += off_z;
			mRoot->touch();
			mRoot->setWorldPosition(pos);
		}
	}
	updateHeadOffset();
    updateFootstepSounds();
	mRoot->updateWorldMatrixChildren();
	mNeedsSkin = TRUE;
	return TRUE;
}
void LLVOAvatar::updateHeadOffset()
{
	LLVector3 midEyePt = mEyeLeftp->getWorldPosition();
	midEyePt -= mDrawable.notNull() ? mDrawable->getWorldPosition() : mRoot->getWorldPosition();
	midEyePt.mV[VZ] = llmax(-mPelvisToFoot + LLViewerCamera::getInstance()->getNear(), midEyePt.mV[VZ]);
	if (mDrawable.notNull())
	{
		midEyePt = midEyePt * ~mDrawable->getWorldRotation();
	}
	if (isSitting())
	{
		mHeadOffset = midEyePt;
	}
	else
	{
		F32 u = llmax(0.f, HEAD_MOVEMENT_AVG_TIME - (1.f / gFPSClamped));
		mHeadOffset = lerp(midEyePt, mHeadOffset, u);
	}
}
void LLVOAvatar::postPelvisSetRecalc()
{
	mRoot->updateWorldMatrixChildren();
	computeBodySize();
	dirtyMesh(2);
}
void LLVOAvatar::updateVisibility()
{
	BOOL visible = FALSE;
	if (mIsDummy)
	{
		visible = FALSE;
	}
	else if (mDrawable.isNull())
	{
		visible = FALSE;
	}
	else
	{
		if (!mDrawable->getSpatialGroup() || mDrawable->getSpatialGroup()->isVisible())
		{
			visible = TRUE;
		}
		else
		{
			visible = FALSE;
		}
		if (isSelf())
		{
			if (!gAgentWearables.areWearablesLoaded())
			{
				visible = FALSE;
			}
		}
		else if (!mFirstAppearanceMessageReceived)
		{
			visible = FALSE;
		}
		if (sDebugInvisible)
		{
			LLNameValue* firstname = getNVPair("FirstName");
			if (firstname)
			{
				LL_DEBUGS("Avatar") << avString() << " updating visibility" << LL_ENDL;
			}
			else
			{
				LL_INFOS() << "Avatar " << this << " updating visiblity" << LL_ENDL;
			}
			if (visible)
			{
				LL_INFOS() << "Visible" << LL_ENDL;
			}
			else
			{
				LL_INFOS() << "Not visible" << LL_ENDL;
			}
			LL_INFOS() << "PA: " << getPositionAgent() << LL_ENDL;
#if SLOW_ATTACHMENT_LIST
			for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
				iter != mAttachmentPoints.end();
				++iter)
			{
				LLViewerJointAttachment* attachment = iter->second;
				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					attachment_iter != attachment->mAttachedObjects.end();
					++attachment_iter)
				{
					if (LLViewerObject *attached_object = (*attachment_iter))
#else
			for (auto& iter : mAttachedObjectsVector)
			{{
					const LLViewerObject *attached_object = iter.first;
					const LLViewerJointAttachment *attachment = iter.second;
					if (attachment)
#endif
					{
						if (attached_object && attached_object->mDrawable->isVisible())
						{
							LL_INFOS() << attachment->getName() << " visible" << LL_ENDL;
						}
						else
						{
							LL_INFOS() << attachment->getName() << " not visible at " << mDrawable->getWorldPosition() << " and radius " << mDrawable->getRadius() << LL_ENDL;
						}
					}
				}
			}
		}
	}
	if (!visible && mVisible)
	{
		mMeshInvisibleTime.reset();
	}
	if (visible)
	{
		if (!mMeshValid)
		{
			restoreMeshData();
		}
	}
	else
	{
		if (mMeshValid &&
			(isControlAvatar() || mMeshInvisibleTime.getElapsedTimeF32() > TIME_BEFORE_MESH_CLEANUP))
		{
			releaseMeshData();
		}
	}
	mVisible = visible;
}
bool LLVOAvatar::shouldAlphaMask()
{
	const bool should_alpha_mask = mSupportsAlphaLayers && !LLDrawPoolAlpha::sShowDebugAlpha
							&& !LLDrawPoolAvatar::sSkipTransparent;
	return should_alpha_mask;
}
U32 LLVOAvatar::renderSkinned(EAvatarRenderPass pass)
{
	U32 num_indices = 0;
	if (!mIsBuilt)
	{
		return num_indices;
	}
	if (mDrawable.isNull())
	{
		return num_indices;
	}
	LLFace* face = mDrawable->getFace(0);
	bool needs_rebuild = !face || !face->getVertexBuffer() || mDrawable->isState(LLDrawable::REBUILD_GEOMETRY);
	if (needs_rebuild || mDirtyMesh)
	{
		if (needs_rebuild || mDirtyMesh >= 2 || mVisibilityRank <= 4)
		{
			updateMeshData();
			mDirtyMesh = 0;
			mNeedsSkin = TRUE;
			mDrawable->clearState(LLDrawable::REBUILD_GEOMETRY);
		}
	}
	if (LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_AVATAR) <= 0)
	{
		if (mNeedsSkin)
		{
			LLViewerJoint* lower_mesh = getViewerJoint(MESH_ID_LOWER_BODY);
			LLViewerJoint* upper_mesh = getViewerJoint(MESH_ID_UPPER_BODY);
			LLViewerJoint* skirt_mesh = getViewerJoint(MESH_ID_SKIRT);
			LLViewerJoint* eyelash_mesh = getViewerJoint(MESH_ID_EYELASH);
			LLViewerJoint* head_mesh = getViewerJoint(MESH_ID_HEAD);
			LLViewerJoint* hair_mesh = getViewerJoint(MESH_ID_HAIR);
			if(upper_mesh)
			{
				upper_mesh->updateJointGeometry();
			}
			if (lower_mesh)
			{
				lower_mesh->updateJointGeometry();
			}
			if( isWearingWearableType( LLWearableType::WT_SKIRT ) )
			{
				if(skirt_mesh)
				{
					skirt_mesh->updateJointGeometry();
				}
			}
			if (!isSelf() || gAgent.needsRenderHead() || LLPipeline::sShadowRender)
			{
				if(eyelash_mesh)
				{
					eyelash_mesh->updateJointGeometry();
				}
				if(head_mesh)
				{
					head_mesh->updateJointGeometry();
				}
				if(hair_mesh)
				{
					hair_mesh->updateJointGeometry();
				}
			}
			mNeedsSkin = FALSE;
			mLastSkinTime = gFrameTimeSeconds;
			LLFace * face = mDrawable->getFace(0);
			if (face)
			{
				LLVertexBuffer* vb = face->getVertexBuffer();
				if (vb)
				{
					vb->flush();
				}
			}
		}
	}
	else
	{
		mNeedsSkin = FALSE;
	}
	if (sDebugInvisible)
	{
		LLNameValue* firstname = getNVPair("FirstName");
		if (firstname)
		{
			LL_DEBUGS("Avatar") << avString() << " in render" << LL_ENDL;
		}
		else
		{
			LL_INFOS() << "Avatar " << this << " in render" << LL_ENDL;
		}
		if (!mIsBuilt)
		{
			LL_INFOS() << "Not built!" << LL_ENDL;
		}
		else if (!gAgent.needsRenderAvatar())
		{
			LL_INFOS() << "Doesn't need avatar render!" << LL_ENDL;
		}
		else
		{
			LL_INFOS() << "Rendering!" << LL_ENDL;
		}
	}
	if (!mIsBuilt)
	{
		return num_indices;
	}
	if (isSelf() && !gAgent.needsRenderAvatar())
	{
		return num_indices;
	}
	if (sShowFootPlane && mDrawable.notNull())
	{
		LLVector3 slaved_pos = mDrawable->getPositionAgent();
		LLVector3 foot_plane_normal(mFootPlane.mV[VX], mFootPlane.mV[VY], mFootPlane.mV[VZ]);
		F32 dist_from_plane = (slaved_pos * foot_plane_normal) - mFootPlane.mV[VW];
		LLVector3 collide_point = slaved_pos;
		collide_point.mV[VZ] -= foot_plane_normal.mV[VZ] * (dist_from_plane + COLLISION_TOLERANCE - FOOT_COLLIDE_FUDGE);
		gGL.begin(LLRender::LINES);
		{
			F32 SQUARE_SIZE = 0.2f;
			gGL.color4f(1.f, 0.f, 0.f, 1.f);
			gGL.vertex3f(collide_point.mV[VX] - SQUARE_SIZE, collide_point.mV[VY] - SQUARE_SIZE, collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] + SQUARE_SIZE, collide_point.mV[VY] - SQUARE_SIZE, collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] + SQUARE_SIZE, collide_point.mV[VY] - SQUARE_SIZE, collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] + SQUARE_SIZE, collide_point.mV[VY] + SQUARE_SIZE, collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] + SQUARE_SIZE, collide_point.mV[VY] + SQUARE_SIZE, collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] - SQUARE_SIZE, collide_point.mV[VY] + SQUARE_SIZE, collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] - SQUARE_SIZE, collide_point.mV[VY] + SQUARE_SIZE, collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] - SQUARE_SIZE, collide_point.mV[VY] - SQUARE_SIZE, collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX], collide_point.mV[VY], collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] + mFootPlane.mV[VX], collide_point.mV[VY] + mFootPlane.mV[VY], collide_point.mV[VZ] + mFootPlane.mV[VZ]);
		}
		gGL.end();
		gGL.flush();
	}
	static LLStat render_stat;
	LLViewerJointMesh::sRenderPass = pass;
	if (pass == AVATAR_RENDER_PASS_SINGLE)
	{
		bool is_muted = LLPipeline::sImpostorRender && isVisuallyMuted();
		const bool should_alpha_mask = !is_muted && shouldAlphaMask();
		LLGLState<GL_ALPHA_TEST> test(should_alpha_mask);
		if (should_alpha_mask && !LLGLSLShader::sNoFixedFunction)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);
		}
		BOOL first_pass = TRUE;
		if (!LLDrawPoolAvatar::sSkipOpaque)
		{
			if (!isSelf() || gAgent.needsRenderHead() || LLPipeline::sShadowRender)
			{
				if (isTextureVisible(TEX_HEAD_BAKED) || isUIAvatar())
				{
					LLViewerJoint* head_mesh = getViewerJoint(MESH_ID_HEAD);
					if (head_mesh)
					{
						num_indices += head_mesh->render(mAdjustedPixelArea, TRUE, mIsDummy);
					}
					first_pass = FALSE;
				}
			}
			if (isTextureVisible(TEX_UPPER_BAKED) || isUIAvatar())
			{
				LLViewerJoint* upper_mesh = getViewerJoint(MESH_ID_UPPER_BODY);
				if (upper_mesh)
				{
					num_indices += upper_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
				}
				first_pass = FALSE;
			}
			if (isTextureVisible(TEX_LOWER_BAKED) || isUIAvatar())
			{
				LLViewerJoint* lower_mesh = getViewerJoint(MESH_ID_LOWER_BODY);
				if (lower_mesh)
				{
					num_indices += lower_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
				}
				first_pass = FALSE;
			}
		}
		if (should_alpha_mask && !LLGLSLShader::sNoFixedFunction)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
		}
		if (!LLDrawPoolAvatar::sSkipTransparent || LLPipeline::sImpostorRender)
		{
			LLGLState<GL_BLEND> blend(!mIsDummy);
			LLGLState<GL_ALPHA_TEST> test(!mIsDummy);
			num_indices += renderTransparent(first_pass);
		}
	}
	LLViewerJointMesh::sRenderPass = AVATAR_RENDER_PASS_SINGLE;
	return num_indices;
}
U32 LLVOAvatar::renderTransparent(BOOL first_pass)
{
	U32 num_indices = 0;
	if( isWearingWearableType( LLWearableType::WT_SKIRT ) && (isUIAvatar() || isTextureVisible(TEX_SKIRT_BAKED)) )
	{
		gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.25f);
		LLViewerJoint* skirt_mesh = getViewerJoint(MESH_ID_SKIRT);
		if (skirt_mesh)
		{
			num_indices += skirt_mesh->render(mAdjustedPixelArea, FALSE);
		}
		first_pass = FALSE;
		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
	}
	if (!isSelf() || gAgent.needsRenderHead() || LLPipeline::sShadowRender)
	{
		if (LLPipeline::sImpostorRender)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);
		}
		if (isTextureVisible(TEX_HEAD_BAKED))
		{
			LLViewerJoint* eyelash_mesh = getViewerJoint(MESH_ID_EYELASH);
			if (eyelash_mesh)
			{
				num_indices += eyelash_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
			}
			first_pass = FALSE;
		}
		bool show_hair = false;
		if (isControlAvatar())
		{
			show_hair = isTextureVisible(TEX_HAIR_BAKED);
		}
		else
		{
			auto image = getImage(TEX_HAIR_BAKED, 0);
			show_hair = LLDrawPoolAlpha::sShowDebugAlpha || (image && image->getID() != IMG_INVISIBLE);
		}
		if (show_hair)
		{
			LLViewerJoint* hair_mesh = getViewerJoint(MESH_ID_HAIR);
			if (hair_mesh)
			{
				num_indices += hair_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
			}
			first_pass = FALSE;
		}
		if (LLPipeline::sImpostorRender)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
		}
	}
	return num_indices;
}
U32 LLVOAvatar::renderRigid()
{
	U32 num_indices = 0;
	if (!mIsBuilt)
	{
		return 0;
	}
	if (isSelf() && (!gAgent.needsRenderAvatar() || !gAgent.needsRenderHead()))
	{
		return 0;
	}
	if (!mIsBuilt)
	{
		return 0;
	}
	const bool should_alpha_mask = shouldAlphaMask();
	LLGLState<GL_ALPHA_TEST> test(should_alpha_mask);
	if (should_alpha_mask && !LLGLSLShader::sNoFixedFunction)
	{
		gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);
	}
	if (isTextureVisible(TEX_EYES_BAKED)  || isUIAvatar())
	{
		LLViewerJoint* eyeball_left = getViewerJoint(MESH_ID_EYEBALL_LEFT);
		LLViewerJoint* eyeball_right = getViewerJoint(MESH_ID_EYEBALL_RIGHT);
		if (eyeball_left)
		{
			num_indices += eyeball_left->render(mAdjustedPixelArea, TRUE, mIsDummy);
		}
		if(eyeball_right)
		{
			num_indices += eyeball_right->render(mAdjustedPixelArea, TRUE, mIsDummy);
		}
	}
	if (should_alpha_mask && !LLGLSLShader::sNoFixedFunction)
	{
		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
	}
	return num_indices;
}
U32 LLVOAvatar::renderImpostor(LLColor4U color, S32 diffuse_channel)
{
	if (!mImpostor.isComplete())
	{
		return 0;
	}
	LLVector3 pos(getRenderPosition()+mImpostorOffset);
	LLVector3 at = (pos - LLViewerCamera::getInstance()->getOrigin());
	at.normalize();
	LLVector3 left = LLViewerCamera::getInstance()->getUpAxis() % at;
	LLVector3 up = at%left;
	left *= mImpostorDim.mV[0];
	up *= mImpostorDim.mV[1];
	LLGLEnable<GL_ALPHA_TEST> test;
	gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.f);
	gGL.color4ubv(color.mV);
	gGL.getTexUnit(diffuse_channel)->bind(&mImpostor);
	gGL.begin(LLRender::TRIANGLE_STRIP);
	gGL.texCoord2f(0,0);
	gGL.vertex3fv((pos+left-up).mV);
	gGL.texCoord2f(1,0);
	gGL.vertex3fv((pos-left-up).mV);
	gGL.texCoord2f(0, 1);
	gGL.vertex3fv((pos + left + up).mV);
	gGL.texCoord2f(1,1);
	gGL.vertex3fv((pos-left+up).mV);
	gGL.end();
	gGL.flush();
	return 6;
}
bool LLVOAvatar::allTexturesCompletelyDownloaded(uuid_set_t& ids) const
{
	for (auto it = ids.begin(); it != ids.end(); ++it)
	{
		LLViewerFetchedTexture *imagep = gTextureList.findImage(*it, TEX_LIST_STANDARD);
		if (imagep && imagep->getDiscardLevel()!=0)
		{
			return false;
		}
	}
	return true;
}
bool LLVOAvatar::allLocalTexturesCompletelyDownloaded() const
{
	uuid_set_t local_ids;
	collectLocalTextureUUIDs(local_ids);
	return allTexturesCompletelyDownloaded(local_ids);
}
bool LLVOAvatar::allBakedTexturesCompletelyDownloaded() const
{
	uuid_set_t baked_ids;
	collectBakedTextureUUIDs(baked_ids);
	return allTexturesCompletelyDownloaded(baked_ids);
}
void LLVOAvatar::bakedTextureOriginCounts(S32 &sb_count,
										  S32 &host_count,
										  S32 &both_count,
										  S32 &neither_count)
{
	sb_count = host_count = both_count = neither_count = 0;
	uuid_set_t baked_ids;
	collectBakedTextureUUIDs(baked_ids);
	for (auto it = baked_ids.begin(); it != baked_ids.end(); ++it)
	{
		LLViewerFetchedTexture *imagep = gTextureList.findImage(*it, TEX_LIST_STANDARD);
		bool has_url = false, has_host = false;
		if (!imagep->getUrl().empty())
		{
			has_url = true;
		}
		if (imagep->getTargetHost().isOk())
		{
			has_host = true;
		}
		if (has_url && !has_host) sb_count++;
		else if (has_host && !has_url) host_count++;
		else if (has_host && has_url) both_count++;
		else if (!has_host && !has_url) neither_count++;
	}
}
std::string LLVOAvatar::bakedTextureOriginInfo()
{
	std::string result;
	uuid_set_t baked_ids;
	collectBakedTextureUUIDs(baked_ids);
	for (auto it = baked_ids.begin(); it != baked_ids.end(); ++it)
	{
		LLViewerFetchedTexture *imagep = gTextureList.findImage(*it, TEX_LIST_STANDARD);
		bool has_url = false, has_host = false;
		if (!imagep->getUrl().empty())
		{
			has_url = true;
		}
		if (imagep->getTargetHost().isOk())
		{
			has_host = true;
		}
		S32 discard = imagep->getDiscardLevel();
		if (has_url && !has_host) result += discard ? "u" : "U";
		else if (has_host && !has_url) result += discard ? "h" : "H";
		else if (has_host && has_url) result += discard ? "x" : "X";
		else if (!has_host && !has_url) result += discard ? "n" : "N";
		if (discard != 0)
		{
			result += llformat("(%d/%d)",discard,imagep->getDesiredDiscardLevel());
		}
	}
	return result;
}
S32Bytes LLVOAvatar::totalTextureMemForUUIDS(uuid_set_t& ids)
{
	S32Bytes result(0);
	for (auto it = ids.begin(); it != ids.end(); ++it)
	{
		LLViewerFetchedTexture *imagep = gTextureList.findImage(*it, TEX_LIST_STANDARD);
		if (imagep)
		{
			result += imagep->getTextureMemory();
		}
	}
	return result;
}
void LLVOAvatar::collectLocalTextureUUIDs(uuid_set_t& ids) const
{
	for (U32 texture_index = 0; texture_index < getNumTEs(); texture_index++)
	{
		LLWearableType::EType wearable_type = LLAvatarAppearanceDictionary::getTEWearableType((ETextureIndex)texture_index);
		U32 num_wearables = gAgentWearables.getWearableCount(wearable_type);
		LLViewerFetchedTexture *imagep = NULL;
		for (U32 wearable_index = 0; wearable_index < num_wearables; wearable_index++)
		{
			imagep = LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index, wearable_index), TRUE);
			if (imagep)
			{
				const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = LLAvatarAppearanceDictionary::getInstance()->getTexture((ETextureIndex)texture_index);
				if (texture_dict && texture_dict->mIsLocalTexture)
				{
					ids.insert(imagep->getID());
				}
			}
		}
	}
	ids.erase(IMG_DEFAULT);
	ids.erase(IMG_DEFAULT_AVATAR);
	ids.erase(IMG_INVISIBLE);
}
void LLVOAvatar::collectBakedTextureUUIDs(uuid_set_t& ids) const
{
	for (U32 texture_index = 0; texture_index < getNumTEs(); texture_index++)
	{
		LLViewerFetchedTexture *imagep = NULL;
		if (isIndexBakedTexture((ETextureIndex) texture_index))
		{
			imagep = LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index,0), TRUE);
			if (imagep)
			{
				ids.insert(imagep->getID());
			}
		}
	}
	ids.erase(IMG_DEFAULT);
	ids.erase(IMG_DEFAULT_AVATAR);
	ids.erase(IMG_INVISIBLE);
}
void LLVOAvatar::collectTextureUUIDs(uuid_set_t& ids)
{
	collectLocalTextureUUIDs(ids);
	collectBakedTextureUUIDs(ids);
}
void LLVOAvatar::releaseOldTextures()
{
	S32Bytes current_texture_mem;
	uuid_set_t baked_texture_ids;
	collectBakedTextureUUIDs(baked_texture_ids);
	S32Bytes new_baked_mem = totalTextureMemForUUIDS(baked_texture_ids);
	uuid_set_t local_texture_ids;
	collectLocalTextureUUIDs(local_texture_ids);
	uuid_set_t new_texture_ids;
	new_texture_ids.insert(baked_texture_ids.begin(),baked_texture_ids.end());
	new_texture_ids.insert(local_texture_ids.begin(),local_texture_ids.end());
	S32Bytes new_total_mem = totalTextureMemForUUIDS(new_texture_ids);
	if (!isSelf() && new_total_mem > new_baked_mem)
	{
			LL_WARNS() << "extra local textures stored for non-self av" << LL_ENDL;
	}
	for (auto it = mTextureIDs.begin(); it != mTextureIDs.end(); ++it)
	{
		if (new_texture_ids.find(*it) == new_texture_ids.end())
		{
			LLViewerFetchedTexture *imagep = gTextureList.findImage(*it, TEX_LIST_STANDARD);
			if (imagep)
			{
				current_texture_mem += imagep->getTextureMemory();
				if (imagep->getTextureState() == LLGLTexture::NO_DELETE)
				{
					imagep->forceActive();
					imagep->forceUpdateBindStats();
				}
			}
		}
	}
	mTextureIDs = new_texture_ids;
}
static LLFastTimer::DeclareTimer FTM_TEXTURE_UPDATE("Update Textures");
void LLVOAvatar::updateTextures()
{
	LL_RECORD_BLOCK_TIME(FTM_TEXTURE_UPDATE);
	releaseOldTextures();
	BOOL render_avatar = TRUE;
	if (mIsDummy)
	{
		return;
	}
	if( isSelf() )
	{
		render_avatar = TRUE;
	}
	else
	{
		if(!isVisible())
		{
			return ;
		}
		render_avatar = !mCulled;
	}
	std::vector<bool> layer_baked;
	for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
	{
		layer_baked.push_back(isTextureDefined(mBakedTextureDatas[i].mTextureIndex));
		if (render_avatar && !gGLManager.mIsDisabled)
		{
			if (layer_baked[i] && !mBakedTextureDatas[i].mIsLoaded)
			{
				gGL.getTexUnit(0)->bind(getImage( mBakedTextureDatas[i].mTextureIndex, 0 ));
			}
		}
	}
	mMaxPixelArea = 0.f;
	mMinPixelArea = 99999999.f;
	mHasGrey = FALSE;
	for (U32 texture_index = 0; texture_index < getNumTEs(); texture_index++)
	{
		LLWearableType::EType wearable_type = LLAvatarAppearanceDictionary::getTEWearableType((ETextureIndex)texture_index);
		U32 num_wearables = gAgentWearables.getWearableCount(wearable_type);
		const LLTextureEntry *te = getTE(texture_index);
		F32 texel_area_ratio = 1.0f;
		if( te )
		{
			texel_area_ratio = fabs(te->mScaleS * te->mScaleT);
		}
		else
		{
			LL_WARNS() << "getTE( " << texture_index << " ) returned 0" <<LL_ENDL;
		}
		LLViewerFetchedTexture *imagep = NULL;
		for (U32 wearable_index = 0; wearable_index < num_wearables; wearable_index++)
		{
			imagep = LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index, wearable_index), TRUE);
			if (imagep)
			{
				const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = LLAvatarAppearanceDictionary::getInstance()->getTexture((ETextureIndex)texture_index);
				const EBakedTextureIndex baked_index = texture_dict ? texture_dict->mBakedTextureIndex : EBakedTextureIndex::BAKED_NUM_INDICES;
				if (texture_dict && texture_dict->mIsLocalTexture)
				{
					addLocalTextureStats((ETextureIndex)texture_index, imagep, texel_area_ratio, render_avatar, mBakedTextureDatas[baked_index].mIsUsed);
				}
			}
		}
		if (isIndexBakedTexture((ETextureIndex) texture_index) && render_avatar)
		{
			const S32 boost_level = getAvatarBakedBoostLevel();
			imagep = LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index,0), TRUE);
			if (isIndexBakedTexture((ETextureIndex)texture_index)
				&& imagep->getID() != IMG_DEFAULT_AVATAR
				&& imagep->getID() != IMG_INVISIBLE
				&& !isUsingServerBakes()
				&& !imagep->getTargetHost().isOk())
			{
				LL_WARNS_ONCE("Texture") << "LLVOAvatar::updateTextures No host for texture "
										 << imagep->getID() << " for avatar "
										 << (isSelf() ? "<myself>" : getID().asString())
										 << " on host " << getRegion()->getHost() << LL_ENDL;
			}
			addBakedTextureStats( imagep, mPixelArea, texel_area_ratio, boost_level );
		}
	}
	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
	{
		setDebugText(llformat("%4.0f:%4.0f", (F32) sqrt(mMinPixelArea),(F32) sqrt(mMaxPixelArea)));
	}
}
void LLVOAvatar::addLocalTextureStats( ETextureIndex idx, LLViewerFetchedTexture* imagep,
									   F32 texel_area_ratio, BOOL render_avatar, BOOL covered_by_baked)
{
	return;
}
const S32 MAX_TEXTURE_UPDATE_INTERVAL = 64 ;
const S32 MAX_TEXTURE_VIRTUAL_SIZE_RESET_INTERVAL = S32_MAX ;
void LLVOAvatar::checkTextureLoading()
{
	static const F32 MAX_INVISIBLE_WAITING_TIME = 15.f ;
	BOOL pause = !isVisible() ;
	if(!pause)
	{
		mInvisibleTimer.reset() ;
	}
	if(mLoadedCallbacksPaused == pause)
	{
		return ;
	}
	if(mCallbackTextureList.empty())
	{
		mLoadedCallbacksPaused = pause ;
		return ;
	}
	if(pause && mInvisibleTimer.getElapsedTimeF32() < MAX_INVISIBLE_WAITING_TIME)
	{
		return ;
	}
	for(LLLoadedCallbackEntry::source_callback_list_t::iterator iter = mCallbackTextureList.begin();
		iter != mCallbackTextureList.end(); ++iter)
	{
		LLViewerFetchedTexture* tex = gTextureList.findImage(*iter) ;
		if(tex)
		{
			if(pause)
			{
				tex->pauseLoadedCallbacks(&mCallbackTextureList) ;
				tex->setMaxVirtualSizeResetInterval(MAX_TEXTURE_UPDATE_INTERVAL);
				tex->resetMaxVirtualSizeResetCounter() ;
			}
			else
			{
				static const F32 START_AREA = 100.f ;
				tex->unpauseLoadedCallbacks(&mCallbackTextureList) ;
				tex->addTextureStats(START_AREA);
			}
		}
	}
	if(!pause)
	{
		updateTextures() ;
	}
	mLoadedCallbacksPaused = pause ;
	return ;
}
const F32  SELF_ADDITIONAL_PRI = 0.75f ;
const F32  ADDITIONAL_PRI = 0.5f;
void LLVOAvatar::addBakedTextureStats( LLViewerFetchedTexture* imagep, F32 pixel_area, F32 texel_area_ratio, S32 boost_level)
{
	imagep->resetTextureStats();
	imagep->setCanUseHTTP(true);
	imagep->setMaxVirtualSizeResetInterval(MAX_TEXTURE_VIRTUAL_SIZE_RESET_INTERVAL);
	imagep->resetMaxVirtualSizeResetCounter() ;
	mMaxPixelArea = llmax(pixel_area, mMaxPixelArea);
	mMinPixelArea = llmin(pixel_area, mMinPixelArea);
	imagep->addTextureStats(pixel_area / texel_area_ratio);
	imagep->setBoostLevel(boost_level);
	if(boost_level != LLGLTexture::BOOST_AVATAR_BAKED_SELF)
	{
		imagep->setAdditionalDecodePriority(ADDITIONAL_PRI) ;
	}
	else
	{
		imagep->setAdditionalDecodePriority(SELF_ADDITIONAL_PRI) ;
	}
}
void LLVOAvatar::setImage(const U8 te, LLViewerTexture *imagep, const U32 index)
{
	setTEImage(te, imagep);
}
LLViewerTexture* LLVOAvatar::getImage(const U8 te, const U32 index) const
{
	return getTEImage(te);
}
const LLTextureEntry* LLVOAvatar::getTexEntry(const U8 te_num) const
{
	return getTE(te_num);
}
void LLVOAvatar::setTexEntry(const U8 index, const LLTextureEntry &te)
{
	setTE(index, te);
}
const std::string LLVOAvatar::getImageURL(const U8 te, const LLUUID &uuid)
{
	llassert(isIndexBakedTexture(ETextureIndex(te)));
	std::string url = "";
	if (isUsingServerBakes())
	{
		const std::string& appearance_service_url = LLAppearanceMgr::instance().getAppearanceServiceURL();
		if (appearance_service_url.empty())
		{
			LL_WARNS() << "AgentAppearanceServiceURL not set - Baked texture requests will fail" << LL_ENDL;
			return url;
		}
		const LLAvatarAppearanceDictionary::TextureEntry* texture_entry = LLAvatarAppearanceDictionary::getInstance()->getTexture((ETextureIndex)te);
		if (texture_entry != NULL)
		{
			url = appearance_service_url + "texture/" + getID().asString() + "/" + texture_entry->mDefaultImageName + "/" + uuid.asString();
		}
	}
	return url;
}
void LLVOAvatar::resolveHeightAgent(const LLVector3 &in_pos_agent, LLVector3 &out_pos_agent, LLVector3 &out_norm)
{
	LLVector3d in_pos_global, out_pos_global;
	in_pos_global = gAgent.getPosGlobalFromAgent(in_pos_agent);
	resolveHeightGlobal(in_pos_global, out_pos_global, out_norm);
	out_pos_agent = gAgent.getPosAgentFromGlobal(out_pos_global);
}
void LLVOAvatar::resolveRayCollisionAgent(const LLVector3d start_pt, const LLVector3d end_pt, LLVector3d &out_pos, LLVector3 &out_norm)
{
	LLViewerObject *obj;
	LLWorld::getInstance()->resolveStepHeightGlobal(this, start_pt, end_pt, out_pos, out_norm, &obj);
}
void LLVOAvatar::resolveHeightGlobal(const LLVector3d &inPos, LLVector3d &outPos, LLVector3 &outNorm)
{
	LLVector3d zVec(0.0f, 0.0f, 0.5f);
	LLVector3d p0 = inPos + zVec;
	LLVector3d p1 = inPos - zVec;
	LLViewerObject *obj;
	LLWorld::getInstance()->resolveStepHeightGlobal(this, p0, p1, outPos, outNorm, &obj);
	if (!obj)
	{
		mStepOnLand = TRUE;
		mStepMaterial = 0;
		mStepObjectVelocity.setVec(0.0f, 0.0f, 0.0f);
	}
	else
	{
		mStepOnLand = FALSE;
		mStepMaterial = obj->getMaterial();
		LLVector3 angularVelocity = obj->getAngularVelocity();
		LLVector3 relativePos = gAgent.getPosAgentFromGlobal(outPos) - obj->getPositionAgent();
		LLVector3 linearComponent = angularVelocity % relativePos;
		mStepObjectVelocity = obj->getVelocity() + linearComponent;
	}
}
const LLUUID& LLVOAvatar::getStepSound() const
{
	if ( mStepOnLand )
	{
		return sStepSoundOnLand;
	}
	return sStepSounds[mStepMaterial];
}
void LLVOAvatar::processAnimationStateChanges()
{
	if ( isAnyAnimationSignaled(AGENT_WALK_ANIMS, NUM_AGENT_WALK_ANIMS) )
	{
		startMotion(ANIM_AGENT_WALK_ADJUST);
		stopMotion(ANIM_AGENT_FLY_ADJUST);
	}
	else if (mInAir && !isSitting())
	{
		stopMotion(ANIM_AGENT_WALK_ADJUST);
        if (mEnableDefaultMotions)
        {
            startMotion(ANIM_AGENT_FLY_ADJUST);
        }
	}
	else
	{
		stopMotion(ANIM_AGENT_WALK_ADJUST);
		stopMotion(ANIM_AGENT_FLY_ADJUST);
	}
	if ( isAnyAnimationSignaled(AGENT_GUN_AIM_ANIMS, NUM_AGENT_GUN_AIM_ANIMS) )
	{
        if (mEnableDefaultMotions)
        {
            startMotion(ANIM_AGENT_TARGET);
        }
		stopMotion(ANIM_AGENT_BODY_NOISE);
	}
	else
	{
		stopMotion(ANIM_AGENT_TARGET);
        if (mEnableDefaultMotions)
        {
            startMotion(ANIM_AGENT_BODY_NOISE);
        }
	}
	auto ao = isSelf() ? AOSystem::getIfExists() : nullptr;
	AnimIterator anim_it;
	for (anim_it = mPlayingAnimations.begin(); anim_it != mPlayingAnimations.end();)
	{
		AnimIterator found_anim = mSignaledAnimations.find(anim_it->first);
		if (found_anim == mSignaledAnimations.end())
		{
			if (ao) ao->stopMotion(anim_it->first);
			processSingleAnimationStateChange(anim_it->first, FALSE);
			LLFloaterExploreAnimations::processAnim(getID(), anim_it->first, false);
			anim_it = mPlayingAnimations.erase(anim_it);
			continue;
		}
		++anim_it;
	}
	for (anim_it = mSignaledAnimations.begin(); anim_it != mSignaledAnimations.end();)
	{
		AnimIterator found_anim = mPlayingAnimations.find(anim_it->first);
		if (found_anim == mPlayingAnimations.end() || found_anim->second != anim_it->second)
		{
			LLFloaterExploreAnimations::processAnim(getID(), anim_it->first, true);
			if (processSingleAnimationStateChange(anim_it->first, TRUE))
			{
				if (ao) ao->startMotion(anim_it->first);
				mPlayingAnimations[anim_it->first] = anim_it->second;
				++anim_it;
				continue;
			}
		}
		++anim_it;
	}
	if (isSelf())
	{
		AnimSourceIterator source_it = mAnimationSources.begin();
		for (source_it = mAnimationSources.begin(); source_it != mAnimationSources.end();)
		{
			if (mSignaledAnimations.find(source_it->second) == mSignaledAnimations.end())
			{
				mAnimationSources.erase(source_it++);
			}
			else
			{
				++source_it;
			}
		}
	}
	stop_glerror();
}
BOOL LLVOAvatar::processSingleAnimationStateChange( const LLUUID& anim_id, BOOL start )
{
    computeBodySize();
	BOOL result = FALSE;
	if ( start )
	{
		static LLCachedControl<bool> play_typing_sound("PlayTypingSound");
		if (anim_id == ANIM_AGENT_TYPE && play_typing_sound)
		{
			if (gAudiop)
			{
				LLVector3d char_pos_global = gAgent.getPosGlobalFromAgent(getCharacterPosition());
				if (LLViewerParcelMgr::getInstance()->canHearSound(char_pos_global)
				    && !LLMuteList::getInstance()->isMuted(getID(), LLMute::flagObjectSounds))
				{
					{
						LLUUID sound_id = LLUUID(gSavedSettings.getString("UISndTyping"));
						gAudiop->triggerSound(sound_id, getID(), 1.0f, LLAudioEngine::AUDIO_TYPE_SFX, char_pos_global);
					}
				}
			}
		}
		else if (anim_id == ANIM_AGENT_SIT_GROUND_CONSTRAINED)
		{
			sitDown(TRUE);
		}
		else if(anim_id == ANIM_AGENT_SNAPSHOT)
		{
			mIdleTimer.reset();
			static LLCachedControl<bool> announce_snapshots("AnnounceSnapshots");
			if (announce_snapshots)
			{
				std::string name;
				LLAvatarNameCache::getNSName(mID, name);
				LLChat chat;
				chat.mFromName = name;
				chat.mText = name + ' ' + LLTrans::getString("took_a_snapshot") + '.';
				chat.mURL = LLAvatarActions::getSLURL(mID);
				chat.mSourceType = CHAT_SOURCE_SYSTEM;
				LLFloaterChat::addChat(chat);
			}
		}
		if (startMotion(anim_id))
		{
			result = TRUE;
		}
		else
		{
			LL_WARNS() << "Failed to start motion!" << LL_ENDL;
		}
	}
	else
	{
		if (anim_id == ANIM_AGENT_SIT_GROUND_CONSTRAINED)
		{
			sitDown(FALSE);
		}
		if ((anim_id == ANIM_AGENT_DO_NOT_DISTURB) && gAgent.isDoNotDisturb())
		{
			gAgent.sendAnimationRequest(ANIM_AGENT_DO_NOT_DISTURB, ANIM_REQUEST_START);
			return result;
		}
		stopMotion(anim_id);
		result = TRUE;
	}
	return result;
}
BOOL LLVOAvatar::isAnyAnimationSignaled(const LLUUID *anim_array, const S32 num_anims) const
{
	for (S32 i = 0; i < num_anims; i++)
	{
		if(mSignaledAnimations.find(anim_array[i]) != mSignaledAnimations.end())
		{
			return TRUE;
		}
	}
	return FALSE;
}
void LLVOAvatar::resetAnimations()
{
	LLKeyframeMotion::flushKeyframeCache();
	flushAllMotions();
}
std::string LLVOAvatar::getIdleTime(bool is_away, bool is_busy, bool is_appearance)
{
	if(	(mNameAway && ! is_away) ||
		(mNameBusy && ! is_busy) ||
		(mNameAppearance && ! is_appearance) ||
		mTyping)
			mIdleTimer.reset();
	static LLCachedControl<bool> ascent_show_idle_time("AscentShowIdleTime", false);
	U32 minutes = 0;
	if(ascent_show_idle_time && !isSelf() && mIdleTimer.getElapsedTimeF32() > 120)
	{
		minutes = (U32)(mIdleTimer.getElapsedTimeF32()/60);
	}
	if(minutes != mIdleMinute)
		clearNameTag();
	mIdleMinute = minutes;
	if(mIdleMinute)
	{
		LLSD args;
		args["[MINUTES]"]=(LLSD::Integer)minutes;
		return LLTrans::getString("AvatarIdle", args);
	}
	return "";
}
LLUUID LLVOAvatar::remapMotionID(const LLUUID& id)
{
	static const LLCachedControl<bool> use_new_walk_run("UseNewWalkRun",true);
	static const LLCachedControl<bool> use_cross_walk_run("UseCrossWalkRun",false);
	LLUUID result = id;
	if ((getSex() == SEX_FEMALE) != use_cross_walk_run)
	{
		if (id == ANIM_AGENT_WALK)
		{
			if (use_new_walk_run)
				result = ANIM_AGENT_FEMALE_WALK_NEW;
			else
				result = ANIM_AGENT_FEMALE_WALK;
		}
		else if (id == ANIM_AGENT_RUN)
		{
			if (use_new_walk_run)
				result = ANIM_AGENT_FEMALE_RUN_NEW;
		}
		else if (id == ANIM_AGENT_SIT)
		{
			result = ANIM_AGENT_SIT_FEMALE;
		}
	}
	else
	{
		if (id == ANIM_AGENT_WALK)
		{
			if (use_new_walk_run)
				result = ANIM_AGENT_WALK_NEW;
		}
		else if (id == ANIM_AGENT_RUN)
		{
			if (use_new_walk_run)
				result = ANIM_AGENT_RUN_NEW;
		}
		else if (id == ANIM_AGENT_SIT_FEMALE)
		{
			result = ANIM_AGENT_SIT;
		}
	}
	return result;
}
BOOL LLVOAvatar::startMotion(const LLUUID& id, F32 time_offset)
{
	if ("62c5de58-cb33-5743-3d07-9e4cd4352864" == id.getString() && gSavedSettings.getBOOL("DisableInternalFlyUpAnimation"))
	{
		return TRUE;
	}
	LL_DEBUGS() << "motion requested " << id.asString() << " " << gAnimLibrary.animationName(id) << LL_ENDL;
	LLUUID remap_id = remapMotionID(id);
	if (remap_id != id)
	{
		LL_DEBUGS() << "motion resultant " << remap_id.asString() << " " << gAnimLibrary.animationName(remap_id) << LL_ENDL;
	}
	if (isSelf() && remap_id == ANIM_AGENT_AWAY)
	{
		gAgent.setAFK();
	}
	return LLCharacter::startMotion(remap_id, time_offset);
}
BOOL LLVOAvatar::stopMotion(const LLUUID& id, BOOL stop_immediate)
{
	LL_DEBUGS() << "motion requested " << id.asString() << " " << gAnimLibrary.animationName(id) << LL_ENDL;
	LLUUID remap_id = remapMotionID(id);
	if (remap_id != id)
	{
		LL_DEBUGS() << "motion resultant " << remap_id.asString() << " " << gAnimLibrary.animationName(remap_id) << LL_ENDL;
	}
	if (isSelf())
	{
		gAgent.onAnimStop(remap_id);
	}
	return LLCharacter::stopMotion(remap_id, stop_immediate);
}
bool LLVOAvatar::hasMotionFromSource(const LLUUID& source_id)
{
	return false;
}
void LLVOAvatar::stopMotionFromSource(const LLUUID& source_id)
{
}
void LLVOAvatar::addDebugText(const std::string& text)
{
	mDebugText.append(1, '\n');
	mDebugText.append(text);
}
const LLUUID& LLVOAvatar::getID() const
{
	return mID;
}
LLJoint *LLVOAvatar::getJoint( const std::string &name )
{
	joint_map_t::iterator iter = std::find_if(mJointMap.begin(), mJointMap.end(), [&name](joint_map_t::value_type &pair) { return strcmp(name.c_str(), pair.first) == 0; });
	LLJoint* jointp = NULL;
	if (iter == mJointMap.end() || iter->second == NULL)
	{
		jointp = mRoot->findJoint(name);
		joint_map_t::value_type entry;
		strncpy(entry.first, name.c_str(), sizeof(entry.first));
		entry.second = jointp;
		mJointMap.emplace_back(entry);
	}
	else
	{
		jointp = iter->second;
	}
	return jointp;
}
LLJoint *LLVOAvatar::getJoint( S32 joint_num )
{
    LLJoint *pJoint = NULL;
    S32 collision_start = mNumBones;
    S32 attachment_start = mNumBones + mCollisionVolumes.size();
    if (joint_num>=attachment_start)
    {
        S32 attachment_id = joint_num - attachment_start + 1;
        attachment_map_t::iterator iter = mAttachmentPoints.find(attachment_id);
        if (iter != mAttachmentPoints.end())
        {
            pJoint = iter->second;
        }
    }
    else if (joint_num>=collision_start)
    {
        S32 collision_id = joint_num-collision_start;
        pJoint = mCollisionVolumes[collision_id];
    }
    else if (joint_num>=0)
    {
        pJoint = mSkeleton[joint_num];
    }
	llassert(!pJoint || pJoint->getJointNum() == joint_num);
    return pJoint;
}
bool LLVOAvatar::getRiggedMeshID(LLViewerObject* pVO, LLUUID& mesh_id)
{
	mesh_id.setNull();
	if ( pVO && pVO->mDrawable )
	{
		LLVOVolume* pVObj = pVO->mDrawable->getVOVolume();
		if ( pVObj )
		{
			const LLMeshSkinInfo* pSkinData = pVObj->getSkinInfo();
			if (pSkinData
				&& pSkinData->mJointNames.size() > JOINT_COUNT_REQUIRED_FOR_FULLRIG
				&& pSkinData->mAlternateBindMatrix.size() > 0 )
					{
						mesh_id = pSkinData->mMeshID;
						return true;
					}
		}
	}
	return false;
}
bool LLVOAvatar::jointIsRiggedTo(const LLJoint *joint) const
{
    if (joint)
    {
        const LLJointRiggingInfoTab& tab = mJointRiggingInfoTab;
        S32 joint_num = joint->getJointNum();
        if (joint_num < tab.size() && tab[joint_num].isRiggedTo())
        {
            return true;
        }
    }
    return false;
}
void LLVOAvatar::clearAttachmentOverrides()
{
    for (S32 i=0; i<LL_CHARACTER_MAX_ANIMATED_JOINTS; i++)
    {
        LLJoint *pJoint = getJoint(i);
        if (pJoint)
        {
			pJoint->clearAttachmentPosOverrides();
			pJoint->clearAttachmentScaleOverrides();
        }
    }
    if (mPelvisFixups.count()>0)
    {
        mPelvisFixups.clear();
        LLJoint* pJointPelvis = getJoint("mPelvis");
        if (pJointPelvis)
        {
			pJointPelvis->setPosition( LLVector3( 0.0f, 0.0f, 0.0f) );
        }
        postPelvisSetRecalc();
    }
    mActiveOverrideMeshes.clear();
    onActiveOverrideMeshesChanged();
}
void LLVOAvatar::rebuildAttachmentOverrides()
{
    clearAttachmentOverrides();
    LLControlAvatar *control_av = asControlAvatar();
    if (control_av)
    {
        LLVOVolume *volp = control_av->mRootVolp;
        if (volp)
        {
            LL_DEBUGS("Avatar") << volp->getID() << " adding attachment overrides for root vol, prim count "
                                << (S32) (1+volp->numChildren()) << LL_ENDL;
            addAttachmentOverridesForObject(volp);
        }
    }
#if SLOW_ATTACHMENT_LIST
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment *attachment_pt = (*iter).second;
		if (attachment_pt)
		{
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator at_it = attachment_pt->mAttachedObjects.begin();
				 at_it != attachment_pt->mAttachedObjects.end(); ++at_it)
			{
				LLViewerObject *vo = *at_it;
#else
	for(auto& iter : mAttachedObjectsVector)
	{{{
				LLViewerObject *vo = iter.first;
#endif
				if (vo && !vo->isAnimatedObject())
				{
					addAttachmentOverridesForObject(vo);
				}
			}
		}
	}
}
void LLVOAvatar::updateAttachmentOverrides()
{
    LL_DEBUGS("AnimatedObjects") << "updating" << LL_ENDL;
    uuid_set_t meshes_seen;
    LLControlAvatar *control_av = asControlAvatar();
    if (control_av)
    {
        LLVOVolume *volp = control_av->mRootVolp;
        if (volp)
        {
            LL_DEBUGS("Avatar") << volp->getID() << " adding attachment overrides for root vol, prim count "
                                << (S32) (1+volp->numChildren()) << LL_ENDL;
            addAttachmentOverridesForObject(volp, &meshes_seen);
        }
    }
#if SLOW_ATTACHMENT_LIST
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment *attachment_pt = (*iter).second;
		if (attachment_pt)
		{
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator at_it = attachment_pt->mAttachedObjects.begin();
				 at_it != attachment_pt->mAttachedObjects.end(); ++at_it)
			{
					LLViewerObject *vo = *at_it;
#else
	for(auto& iter : mAttachedObjectsVector)
	{{{
				LLViewerObject *vo = iter.first;
#endif
				if (!vo->isAnimatedObject())
				{
					addAttachmentOverridesForObject(vo, &meshes_seen);
				}
			}
		}
	}
    uuid_set_t active_override_meshes = mActiveOverrideMeshes;
    for (auto it = active_override_meshes.begin(); it != active_override_meshes.end(); ++it)
    {
        if (meshes_seen.find(*it) == meshes_seen.end())
        {
            removeAttachmentOverridesForObject(*it);
        }
    }
}
void LLVOAvatar::addAttachmentOverridesForObject(LLViewerObject *vo, uuid_set_t* meshes_seen, bool recursive)
{
    if (vo->getAvatar() != this && vo->getAvatarAncestor() != this)
    {
		LL_WARNS("Avatar") << "called with invalid avatar" << LL_ENDL;
        return;
    }
    LL_DEBUGS("AnimatedObjects") << "adding" << LL_ENDL;
    if (recursive)
    {
        LLViewerObject::const_child_list_t& children = vo->getChildren();
        for (LLViewerObject::const_child_list_t::const_iterator it = children.begin();
             it != children.end(); ++it)
        {
            LLViewerObject *childp = *it;
            addAttachmentOverridesForObject(childp, meshes_seen, true);
        }
    }
	LLVOVolume *vobj = vo->asVolume();
	if (vobj && vobj->isRiggedMesh() &&
		vobj->getVolume() && vobj->getVolume()->isMeshAssetLoaded() && gMeshRepo.meshRezEnabled())
	{
		bool pelvisGotSet = false;
		const LLMeshSkinInfo*  pSkinData = vobj->getSkinInfo();
		const U32 bindCnt = pSkinData->mAlternateBindMatrix.size();
        const U32 jointCnt = pSkinData->mJointNames.size();
		if ((bindCnt > 0) && (bindCnt != jointCnt))
		{
			LL_WARNS_ONCE() << "invalid mesh, bindCnt " << bindCnt << "!= jointCnt " << jointCnt << ", joint overrides will be ignored." << LL_ENDL;
		}
		if ((bindCnt > 0) && (bindCnt == jointCnt))
		{
			const F32 pelvisZOffset = pSkinData->mPelvisOffset;
			const LLUUID& mesh_id = pSkinData->mMeshID;
            if (meshes_seen)
            {
                meshes_seen->insert(mesh_id);
            }
            bool mesh_overrides_loaded = (mActiveOverrideMeshes.find(mesh_id) != mActiveOverrideMeshes.end());
			bool fullRig = (jointCnt>=JOINT_COUNT_REQUIRED_FOR_FULLRIG) ? true : false;
			if ( fullRig && !mesh_overrides_loaded )
			{
				for ( U32 i=0; i<jointCnt; ++i )
				{
					std::string lookingForJoint = pSkinData->mJointNames[i].c_str();
					LLJoint* pJoint = getJoint( lookingForJoint );
					if ( pJoint )
					{
						const LLVector3& jointPos = pSkinData->mAlternateBindMatrix[i].getTranslation();
						if (pJoint->aboveJointPosThreshold(jointPos))
						{
							bool override_changed;
							pJoint->addAttachmentPosOverride( jointPos, mesh_id, avString(), override_changed );
							if (override_changed)
							{
								if ( lookingForJoint == "mPelvis" )
								{
									pelvisGotSet = true;
								}
							}
							if (pSkinData->mLockScaleIfJointPosition)
							{
								pJoint->addAttachmentScaleOverride(pJoint->getDefaultScale(), mesh_id, avString());
							}
						}
					}
				}
				if (pelvisZOffset != 0.0F)
				{
					F32 pelvis_fixup_before;
					bool has_fixup_before =  hasPelvisFixup(pelvis_fixup_before);
					addPelvisFixup( pelvisZOffset, mesh_id );
					F32 pelvis_fixup_after;
					hasPelvisFixup(pelvis_fixup_after);
					if (!has_fixup_before || (pelvis_fixup_before != pelvis_fixup_after))
					{
						pelvisGotSet = true;
                    }
				}
                mActiveOverrideMeshes.insert(mesh_id);
                onActiveOverrideMeshesChanged();
			}
		}
		if (pelvisGotSet)
		{
			postPelvisSetRecalc();
		}
	}
}
void LLVOAvatar::getAttachmentOverrideNames(std::set<std::string>& pos_names, std::set<std::string>& scale_names) const
{
    LLVector3 pos;
    LLVector3 scale;
    LLUUID mesh_id;
	for (avatar_joint_list_t::const_iterator iter = mSkeleton.begin();
         iter != mSkeleton.end(); ++iter)
	{
		const LLJoint* pJoint = (*iter);
		if (pJoint && pJoint->hasAttachmentPosOverride(pos,mesh_id))
		{
            pos_names.insert(pJoint->getName());
		}
		if (pJoint && pJoint->hasAttachmentScaleOverride(scale,mesh_id))
		{
            scale_names.insert(pJoint->getName());
		}
	}
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		const LLViewerJointAttachment *attachment_pt = (*iter).second;
        if (attachment_pt && attachment_pt->hasAttachmentPosOverride(pos,mesh_id))
        {
            pos_names.insert(attachment_pt->getName());
        }
    }
}
void LLVOAvatar::showAttachmentOverrides(bool verbose) const
{
    std::set<std::string> pos_names, scale_names;
    getAttachmentOverrideNames(pos_names, scale_names);
    if (pos_names.size())
    {
        std::stringstream ss;
        std::copy(pos_names.begin(), pos_names.end(), std::ostream_iterator<std::string>(ss, ","));
        LL_INFOS() << getFullname() << " attachment positions defined for joints: " << ss.str() << "\n" << LL_ENDL;
    }
    else
    {
        LL_DEBUGS("Avatar") << getFullname() << " no attachment positions defined for any joints" << "\n" << LL_ENDL;
    }
    if (scale_names.size())
    {
        std::stringstream ss;
        std::copy(scale_names.begin(), scale_names.end(), std::ostream_iterator<std::string>(ss, ","));
        LL_INFOS() << getFullname() << " attachment scales defined for joints: " << ss.str() << "\n" << LL_ENDL;
    }
    else
    {
        LL_INFOS() << getFullname() << " no attachment scales defined for any joints" << "\n" << LL_ENDL;
    }
    if (!verbose)
    {
        return;
    }
    LLVector3 pos, scale;
    LLUUID mesh_id;
    S32 count = 0;
	for (avatar_joint_list_t::const_iterator iter = mSkeleton.begin();
         iter != mSkeleton.end(); ++iter)
	{
		const LLJoint* pJoint = (*iter);
		if (pJoint && pJoint->hasAttachmentPosOverride(pos,mesh_id))
		{
			pJoint->showAttachmentPosOverrides(getFullname());
            count++;
		}
		if (pJoint && pJoint->hasAttachmentScaleOverride(scale,mesh_id))
		{
			pJoint->showAttachmentScaleOverrides(getFullname());
            count++;
        }
	}
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		const LLViewerJointAttachment *attachment_pt = (*iter).second;
        if (attachment_pt && attachment_pt->hasAttachmentPosOverride(pos,mesh_id))
        {
            attachment_pt->showAttachmentPosOverrides(getFullname());
            count++;
        }
    }
    if (count)
    {
        LL_DEBUGS("Avatar") << avString() << " end of pos, scale overrides" << LL_ENDL;
        LL_DEBUGS("Avatar") << "=================================" << LL_ENDL;
    }
}
void LLVOAvatar::removeAttachmentOverridesForObject(LLViewerObject *vo)
{
    if (vo->getAvatar() != this && vo->getAvatarAncestor() != this)
	{
		LL_WARNS("Avatar") << "called with invalid avatar" << LL_ENDL;
        return;
	}
	LLViewerObject::const_child_list_t& children = vo->getChildren();
	for (LLViewerObject::const_child_list_t::const_iterator it = children.begin();
		 it != children.end(); ++it)
	{
		LLViewerObject *childp = *it;
		removeAttachmentOverridesForObject(childp);
	}
	LLUUID mesh_id;
	if (getRiggedMeshID(vo,mesh_id))
	{
		removeAttachmentOverridesForObject(mesh_id);
	}
}
void LLVOAvatar::removeAttachmentOverridesForObject(const LLUUID& mesh_id)
{
	LLJoint* pJointPelvis = getJoint("mPelvis");
    const std::string av_string = avString();
    for (S32 joint_num = 0; joint_num < LL_CHARACTER_MAX_ANIMATED_JOINTS; joint_num++)
    {
        LLJoint *pJoint = getJoint(joint_num);
		if ( pJoint )
		{
            bool dummy;
			pJoint->removeAttachmentPosOverride(mesh_id, av_string, dummy);
			pJoint->removeAttachmentScaleOverride(mesh_id, av_string);
		}
		if ( pJoint && pJoint == pJointPelvis)
		{
			removePelvisFixup( mesh_id );
			pJoint->setPosition( LLVector3( 0.0f, 0.0f, 0.0f) );
		}
	}
	postPelvisSetRecalc();
    mActiveOverrideMeshes.erase(mesh_id);
    onActiveOverrideMeshesChanged();
}
LLVector3 LLVOAvatar::getCharacterPosition()
{
	if (mDrawable.notNull())
	{
		return mDrawable->getPositionAgent();
	}
	else
	{
		return getPositionAgent();
	}
}
LLQuaternion LLVOAvatar::getCharacterRotation()
{
	return getRotation();
}
LLVector3 LLVOAvatar::getCharacterVelocity()
{
	return getVelocity() - mStepObjectVelocity;
}
LLVector3 LLVOAvatar::getCharacterAngularVelocity()
{
	return getAngularVelocity();
}
void LLVOAvatar::getGround(const LLVector3 &in_pos_agent, LLVector3 &out_pos_agent, LLVector3 &outNorm)
{
	LLVector3d z_vec(0.0f, 0.0f, 1.0f);
	LLVector3d p0_global, p1_global;
	if (isUIAvatar())
	{
		outNorm.setVec(z_vec);
		out_pos_agent = in_pos_agent;
		return;
	}
	p0_global = gAgent.getPosGlobalFromAgent(in_pos_agent) + z_vec;
	p1_global = gAgent.getPosGlobalFromAgent(in_pos_agent) - z_vec;
	LLViewerObject *obj;
	LLVector3d out_pos_global;
	LLWorld::getInstance()->resolveStepHeightGlobal(this, p0_global, p1_global, out_pos_global, outNorm, &obj);
	out_pos_agent = gAgent.getPosAgentFromGlobal(out_pos_global);
}
F32 LLVOAvatar::getTimeDilation()
{
	return mRegionp ? mRegionp->getTimeDilation() : 1.f;
}
F32 LLVOAvatar::getPixelArea() const
{
	if (isUIAvatar())
	{
		return 100000.f;
	}
	return mPixelArea;
}
LLVector3d	LLVOAvatar::getPosGlobalFromAgent(const LLVector3 &position)
{
	return gAgent.getPosGlobalFromAgent(position);
}
LLVector3	LLVOAvatar::getPosAgentFromGlobal(const LLVector3d &position)
{
	return gAgent.getPosAgentFromGlobal(position);
}
void LLVOAvatar::requestStopMotion( LLMotion* motion )
{
}
BOOL LLVOAvatar::loadSkeletonNode ()
{
	if (!LLAvatarAppearance::loadSkeletonNode())
	{
		return FALSE;
	}
    bool ignore_hud_joints = false;
    initAttachmentPoints(ignore_hud_joints);
	return TRUE;
}
void LLVOAvatar::initAttachmentPoints(bool ignore_hud_joints)
{
	LLAvatarXmlInfo::attachment_info_list_t::iterator iter;
	for (iter = sAvatarXmlInfo->mAttachmentInfoList.begin();
		 iter != sAvatarXmlInfo->mAttachmentInfoList.end();
		 ++iter)
	{
		LLAvatarXmlInfo::LLAvatarAttachmentInfo *info = *iter;
		if (info->mIsHUDAttachment && (!isSelf() || ignore_hud_joints))
		{
			continue;
		}
		S32 attachmentID = info->mAttachmentID;
		if (attachmentID < 1 || attachmentID > 255)
		{
			LL_WARNS() << "Attachment point out of range [1-255]: " << attachmentID << " on attachment point " << info->mName << LL_ENDL;
			continue;
		}
		LLViewerJointAttachment* attachment = NULL;
		bool newly_created = false;
		if (mAttachmentPoints.find(attachmentID) == mAttachmentPoints.end())
		{
			attachment = new LLViewerJointAttachment();
			newly_created = true;
		}
		else
		{
			attachment = mAttachmentPoints[attachmentID];
		}
		attachment->setName(info->mName);
		LLJoint *parent_joint = getJoint(info->mJointName);
		if (!parent_joint)
		{
			LL_WARNS() << "No parent joint by name " << info->mJointName << " found for attachment point " << info->mName << LL_ENDL;
			if( newly_created )
			{
				delete attachment;
			}
			continue;
		}
		if (info->mHasPosition)
		{
			attachment->setOriginalPosition(info->mPosition);
			attachment->setDefaultPosition(info->mPosition);
		}
		if (info->mHasRotation)
		{
			LLQuaternion rotation;
			rotation.setQuat(info->mRotationEuler.mV[VX] * DEG_TO_RAD,
							 info->mRotationEuler.mV[VY] * DEG_TO_RAD,
							 info->mRotationEuler.mV[VZ] * DEG_TO_RAD);
			attachment->setRotation(rotation);
		}
		int group = info->mGroup;
		if (group >= 0)
		{
			if (group < 0 || group >= NUM_ATTACHMENT_GROUPS )
			{
				LL_WARNS() << "Invalid group number (" << group << ") for attachment point " << info->mName << LL_ENDL;
			}
			else
			{
				attachment->setGroup(group);
			}
		}
		attachment->setPieSlice(info->mPieMenuSlice);
		attachment->setVisibleInFirstPerson(info->mVisibleFirstPerson);
		attachment->setIsHUDAttachment(info->mIsHUDAttachment);
		attachment->setJointNum(mNumBones + mCollisionVolumes.size() + attachmentID - 1);
		 if (newly_created)
		{
			mAttachmentPoints[attachmentID] = attachment;
			parent_joint->addChild(attachment);
		}
	}
}
void LLVOAvatar::updateVisualParams()
{
	ESex avatar_sex = (getVisualParamWeight("male") > 0.5f) ? SEX_MALE : SEX_FEMALE;
	if (getSex() != avatar_sex)
	{
		if (mIsSitting && findMotion(avatar_sex == SEX_MALE ? ANIM_AGENT_SIT_FEMALE : ANIM_AGENT_SIT) != NULL)
		{
			stopMotion(ANIM_AGENT_SIT);
			setSex(avatar_sex);
			startMotion(ANIM_AGENT_SIT);
		}
		else
		{
			setSex(avatar_sex);
		}
	}
	LLCharacter::updateVisualParams();
	if (mLastSkeletonSerialNum != mSkeletonSerialNum)
	{
		computeBodySize();
		mLastSkeletonSerialNum = mSkeletonSerialNum;
		mRoot->updateWorldMatrixChildren();
	}
	dirtyMesh();
	updateHeadOffset();
}
BOOL LLVOAvatar::isActive() const
{
	return TRUE;
}
static LLFastTimer::DeclareTimer FTM_PIXEL_AREA("Pixel Area");
void LLVOAvatar::setPixelAreaAndAngle(LLAgent &agent)
{
	LL_RECORD_BLOCK_TIME(FTM_PIXEL_AREA);
	if (mDrawable.isNull())
	{
		return;
	}
	const LLVector4a* ext = mDrawable->getSpatialExtents();
	LLVector4a center;
	center.setAdd(ext[1], ext[0]);
	center.mul(0.5f);
	LLVector4a size;
	size.setSub(ext[1], ext[0]);
	size.mul(0.5f);
	mImpostorPixelArea = LLPipeline::calcPixelArea(center, size, *LLViewerCamera::getInstance());
	F32 range = mDrawable->mDistanceWRTCamera;
	if (range < 0.001f)
	{
		mAppAngle = 180.f;
	}
	else
	{
		F32 radius = size.getLength3().getF32();
		mAppAngle = (F32) atan2( radius, range) * RAD_TO_DEG;
	}
	if( isSelf() )
	{
		mPixelArea = llmax( mPixelArea, F32(getTexImageSize() / 16) );
	}
}
BOOL LLVOAvatar::updateJointLODs()
{
	const F32 MAX_PIXEL_AREA = 100000000.f;
	F32 lod_factor = (sLODFactor * AVATAR_LOD_TWEAK_RANGE + (1.f - AVATAR_LOD_TWEAK_RANGE));
	F32 avatar_num_min_factor = clamp_rescale(sLODFactor, 0.f, 1.f, 0.25f, 0.6f);
	F32 avatar_num_factor = clamp_rescale((F32)sNumVisibleAvatars, 8, 25, 1.f, avatar_num_min_factor);
	F32 area_scale = 0.16f;
		if (isSelf())
		{
			if(gAgentCamera.cameraCustomizeAvatar() || gAgentCamera.cameraMouselook())
			{
				mAdjustedPixelArea = MAX_PIXEL_AREA;
			}
			else
			{
				mAdjustedPixelArea = mPixelArea*area_scale;
			}
		}
		else if (mIsDummy)
		{
			mAdjustedPixelArea = MAX_PIXEL_AREA;
		}
		else
		{
			mAdjustedPixelArea = (F32)mPixelArea * area_scale * lod_factor * lod_factor * avatar_num_factor * avatar_num_factor;
		}
		LLViewerJoint* root = dynamic_cast<LLViewerJoint*>(mRoot);
		BOOL res = FALSE;
		if (root)
		{
			res = root->updateLOD(mAdjustedPixelArea, TRUE);
		}
 		if (res)
		{
			sNumLODChangesThisFrame++;
			dirtyMesh(2);
			return TRUE;
		}
	return FALSE;
}
LLDrawable *LLVOAvatar::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);
	LLDrawPoolAvatar *poolp = (LLDrawPoolAvatar*) gPipeline.getPool(LLDrawPool::POOL_AVATAR);
	mDrawable->setState(LLDrawable::ACTIVE);
	mDrawable->addFace(poolp, NULL);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_AVATAR);
	mNumInitFaces = mDrawable->getNumFaces() ;
	dirtyMesh(2);
	return mDrawable;
}
void LLVOAvatar::updateGL()
{
	if (mMeshTexturesDirty)
	{
		updateMeshTextures();
		mMeshTexturesDirty = FALSE;
	}
}
static LLFastTimer::DeclareTimer FTM_UPDATE_AVATAR("Update Avatar");
BOOL LLVOAvatar::updateGeometry(LLDrawable *drawable)
{
	LL_RECORD_BLOCK_TIME(FTM_UPDATE_AVATAR);
 	if (!(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_AVATAR)))
	{
		return TRUE;
	}
	if (!mMeshValid)
	{
		return TRUE;
	}
	if (!drawable)
	{
		LL_ERRS() << "LLVOAvatar::updateGeometry() called with NULL drawable" << LL_ENDL;
	}
	return TRUE;
}
void LLVOAvatar::updateSexDependentLayerSets( bool upload_bake )
{
	invalidateComposite( mBakedTextureDatas[BAKED_HEAD].mTexLayerSet, upload_bake );
	invalidateComposite( mBakedTextureDatas[BAKED_UPPER].mTexLayerSet, upload_bake );
	invalidateComposite( mBakedTextureDatas[BAKED_LOWER].mTexLayerSet, upload_bake );
}
void LLVOAvatar::dirtyMesh()
{
	dirtyMesh(1);
}
void LLVOAvatar::dirtyMesh(S32 priority)
{
	mDirtyMesh = llmax(mDirtyMesh, priority);
}
LLViewerJoint*	LLVOAvatar::getViewerJoint(S32 idx)
{
	return dynamic_cast<LLViewerJoint*>(mMeshLOD[idx]);
}
void LLVOAvatar::hideSkirt()
{
	mMeshLOD[MESH_ID_SKIRT]->setVisible(FALSE, TRUE);
}
BOOL LLVOAvatar::setParent(LLViewerObject* parent)
{
	BOOL ret ;
	if (parent == NULL)
	{
		getOffObject();
		ret = LLViewerObject::setParent(parent);
		if (isSelf())
		{
			gAgentCamera.resetCamera();
		}
	}
	else
	{
		ret = LLViewerObject::setParent(parent);
		if (ret)
		{
			sitOnObject(parent);
		}
	}
	return ret ;
}
void LLVOAvatar::addChild(LLViewerObject *childp)
{
	childp->extractAttachmentItemID();
	if (isSelf())
	{
	    const LLUUID& item_id = childp->getAttachmentItemID();
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		LL_DEBUGS("Avatar") << "ATT attachment child added " << (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
	}
	LLViewerObject::addChild(childp);
	if (childp->mDrawable)
	{
		if (!attachObject(childp))
		{
			LL_WARNS() << "ATT addChild() failed for "
					<< childp->getID()
					<< " item " << childp->getAttachmentItemID()
					<< LL_ENDL;
		}
	}
	else
	{
		mPendingAttachment.push_back(childp);
	}
}
void LLVOAvatar::removeChild(LLViewerObject *childp)
{
	LLViewerObject::removeChild(childp);
	if (!detachObject(childp))
	{
		LL_WARNS() << "Calling detach on non-attached object " << LL_ENDL;
	}
}
LLViewerJointAttachment* LLVOAvatar::getTargetAttachmentPoint(LLViewerObject* viewer_object)
{
	S32 attachmentID = ATTACHMENT_ID_FROM_STATE(viewer_object->getAttachmentState());
	if (attachmentID & ATTACHMENT_ADD)
	{
		LL_WARNS() << "Got an attachment with ATTACHMENT_ADD mask, removing ( attach pt:" << attachmentID << " )" << LL_ENDL;
		attachmentID &= ~ATTACHMENT_ADD;
	}
	LLViewerJointAttachment* attachment = get_if_there(mAttachmentPoints, attachmentID, (LLViewerJointAttachment*)NULL);
	if (!attachment)
	{
		LL_WARNS() << "Object attachment point invalid: " << attachmentID
			<< " trying to use 1 (chest)"
			<< LL_ENDL;
		attachment = get_if_there(mAttachmentPoints, 1, (LLViewerJointAttachment*)NULL);
		if (attachment)
		{
			LL_WARNS() << "Object attachment point invalid: " << attachmentID
				<< " on object " << viewer_object->getID()
				<< " attachment item " << viewer_object->getAttachmentItemID()
				<< " falling back to 1 (chest)"
				<< LL_ENDL;
		}
		else
		{
			LL_WARNS() << "Object attachment point invalid: " << attachmentID
				<< " on object " << viewer_object->getID()
				<< " attachment item " << viewer_object->getAttachmentItemID()
				<< "Unable to use fallback attachment point 1 (chest)"
				<< LL_ENDL;
		}
	}
	return attachment;
}
const LLViewerJointAttachment *LLVOAvatar::attachObject(LLViewerObject *viewer_object)
{
	if (isSelf())
	{
		const LLUUID& item_id = viewer_object->getAttachmentItemID();
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		LL_DEBUGS("Avatar") << "ATT attaching object "
							<< (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
	}
	LLViewerJointAttachment* attachment = getTargetAttachmentPoint(viewer_object);
	if (!attachment || !attachment->addObject(viewer_object))
	{
		const LLUUID& item_id = viewer_object->getAttachmentItemID();
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		LL_WARNS("Avatar") << "ATT attach failed "
						   << (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
		return 0;
	}
	std::pair<LLViewerObject*, LLViewerJointAttachment*> const val(viewer_object, attachment);
	if (std::find(mAttachedObjectsVector.begin(), mAttachedObjectsVector.end(), val) == mAttachedObjectsVector.end())
	{
		mAttachedObjectsVector.push_back(val);
	}
    if (!viewer_object->isAnimatedObject())
    {
        updateAttachmentOverrides();
    }
	updateVisualComplexity();
	if (viewer_object->isSelected())
	{
		LLSelectMgr::getInstance()->updateSelectionCenter();
		LLSelectMgr::getInstance()->updatePointAt();
	}
	viewer_object->refreshBakeTexture();
	LLViewerObject::const_child_list_t& child_list = viewer_object->getChildren();
	for (const auto& iter : child_list)
    {
		LLViewerObject* objectp = iter;
		if (objectp)
		{
			objectp->refreshBakeTexture();
		}
	}
	updateMeshVisibility();
	return attachment;
}
U32 LLVOAvatar::getNumAttachments() const
{
	U32 num_attachments = 0;
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		const LLViewerJointAttachment *attachment_pt = (*iter).second;
		num_attachments += attachment_pt->getNumObjects();
	}
	return num_attachments;
}
BOOL LLVOAvatar::canAttachMoreObjects(U32 n) const
{
	return (getNumAttachments() + n) <= (U32)LLAgentBenefitsMgr::current().getAttachmentLimit();
}
U32 LLVOAvatar::getNumAnimatedObjectAttachments() const
{
	U32 num_attachments = 0;
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		const LLViewerJointAttachment *attachment_pt = (*iter).second;
		num_attachments += attachment_pt->getNumAnimatedObjects();
	}
	return num_attachments;
}
U32 LLVOAvatar::getMaxAnimatedObjectAttachments() const
{
    if (gSavedSettings.getBOOL("AnimatedObjectsIgnoreLimits"))
        return U32_MAX;
    return LLAgentBenefitsMgr::current().getAnimatedObjectLimit();
}
BOOL LLVOAvatar::canAttachMoreAnimatedObjects(U32 n) const
{
	return (getNumAnimatedObjectAttachments() + n) <= getMaxAnimatedObjectAttachments();
}
void LLVOAvatar::lazyAttach()
{
	std::vector<LLPointer<LLViewerObject> > still_pending;
	for (U32 i = 0; i < mPendingAttachment.size(); i++)
	{
		LLPointer<LLViewerObject> cur_attachment = mPendingAttachment[i];
		if (cur_attachment->mDrawable)
		{
			if (isSelf())
			{
				const LLUUID& item_id = cur_attachment->getAttachmentItemID();
				LLViewerInventoryItem *item = gInventory.getItem(item_id);
				LL_DEBUGS("Avatar") << "ATT attaching object "
									<< (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
			}
			if (!attachObject(cur_attachment))
			{
				LL_WARNS() << "attachObject() failed for "
					<< cur_attachment->getID()
					<< " item " << cur_attachment->getAttachmentItemID()
					<< LL_ENDL;
			}
		}
		else
		{
			still_pending.push_back(cur_attachment);
		}
	}
	mPendingAttachment = still_pending;
}
void LLVOAvatar::resetHUDAttachments()
{
#if SLOW_ATTACHMENT_LIST
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (attachment->getIsHUDAttachment())
		{
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
				 attachment_iter != attachment->mAttachedObjects.end();
				 ++attachment_iter)
			{
				const LLViewerObject* attached_object = (*attachment_iter);
#else
	for(auto& iter : mAttachedObjectsVector)
	{{{
				const LLViewerJointAttachment* attachment = iter.second;
				if (!attachment->getIsHUDAttachment())
					continue;
				const LLViewerObject* attached_object = iter.first;
#endif
				if (attached_object && attached_object->mDrawable.notNull())
				{
					gPipeline.markMoved(attached_object->mDrawable);
				}
			}
		}
	}
}
void LLVOAvatar::rebuildRiggedAttachments( void )
{
#if SLOW_ATTACHMENT_LIST
	for ( attachment_map_t::iterator iter = mAttachmentPoints.begin(); iter != mAttachmentPoints.end(); ++iter )
	{
		LLViewerJointAttachment* pAttachment = iter->second;
		LLViewerJointAttachment::attachedobjs_vec_t::iterator attachmentIterEnd = pAttachment->mAttachedObjects.end();
		for ( LLViewerJointAttachment::attachedobjs_vec_t::iterator attachmentIter = pAttachment->mAttachedObjects.begin();
			 attachmentIter != attachmentIterEnd; ++attachmentIter)
		{
			const LLViewerObject* pAttachedObject =  *attachmentIter;
#else
	for(auto& iter : mAttachedObjectsVector)
	{{
			const LLViewerObject* pAttachedObject = iter.first;
			const LLViewerJointAttachment* pAttachment = iter.second;
#endif
			if ( pAttachment && pAttachedObject->mDrawable.notNull() )
			{
				gPipeline.markRebuild(pAttachedObject->mDrawable);
			}
		}
	}
}
void LLVOAvatar::cleanupAttachedMesh( LLViewerObject* pVO )
{
	LLUUID mesh_id;
	if (getRiggedMeshID(pVO, mesh_id))
	{
		if ( gAgentCamera.cameraCustomizeAvatar() )
		{
			gAgent.unpauseAnimation();
			gAgentCamera.changeCameraToCustomizeAvatar();
		}
	}
}
BOOL LLVOAvatar::detachObject(LLViewerObject *viewer_object)
{
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (attachment->isObjectAttached(viewer_object))
		{
			updateVisualComplexity();
			vector_replace_with_last(mAttachedObjectsVector,std::make_pair(viewer_object,attachment));
			bool is_animated_object = viewer_object->isAnimatedObject();
			cleanupAttachedMesh( viewer_object );
			attachment->removeObject(viewer_object);
			if (!is_animated_object)
			{
				updateAttachmentOverrides();
			}
			viewer_object->refreshBakeTexture();
			LLViewerObject::const_child_list_t& child_list = viewer_object->getChildren();
			for (const auto& iter1 : child_list)
            {
				LLViewerObject* objectp = iter1;
				if (objectp)
            {
					objectp->refreshBakeTexture();
				}
            }
			updateMeshVisibility();
			LL_DEBUGS() << "Detaching object " << viewer_object->mID << " from " << attachment->getName() << LL_ENDL;
			return TRUE;
		}
	}
	std::vector<LLPointer<LLViewerObject> >::iterator iter = std::find(mPendingAttachment.begin(), mPendingAttachment.end(), viewer_object);
	if (iter != mPendingAttachment.end())
	{
		mPendingAttachment.erase(iter);
		return TRUE;
	}
	return FALSE;
}
void LLVOAvatar::sitDown(BOOL bSitting)
{
	if (isSitting() != bSitting) mIdleTimer.reset();
	mIsSitting = bSitting;
	if (isSelf())
	{
		if (rlv_handler_t::isEnabled())
		{
			gRlvHandler.onSitOrStand(bSitting);
		}
	}
}
void LLVOAvatar::sitOnObject(LLViewerObject *sit_object)
{
	if (isSelf())
	{
		LLFirstUse::useSit();
		gAgent.setFlying(FALSE);
		gAgentCamera.setThirdPersonHeadOffset(LLVector3::zero);
		gAgentCamera.startCameraAnimation();
		gAgent.stopAutoPilot();
		gAgentCamera.setupSitCamera();
		if (gAgentCamera.getForceMouselook())
		{
			gAgentCamera.changeCameraToMouselook();
		}
	}
	if (mDrawable.isNull() || sit_object->mDrawable.isNull())
	{
		return;
	}
	LLQuaternion inv_obj_rot = ~sit_object->getRenderRotation();
	LLVector3 obj_pos = sit_object->getRenderPosition();
	LLVector3 rel_pos = getRenderPosition() - obj_pos;
	rel_pos.rotVec(inv_obj_rot);
	mDrawable->mXform.setPosition(rel_pos);
	mDrawable->mXform.setRotation(mDrawable->getWorldRotation() * inv_obj_rot);
	gPipeline.markMoved(mDrawable, TRUE);
	sitDown(TRUE);
	mRoot->getXform()->setParent(&sit_object->mDrawable->mXform);
	mRoot->setPosition(getPosition());
	mRoot->updateWorldMatrixChildren();
	stopMotion(ANIM_AGENT_BODY_NOISE);
}
void LLVOAvatar::getOffObject()
{
	if (mDrawable.isNull())
	{
		return;
	}
	LLViewerObject* sit_object = (LLViewerObject*)getParent();
	if (sit_object)
	{
		stopMotionFromSource(sit_object->getID());
		LLFollowCamMgr::setCameraActive(sit_object->getID(), FALSE);
		LLViewerObject::const_child_list_t& child_list = sit_object->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); ++iter)
		{
			LLViewerObject* child_objectp = *iter;
			stopMotionFromSource(child_objectp->getID());
			LLFollowCamMgr::setCameraActive(child_objectp->getID(), FALSE);
		}
	}
	LLVector3 cur_position_world = mDrawable->getWorldPosition();
	LLQuaternion cur_rotation_world = mDrawable->getWorldRotation();
	if (mLastRootPos.length() >= MAX_STANDOFF_FROM_ORIGIN
		&& (cur_position_world.length() < MAX_STANDOFF_FROM_ORIGIN
			|| dist_vec(cur_position_world, mLastRootPos) > MAX_STANDOFF_DISTANCE_CHANGE))
	{
		cur_position_world = mLastRootPos;
	}
	mDrawable->mXform.setPosition(cur_position_world);
	mDrawable->mXform.setRotation(cur_rotation_world);
	gPipeline.markMoved(mDrawable, TRUE);
	sitDown(FALSE);
	mRoot->getXform()->setParent(NULL);
	mRoot->setPosition(cur_position_world);
	mRoot->setRotation(cur_rotation_world);
	mRoot->getXform()->update();
	if (mEnableDefaultMotions)
	{
	startMotion(ANIM_AGENT_BODY_NOISE);
	}
	if (isSelf())
	{
		LLQuaternion av_rot = gAgent.getFrameAgent().getQuaternion();
		LLQuaternion obj_rot = sit_object ? sit_object->getRenderRotation() : LLQuaternion::DEFAULT;
		av_rot = av_rot * obj_rot;
		LLVector3 at_axis = LLVector3::x_axis;
		at_axis = at_axis * av_rot;
		at_axis.mV[VZ] = 0.f;
		at_axis.normalize();
		gAgent.resetAxes(at_axis);
		gAgentCamera.setThirdPersonHeadOffset(LLVector3(0.f, 0.f, 1.f));
		gAgentCamera.setSitCamera(LLUUID::null);
		if (sit_object && !sit_object->permYouOwner() && gSavedSettings.getBOOL("RevokePermsOnStandUp"))
		{
			U32 permissions = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_TRIGGER_ANIMATION] | LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_OVERRIDE_ANIMATIONS];
			gAgent.sendRevokePermissions(sit_object->getID(), permissions);
		}
	}
}
LLVOAvatar* LLVOAvatar::findAvatarFromAttachment( LLViewerObject* obj )
{
	if( obj->isAttachment() )
	{
		do
		{
			obj = (LLViewerObject*) obj->getParent();
		}
		while( obj && !obj->isAvatar() );
		if( obj && !obj->isDead() )
		{
			return (LLVOAvatar*)obj;
		}
	}
	return NULL;
}
S32 LLVOAvatar::getAttachmentCount()
{
	S32 count = mAttachmentPoints.size();
	return count;
}
BOOL LLVOAvatar::isWearingWearableType(LLWearableType::EType type) const
{
	if (mIsDummy) return TRUE;
	if (isSelf())
	{
		return LLAvatarAppearance::isWearingWearableType(type);
	}
	switch(type)
	{
		case LLWearableType::WT_SHAPE:
		case LLWearableType::WT_SKIN:
		case LLWearableType::WT_HAIR:
		case LLWearableType::WT_EYES:
			return TRUE;
		default:
			break;
	}
	for (LLAvatarAppearanceDictionary::Textures::const_iterator tex_iter = LLAvatarAppearanceDictionary::getInstance()->getTextures().begin();
		 tex_iter != LLAvatarAppearanceDictionary::getInstance()->getTextures().end();
		 ++tex_iter)
	{
		const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = tex_iter->second;
		if (texture_dict->mWearableType == type)
		{
			if (texture_dict->mIsUsedByBakedTexture)
			{
				const EBakedTextureIndex baked_index = texture_dict->mBakedTextureIndex;
				return isTextureDefined(LLAvatarAppearanceDictionary::getInstance()->getBakedTexture(baked_index)->mTextureIndex);
			}
			return FALSE;
		}
	}
	return FALSE;
}
BOOL LLVOAvatar::isWearingAttachment( const LLUUID& inv_item_id )
{
	const LLUUID& base_inv_item_id = gInventory.getLinkedItemID(inv_item_id);
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end(); )
	{
		attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if(attachment->getAttachedObject(base_inv_item_id))
		{
			return TRUE;
		}
	}
	return FALSE;
}
LLViewerObject* LLVOAvatar::getWornAttachment( const LLUUID& inv_item_id )
{
	const LLUUID& base_inv_item_id = gInventory.getLinkedItemID(inv_item_id);
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end(); )
	{
		attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
 		if (LLViewerObject *attached_object = attachment->getAttachedObject(base_inv_item_id))
		{
			return attached_object;
		}
	}
	return NULL;
}
LLViewerObject *	LLVOAvatar::findAttachmentByID( const LLUUID & target_id ) const
{
#if SLOW_ATTACHMENT_LIST
	for(attachment_map_t::const_iterator attachment_points_iter = mAttachmentPoints.begin();
		attachment_points_iter != mAttachmentPoints.end();
		++attachment_points_iter)
	{
		LLViewerJointAttachment* attachment = attachment_points_iter->second;
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			LLViewerObject *attached_object = (*attachment_iter);
#else
	for(auto& iter : mAttachedObjectsVector)
	{{
			LLViewerObject* attached_object =  iter.first;
#endif
			if (attached_object &&
				attached_object->getID() == target_id)
			{
				return attached_object;
			}
		}
	}
	return NULL;
}
void LLVOAvatar::invalidateComposite( LLTexLayerSet* layerset, BOOL upload_result )
{
}
void LLVOAvatar::invalidateAll()
{
}
void LLVOAvatar::onGlobalColorChanged(const LLTexGlobalColor* global_color, bool upload_bake )
{
	if (global_color == mTexSkinColor)
	{
		invalidateComposite( mBakedTextureDatas[BAKED_HEAD].mTexLayerSet, upload_bake );
		invalidateComposite( mBakedTextureDatas[BAKED_UPPER].mTexLayerSet, upload_bake );
		invalidateComposite( mBakedTextureDatas[BAKED_LOWER].mTexLayerSet, upload_bake );
	}
	else if (global_color == mTexHairColor)
	{
		invalidateComposite( mBakedTextureDatas[BAKED_HEAD].mTexLayerSet, upload_bake );
		invalidateComposite( mBakedTextureDatas[BAKED_HAIR].mTexLayerSet, upload_bake );
		if (!isTextureDefined(mBakedTextureDatas[BAKED_HAIR].mTextureIndex))
		{
			LLColor4 color = mTexHairColor->getColor();
			avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[BAKED_HAIR].mJointMeshes.begin();
			avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[BAKED_HAIR].mJointMeshes.end();
			for (; iter != end; ++iter)
			{
				LLAvatarJointMesh* mesh = (*iter);
				if (mesh)
				{
					mesh->setColor( color );
				}
			}
		}
	}
	else if (global_color == mTexEyeColor)
	{
		invalidateComposite( mBakedTextureDatas[BAKED_EYES].mTexLayerSet,  upload_bake );
	}
	updateMeshTextures();
}
bool LLVOAvatar::shouldRenderRigged() const
{
    return true;
}
BOOL LLVOAvatar::isVisible() const
{
	return mDrawable.notNull()
		&& (!mOrphaned || isSelf())
		&& (mDrawable->isVisible() || mIsDummy);
}
BOOL LLVOAvatar::getIsCloud() const
{
	if (mIsDummy)
	{
		return false;
	}
	return (   ((const_cast<LLVOAvatar*>(this))->visualParamWeightsAreDefault())
			|| (   !isTextureDefined(TEX_LOWER_BAKED)
				|| !isTextureDefined(TEX_UPPER_BAKED)
				|| !isTextureDefined(TEX_HEAD_BAKED)
				)
			);
}
void LLVOAvatar::updateRezzedStatusTimers()
{
	S32 rez_status = getRezzedStatus();
	if (rez_status != mLastRezzedStatus)
	{
		LL_DEBUGS("Avatar") << avString() << "rez state change: " << mLastRezzedStatus << " -> " << rez_status << LL_ENDL;
		if (mLastRezzedStatus == -1 && rez_status != -1)
		{
			for (S32 i = 1; i < 4; i++)
			{
				startPhase("load_" + LLVOAvatar::rezStatusToString(i));
				startPhase("first_load_" + LLVOAvatar::rezStatusToString(i));
			}
		}
		if (rez_status < mLastRezzedStatus)
		{
			for (S32 i = rez_status+1; i <= mLastRezzedStatus; i++)
			{
				startPhase("load_" + LLVOAvatar::rezStatusToString(i));
			}
		}
		else if (rez_status > mLastRezzedStatus)
		{
			for (S32 i = llmax(mLastRezzedStatus+1,1); i <= rez_status; i++)
			{
				stopPhase("load_" + LLVOAvatar::rezStatusToString(i));
				stopPhase("first_load_" + LLVOAvatar::rezStatusToString(i), false);
			}
			if (rez_status == 3)
			{
				selfStopPhase("update_appearance_from_cof");
				selfStopPhase("wear_inventory_category", false);
				selfStopPhase("process_initial_wearables_update", false);
				updateVisualComplexity();
			}
		}
		mLastRezzedStatus = rez_status;
	}
}
void LLVOAvatar::clearPhases()
{
	getPhases().clearPhases();
}
void LLVOAvatar::startPhase(const std::string& phase_name)
{
	F32 elapsed = 0.0;
	bool completed = false;
	bool found = getPhases().getPhaseValues(phase_name, elapsed, completed);
	if (found)
	{
		if (!completed)
		{
			LL_DEBUGS("Avatar") << avString() << "no-op, start when started already for " << phase_name << LL_ENDL;
			return;
		}
	}
	LL_DEBUGS("Avatar") << "started phase " << phase_name << LL_ENDL;
	getPhases().startPhase(phase_name);
}
void LLVOAvatar::stopPhase(const std::string& phase_name, bool err_check)
{
	F32 elapsed = 0.0;
	bool completed = false;
	if (getPhases().getPhaseValues(phase_name, elapsed, completed))
	{
		if (!completed)
		{
			getPhases().stopPhase(phase_name);
			completed = true;
			logMetricsTimerRecord(phase_name, elapsed, completed);
			LL_DEBUGS("Avatar") << avString() << "stopped phase " << phase_name << " elapsed " << elapsed << LL_ENDL;
		}
		else
		{
			if (err_check)
			{
				LL_DEBUGS("Avatar") << "no-op, stop when stopped already for " << phase_name << LL_ENDL;
			}
		}
	}
	else
	{
		if (err_check)
		{
			LL_DEBUGS("Avatar") << "no-op, stop when not started for " << phase_name << LL_ENDL;
		}
	}
}
void LLVOAvatar::logPendingPhases()
{
	if (!isAgentAvatarValid())
	{
		return;
	}
	for (LLViewerStats::phase_map_t::iterator it = getPhases().begin();
		 it != getPhases().end();
		 ++it)
	{
		const std::string& phase_name = it->first;
		F32 elapsed;
		bool completed;
		if (getPhases().getPhaseValues(phase_name, elapsed, completed))
		{
			if (!completed)
			{
				logMetricsTimerRecord(phase_name, elapsed, completed);
			}
		}
	}
}
void LLVOAvatar::logPendingPhasesAllAvatars()
{
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		if( inst->isDead() )
		{
			continue;
		}
		inst->logPendingPhases();
	}
}
void LLVOAvatar::logMetricsTimerRecord(const std::string& phase_name, F32 elapsed, bool completed)
{
	if (!isAgentAvatarValid())
	{
		return;
	}
	LLSD record;
	record["timer_name"] = phase_name;
	record["avatar_id"] = getID();
	record["elapsed"] = elapsed;
	record["completed"] = completed;
	U32 grid_x(0), grid_y(0);
	if (getRegion())
	{
		record["central_bake_version"] = LLSD::Integer(getRegion()->getCentralBakeVersion());
		grid_from_region_handle(getRegion()->getHandle(), &grid_x, &grid_y);
	}
	record["grid_x"] = LLSD::Integer(grid_x);
	record["grid_y"] = LLSD::Integer(grid_y);
	record["is_using_server_bakes"] = ((bool) isUsingServerBakes());
	record["is_self"] = isSelf();
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->addMetricsTimerRecord(record);
	}
}
BOOL LLVOAvatar::updateIsFullyLoaded()
{
	const BOOL loading = getIsCloud();
	updateRezzedStatusTimers();
	updateRuthTimer(loading);
	return processFullyLoadedChange(loading);
}
void LLVOAvatar::updateRuthTimer(bool loading)
{
	if (isSelf() || !loading)
	{
		return;
	}
	if (mPreviousFullyLoaded)
	{
		mRuthTimer.reset();
		debugAvatarRezTime("AvatarRezCloudNotification","became cloud");
	}
	const F32 LOADING_TIMEOUT__SECONDS = 120.f;
	if (mRuthTimer.getElapsedTimeF32() > LOADING_TIMEOUT__SECONDS)
	{
		LL_DEBUGS("Avatar") << avString()
				<< "Ruth Timer timeout: Missing texture data for '" << getFullname() << "' "
				<< "( Params loaded : " << !visualParamWeightsAreDefault() << " ) "
				<< "( Lower : " << isTextureDefined(TEX_LOWER_BAKED) << " ) "
				<< "( Upper : " << isTextureDefined(TEX_UPPER_BAKED) << " ) "
				<< "( Head : " << isTextureDefined(TEX_HEAD_BAKED) << " )."
				<< LL_ENDL;
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarTexturesRequest(getID());
		mRuthTimer.reset();
	}
}
BOOL LLVOAvatar::processFullyLoadedChange(bool loading)
{
	const F32 PAUSE = 1.f;
	if (loading)
		mFullyLoadedTimer.reset();
	mFullyLoaded = (mFullyLoadedTimer.getElapsedTimeF32() > PAUSE);
	if (!mPreviousFullyLoaded && !loading && mFullyLoaded)
	{
		debugAvatarRezTime("AvatarRezNotification","fully loaded");
	}
	const S32 UPDATE_RATE = 30;
	BOOL changed =
		((mFullyLoaded != mPreviousFullyLoaded) ||
		 (!mFullyLoadedInitialized) ||
		 (mFullyLoadedFrameCounter % UPDATE_RATE == 0));
	mPreviousFullyLoaded = mFullyLoaded;
	mFullyLoadedInitialized = TRUE;
	mFullyLoadedFrameCounter++;
	return changed;
}
BOOL LLVOAvatar::isFullyLoaded() const
{
	static const LLCachedControl<bool> render_unloaded_avatar("RenderUnloadedAvatar", false);
	return (render_unloaded_avatar && !isSelf()) ||(mFullyLoaded);
}
bool LLVOAvatar::isTooComplex() const
{
	static const LLCachedControl<S32> always_render_friends("AlwaysRenderFriends", 0);
	bool too_complex;
	if (isSelf() || (always_render_friends && always_render_friends != 3 && !isControlAvatar() && LLAvatarTracker::instance().isBuddy(getID())))
	{
		too_complex = false;
	}
	else if (always_render_friends >= 2 && !isControlAvatar())
	{
		too_complex = true;
	}
	else
	{
		static LLCachedControl<U32> max_render_cost(gSavedSettings, "RenderAvatarMaxComplexity", 0U);
		static LLCachedControl<F32> max_attachment_area(gSavedSettings, "RenderAutoMuteSurfaceAreaLimit", 1000.0f);
		too_complex = (   max_render_cost > 0
						  && (mVisualComplexity > max_render_cost
								|| (max_attachment_area > 0.0f && mAttachmentSurfaceArea > max_attachment_area)
							  ));
	}
	return too_complex;
}
LLMotion* LLVOAvatar::findMotion(const LLUUID& id) const
{
	return mMotionController.findMotion(id);
}
void LLVOAvatar::debugColorizeSubMeshes(U32 i, const LLColor4& color)
{
	static LLCachedControl<bool> debug_avatar_comp_baked(gSavedSettings, "DebugAvatarCompositeBaked");
	if (debug_avatar_comp_baked)
	{
		avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[i].mJointMeshes.begin();
		avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[i].mJointMeshes.end();
		for (; iter != end; ++iter)
		{
			LLAvatarJointMesh* mesh = (*iter);
			if (mesh)
			{
				mesh->setColor(color);
			}
		}
	}
}
void LLVOAvatar::updateMeshVisibility()
{
	bool bake_flag[BAKED_NUM_INDICES];
	memset(bake_flag, 0, BAKED_NUM_INDICES*sizeof(bool));
	for (auto& attachment_point : mAttachmentPoints)
    {
		LLViewerJointAttachment* attachment = attachment_point.second;
		if (attachment)
		{
			for (auto objectp : attachment->mAttachedObjects)
            {
                if (objectp)
				{
					for (int face_index = 0; face_index < objectp->getNumTEs(); face_index++)
					{
						LLTextureEntry* tex_entry = objectp->getTE(face_index);
						bake_flag[BAKED_HEAD] |= (tex_entry->getID() == IMG_USE_BAKED_HEAD);
						bake_flag[BAKED_EYES] |= (tex_entry->getID() == IMG_USE_BAKED_EYES);
						bake_flag[BAKED_HAIR] |= (tex_entry->getID() == IMG_USE_BAKED_HAIR);
						bake_flag[BAKED_LOWER] |= (tex_entry->getID() == IMG_USE_BAKED_LOWER);
						bake_flag[BAKED_UPPER] |= (tex_entry->getID() == IMG_USE_BAKED_UPPER);
						bake_flag[BAKED_SKIRT] |= (tex_entry->getID() == IMG_USE_BAKED_SKIRT);
						bake_flag[BAKED_LEFT_ARM] |= (tex_entry->getID() == IMG_USE_BAKED_LEFTARM);
						bake_flag[BAKED_LEFT_LEG] |= (tex_entry->getID() == IMG_USE_BAKED_LEFTLEG);
						bake_flag[BAKED_AUX1] |= (tex_entry->getID() == IMG_USE_BAKED_AUX1);
						bake_flag[BAKED_AUX2] |= (tex_entry->getID() == IMG_USE_BAKED_AUX2);
						bake_flag[BAKED_AUX3] |= (tex_entry->getID() == IMG_USE_BAKED_AUX3);
					}
				}
				LLViewerObject::const_child_list_t& child_list = objectp->getChildren();
				for (const auto& iter1 : child_list)
                {
					LLViewerObject* objectchild = iter1;
					if (objectchild)
					{
						for (int face_index = 0; face_index < objectchild->getNumTEs(); face_index++)
						{
							LLTextureEntry* tex_entry = objectchild->getTE(face_index);
							bake_flag[BAKED_HEAD] |= (tex_entry->getID() == IMG_USE_BAKED_HEAD);
							bake_flag[BAKED_EYES] |= (tex_entry->getID() == IMG_USE_BAKED_EYES);
							bake_flag[BAKED_HAIR] |= (tex_entry->getID() == IMG_USE_BAKED_HAIR);
							bake_flag[BAKED_LOWER] |= (tex_entry->getID() == IMG_USE_BAKED_LOWER);
							bake_flag[BAKED_UPPER] |= (tex_entry->getID() == IMG_USE_BAKED_UPPER);
							bake_flag[BAKED_SKIRT] |= (tex_entry->getID() == IMG_USE_BAKED_SKIRT);
							bake_flag[BAKED_LEFT_ARM] |= (tex_entry->getID() == IMG_USE_BAKED_LEFTARM);
							bake_flag[BAKED_LEFT_LEG] |= (tex_entry->getID() == IMG_USE_BAKED_LEFTLEG);
							bake_flag[BAKED_AUX1] |= (tex_entry->getID() == IMG_USE_BAKED_AUX1);
							bake_flag[BAKED_AUX2] |= (tex_entry->getID() == IMG_USE_BAKED_AUX2);
							bake_flag[BAKED_AUX3] |= (tex_entry->getID() == IMG_USE_BAKED_AUX3);
						}
					}
				}
			}
		}
	}
	for (size_t i = 0; i < mMeshLOD.size(); i++)
	{
		LLAvatarJoint* joint = mMeshLOD[i];
		if (i == MESH_ID_HAIR)
		{
			joint->setVisible(!bake_flag[BAKED_HAIR], TRUE);
		}
		else if (i == MESH_ID_HEAD)
		{
			joint->setVisible(!bake_flag[BAKED_HEAD], TRUE);
		}
		else if (i == MESH_ID_SKIRT)
		{
			joint->setVisible(!bake_flag[BAKED_SKIRT], TRUE);
		}
		else if (i == MESH_ID_UPPER_BODY)
		{
			joint->setVisible(!bake_flag[BAKED_UPPER], TRUE);
		}
		else if (i == MESH_ID_LOWER_BODY)
		{
			joint->setVisible(!bake_flag[BAKED_LOWER], TRUE);
		}
		else if (i == MESH_ID_EYEBALL_LEFT)
		{
			joint->setVisible(!bake_flag[BAKED_EYES], TRUE);
		}
		else if (i == MESH_ID_EYEBALL_RIGHT)
		{
			joint->setVisible(!bake_flag[BAKED_EYES], TRUE);
		}
		else if (i == MESH_ID_EYELASH)
		{
			joint->setVisible(!bake_flag[BAKED_HEAD], TRUE);
		}
	}
}
void LLVOAvatar::updateMeshTextures()
{
	static S32 update_counter = 0;
	mBakedTextureDebugText.clear();
	for (U32 i=0; i < getNumTEs(); i++)
	{
		const LLViewerTexture* te_image = getImage(i, 0);
		if(!te_image || te_image->getID().isNull() || (te_image->getID() == IMG_DEFAULT))
		{
			const LLUUID& image_id = (i == TEX_HAIR ? IMG_DEFAULT : IMG_DEFAULT_AVATAR);
			setImage(i, LLViewerTextureManager::getFetchedTexture(image_id), 0);
		}
	}
	const BOOL other_culled = !isSelf() && mCulled;
	LLLoadedCallbackEntry::source_callback_list_t* src_callback_list = NULL ;
	BOOL paused = FALSE;
	if(!isSelf())
	{
		src_callback_list = &mCallbackTextureList ;
		paused = !isVisible();
	}
	std::vector<BOOL> is_layer_baked;
	is_layer_baked.resize(mBakedTextureDatas.size(), false);
	std::vector<BOOL> use_lkg_baked_layer;
	use_lkg_baked_layer.resize(mBakedTextureDatas.size(), false);
	mBakedTextureDebugText += llformat("%06d\n",update_counter++);
	mBakedTextureDebugText += "indx layerset linvld ltda ilb ulkg ltid\n";
	for (U32 i=0; i < mBakedTextureDatas.size(); i++)
	{
		is_layer_baked[i] = isTextureDefined(mBakedTextureDatas[i].mTextureIndex);
		LLViewerTexLayerSet* layerset = NULL;
		bool layerset_invalid = false;
		if (!other_culled)
		{
			layerset = getTexLayerSet(i);
			layerset_invalid = layerset && ( !layerset->getViewerComposite()->isInitialized()
											 || !layerset->isLocalTextureDataAvailable() );
			use_lkg_baked_layer[i] = (!is_layer_baked[i]
									  && (mBakedTextureDatas[i].mLastTextureID != IMG_DEFAULT_AVATAR)
									  && layerset_invalid);
			if (use_lkg_baked_layer[i])
			{
				layerset->setUpdatesEnabled(TRUE);
			}
		}
		else
		{
			use_lkg_baked_layer[i] = (!is_layer_baked[i]
									  && mBakedTextureDatas[i].mLastTextureID != IMG_DEFAULT_AVATAR);
		}
		std::string last_id_string;
		if (mBakedTextureDatas[i].mLastTextureID == IMG_DEFAULT_AVATAR)
			last_id_string = "A";
		else if (mBakedTextureDatas[i].mLastTextureID == IMG_DEFAULT)
			last_id_string = "D";
		else if (mBakedTextureDatas[i].mLastTextureID == IMG_INVISIBLE)
			last_id_string = "I";
		else
			last_id_string = "*";
		bool is_ltda = layerset
			&& layerset->getViewerComposite()->isInitialized()
			&& layerset->isLocalTextureDataAvailable();
		mBakedTextureDebugText += llformat("%4d   %4s     %4d %4d %4d %4d %4s\n",
										   i,
										   (layerset?"*":"0"),
										   layerset_invalid,
										   is_ltda,
										   is_layer_baked[i],
										   use_lkg_baked_layer[i],
										   last_id_string.c_str());
	}
	mSupportsAlphaLayers = isSelf() || is_layer_baked[BAKED_HAIR];
	for (U32 i=0; i < mBakedTextureDatas.size(); i++)
	{
		debugColorizeSubMeshes(i, LLColor4::white);
		LLViewerTexLayerSet* layerset = getTexLayerSet(i);
		if (use_lkg_baked_layer[i] && !isUsingLocalAppearance() )
		{
			LLViewerFetchedTexture* baked_img;
#ifndef LL_RELEASE_FOR_DOWNLOAD
			LLViewerFetchedTexture* existing_baked_img = LLViewerTextureManager::getFetchedTexture(mBakedTextureDatas[i].mLastTextureID);
#endif
			ETextureIndex te = ETextureIndex(mBakedTextureDatas[i].mTextureIndex);
			const std::string url = getImageURL(te, mBakedTextureDatas[i].mLastTextureID);
			const LLGLTexture::EBoostLevel bake_boost = isSelf()
				? LLGLTexture::BOOST_AVATAR_BAKED_SELF
				: LLGLTexture::BOOST_AVATAR_BAKED;
			if (!url.empty())
			{
				baked_img = LLViewerTextureManager::getFetchedTextureFromUrl(url, FTT_HOST_BAKE, TRUE, bake_boost, LLViewerTexture::LOD_TEXTURE, 0, 0, mBakedTextureDatas[i].mLastTextureID);
			}
			else
			{
				const LLHost target_host = getObjectHost();
				if (!target_host.isOk())
				{
					LL_WARNS() << "updateMeshTextures: invalid host for object: " << getID() << LL_ENDL;
				}
				baked_img = LLViewerTextureManager::getFetchedTextureFromHost( mBakedTextureDatas[i].mLastTextureID, FTT_HOST_BAKE, target_host );
			}
			llassert(baked_img == existing_baked_img);
			if (baked_img)
			{
				baked_img->setBoostLevel(bake_boost);
			}
			mBakedTextureDatas[i].mIsUsed = TRUE;
			debugColorizeSubMeshes(i,LLColor4::red);
			avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[i].mJointMeshes.begin();
			avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[i].mJointMeshes.end();
			for (; iter != end; ++iter)
			{
				LLAvatarJointMesh* mesh = (*iter);
				if (mesh)
				{
					mesh->setTexture( baked_img );
				}
			}
		}
		else if (!isUsingLocalAppearance() && is_layer_baked[i])
		{
			LLViewerFetchedTexture* baked_img =
				LLViewerTextureManager::staticCastToFetchedTexture(
					getImage( mBakedTextureDatas[i].mTextureIndex, 0 ), TRUE) ;
			if( baked_img->getID() == mBakedTextureDatas[i].mLastTextureID )
			{
				useBakedTexture( baked_img->getID() );
                                mLoadedCallbacksPaused |= !isVisible();
                                checkTextureLoading();
			}
			else
			{
				mBakedTextureDatas[i].mIsLoaded = FALSE;
				if (baked_img)
				{
					baked_img->setBoostLevel(isSelf()
						? LLGLTexture::BOOST_AVATAR_BAKED_SELF
						: LLGLTexture::BOOST_AVATAR_BAKED);
				}
				if ( (baked_img->getID() != IMG_INVISIBLE) &&
					 ((i == BAKED_HEAD) || (i == BAKED_UPPER) || (i == BAKED_LOWER)) )
				{
					const BOOL need_aux = !isUsingServerBakes();
					baked_img->setLoadedCallback(onBakedTextureMasksLoaded, MORPH_MASK_REQUESTED_DISCARD, TRUE, need_aux, new LLTextureMaskData( mID ),
						src_callback_list, paused);
				}
				baked_img->setLoadedCallback(onBakedTextureLoaded, SWITCH_TO_BAKED_DISCARD, FALSE, FALSE, new LLUUID( mID ),
					src_callback_list, paused );
				mLoadedCallbacksPaused |= paused;
				checkTextureLoading();
			}
		}
		else if (layerset && isUsingLocalAppearance())
		{
			debugColorizeSubMeshes(i,LLColor4::yellow );
			layerset->createComposite();
			layerset->setUpdatesEnabled( TRUE );
			mBakedTextureDatas[i].mIsUsed = FALSE;
			avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[i].mJointMeshes.begin();
			avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[i].mJointMeshes.end();
			for (; iter != end; ++iter)
			{
				LLAvatarJointMesh* mesh = (*iter);
				if (mesh)
				{
					mesh->setLayerSet( layerset );
				}
			}
		}
		else
		{
			debugColorizeSubMeshes(i,LLColor4::blue);
		}
	}
	if (!is_layer_baked[BAKED_HAIR])
	{
		const LLColor4 color = mTexHairColor ? mTexHairColor->getColor() : LLColor4(1,1,1,1);
		LLViewerTexture* hair_img = getImage( TEX_HAIR, 0 );
		avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[BAKED_HAIR].mJointMeshes.begin();
		avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[BAKED_HAIR].mJointMeshes.end();
		for (; iter != end; ++iter)
		{
			LLAvatarJointMesh* mesh = (*iter);
			if (mesh)
			{
				mesh->setColor( color );
				mesh->setTexture( hair_img );
			}
		}
	}
	for (LLAvatarAppearanceDictionary::BakedTextures::const_iterator baked_iter =
			 LLAvatarAppearanceDictionary::getInstance()->getBakedTextures().begin();
		 baked_iter != LLAvatarAppearanceDictionary::getInstance()->getBakedTextures().end();
		 ++baked_iter)
	{
		const EBakedTextureIndex baked_index = baked_iter->first;
		const LLAvatarAppearanceDictionary::BakedEntry *baked_dict = baked_iter->second;
		for (texture_vec_t::const_iterator local_tex_iter = baked_dict->mLocalTextures.begin();
			 local_tex_iter != baked_dict->mLocalTextures.end();
			 ++local_tex_iter)
		{
			const ETextureIndex texture_index = *local_tex_iter;
			const BOOL is_baked_ready = (is_layer_baked[baked_index] && mBakedTextureDatas[baked_index].mIsLoaded) || other_culled;
			if (isSelf())
			{
				setBakedReady(texture_index, is_baked_ready);
			}
		}
	}
	static bool call_remove_missing = true;
	if (call_remove_missing)
	{
		call_remove_missing = false;
		removeMissingBakedTextures();
		call_remove_missing = true;
	}
	for (auto& attachment_point : mAttachmentPoints)
    {
		LLViewerJointAttachment* attachment = attachment_point.second;
		for (auto attached_object : attachment->mAttachedObjects)
        {
            if (attached_object && !attached_object->isDead())
			{
				attached_object->refreshBakeTexture();
				LLViewerObject::const_child_list_t& child_list = attached_object->getChildren();
				for (const auto& iter : child_list)
                {
					LLViewerObject* objectp = iter;
					if (objectp && !objectp->isDead())
					{
						objectp->refreshBakeTexture();
					}
				}
			}
		}
	}
}
void LLVOAvatar::setLocalTexture( ETextureIndex type, LLViewerTexture* in_tex, BOOL baked_version_ready, U32 index )
{
	llassert(0);
}
void LLVOAvatar::setBakedReady(LLAvatarAppearanceDefines::ETextureIndex type, BOOL baked_version_exists, U32 index)
{
	llassert(0);
}
void LLVOAvatar::setNameFromChat(const std::string &text)
{
	if(!mCCSChatTextOverride || mCCSChatText != text)
		clearNameTag();
	mCCSChatTextOverride = true;
	mCCSChatText = text;
}
void LLVOAvatar::clearNameFromChat()
{
	if(mCCSChatTextOverride || mCCSChatText != "")
		clearNameTag();
	mCCSChatTextOverride = false;
	mCCSChatText = "";
}
void LLVOAvatar::addChat(const LLChat& chat)
{
	std::deque<LLChat>::iterator chat_iter;
	mChats.push_back(chat);
	S32 chat_length = 0;
	for( chat_iter = mChats.begin(); chat_iter != mChats.end(); ++chat_iter)
	{
		chat_length += chat_iter->mText.size();
	}
	chat_iter = mChats.begin();
	while ((chat_length > MAX_BUBBLE_CHAT_LENGTH || mChats.size() > MAX_BUBBLE_CHAT_UTTERANCES) && chat_iter != mChats.end())
	{
		chat_length -= chat_iter->mText.size();
		mChats.pop_front();
		chat_iter = mChats.begin();
	}
	mChatTimer.reset();
	mIdleTimer.reset();
}
void LLVOAvatar::clearChat()
{
	mChats.clear();
}
void LLVOAvatar::applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components, LLAvatarAppearanceDefines::EBakedTextureIndex index)
{
	if (index >= BAKED_NUM_INDICES)
	{
		LL_WARNS() << "invalid baked texture index passed to applyMorphMask" << LL_ENDL;
		return;
	}
	for (morph_list_t::const_iterator iter = mBakedTextureDatas[index].mMaskedMorphs.begin();
		 iter != mBakedTextureDatas[index].mMaskedMorphs.end(); ++iter)
	{
		const LLMaskedMorph* maskedMorph = (*iter);
		LLPolyMorphTarget* morph_target = dynamic_cast<LLPolyMorphTarget*>(maskedMorph->mMorphTarget);
		if (morph_target)
		{
			morph_target->applyMask(tex_data, width, height, num_components, maskedMorph->mInvert);
		}
	}
}
BOOL LLVOAvatar::morphMaskNeedsUpdate(LLAvatarAppearanceDefines::EBakedTextureIndex index)
{
	if (index >= BAKED_NUM_INDICES)
	{
		return FALSE;
	}
	if (!mBakedTextureDatas[index].mMaskedMorphs.empty())
	{
		if (isSelf())
		{
			LLViewerTexLayerSet *layer_set = getTexLayerSet(index);
			if (layer_set)
			{
				return !layer_set->isMorphValid();
			}
		}
		else
		{
			return FALSE;
		}
	}
	return FALSE;
}
void LLVOAvatar::releaseComponentTextures()
{
	if (isTextureDefined(TEX_HAIR_BAKED) && getImage(TEX_HAIR_BAKED,0)->getID() == getImage(TEX_SKIRT_BAKED,0)->getID())
	{
		if (getImage(TEX_HAIR_BAKED,0)->getID() != IMG_INVISIBLE)
		{
			setTETexture(TEX_HAIR_BAKED, IMG_DEFAULT_AVATAR);
		}
	}
	for (U8 baked_index = 0; baked_index < BAKED_NUM_INDICES; baked_index++)
	{
		const LLAvatarAppearanceDictionary::BakedEntry * bakedDicEntry = LLAvatarAppearanceDictionary::getInstance()->getBakedTexture((EBakedTextureIndex)baked_index);
		if (!isTextureDefined(bakedDicEntry->mTextureIndex)
			&& ( (baked_index != BAKED_SKIRT) || isWearingWearableType(LLWearableType::WT_SKIRT) ))
		{
			continue;
		}
		for (U8 texture = 0; texture < bakedDicEntry->mLocalTextures.size(); texture++)
		{
			const U8 te = (ETextureIndex)bakedDicEntry->mLocalTextures[texture];
			setTETexture(te, IMG_DEFAULT_AVATAR);
		}
	}
}
void LLVOAvatar::dumpAvatarTEs( const std::string& context ) const
{
	LL_DEBUGS("Avatar") << avString() << (isSelf() ? "Self: " : "Other: ") << context << LL_ENDL;
	for (LLAvatarAppearanceDictionary::Textures::const_iterator iter = LLAvatarAppearanceDictionary::getInstance()->getTextures().begin();
		 iter != LLAvatarAppearanceDictionary::getInstance()->getTextures().end();
		 ++iter)
	{
		const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = iter->second;
		const LLViewerTexture* te_image = getImage(iter->first,0);
		if( !te_image )
		{
			LL_DEBUGS("Avatar") << avString() << "       " << texture_dict->mName << ": null ptr" << LL_ENDL;
		}
		else if( te_image->getID().isNull() )
		{
			LL_DEBUGS("Avatar") << avString() << "       " << texture_dict->mName << ": null UUID" << LL_ENDL;
		}
		else if( te_image->getID() == IMG_DEFAULT )
		{
			LL_DEBUGS("Avatar") << avString() << "       " << texture_dict->mName << ": IMG_DEFAULT" << LL_ENDL;
		}
		else if (te_image->getID() == IMG_INVISIBLE)
		{
			LL_DEBUGS("Avatar") << avString() << "       " << texture_dict->mName << ": IMG_INVISIBLE" << LL_ENDL;
		}
		else if( te_image->getID() == IMG_DEFAULT_AVATAR )
		{
			LL_DEBUGS("Avatar") << avString() << "       " << texture_dict->mName << ": IMG_DEFAULT_AVATAR" << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("Avatar") << avString() << "       " << texture_dict->mName << ": " << te_image->getID() << LL_ENDL;
		}
	}
}
void LLVOAvatar::clampAttachmentPositions()
{
	if (isDead())
	{
		return;
	}
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (attachment)
		{
			attachment->clampObjectPosition();
		}
	}
}
BOOL LLVOAvatar::hasHUDAttachment() const
{
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (attachment->getIsHUDAttachment() && attachment->getNumObjects() > 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}
LLBBox LLVOAvatar::getHUDBBox() const
{
	LLBBox bbox;
#if SLOW_ATTACHMENT_LIST
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (attachment->getIsHUDAttachment())
		{
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
				 attachment_iter != attachment->mAttachedObjects.end();
				 ++attachment_iter)
			{
				const LLViewerObject* attached_object = (*attachment_iter);
#else
	for(auto& iter : mAttachedObjectsVector)
	{{{
				const LLViewerJointAttachment* attachment = iter.second;
				if (!attachment || !attachment->getIsHUDAttachment())
					continue;
				const LLViewerObject* attached_object =  iter.first;
#endif
				if (attached_object == NULL)
				{
					LL_WARNS() << "HUD attached object is NULL!" << LL_ENDL;
					continue;
				}
				bbox.addPointLocal(attached_object->getPosition());
				bbox.addBBoxAgent(attached_object->getBoundingBoxAgent());
				LLViewerObject::const_child_list_t& child_list = attached_object->getChildren();
				for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
					 iter != child_list.end();
					 ++iter)
				{
					const LLViewerObject* child_objectp = *iter;
					bbox.addBBoxAgent(child_objectp->getBoundingBoxAgent());
				}
			}
		}
	}
	return bbox;
}
void LLVOAvatar::onFirstTEMessageReceived()
{
	LL_DEBUGS("Avatar") << avString() << LL_ENDL;
	if( !mFirstTEMessageReceived )
	{
		mFirstTEMessageReceived = TRUE;
		LLLoadedCallbackEntry::source_callback_list_t* src_callback_list = NULL ;
		BOOL paused = FALSE ;
		if(!isSelf())
		{
			src_callback_list = &mCallbackTextureList ;
			paused = !isVisible();
		}
		for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
		{
			const BOOL layer_baked = isTextureDefined(mBakedTextureDatas[i].mTextureIndex);
			if (layer_baked)
			{
				LLViewerFetchedTexture* image = LLViewerTextureManager::staticCastToFetchedTexture(getImage( mBakedTextureDatas[i].mTextureIndex, 0 ), TRUE) ;
				mBakedTextureDatas[i].mLastTextureID = image->getID();
				image->setBoostLevel(isSelf()
					? LLGLTexture::BOOST_AVATAR_BAKED_SELF
					: LLGLTexture::BOOST_AVATAR_BAKED);
				if ( (image->getID() != IMG_INVISIBLE) && ((i == BAKED_HEAD) || (i == BAKED_UPPER) || (i == BAKED_LOWER)) )
				{
					const BOOL need_aux = !isUsingServerBakes();
					image->setLoadedCallback( onBakedTextureMasksLoaded, MORPH_MASK_REQUESTED_DISCARD, TRUE, need_aux, new LLTextureMaskData( mID ),
						src_callback_list, paused);
				}
				LL_DEBUGS("Avatar") << avString() << "layer_baked, setting onInitialBakedTextureLoaded as callback" << LL_ENDL;
				image->setLoadedCallback( onInitialBakedTextureLoaded, MAX_DISCARD_LEVEL, FALSE, FALSE, new LLUUID( mID ),
					src_callback_list, paused );
                               mLoadedCallbacksPaused |= paused;
			}
		}
		mMeshTexturesDirty = TRUE;
		gPipeline.markGLRebuild(this);
	}
}
bool LLVOAvatar::visualParamWeightsAreDefault()
{
	bool rtn = true;
	bool is_wearing_skirt = isWearingWearableType(LLWearableType::WT_SKIRT);
	for (LLVisualParam *param = getFirstVisualParam();
	     param;
	     param = getNextVisualParam())
	{
		if (param->isTweakable())
		{
			LLViewerVisualParam* vparam = dynamic_cast<LLViewerVisualParam*>(param);
			llassert(vparam);
			bool is_skirt_param = vparam &&
				LLWearableType::WT_SKIRT == vparam->getWearableType();
			if (param->getWeight() != param->getDefaultWeight() &&
			    (is_wearing_skirt || !is_skirt_param))
			{
				rtn = false;
				break;
			}
		}
	}
	return rtn;
}
void dump_visual_param(apr_file_t* file, LLVisualParam const* viewer_param, F32 value)
{
	S32 u8_value = F32_to_U8(value,viewer_param->getMinWeight(),viewer_param->getMaxWeight());
	apr_file_printf(file, "    <param id=\"%d\" name=\"%s\" value=\"%.3f\" u8=\"%d\" type=\"%s\" wearable=\"%s\"/>\n",
					viewer_param->getID(), viewer_param->getName().c_str(), value, u8_value, viewer_param->getTypeString(),
					viewer_param->getDumpWearableTypeName().c_str());
}
void LLVOAvatar::dumpAppearanceMsgParams( const std::string& dump_prefix,
	const LLAppearanceMessageContents& contents)
{
	std::string outfilename = get_sequential_numbered_file_name(dump_prefix,".xml");
	const std::vector<F32>& params_for_dump = contents.mParamWeights;
	const LLTEContents& tec = contents.mTEContents;
	LLAPRFile outfile;
	std::string fullpath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,outfilename);
	outfile.open(fullpath, LL_APR_WB );
	apr_file_t* file = outfile.getFileHandle();
	if (!file)
	{
		return;
	}
	else
	{
		LL_DEBUGS("Avatar") << "dumping appearance message to " << fullpath << LL_ENDL;
	}
	apr_file_printf(file, "<header>\n");
	apr_file_printf(file, "\t\t<cof_version %i />\n", contents.mCOFVersion);
	apr_file_printf(file, "\t\t<appearance_version %i />\n", contents.mAppearanceVersion);
	apr_file_printf(file, "</header>\n");
	apr_file_printf(file, "\n<params>\n");
	LLVisualParam* param = getFirstVisualParam();
	for (const auto& param_for_dump : params_for_dump)
	{
		while( param && ((param->getGroup() != VISUAL_PARAM_GROUP_TWEAKABLE) &&
						 (param->getGroup() != VISUAL_PARAM_GROUP_TRANSMIT_NOT_TWEAKABLE)) )
		{
			param = getNextVisualParam();
		}
		LLViewerVisualParam* viewer_param = (LLViewerVisualParam*)param;
		F32 value = param_for_dump;
		dump_visual_param(file, viewer_param, value);
		param = getNextVisualParam();
	}
	apr_file_printf(file, "</params>\n");
	apr_file_printf(file, "\n<textures>\n");
	for (U32 i = 0; i < tec.face_count; i++)
	{
		std::string uuid_str;
		((LLUUID*)tec.image_data)[i].toString(uuid_str);
		apr_file_printf( file, "\t\t<texture te=\"%i\" uuid=\"%s\"/>\n", i, uuid_str.c_str());
	}
	apr_file_printf(file, "</textures>\n");
}
void LLVOAvatar::parseAppearanceMessage(LLMessageSystem* mesgsys, LLAppearanceMessageContents& contents)
{
	parseTEMessage(mesgsys, _PREHASH_ObjectData, -1, contents.mTEContents);
	if (mesgsys->has(_PREHASH_AppearanceData))
	{
		U8 av_u8;
		mesgsys->getU8Fast(_PREHASH_AppearanceData, _PREHASH_AppearanceVersion, av_u8, 0);
		contents.mAppearanceVersion = av_u8;
		LL_DEBUGS("Avatar") << "appversion set by AppearanceData field: " << contents.mAppearanceVersion << LL_ENDL;
		mesgsys->getS32Fast(_PREHASH_AppearanceData, _PREHASH_CofVersion, contents.mCOFVersion, 0);
	}
	contents.mHoverOffsetWasSet = false;
	if (mesgsys->has(_PREHASH_AppearanceHover))
	{
		LLVector3 hover;
		mesgsys->getVector3Fast(_PREHASH_AppearanceHover, _PREHASH_HoverHeight, hover);
		LL_DEBUGS("Avatar") << avString() << " hover received " << hover.mV[ VX ] << "," << hover.mV[ VY ] << "," << hover.mV[ VZ ] << LL_ENDL;
		contents.mHoverOffset = hover;
		contents.mHoverOffsetWasSet = true;
	}
	S32 num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_VisualParam);
	bool drop_visual_params_debug = gSavedSettings.getBOOL("BlockSomeAvatarAppearanceVisualParams") && (ll_rand(2) == 0);
	if( num_blocks > 1 && !drop_visual_params_debug)
	{
		LL_DEBUGS("Avatar") << avString() << " handle visual params, num_blocks " << num_blocks << LL_ENDL;
		LLVisualParam* param = getFirstVisualParam();
		llassert(param);
		if (!param)
		{
			LL_WARNS() << "No visual params!" << LL_ENDL;
		}
		else
		{
			for( S32 i = 0; i < num_blocks; i++ )
			{
				while( param && ((param->getGroup() != VISUAL_PARAM_GROUP_TWEAKABLE) &&
								 (param->getGroup() != VISUAL_PARAM_GROUP_TRANSMIT_NOT_TWEAKABLE)) )
				{
					param = getNextVisualParam();
				}
				if( !param )
				{
					break;
				}
				U8 value;
				mesgsys->getU8Fast(_PREHASH_VisualParam, _PREHASH_ParamValue, value, i);
				F32 newWeight = U8_to_F32(value, param->getMinWeight(), param->getMaxWeight());
				contents.mParamWeights.push_back(newWeight);
				contents.mParams.push_back(param);
				param = getNextVisualParam();
			}
		}
		const S32 expected_tweakable_count = getVisualParamCountInGroup(VISUAL_PARAM_GROUP_TWEAKABLE) +
											 getVisualParamCountInGroup(VISUAL_PARAM_GROUP_TRANSMIT_NOT_TWEAKABLE);
		if (num_blocks != expected_tweakable_count)
		{
			LL_DEBUGS("Avatar") << "Number of params in AvatarAppearance msg (" << num_blocks << ") does not match number of tweakable params in avatar xml file (" << expected_tweakable_count << ").  Processing what we can.  object: " << getID() << LL_ENDL;
		}
	}
	else
	{
		if (drop_visual_params_debug)
		{
			LL_INFOS() << "Debug-faked lack of parameters on AvatarAppearance for object: "  << getID() << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("Avatar") << "AvatarAppearance msg received without any parameters, object: " << getID() << LL_ENDL;
		}
	}
	LLVisualParam* appearance_version_param = getVisualParam(11000);
	if (appearance_version_param)
	{
		std::vector<LLVisualParam*>::iterator it = std::find(contents.mParams.begin(), contents.mParams.end(),appearance_version_param);
		if (it != contents.mParams.end())
		{
			S32 index = it - contents.mParams.begin();
			contents.mParamAppearanceVersion = ll_round(contents.mParamWeights[index]);
			LL_DEBUGS("Avatar") << "appversion req by appearance_version param: " << contents.mParamAppearanceVersion << LL_ENDL;
		}
	}
}
bool resolve_appearance_version(const LLAppearanceMessageContents& contents, S32& appearance_version)
{
	appearance_version = -1;
	if ((contents.mAppearanceVersion) >= 0 &&
		(contents.mParamAppearanceVersion >= 0) &&
		(contents.mAppearanceVersion != contents.mParamAppearanceVersion))
	{
		LL_WARNS() << "inconsistent appearance_version settings - field: " <<
			contents.mAppearanceVersion << ", param: " <<  contents.mParamAppearanceVersion << LL_ENDL;
		return false;
	}
	if (contents.mParamAppearanceVersion >= 0)
	{
		appearance_version = contents.mParamAppearanceVersion;
	}
	else if (contents.mAppearanceVersion > 0)
	{
		appearance_version = contents.mAppearanceVersion;
	}
	else
	{
		appearance_version = 1;
	}
	LL_DEBUGS("Avatar") << "appearance version info - field " << contents.mAppearanceVersion
						<< " param: " << contents.mParamAppearanceVersion
						<< " final: " << appearance_version << LL_ENDL;
	return true;
}
void LLVOAvatar::processAvatarAppearance( LLMessageSystem* mesgsys )
{
    static S32 largestSelfCOFSeen(LLViewerInventoryCategory::VERSION_UNKNOWN);
	LL_DEBUGS("Avatar") << "starts" << LL_ENDL;
	bool enable_verbose_dumps = gSavedSettings.getBOOL("DebugAvatarAppearanceMessage");
	std::string dump_prefix = getFullname() + "_" + (isSelf()?"s":"o") + "_";
	if (gSavedSettings.getBOOL("BlockAvatarAppearanceMessages"))
	{
		LL_WARNS() << "Blocking AvatarAppearance message" << LL_ENDL;
		return;
	}
	mLastAppearanceMessageTimer.reset();
	LLPointer<LLAppearanceMessageContents> contents(new LLAppearanceMessageContents);
	parseAppearanceMessage(mesgsys, *contents);
	if (enable_verbose_dumps)
	{
		dumpAppearanceMsgParams(dump_prefix + "appearance_msg", *contents);
	}
	S32 appearance_version;
	if (!resolve_appearance_version(*contents, appearance_version))
	{
		LL_WARNS() << "bad appearance version info, discarding" << LL_ENDL;
		return;
	}
	llassert(appearance_version > 0);
	if (appearance_version > 1)
	{
		LL_WARNS() << "unsupported appearance version " << appearance_version << ", discarding appearance message" << LL_ENDL;
		return;
	}
	S32 thisAppearanceVersion(contents->mCOFVersion);
	if( isSelf() )
	{
		LL_DEBUGS("Avatar") << "thisAppearanceVersion " << thisAppearanceVersion
				<< " mLastUpdateRequestCOFVersion " << mLastUpdateRequestCOFVersion
				<<  " my_cof_version " << LLAppearanceMgr::instance().getCOFVersion() << LL_ENDL;
        if (largestSelfCOFSeen > thisAppearanceVersion)
		{
            LL_WARNS("Avatar") << "Already processed appearance for COF version " <<
                largestSelfCOFSeen << ", discarding appearance with COF " << thisAppearanceVersion << LL_ENDL;
			return;
		}
        largestSelfCOFSeen = thisAppearanceVersion;
	}
	else
	{
		LL_DEBUGS("Avatar") << "appearance message received" << LL_ENDL;
	}
	if (isSelf()
		&& (appearance_version>0)
		&& (thisAppearanceVersion < mLastUpdateRequestCOFVersion))
	{
		LL_WARNS() << "Stale appearance update, wanted version " << mLastUpdateRequestCOFVersion
				<< ", got " << thisAppearanceVersion << LL_ENDL;
		return;
	}
	if (isSelf() && isEditingAppearance())
	{
		LL_DEBUGS("Avatar") << "ignoring appearance message while in appearance edit" << LL_ENDL;
		return;
	}
	S32 num_params = contents->mParamWeights.size();
	if (num_params <= 1)
	{
		LL_DEBUGS("Avatar") << "ignoring appearance message due to lack of params" << LL_ENDL;
		return;
	}
    LL_INFOS("Avatar") << "Processing appearance message version " << thisAppearanceVersion << LL_ENDL;
	setIsUsingServerBakes(appearance_version > 0);
	mLastUpdateReceivedCOFVersion = thisAppearanceVersion;
	SHClientTagMgr::instance().updateAvatarTag(this);
	mLastProcessedAppearance = contents;
	bool slam_params = false;
	applyParsedAppearanceMessage(*contents, slam_params);
}
void LLVOAvatar::applyParsedAppearanceMessage(LLAppearanceMessageContents& contents, bool slam_params)
{
	S32 num_params = contents.mParamWeights.size();
	ESex old_sex = getSex();
	if (applyParsedTEMessage(contents.mTEContents) > 0 && isChanged(TEXTURE))
	{
		updateVisualComplexity();
	}
	for (U8 baked_index = 0; baked_index < mBakedTextureDatas.size(); baked_index++)
	{
		if (!isTextureDefined(mBakedTextureDatas[baked_index].mTextureIndex)
			&& mBakedTextureDatas[baked_index].mLastTextureID != IMG_DEFAULT
			&& baked_index != BAKED_SKIRT && baked_index != BAKED_LEFT_ARM && baked_index != BAKED_LEFT_LEG && baked_index != BAKED_AUX1 && baked_index != BAKED_AUX2 && baked_index != BAKED_AUX3)
		{
			LL_DEBUGS("Avatar") << avString() << " baked_index " << (S32) baked_index << " using mLastTextureID " << mBakedTextureDatas[baked_index].mLastTextureID << LL_ENDL;
			setTEImage(mBakedTextureDatas[baked_index].mTextureIndex,
					LLViewerTextureManager::getFetchedTexture(mBakedTextureDatas[baked_index].mLastTextureID, FTT_HOST_BAKE, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE));
		}
		else
		{
			LL_DEBUGS("Avatar") << avString() << " baked_index " << (S32) baked_index << " using texture id "
								<< getTE(mBakedTextureDatas[baked_index].mTextureIndex)->getID() << LL_ENDL;
		}
	}
	BOOL is_first_appearance_message = !mFirstAppearanceMessageReceived;
	mFirstAppearanceMessageReceived = TRUE;
	LL_DEBUGS("Avatar") << avString() << "processAvatarAppearance start " << mID
			<< " first? " << is_first_appearance_message << " self? " << isSelf() << LL_ENDL;
	if (is_first_appearance_message )
	{
		onFirstTEMessageReceived();
	}
	setCompositeUpdatesEnabled( FALSE );
	gPipeline.markGLRebuild(this);
	mHasPhysicsParameters = false;
	if( num_params > 1)
	{
		LL_DEBUGS("Avatar") << avString() << " handle visual params, num_params " << num_params << LL_ENDL;
		BOOL params_changed = FALSE;
		BOOL interp_params = FALSE;
		S32 params_changed_count = 0;
		for( S32 i = 0; i < num_params; i++ )
		{
			LLVisualParam* param = contents.mParams[i];
			F32 newWeight = contents.mParamWeights[i];
			if (!mHasPhysicsParameters && param->getID() == 10000)
				mHasPhysicsParameters = true;
			if (slam_params || is_first_appearance_message || (param->getWeight() != newWeight))
			{
				params_changed = TRUE;
				params_changed_count++;
				if(is_first_appearance_message || slam_params )
				{
					param->setWeight(newWeight, FALSE);
				}
				else
				{
					interp_params = TRUE;
					param->setAnimationTarget(newWeight, FALSE);
				}
			}
		}
		const S32 expected_tweakable_count = getVisualParamCountInGroup(VISUAL_PARAM_GROUP_TWEAKABLE) +
											 getVisualParamCountInGroup(VISUAL_PARAM_GROUP_TRANSMIT_NOT_TWEAKABLE);
		if (num_params != expected_tweakable_count)
		{
			LL_DEBUGS("Avatar") << "Number of params in AvatarAppearance msg (" << num_params << ") does not match number of tweakable params in avatar xml file (" << expected_tweakable_count << ").  Processing what we can.  object: " << getID() << LL_ENDL;
		}
		LL_DEBUGS("Avatar") << "Changed " << params_changed_count << " params" << LL_ENDL;
		if (params_changed)
		{
			if (interp_params)
			{
				startAppearanceAnimation();
			}
			updateVisualParams();
			ESex new_sex = getSex();
			if( old_sex != new_sex )
			{
				updateSexDependentLayerSets( FALSE );
			}
		}
		llassert( getSex() == ((getVisualParamWeight( "male" ) > 0.5f) ? SEX_MALE : SEX_FEMALE) );
	}
	else
	{
		LL_DEBUGS("Avatar") << avString() << "no visual params" << LL_ENDL;
		const F32 LOADING_TIMEOUT_SECONDS = 60.f;
		if (visualParamWeightsAreDefault() && mRuthTimer.getElapsedTimeF32() > LOADING_TIMEOUT_SECONDS)
		{
			LL_INFOS() << "Re-requesting AvatarAppearance for object: "  << getID() << LL_ENDL;
			LLAvatarPropertiesProcessor::getInstance()->sendAvatarTexturesRequest(getID());
			mRuthTimer.reset();
		}
		else
		{
			LL_INFOS() << "That's okay, we already have a non-default shape for object: "  << getID() << LL_ENDL;
		}
	}
	if (contents.mHoverOffsetWasSet && !isSelf())
	{
		setHoverOffset(contents.mHoverOffset);
		LL_INFOS("Avatar") << avString() << "setting hover from message" << contents.mHoverOffset[2] << LL_ENDL;
	}
	if (!contents.mHoverOffsetWasSet && !isSelf())
	{
		LL_WARNS() << avString() << "zeroing hover because not defined in appearance message" << LL_ENDL;
		setHoverOffset(LLVector3(0.0, 0.0, 0.0));
	}
	setCompositeUpdatesEnabled( TRUE );
	LLVOAvatar::cullAvatarsByPixelArea();
	if (isSelf())
	{
		mUseLocalAppearance = false;
	}
	updateMeshTextures();
	updateMeshVisibility();
}
LLViewerTexture* LLVOAvatar::getBakedTextureForAttachment(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index)
{
	if (baked_index < 0 || baked_index >= BAKED_NUM_INDICES)
	{
		return LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	}
	const LLGLTexture::EBoostLevel bake_boost = isSelf()
		? LLGLTexture::BOOST_AVATAR_BAKED_SELF
		: LLGLTexture::BOOST_AVATAR_BAKED;
	LLViewerTexture* bakedTexture = getBakedTexture((U8)baked_index);
	if (bakedTexture && !bakedTexture->isMissingAsset())
	{
		if (LLViewerFetchedTexture* fetched = LLViewerTextureManager::staticCastToFetchedTexture(bakedTexture, FALSE))
		{
			if (fetched->getBoostLevel() < bake_boost)
			{
				fetched->setBoostLevel(bake_boost);
			}
			return fetched;
		}
		if (!isEditingAppearance())
		{
			return bakedTexture;
		}
	}
	const LLUUID& last_id = mBakedTextureDatas[baked_index].mLastTextureID;
	if (last_id.notNull()
		&& last_id != IMG_DEFAULT
		&& last_id != IMG_DEFAULT_AVATAR
		&& last_id != IMG_INVISIBLE)
	{
		LLViewerFetchedTexture* pending = getBakedTextureImage(
			mBakedTextureDatas[baked_index].mTextureIndex, last_id);
		if (pending && !pending->isMissingAsset())
		{
			pending->setBoostLevel(bake_boost);
			return pending;
		}
	}
	return LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
}
LLViewerTexture* LLVOAvatar::getBakedTexture(const U8 te)
{
	if (te < 0 || te >= BAKED_NUM_INDICES)
	{
		return NULL;
	}
	BOOL is_layer_baked = isTextureDefined(mBakedTextureDatas[te].mTextureIndex);
	LLViewerTexLayerSet* layerset = NULL;
	layerset = getTexLayerSet(te);
	if (!isEditingAppearance() && is_layer_baked)
	{
		LLViewerFetchedTexture* baked_img = LLViewerTextureManager::staticCastToFetchedTexture(getImage(mBakedTextureDatas[te].mTextureIndex, 0), TRUE);
		return baked_img;
	}
	else if (layerset && isEditingAppearance())
	{
		layerset->createComposite();
		layerset->setUpdatesEnabled(TRUE);
		return layerset->getViewerComposite();
	}
	return NULL;
}
void LLVOAvatar::getAnimLabels( std::vector<std::string>* labels )
{
	S32 i;
	labels->reserve(gUserAnimStatesCount);
	for( i = 0; i < gUserAnimStatesCount; i++ )
	{
		labels->push_back( LLAnimStateLabels::getStateLabel( gUserAnimStates[i].mName ) );
	}
	labels->push_back( "Away From Keyboard" );
}
void LLVOAvatar::getAnimNames( std::vector<std::string>* names )
{
	S32 i;
	names->reserve(gUserAnimStatesCount);
	for( i = 0; i < gUserAnimStatesCount; i++ )
	{
		names->push_back( std::string(gUserAnimStates[i].mName) );
	}
	names->push_back( "enter_away_from_keyboard_state" );
}
void LLVOAvatar::onBakedTextureMasksLoaded( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata )
{
	if (!userdata) return;
	const LLUUID id = src_vi->getID();
	LLTextureMaskData* maskData = (LLTextureMaskData*) userdata;
	LLVOAvatar* self = gObjectList.findAvatar( maskData->mAvatarID );
	if(self && success && (discard_level < maskData->mLastDiscardLevel - 2 || discard_level == 0))
	{
		if(aux_src && aux_src->getComponents() == 1)
		{
			if (!aux_src->getData())
			{
				LL_WARNS() << "No auxiliary source (morph mask) data for image id " << id << LL_ENDL;
				return;
			}
			auto gl_name = LLImageGL::createTextureName();
			stop_glerror();
			gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, gl_name->getTexName());
			stop_glerror();
			LLImageGL::setManualImage(
				GL_TEXTURE_2D, 0, GL_ALPHA8,
				aux_src->getWidth(), aux_src->getHeight(),
				GL_ALPHA, GL_UNSIGNED_BYTE, aux_src->getData());
			stop_glerror();
			gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
			BOOL found_texture_id = false;
			for (LLAvatarAppearanceDictionary::Textures::const_iterator iter = LLAvatarAppearanceDictionary::getInstance()->getTextures().begin();
				 iter != LLAvatarAppearanceDictionary::getInstance()->getTextures().end();
				 ++iter)
			{
				const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = iter->second;
				if (texture_dict->mIsUsedByBakedTexture)
				{
					const ETextureIndex texture_index = iter->first;
					const LLViewerTexture *baked_img = self->getImage(texture_index, 0);
					if (baked_img && id == baked_img->getID())
					{
						const EBakedTextureIndex baked_index = texture_dict->mBakedTextureIndex;
						self->applyMorphMask(aux_src->getData(), aux_src->getWidth(), aux_src->getHeight(), 1, baked_index);
						maskData->mLastDiscardLevel = discard_level;
						self->mBakedTextureDatas[baked_index].mMaskTexName = gl_name;
						found_texture_id = true;
						break;
					}
				}
			}
			if (!found_texture_id)
			{
				LL_INFOS() << "unexpected image id: " << id << LL_ENDL;
			}
			self->dirtyMesh();
		}
		else
		{
			LL_DEBUGS("Avatar") << "Masks loaded callback but NO aux source, id " << id << LL_ENDL;
		}
	}
	if (final || !success)
	{
		delete maskData;
	}
}
void LLVOAvatar::onInitialBakedTextureLoaded( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata )
{
	LLUUID *avatar_idp = (LLUUID *)userdata;
	LLVOAvatar *selfp = gObjectList.findAvatar(*avatar_idp);
	if (selfp)
	{
		LL_DEBUGS("Avatar") << selfp->avString() << "discard_level " << discard_level << " success " << success << " final " << final << LL_ENDL;
	}
	if (!success && selfp)
	{
		selfp->removeMissingBakedTextures();
	}
	if (final || !success )
	{
		delete avatar_idp;
	}
}
void LLVOAvatar::onBakedTextureLoaded(BOOL success,
									  LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src,
									  S32 discard_level, BOOL final, void* userdata)
{
	LL_DEBUGS("Avatar") << "onBakedTextureLoaded: " << src_vi->getID() << LL_ENDL;
	LLUUID id = src_vi->getID();
	LLUUID *avatar_idp = (LLUUID *)userdata;
	LLVOAvatar *selfp = gObjectList.findAvatar(*avatar_idp);
	if (selfp)
	{
		LL_DEBUGS("Avatar") << selfp->avString() << "discard_level " << discard_level << " success " << success << " final " << final << " id " << src_vi->getID() << LL_ENDL;
	}
	if (selfp && !success)
	{
		selfp->removeMissingBakedTextures();
	}
	if( final || !success )
	{
		delete avatar_idp;
	}
	if( selfp && success && final )
	{
		selfp->useBakedTexture( id );
	}
}
void LLVOAvatar::useBakedTexture( const LLUUID& id )
{
	for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
	{
		LLViewerTexture* image_baked = getImage( mBakedTextureDatas[i].mTextureIndex, 0 );
		if (id == image_baked->getID())
		{
			mBakedTextureDatas[i].mIsLoaded = true;
			mBakedTextureDatas[i].mLastTextureID = id;
			mBakedTextureDatas[i].mIsUsed = true;
			if (isUsingLocalAppearance())
			{
				LL_INFOS() << "not changing to baked texture while isUsingLocalAppearance" << LL_ENDL;
			}
			else
			{
				debugColorizeSubMeshes(i,LLColor4::green);
				avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[i].mJointMeshes.begin();
				avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[i].mJointMeshes.end();
				for (; iter != end; ++iter)
				{
					LLAvatarJointMesh* mesh = (*iter);
					if (mesh)
					{
						mesh->setTexture( image_baked );
					}
				}
			}
			const LLAvatarAppearanceDictionary::BakedEntry *baked_dict =
				LLAvatarAppearanceDictionary::getInstance()->getBakedTexture((EBakedTextureIndex)i);
			for (texture_vec_t::const_iterator local_tex_iter = baked_dict->mLocalTextures.begin();
				 local_tex_iter != baked_dict->mLocalTextures.end();
				 ++local_tex_iter)
			{
				if (isSelf()) setBakedReady(*local_tex_iter, TRUE);
			}
			if (i == BAKED_HAIR)
			{
				avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[i].mJointMeshes.begin();
				avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[i].mJointMeshes.end();
				for (; iter != end; ++iter)
				{
					LLAvatarJointMesh* mesh = (*iter);
					if (mesh)
					{
						mesh->setColor( LLColor4::white );
					}
				}
			}
		}
	}
	dirtyMesh();
}
std::string get_sequential_numbered_file_name(const std::string& prefix,
											  const std::string& suffix)
{
	typedef std::map<std::string,S32> file_num_type;
	static  file_num_type file_nums;
	file_num_type::iterator it = file_nums.find(prefix);
	S32 num = 0;
	if (it != file_nums.end())
	{
		num = it->second;
	}
	file_nums[prefix] = num+1;
	std::string outfilename = prefix + " " + llformat("%04d",num) + ".xml";
	std::replace(outfilename.begin(),outfilename.end(),' ','_');
	return outfilename;
}
void dump_sequential_xml(const std::string outprefix, const LLSD& content)
{
	std::string outfilename = get_sequential_numbered_file_name(outprefix,".xml");
	std::string fullpath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,outfilename);
	std::ofstream ofs(fullpath.c_str(), std::ios_base::out);
	ofs << LLSDOStreamer<LLSDXMLFormatter>(content, LLSDFormatter::OPTIONS_PRETTY);
	LL_DEBUGS("Avatar") << "results saved to: " << fullpath << LL_ENDL;
}
void LLVOAvatar::getSortedJointNames(S32 joint_type, std::vector<std::string>& result) const
{
    result.clear();
    if (joint_type==0)
    {
        avatar_joint_list_t::const_iterator iter = mSkeleton.begin();
        avatar_joint_list_t::const_iterator end  = mSkeleton.end();
		for (; iter != end; ++iter)
		{
			LLJoint* pJoint = (*iter);
            result.push_back(pJoint->getName());
        }
    }
    else if (joint_type==1)
    {
        for (const auto& pJoint : mCollisionVolumes)
        {
            result.push_back(pJoint->getName());
        }
    }
    else if (joint_type==2)
    {
		for (LLVOAvatar::attachment_map_t::const_iterator iter = mAttachmentPoints.begin();
			 iter != mAttachmentPoints.end(); ++iter)
		{
			LLViewerJointAttachment* pJoint = iter->second;
			if (!pJoint) continue;
            result.push_back(pJoint->getName());
        }
    }
    std::sort(result.begin(), result.end());
}
void LLVOAvatar::dumpArchetypeXML(const std::string& prefix, bool group_by_wearables )
{
	std::string outprefix(prefix);
	if (outprefix.empty())
	{
		outprefix = getFullname() + (isSelf()?"_s":"_o");
	}
	std::string outfilename = get_sequential_numbered_file_name(outprefix,".xml");
	std::string fullpath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,outfilename);
	dumpArchetypeXML_cont(fullpath, group_by_wearables);
}
void LLVOAvatar::dumpArchetypeXML_cont(std::string const& fullpath, bool group_by_wearables)
{
	try
	{
	  AIXMLLindenGenepool linden_genepool(fullpath);
	  if (group_by_wearables)
	  {
		  for (S32 type = LLWearableType::WT_SHAPE; type < LLWearableType::WT_COUNT; type++)
		  {
			  AIArchetype archetype((LLWearableType::EType)type);
			  for (LLVisualParam* param = getFirstVisualParam(); param; param = getNextVisualParam())
			  {
				  LLViewerVisualParam* viewer_param = (LLViewerVisualParam*)param;
				  if( (viewer_param->getWearableType() == type) &&
					  (viewer_param->isTweakable() ) )
				  {
					  archetype.add(AIVisualParamIDValuePair(param));
				  }
			  }
			  for (U8 te = 0; te < TEX_NUM_INDICES; te++)
			  {
				  if (LLAvatarAppearanceDictionary::getTEWearableType((ETextureIndex)te) == type)
				  {
					  LLViewerTexture* te_image = getImage((ETextureIndex)te, 0);
					  if( te_image )
					  {
						  archetype.add(AITextureIDUUIDPair(te, te_image->getID()));
					  }
				  }
			  }
			  linden_genepool.child(archetype);
		  }
	  }
	  else
	  {
		  AIArchetype archetype;
		  for (LLVisualParam* param = getFirstVisualParam(); param; param = getNextVisualParam())
		  {
			  archetype.add(AIVisualParamIDValuePair(param));
		  }
		  for (U8 te = 0; te < TEX_NUM_INDICES; te++)
		  {
			  {
				  LLViewerTexture* te_image = getImage((ETextureIndex)te, 0);
				  if( te_image )
				  {
					  archetype.add(AITextureIDUUIDPair(te, te_image->getID()));
				  }
			  }
		  }
		  linden_genepool.child(archetype);
	  }
#if 0
	  bool ultra_verbose = false;
	  if (isSelf() && ultra_verbose)
	  {
		  gAgentAvatarp->dumpWearableInfo(outfile);
	  }
#endif
	}
	catch (AIAlert::Error const& error)
	{
		AIAlert::add_modal("AIXMLdumpArchetypeXMLError", AIArgs("[FILE]", fullpath), error);
	}
}
void LLVOAvatar::setVisibilityRank(U32 rank)
{
	if (mDrawable.isNull() || mDrawable->isDead())
	{
		return;
	}
	mVisibilityRank = rank;
}
S32 LLVOAvatar::getUnbakedPixelAreaRank()
{
	S32 rank = 1;
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		if (inst == this)
		{
			return rank;
		}
		else if (!inst->isDead() && !inst->isFullyBaked())
		{
			rank++;
		}
	}
	llassert(0);
	return 0;
}
struct CompareScreenAreaGreater
{
	BOOL operator()(const LLCharacter* const& lhs, const LLCharacter* const& rhs)
	{
		return lhs->getPixelArea() > rhs->getPixelArea();
	}
};
void LLVOAvatar::cullAvatarsByPixelArea()
{
	std::sort(LLCharacter::sInstances.begin(), LLCharacter::sInstances.end(), CompareScreenAreaGreater());
	U32 rank = 2;
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		BOOL culled;
		if (inst->isSelf() || inst->isFullyBaked())
		{
			culled = FALSE;
		}
		else
		{
			culled = TRUE;
		}
		if (inst->mCulled != culled)
		{
			inst->mCulled = culled;
			LL_DEBUGS() << "avatar " << inst->getID() << (culled ? " start culled" : " start not culled" ) << LL_ENDL;
			inst->updateMeshTextures();
		}
		if (inst->isSelf())
		{
			inst->setVisibilityRank(1);
		}
		else if (inst->mDrawable.notNull() && inst->mDrawable->isVisible())
		{
			inst->setVisibilityRank(rank++);
		}
	}
	S32 grey_avatars = 0;
	if (!LLVOAvatar::areAllNearbyInstancesBaked(grey_avatars))
	{
		if (gFrameTimeSeconds != sUnbakedUpdateTime)
		{
			sUnbakedUpdateTime = gFrameTimeSeconds;
			sUnbakedTime += gFrameIntervalSeconds.value();
		}
		if (grey_avatars > 0)
		{
			if (gFrameTimeSeconds != sGreyUpdateTime)
			{
				sGreyUpdateTime = gFrameTimeSeconds;
				sGreyTime += gFrameIntervalSeconds.value();
			}
		}
	}
}
void LLVOAvatar::startAppearanceAnimation()
{
	if(!mAppearanceAnimating)
	{
		mAppearanceAnimating = TRUE;
		mAppearanceMorphTimer.reset();
		mLastAppearanceBlendTime = 0.f;
	}
}
BOOL LLVOAvatar::isUsingServerBakes() const
{
#if 1
	LLVisualParam* appearance_version_param = getVisualParam(11000);
	llassert(appearance_version_param);
	F32 wt = appearance_version_param->getWeight();
	F32 expect_wt = mUseServerBakes ? 1.0 : 0.0;
	if (!is_approx_equal(wt,expect_wt))
	{
		LL_WARNS() << "wt " << wt << " differs from expected " << expect_wt << LL_ENDL;
	}
#endif
	return mUseServerBakes;
}
void LLVOAvatar::setIsUsingServerBakes(BOOL newval)
{
	mUseServerBakes = newval;
	LLVisualParam* appearance_version_param = getVisualParam(11000);
	llassert(appearance_version_param);
	appearance_version_param->setWeight(newval ? 1.0 : 0.0, false);
}
void LLVOAvatar::removeMissingBakedTextures()
{
}
void LLVOAvatar::updateRegion(LLViewerRegion *regionp)
{
	LLViewerObject::updateRegion(regionp);
}
std::string LLVOAvatar::getFullname() const
{
	std::string name;
	LLNameValue* first = getNVPair("FirstName");
	LLNameValue* last  = getNVPair("LastName");
	if (first && last)
	{
		name = LLCacheName::buildFullName( first->getString(), last->getString() );
	}
	return name;
}
LLHost LLVOAvatar::getObjectHost() const
{
	LLViewerRegion* region = getRegion();
	if (region && !isDead())
	{
		return region->getHost();
	}
	else
	{
		return LLHost::invalid;
	}
}
void LLVOAvatar::updateFreezeCounter(S32 counter)
{
	if(counter)
	{
		sFreezeCounter = counter;
	}
	else if(sFreezeCounter > 0)
	{
		sFreezeCounter--;
	}
	else
	{
		sFreezeCounter = 0;
	}
}
BOOL LLVOAvatar::updateLOD()
{
	if (mDrawable.isNull())
	{
		return FALSE;
	}
	if (isImpostor() && 0 != mDrawable->getNumFaces() && mDrawable->getFace(0)->hasGeometry())
	{
		return TRUE;
	}
	BOOL res = updateJointLODs();
	LLFace* facep = mDrawable->getFace(0);
	if (!facep || !facep->getVertexBuffer())
	{
		dirtyMesh(2);
	}
	if (mDirtyMesh >= 2 || mDrawable->isState(LLDrawable::REBUILD_GEOMETRY))
	{
		updateMeshData();
		mDirtyMesh = 0;
		mNeedsSkin = TRUE;
		mDrawable->clearState(LLDrawable::REBUILD_GEOMETRY);
	}
	updateVisibility();
	return res;
}
void LLVOAvatar::updateLODRiggedAttachments()
{
	updateLOD();
	rebuildRiggedAttachments();
}
void showRigInfoTabExtents(LLVOAvatar *avatar, LLJointRiggingInfoTab& tab, S32& count_rigged, S32& count_box)
{
    count_rigged = count_box = 0;
    LLVector4a zero_vec;
    zero_vec.clear();
    for (S32 i=0; i<tab.size(); i++)
    {
        if (tab[i].isRiggedTo())
        {
            count_rigged++;
            LLJoint *joint = avatar->getJoint(i);
            LL_DEBUGS("RigSpam") << "joint " << i << " name " << joint->getName() << " box "
                                 << tab[i].getRiggedExtents()[0] << ", " << tab[i].getRiggedExtents()[1] << LL_ENDL;
            if ((!tab[i].getRiggedExtents()[0].equals3(zero_vec)) ||
                (!tab[i].getRiggedExtents()[1].equals3(zero_vec)))
            {
                count_box++;
            }
       }
    }
}
void LLVOAvatar::getAssociatedVolumes(std::vector<LLVOVolume*>& volumes)
{
#if SLOW_ATTACHMENT_LIST
	for ( LLVOAvatar::attachment_map_t::iterator iter = mAttachmentPoints.begin(); iter != mAttachmentPoints.end(); ++iter )
	{
		LLViewerJointAttachment* attachment = iter->second;
		LLViewerJointAttachment::attachedobjs_vec_t::iterator attach_end = attachment->mAttachedObjects.end();
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attach_iter = attachment->mAttachedObjects.begin();
			 attach_iter != attach_end; ++attach_iter)
		{
			LLViewerObject* attached_object =  *attach_iter;
#else
	for(auto& iter : mAttachedObjectsVector)
	{{
			LLViewerObject* attached_object = iter.first;
#endif
            LLVOVolume *volume = attached_object->asVolume();
            if (volume)
            {
                volumes.push_back(volume);
                if (volume->isAnimatedObject())
                {
                    continue;
                }
            }
            LLViewerObject::const_child_list_t& children = attached_object->getChildren();
			for (LLViewerObject* childp : children)
            {
				if (!childp)
					continue;
                LLVOVolume *volume = childp->asVolume();
                if (volume)
                {
                    volumes.push_back(volume);
                }
            }
        }
    }
    LLControlAvatar *control_av = asControlAvatar();
    if (control_av)
    {
        LLVOVolume *volp = control_av->mRootVolp;
        if (volp)
        {
            volumes.push_back(volp);
            LLViewerObject::const_child_list_t& children = volp->getChildren();
            for (LLViewerObject* childp : children)
            {
                LLVOVolume *volume = childp ? childp->asVolume() : nullptr;
                if (volume)
                {
                    volumes.push_back(volume);
                }
            }
        }
    }
}
static LLTrace::BlockTimerStatHandle FTM_AVATAR_RIGGING_INFO_UPDATE("Av Upd Rig Info");
static LLTrace::BlockTimerStatHandle FTM_AVATAR_RIGGING_KEY_UPDATE("Av Upd Rig Key");
static LLTrace::BlockTimerStatHandle FTM_AVATAR_RIGGING_AVOL_UPDATE("Av Upd Avol");
void LLVOAvatar::updateRiggingInfo()
{
	LL_RECORD_BLOCK_TIME(FTM_AVATAR_RIGGING_INFO_UPDATE);
	LL_DEBUGS("RigSpammish") << getFullname() << " updating rig tab" << LL_ENDL;
	std::vector<LLVOVolume*> volumes;
	{
		LL_RECORD_BLOCK_TIME(FTM_AVATAR_RIGGING_AVOL_UPDATE);
		getAssociatedVolumes(volumes);
	}
	std::map<LLUUID,S32> curr_rigging_info_key;
	{
		LL_RECORD_BLOCK_TIME(FTM_AVATAR_RIGGING_KEY_UPDATE);
		for (std::vector<LLVOVolume*>::iterator it = volumes.begin(); it != volumes.end(); ++it)
		{
			LLVOVolume *vol = *it;
			if (vol->isRiggedMesh() && vol->getVolume() && vol->getVolume()->isMeshAssetLoaded())
			{
				const LLUUID& mesh_id = vol->getVolume()->getParams().getSculptID();
				S32 max_lod = llmax(vol->getLOD(), vol->mLastRiggingInfoLOD);
				curr_rigging_info_key[mesh_id] = max_lod;
			}
		}
		if (curr_rigging_info_key == mLastRiggingInfoKey)
		{
			return;
		}
	}
	mLastRiggingInfoKey = curr_rigging_info_key;
	mJointRiggingInfoTab.clear();
	for (std::vector<LLVOVolume*>::iterator it = volumes.begin(); it != volumes.end(); ++it)
	{
		LLVOVolume *vol = *it;
		vol->updateRiggingInfo();
		mJointRiggingInfoTab.merge(vol->mJointRiggingInfoTab);
	}
	LL_DEBUGS("RigSpammish") << getFullname() << " after update rig tab:" << LL_ENDL;
	S32 joint_count, box_count;
	showRigInfoTabExtents(this, mJointRiggingInfoTab, joint_count, box_count);
	LL_DEBUGS("RigSpammish") << "uses " << joint_count << " joints " << " nonzero boxes: " << box_count << LL_ENDL;
}
void LLVOAvatar::updateSoftwareSkinnedVertices(const LLMeshSkinInfo* skin, const LLVector4a* weight, const LLVolumeFace& vol_face, LLVertexBuffer *buffer)
{
	LLStrider<LLVector3> position;
	LLStrider<LLVector3> normal;
	bool has_normal = buffer->hasDataType(LLVertexBuffer::TYPE_NORMAL);
	buffer->getVertexStrider(position);
	if (has_normal)
	{
		buffer->getNormalStrider(normal);
	}
	LLVector4a* pos = (LLVector4a*) position.get();
	LLVector4a* norm = has_normal ? (LLVector4a*) normal.get() : NULL;
	LLMatrix4a mat[LL_MAX_JOINTS_PER_MESH_OBJECT];
	U32 count = LLSkinningUtil::getMeshJointCount(skin);
	LLSkinningUtil::initSkinningMatrixPalette(mat, count, skin, this, true);
	LLSkinningUtil::checkSkinWeights(weight, buffer->getNumVerts(), skin);
	LLMatrix4a bind_shape_matrix;
	bind_shape_matrix.loadu(skin->mBindShapeMatrix);
	LLVector4a av_pos;
	av_pos.load3(getPosition().mV);
	const U32 max_joints = LLSkinningUtil::getMaxJointCount();
	for (U32 j = 0; j < (U32)buffer->getNumVerts(); ++j)
	{
		LLMatrix4a final_mat;
		LLSkinningUtil::getPerVertexSkinMatrix(weight[j].getF32ptr(), mat, false, final_mat, max_joints);
		LLVector4a& v = vol_face.mPositions[j];
		LLVector4a t;
		bind_shape_matrix.affineTransform(v, t);
		final_mat.affineTransform(t, pos[j]);
		pos[j].add(av_pos);
		if (norm)
		{
			LLVector4a& n = vol_face.mNormals[j];
			final_mat.invert();
			final_mat.transpose();
			final_mat.affineTransform(n, norm[j]);
		}
	}
}
void LLVOAvatar::onActiveOverrideMeshesChanged()
{
	mJointRiggingInfoTab.setNeedsUpdate(true);
}
U32 LLVOAvatar::getPartitionType() const
{
	return LLViewerRegion::PARTITION_ATTACHMENT;
}
void LLVOAvatar::updateImpostors()
{
	LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
	std::vector<LLCharacter*> instances_copy = LLCharacter::sInstances;
	for (std::vector<LLCharacter*>::iterator iter = instances_copy.begin();
		iter != instances_copy.end(); ++iter)
	{
		LLVOAvatar* avatar = (LLVOAvatar*) *iter;
		if (!avatar->isDead() && avatar->needsImpostorUpdate() && avatar->isVisible() && avatar->isImpostor())
		{
			gPipeline.generateImpostor(avatar);
		}
	}
}
BOOL LLVOAvatar::isImpostor() const
{
	return (isVisuallyMuted() || (sUseImpostors && mUpdatePeriod >= IMPOSTOR_PERIOD)) ? TRUE : FALSE;
}
BOOL LLVOAvatar::shouldImpostor(const U32 rank_factor) const
{
	return (!isSelf() && sUseImpostors && mVisibilityRank > (sMaxVisible * rank_factor));
}
BOOL LLVOAvatar::needsImpostorUpdate() const
{
	return mNeedsImpostorUpdate;
}
const LLVector3& LLVOAvatar::getImpostorOffset() const
{
	return mImpostorOffset;
}
const LLVector2& LLVOAvatar::getImpostorDim() const
{
	return mImpostorDim;
}
void LLVOAvatar::setImpostorDim(const LLVector2& dim)
{
	mImpostorDim = dim;
}
void LLVOAvatar::cacheImpostorValues()
{
	getImpostorValues(mImpostorExtents, mImpostorAngle, mImpostorDistance);
}
void LLVOAvatar::getImpostorValues(LLVector4a* extents, LLVector3& angle, F32& distance) const
{
	const LLVector4a* ext = mDrawable->getSpatialExtents();
	extents[0] = ext[0];
	extents[1] = ext[1];
	LLVector3 at = LLViewerCamera::getInstance()->getOrigin()-(getRenderPosition()+mImpostorOffset);
	distance = at.normalize();
	F32 da = 1.f - (at*LLViewerCamera::getInstance()->getAtAxis());
	angle.mV[0] = LLViewerCamera::getInstance()->getYaw()*da;
	angle.mV[1] = LLViewerCamera::getInstance()->getPitch()*da;
	angle.mV[2] = da;
}
void LLVOAvatar::idleUpdateRenderComplexity()
{
	if (isControlAvatar())
	{
		LLControlAvatar *cav = asControlAvatar();
		bool is_attachment = cav && cav->mRootVolp && cav->mRootVolp->isAttachment();
		if (is_attachment)
		{
			return;
		}
	}
	if (mComplexityTimer.getElapsedTimeF32() > 5.f)
	{
		calculateUpdateRenderComplexity();
		mComplexityTimer.start();
	}
	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHAME))
	{
		std::string info_line;
		F32 red_level;
		F32 green_level;
		LLColor4 info_color;
		LLFontGL::StyleFlags info_style;
		if ( !mText )
		{
			initHudText();
			mText->setFadeDistance(20.0, 5.0);
		}
		else
		{
			mText->clearString();
		}
		static LLCachedControl<U32> max_render_cost(gSavedSettings, "RenderAvatarMaxComplexity", 0);
		info_line = llformat("%d Complexity", mVisualComplexity);
		if (max_render_cost != 0)
		{
			green_level = 1.f-llclamp(((F32) mVisualComplexity-(F32)max_render_cost)/(F32)max_render_cost, 0.f, 1.f);
			red_level   = llmin((F32) mVisualComplexity/(F32)max_render_cost, 1.f);
			info_color.set(red_level, green_level, 0.0, 1.0);
			info_style = (  mVisualComplexity > max_render_cost
						  ? LLFontGL::BOLD : LLFontGL::NORMAL );
		}
		else
		{
			info_color.set(LLColor4::grey);
			info_style = LLFontGL::NORMAL;
		}
		mText->addLine(info_line, info_color, info_style);
		info_line = llformat("%d rank", mVisibilityRank);
		info_color.set(isImpostor() ? LLColor4::grey : (isControlAvatar() ? LLColor4::yellow : LLColor4::white));
		info_style = LLFontGL::NORMAL;
		mText->addLine(info_line, info_color, info_style);
		mText->addLine(std::string("VisTris ") + LLStringOps::getReadableNumber(mAttachmentVisibleTriangleCount),
					   info_color, info_style);
		mText->addLine(std::string("EstMaxTris ") + LLStringOps::getReadableNumber(mAttachmentEstTriangleCount),
					  info_color, info_style);
		static LLCachedControl<F32> max_attachment_area(gSavedSettings, "RenderAutoMuteSurfaceAreaLimit", 1000.0f);
		info_line = llformat("%.0f m^2", mAttachmentSurfaceArea);
		if (max_render_cost != 0 && max_attachment_area != 0)
		{
			green_level = 1.f-llclamp((mAttachmentSurfaceArea-max_attachment_area)/max_attachment_area, 0.f, 1.f);
			red_level   = llmin(mAttachmentSurfaceArea/max_attachment_area, 1.f);
			info_color.set(red_level, green_level, 0.0, 1.0);
			info_style = (  mAttachmentSurfaceArea > max_attachment_area
						  ? LLFontGL::BOLD : LLFontGL::NORMAL );
		}
		else
		{
			info_color.set(LLColor4::grey);
			info_style = LLFontGL::NORMAL;
		}
		mText->addLine(info_line, info_color, info_style);
		updateText();
	}
}
void LLVOAvatar::updateVisualComplexity()
{
	LL_DEBUGS("AvatarRender") << "avatar " << getID() << " appearance changed" << LL_ENDL;
	mVisualComplexityStale = true;
}
void LLVOAvatar::accountRenderComplexityForObject(
	const LLViewerObject *attached_object,
	const F32 max_attachment_complexity,
	LLVOVolume::texture_cost_t& textures,
	U32& cost
)
{
	if (attached_object && !attached_object->isHUDAttachment())
	{
		mAttachmentVisibleTriangleCount += attached_object->recursiveGetTriangleCount();
		mAttachmentEstTriangleCount += attached_object->recursiveGetEstTrianglesMax();
		mAttachmentSurfaceArea += attached_object->recursiveGetScaledSurfaceArea();
		textures.clear();
		const LLDrawable* drawable = attached_object->mDrawable;
		if (drawable)
		{
			const LLVOVolume* volume = drawable->getVOVolume();
			if (volume)
			{
				F32 attachment_total_cost = 0;
				F32 attachment_volume_cost = 0;
				F32 attachment_texture_cost = 0;
				F32 attachment_children_cost = 0;
				const F32 animated_object_attachment_surcharge = 1000;
				if (attached_object->isAnimatedObject())
				{
					attachment_volume_cost += animated_object_attachment_surcharge;
				}
				attachment_volume_cost += volume->getRenderCost(textures);
				const_child_list_t children = volume->getChildren();
				for (const_child_list_t::const_iterator child_iter = children.begin();
					child_iter != children.end();
					++child_iter)
				{
					LLViewerObject* child_obj = *child_iter;
                    LLVOVolume *child = child_obj ? child_obj->asVolume() : nullptr;
					if (child)
					{
						attachment_children_cost += child->getRenderCost(textures);
					}
				}
				for (LLVOVolume::texture_cost_t::iterator volume_texture = textures.begin();
					volume_texture != textures.end();
					++volume_texture)
				{
					attachment_texture_cost += volume_texture->second;
				}
				attachment_total_cost = attachment_volume_cost + attachment_texture_cost + attachment_children_cost;
				LL_DEBUGS("ARCdetail") << "Attachment costs " << attached_object->getAttachmentItemID()
					<< " total: " << attachment_total_cost
					<< ", volume: " << attachment_volume_cost
					<< ", textures: " << attachment_texture_cost
					<< ", " << volume->numChildren()
					<< " children: " << attachment_children_cost
					<< LL_ENDL;
				cost += (U32)llclamp(attachment_total_cost, MIN_ATTACHMENT_COMPLEXITY, max_attachment_complexity);
			}
		}
	}
	if (isSelf()
		&& attached_object
		&& attached_object->isHUDAttachment()
		&& !attached_object->isTempAttachment()
		&& attached_object->mDrawable)
	{
		textures.clear();
		mAttachmentSurfaceArea += attached_object->recursiveGetScaledSurfaceArea();
#if 0
		const LLVOVolume* volume = attached_object->mDrawable->getVOVolume();
		if (volume)
		{
			LLHUDComplexity hud_object_complexity;
			hud_object_complexity.objectName = attached_object->getAttachmentItemName();
			hud_object_complexity.objectId = attached_object->getAttachmentItemID();
			std::string joint_name;
			gAgentAvatarp->getAttachedPointName(attached_object->getAttachmentItemID(), joint_name);
			hud_object_complexity.jointName = joint_name;
			hud_object_complexity.objectsCost += volume->getRenderCost(textures);
			hud_object_complexity.objectsCount++;
			LLViewerObject::const_child_list_t& child_list = attached_object->getChildren();
			for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
				iter != child_list.end(); ++iter)
			{
				LLViewerObject* childp = *iter;
                const LLVOVolume* chld_volume = childp ? childp->asVolume() : nullptr;
				if (chld_volume)
				{
					hud_object_complexity.objectsCost += chld_volume->getRenderCost(textures);
					hud_object_complexity.objectsCount++;
				}
			}
			hud_object_complexity.texturesCount += textures.size();
			for (LLVOVolume::texture_cost_t::iterator volume_texture = textures.begin();
				volume_texture != textures.end();
				++volume_texture)
			{
				hud_object_complexity.texturesCost += volume_texture->second;
				LLViewerFetchedTexture *tex = LLViewerTextureManager::getFetchedTexture(volume_texture->first);
				if (tex)
				{
					hud_object_complexity.texturesMemoryTotal += tex->getTextureMemory();
					if (tex->getOriginalHeight() * tex->getOriginalWidth() >= HUD_OVERSIZED_TEXTURE_DATA_SIZE)
					{
						hud_object_complexity.largeTexturesCount++;
					}
				}
			}
			hud_complexity_list.push_back(hud_object_complexity);
		}
#endif
	}
}
void LLVOAvatar::calculateUpdateRenderComplexity()
{
	static const U32 COMPLEXITY_BODY_PART_COST = 200;
	static LLCachedControl<F32> max_complexity_setting(gSavedSettings,"MaxAttachmentComplexity");
	F32 max_attachment_complexity = max_complexity_setting;
	max_attachment_complexity = llmax(max_attachment_complexity, DEFAULT_MAX_ATTACHMENT_COMPLEXITY);
	static uuid_set_t all_textures;
	if (mVisualComplexityStale)
	{
		U32 cost = VISUAL_COMPLEXITY_UNKNOWN;
		LLVOVolume::texture_cost_t textures;
		for (U8 baked_index = 0; baked_index < BAKED_NUM_INDICES; baked_index++)
		{
			const LLAvatarAppearanceDictionary::BakedEntry *baked_dict
				= LLAvatarAppearanceDictionary::getInstance()->getBakedTexture((EBakedTextureIndex)baked_index);
			ETextureIndex tex_index = baked_dict->mTextureIndex;
			if ((tex_index != TEX_SKIRT_BAKED) || (isWearingWearableType(LLWearableType::WT_SKIRT)))
			{
				if (isTextureVisible(tex_index))
				{
					cost +=COMPLEXITY_BODY_PART_COST;
				}
			}
		}
		LL_DEBUGS("ARCdetail") << "Avatar body parts complexity: " << cost << LL_ENDL;
		mAttachmentVisibleTriangleCount = 0;
		mAttachmentEstTriangleCount = 0.f;
		mAttachmentSurfaceArea = 0.f;
		LLControlAvatar *control_av = asControlAvatar();
		if (control_av)
		{
			LLVOVolume *volp = control_av->mRootVolp;
			if (volp && !volp->isAttachment())
			{
				accountRenderComplexityForObject(volp, max_attachment_complexity,
												 textures, cost);
			}
		}
#if SLOW_ATTACHMENT_LIST
		for (attachment_map_t::const_iterator attachment_point = mAttachmentPoints.begin();
			 attachment_point != mAttachmentPoints.end();
			 ++attachment_point)
		{
			LLViewerJointAttachment* attachment = attachment_point->second;
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					attachment_iter != attachment->mAttachedObjects.end();
					++attachment_iter)
			{
				const LLViewerObject* attached_object = (*attachment_iter);
#else
		for(auto& iter : mAttachedObjectsVector)
		{{
				const LLViewerObject* attached_object = iter.first;
#endif
				accountRenderComplexityForObject(attached_object, max_attachment_complexity,
												 textures, cost);
			}
		}
		if (isSelf())
		{
			for (LLVOVolume::texture_cost_t::iterator it = textures.begin(); it != textures.end(); ++it)
			{
				LLUUID image_id = it->first;
				if( ! (image_id.isNull() || image_id == IMG_DEFAULT || image_id == IMG_DEFAULT_AVATAR)
				   && (all_textures.find(image_id) == all_textures.end()))
				{
					LL_DEBUGS("ARCdetail") << "attachment_texture: " << image_id.asString() << LL_ENDL;
					all_textures.insert(image_id);
				}
			}
			for (LLAvatarAppearanceDictionary::Textures::const_iterator iter = LLAvatarAppearanceDictionary::getInstance()->getTextures().begin();
					iter != LLAvatarAppearanceDictionary::getInstance()->getTextures().end();
					++iter)
			{
				const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = iter->second;
				const LLViewerTexture* te_image = getImage(iter->first,0);
				if (!te_image)
					continue;
				LLUUID image_id = te_image->getID();
				if( image_id.isNull() || image_id == IMG_DEFAULT || image_id == IMG_DEFAULT_AVATAR)
					continue;
				if (all_textures.find(image_id) == all_textures.end())
				{
					LL_DEBUGS("ARCdetail") << "local_texture: " << texture_dict->mName << ": " << image_id << LL_ENDL;
					all_textures.insert(image_id);
				}
			}
		}
		mVisualComplexity = cost;
		mVisualComplexityStale = false;
	}
}
BOOL LLVOAvatar::isIndexLocalTexture(ETextureIndex index)
{
	return (index < 0 || index >= TEX_NUM_INDICES)
		? false
		: LLAvatarAppearanceDictionary::getInstance()->getTexture(index)->mIsLocalTexture;
}
BOOL LLVOAvatar::isIndexBakedTexture(ETextureIndex index)
{
	return (index < 0 || index >= TEX_NUM_INDICES)
		? false
		: LLAvatarAppearanceDictionary::getInstance()->getTexture(index)->mIsBakedTexture;
}
const std::string LLVOAvatar::getBakedStatusForPrintout() const
{
	std::string line;
	for (LLAvatarAppearanceDictionary::Textures::const_iterator iter = LLAvatarAppearanceDictionary::getInstance()->getTextures().begin();
		 iter != LLAvatarAppearanceDictionary::getInstance()->getTextures().end();
		 ++iter)
	{
		const ETextureIndex index = iter->first;
		const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = iter->second;
		if (texture_dict->mIsBakedTexture)
		{
			line += texture_dict->mName;
			if (isTextureDefined(index))
			{
				line += "_baked";
			}
			line += " ";
		}
	}
	return line;
}
S32 LLVOAvatar::getTexImageSize() const
{
	return TEX_IMAGE_SIZE_OTHER;
}
F32 calc_bouncy_animation(F32 x)
{
	return -(cosf(x * F_PI * 2.5f - F_PI_BY_TWO))*(0.4f + x * -0.1f) + x * 1.3f;
}
BOOL LLVOAvatar::isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex te, U32 index ) const
{
	if (isIndexLocalTexture(te))
	{
		return FALSE;
	}
	if( !getImage( te, index ) )
	{
		LL_WARNS() << "getImage( " << te << ", " << index << " ) returned 0" << LL_ENDL;
		return FALSE;
	}
	return (getImage(te, index)->getID() != IMG_DEFAULT_AVATAR &&
			getImage(te, index)->getID() != IMG_DEFAULT);
}
BOOL LLVOAvatar::isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const
{
	if (isIndexLocalTexture(type))
	{
		return isTextureDefined(type, index);
	}
	else
	{
		return ((isTextureDefined(type) || isSelf())
				&& (getTEImage(type)->getID() != IMG_INVISIBLE
				|| LLDrawPoolAlpha::sShowDebugAlpha));
	}
}
BOOL LLVOAvatar::isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerWearable *wearable) const
{
	return FALSE;
}
