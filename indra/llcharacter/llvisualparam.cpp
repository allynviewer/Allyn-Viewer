/** 
 * @file llvisualparam.cpp
 * @brief Implementation of LLPolyMesh class.
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
#include "linden_common.h"
#include "llvisualparam.h"
LLVisualParamInfo::LLVisualParamInfo()
	:
	mID( -1 ),
	mGroup( VISUAL_PARAM_GROUP_TWEAKABLE ),
	mMinWeight( 0.f ),
	mMaxWeight( 1.f ),
	mDefaultWeight( 0.f ),
	mSex( SEX_BOTH )
{
}
BOOL LLVisualParamInfo::parseXml(LLXmlTreeNode *node)
{
	static LLStdStringHandle id_string = LLXmlTree::addAttributeString("id");
	node->getFastAttributeS32( id_string, mID );
	U32 group = 0;
	static LLStdStringHandle group_string = LLXmlTree::addAttributeString("group");
	if( node->getFastAttributeU32( group_string, group ) )
	{
		if( group < NUM_VISUAL_PARAM_GROUPS )
		{
			mGroup = (EVisualParamGroup)group;
		}
	}
	static LLStdStringHandle value_min_string = LLXmlTree::addAttributeString("value_min");
	static LLStdStringHandle value_max_string = LLXmlTree::addAttributeString("value_max");
	node->getFastAttributeF32( value_min_string, mMinWeight );
	node->getFastAttributeF32( value_max_string, mMaxWeight );
	F32 default_weight = 0;
	static LLStdStringHandle value_default_string = LLXmlTree::addAttributeString("value_default");
	if( node->getFastAttributeF32( value_default_string, default_weight ) )
	{
		mDefaultWeight = llclamp( default_weight, mMinWeight, mMaxWeight );
		if( default_weight != mDefaultWeight )
		{
			LL_WARNS() << "value_default attribute is out of range in node " << mName << " " << default_weight << LL_ENDL;
		}
	}
	std::string sex = "both";
	static LLStdStringHandle sex_string = LLXmlTree::addAttributeString("sex");
	node->getFastAttributeString( sex_string, sex );
	if( sex == "both" )
	{
		mSex = SEX_BOTH;
	}
	else if( sex == "male" )
	{
		mSex = SEX_MALE;
	}
	else if( sex == "female" )
	{
		mSex = SEX_FEMALE;
	}
	else
	{
		LL_WARNS() << "Avatar file: <param> has invalid sex attribute: " << sex << LL_ENDL;
		return FALSE;
	}
	static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
	if( !node->getFastAttributeString( name_string, mName ) )
	{
		LL_WARNS() << "Avatar file: <param> is missing name attribute" << LL_ENDL;
		return FALSE;
	}
	static LLStdStringHandle label_string = LLXmlTree::addAttributeString("label");
	if( !node->getFastAttributeString( label_string, mDisplayName ) )
	{
		mDisplayName = mName;
	}
	LLStringUtil::toLower(mName);
	static LLStdStringHandle label_min_string = LLXmlTree::addAttributeString("label_min");
	if( !node->getFastAttributeString( label_min_string, mMinName ) )
	{
		mMinName = "Less";
	}
	static LLStdStringHandle label_max_string = LLXmlTree::addAttributeString("label_max");
	if( !node->getFastAttributeString( label_max_string, mMaxName ) )
	{
		mMaxName = "More";
	}
	return TRUE;
}
void LLVisualParamInfo::toStream(std::ostream &out)
{
	out <<  mID << "\t";
	out << mName << "\t";
	out <<  mDisplayName << "\t";
	out <<  mMinName << "\t";
	out <<  mMaxName << "\t";
	out <<  mGroup << "\t";
	out <<  mMinWeight << "\t";
	out <<  mMaxWeight << "\t";
	out <<  mDefaultWeight << "\t";
	out <<  mSex << "\t";
}
LLVisualParam::LLVisualParam()
	: mCurWeight( 0.f ),
	mLastWeight( 0.f ),
	mNext( NULL ),
	mTargetWeight( 0.f ),
	mIsAnimating( FALSE ),
	mIsDummy(FALSE),
	mID( -1 ),
	mInfo( 0 ),
	mParamLocation(LOC_UNKNOWN)
{
}
LLVisualParam::LLVisualParam(const LLVisualParam& pOther)
	: mCurWeight(pOther.mCurWeight),
	mLastWeight(pOther.mLastWeight),
	mNext(pOther.mNext),
	mTargetWeight(pOther.mTargetWeight),
	mIsAnimating(pOther.mIsAnimating),
	mIsDummy(pOther.mIsDummy),
	mID(pOther.mID),
	mInfo(pOther.mInfo),
	mParamLocation(pOther.mParamLocation)
{
}
LLVisualParam::~LLVisualParam()
{
	delete mNext;
	mNext = NULL;
}
void LLVisualParam::setWeight(F32 weight, bool upload_bake)
{
	if (mIsAnimating)
	{
		mCurWeight = weight;
	}
	else if (mInfo)
	{
		mCurWeight = llclamp(weight, mInfo->mMinWeight, mInfo->mMaxWeight);
	}
	else
	{
		mCurWeight = weight;
	}
	if (mNext)
	{
		mNext->setWeight(weight, upload_bake);
	}
}
void LLVisualParam::setAnimationTarget(F32 target_value, bool upload_bake)
{
	if (mIsDummy)
	{
		setWeight(target_value, upload_bake);
		mTargetWeight = mCurWeight;
		return;
	}
	if (mInfo)
	{
		if (isTweakable())
		{
			mTargetWeight = llclamp(target_value, mInfo->mMinWeight, mInfo->mMaxWeight);
		}
	}
	else
	{
		mTargetWeight = target_value;
	}
	mIsAnimating = TRUE;
	if (mNext)
	{
		mNext->setAnimationTarget(target_value, upload_bake);
	}
}
void LLVisualParam::setNextParam( LLVisualParam *next )
{
	llassert(!mNext);
	llassert(getWeight() == getDefaultWeight());
	mNext = next;
}
void LLVisualParam::clearNextParam()
{
	mNext = NULL;
}
void LLVisualParam::animate( F32 delta, bool upload_bake )
{
	if (mIsAnimating)
	{
		F32 new_weight = ((mTargetWeight - mCurWeight) * delta) + mCurWeight;
		setWeight(new_weight, upload_bake);
	}
}
void LLVisualParam::stopAnimating(bool upload_bake)
{
	if (mIsAnimating && isTweakable())
	{
		mIsAnimating = FALSE;
		setWeight(mTargetWeight, upload_bake);
	}
}
BOOL LLVisualParam::linkDrivenParams(visual_param_mapper mapper, BOOL only_cross_params)
{
	return TRUE;
}
void LLVisualParam::resetDrivenParams()
{
	return;
}
const std::string param_location_name(const EParamLocation& loc)
{
	switch (loc)
	{
		case LOC_UNKNOWN: return "unknown";
		case LOC_AV_SELF: return "self";
		case LOC_AV_OTHER: return "other";
		case LOC_WEARABLE: return "wearable";
		default: return "error";
	}
}
void LLVisualParam::setParamLocation(EParamLocation loc)
{
	if (mParamLocation == LOC_UNKNOWN || loc == LOC_UNKNOWN)
	{
		mParamLocation = loc;
	}
	else if (mParamLocation == loc)
	{
	}
	else
	{
		LL_DEBUGS() << "param location is already " << mParamLocation << ", not slamming to " << loc << LL_ENDL;
	}
}
