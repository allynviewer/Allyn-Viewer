/** 
 * @file llbvhloader.cpp
 * @brief Translates a BVH files to LindenLabAnimation format.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "llbvhloader.h"
#include <boost/tokenizer.hpp>
#include "lldatapacker.h"
#include "lldir.h"
#include "llkeyframemotion.h"
#include "llquantize.h"
#include "llstl.h"
#define INCHES_TO_METERS 0.02540005f
const F32 POSITION_KEYFRAME_THRESHOLD_SQUARED = 0.03f * 0.03f;
const F32 ROTATION_KEYFRAME_THRESHOLD = 0.01f;
const F32 POSITION_MOTION_THRESHOLD_SQUARED = 0.001f * 0.001f;
const F32 ROTATION_MOTION_THRESHOLD = 0.001f;
char gInFile[1024];
char gOutFile[1024];
const char *find_next_whitespace(const char *p)
{
	while(*p && isspace(*p)) p++;
	while(*p && !isspace(*p)) p++;
	return p;
}
LLQuaternion::Order bvhStringToOrder( char *str )
{
	char order[4];
	order[0] = str[2];
	order[1] = str[1];
	order[2] = str[0];
	order[3] = 0;
	LLQuaternion::Order retVal = StringToOrder( order );
	return retVal;
}
LLBVHLoader::LLBVHLoader(const char* buffer, ELoadStatus &loadStatus, S32 &errorLine)
{
	reset();
	errorLine = 0;
	mStatus = loadTranslationTable("anim.ini");
	loadStatus = mStatus;
	LL_INFOS() << "Load Status 00 : " << loadStatus << LL_ENDL;
	if (mStatus == E_ST_NO_XLT_FILE)
	{
		loadStatus = mStatus;
		return;
	}
	else
	{
		if (mStatus != E_ST_OK)
		{
			errorLine = getLineNumber();
			loadStatus = mStatus;
			return;
		}
	}
	char error_text[128];
	S32 error_line;
	mStatus = loadBVHFile(buffer, error_text, error_line);
	if (mStatus != E_ST_OK)
	{
		loadStatus = mStatus;
		errorLine = getLineNumber();
		return;
	}
	applyTranslations();
	optimize();
	mInitialized = TRUE;
}
LLBVHLoader::~LLBVHLoader()
{
	std::for_each(mJoints.begin(),mJoints.end(),DeletePointer());
}
ELoadStatus LLBVHLoader::loadTranslationTable(const char *fileName)
{
	mLineNumber = 0;
	mTranslations.clear();
	mConstraints.clear();
	std::string path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,fileName);
	llifstream infstream(path);
	if (!infstream.is_open())
		return E_ST_NO_XLT_FILE;
	LL_INFOS() << "NOTE: Loading translation table: " << fileName << LL_ENDL;
	if ( ! getLine(infstream) )
		return E_ST_EOF;
	if ( strncmp(mLine, "Translations 1.0", 16) )
		return E_ST_NO_XLT_HEADER;
	BOOL loadingGlobals = FALSE;
	Translation *trans = NULL;
	while ( getLine(infstream) )
	{
		char token[128];
		if ( sscanf(mLine, " %127s", token) != 1 )
			continue;
		if (token[0] == '#')
			continue;
		if (token[0] == '[')
		{
			char name[128];
			if ( sscanf(mLine, " [%127[^]]", name) != 1 )
				return E_ST_NO_XLT_NAME;
			if (strcmp(name, "GLOBALS")==0)
			{
				loadingGlobals = TRUE;
				continue;
			}
			else
			{
				loadingGlobals = FALSE;
				Translation &newTrans = mTranslations[ name ];
				trans = &newTrans;
				continue;
			}
		}
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "emote")==0)
		{
			char emote_str[1024];
			if ( sscanf(mLine, " %*s = %1023s", emote_str) != 1 )
				return E_ST_NO_XLT_EMOTE;
			mEmoteName.assign( emote_str );
			continue;
		}
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "priority")==0)
		{
			S32 priority;
			if ( sscanf(mLine, " %*s = %d", &priority) != 1 )
				return E_ST_NO_XLT_PRIORITY;
			mPriority = priority;
			continue;
		}
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "loop")==0)
		{
			char trueFalse[128];
			trueFalse[0] = '\0';
			F32 loop_in = 0.f;
			F32 loop_out = 1.f;
			if ( sscanf(mLine, " %*s = %f %f", &loop_in, &loop_out) == 2 )
			{
				mLoop = TRUE;
			}
			else if ( sscanf(mLine, " %*s = %127s", trueFalse) == 1 )
			{
				mLoop = (LLStringUtil::compareInsensitive(trueFalse, "true")==0);
			}
			else
			{
				return E_ST_NO_XLT_LOOP;
			}
			mLoopInPoint = loop_in * mDuration;
			mLoopOutPoint = loop_out * mDuration;
			continue;
		}
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "easein")==0)
		{
			F32 duration;
			char type[128];
			if ( sscanf(mLine, " %*s = %f %127s", &duration, type) != 2 )
				return E_ST_NO_XLT_EASEIN;
			mEaseIn = duration;
			continue;
		}
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "easeout")==0)
		{
			F32 duration;
			char type[128];
			if ( sscanf(mLine, " %*s = %f %127s", &duration, type) != 2 )
				return E_ST_NO_XLT_EASEOUT;
			mEaseOut = duration;
			continue;
		}
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "hand")==0)
		{
			S32 handMorph;
			if (sscanf(mLine, " %*s = %d", &handMorph) != 1)
				return E_ST_NO_XLT_HAND;
			mHand = handMorph;
			continue;
		}
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "constraint")==0)
		{
			Constraint constraint;
			if(sscanf(
				mLine,
				" %*s = %d %f %f %f %f %15s %f %f %f %15s %f %f %f %f %f %f",
				&constraint.mChainLength,
				&constraint.mEaseInStart,
				&constraint.mEaseInStop,
				&constraint.mEaseOutStart,
				&constraint.mEaseOutStop,
				constraint.mSourceJointName,
				&constraint.mSourceOffset.mV[VX],
				&constraint.mSourceOffset.mV[VY],
				&constraint.mSourceOffset.mV[VZ],
				constraint.mTargetJointName,
				&constraint.mTargetOffset.mV[VX],
				&constraint.mTargetOffset.mV[VY],
				&constraint.mTargetOffset.mV[VZ],
				&constraint.mTargetDir.mV[VX],
				&constraint.mTargetDir.mV[VY],
				&constraint.mTargetDir.mV[VZ]) != 16)
			{
				if(sscanf(
					mLine,
					" %*s = %d %f %f %f %f %15s %f %f %f %15s %f %f %f",
					&constraint.mChainLength,
					&constraint.mEaseInStart,
					&constraint.mEaseInStop,
					&constraint.mEaseOutStart,
					&constraint.mEaseOutStop,
					constraint.mSourceJointName,
					&constraint.mSourceOffset.mV[VX],
					&constraint.mSourceOffset.mV[VY],
					&constraint.mSourceOffset.mV[VZ],
					constraint.mTargetJointName,
					&constraint.mTargetOffset.mV[VX],
					&constraint.mTargetOffset.mV[VY],
					&constraint.mTargetOffset.mV[VZ]) != 13)
				{
					return E_ST_NO_CONSTRAINT;
				}
			}
			else
			{
				if (!constraint.mTargetDir.isExactlyZero())
				{
					constraint.mTargetDir.normVec();
				}
			}
			constraint.mConstraintType = CONSTRAINT_TYPE_POINT;
			mConstraints.push_back(constraint);
			continue;
		}
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "planar_constraint")==0)
		{
			Constraint constraint;
			if(sscanf(
				mLine,
				" %*s = %d %f %f %f %f %15s %f %f %f %15s %f %f %f %f %f %f",
				&constraint.mChainLength,
				&constraint.mEaseInStart,
				&constraint.mEaseInStop,
				&constraint.mEaseOutStart,
				&constraint.mEaseOutStop,
				constraint.mSourceJointName,
				&constraint.mSourceOffset.mV[VX],
				&constraint.mSourceOffset.mV[VY],
				&constraint.mSourceOffset.mV[VZ],
				constraint.mTargetJointName,
				&constraint.mTargetOffset.mV[VX],
				&constraint.mTargetOffset.mV[VY],
				&constraint.mTargetOffset.mV[VZ],
				&constraint.mTargetDir.mV[VX],
				&constraint.mTargetDir.mV[VY],
				&constraint.mTargetDir.mV[VZ]) != 16)
			{
				if(sscanf(
					mLine,
					" %*s = %d %f %f %f %f %15s %f %f %f %15s %f %f %f",
					&constraint.mChainLength,
					&constraint.mEaseInStart,
					&constraint.mEaseInStop,
					&constraint.mEaseOutStart,
					&constraint.mEaseOutStop,
					constraint.mSourceJointName,
					&constraint.mSourceOffset.mV[VX],
					&constraint.mSourceOffset.mV[VY],
					&constraint.mSourceOffset.mV[VZ],
					constraint.mTargetJointName,
					&constraint.mTargetOffset.mV[VX],
					&constraint.mTargetOffset.mV[VY],
					&constraint.mTargetOffset.mV[VZ]) != 13)
				{
					return E_ST_NO_CONSTRAINT;
				}
			}
			else
			{
				if (!constraint.mTargetDir.isExactlyZero())
				{
					constraint.mTargetDir.normVec();
				}
			}
			constraint.mConstraintType = CONSTRAINT_TYPE_PLANE;
			mConstraints.push_back(constraint);
			continue;
		}
		if ( ! trans )
			return E_ST_NO_XLT_NAME;
		if ( LLStringUtil::compareInsensitive(token, "ignore")==0 )
		{
			char trueFalse[128];
			if ( sscanf(mLine, " %*s = %127s", trueFalse) != 1 )
				return E_ST_NO_XLT_IGNORE;
			trans->mIgnore = (LLStringUtil::compareInsensitive(trueFalse, "true")==0);
			continue;
		}
		if ( LLStringUtil::compareInsensitive(token, "relativepos")==0 )
		{
			F32 x, y, z;
			char relpos[128];
			if ( sscanf(mLine, " %*s = %f %f %f", &x, &y, &z) == 3 )
			{
				trans->mRelativePosition.setVec( x, y, z );
			}
			else if ( sscanf(mLine, " %*s = %127s", relpos) == 1 )
			{
				if ( LLStringUtil::compareInsensitive(relpos, "firstkey")==0 )
				{
					trans->mRelativePositionKey = TRUE;
				}
				else
				{
					return E_ST_NO_XLT_RELATIVE;
				}
			}
			else
			{
				return E_ST_NO_XLT_RELATIVE;
			}
			continue;
		}
		if ( LLStringUtil::compareInsensitive(token, "relativerot")==0 )
		{
			char relpos[128];
			if ( sscanf(mLine, " %*s = %127s", relpos) == 1 )
			{
				if ( LLStringUtil::compareInsensitive(relpos, "firstkey")==0 )
				{
					trans->mRelativeRotationKey = TRUE;
				}
				else
				{
					return E_ST_NO_XLT_RELATIVE;
				}
			}
			else
			{
				return E_ST_NO_XLT_RELATIVE;
			}
			continue;
		}
		if ( LLStringUtil::compareInsensitive(token, "outname")==0 )
		{
			char outName[128];
			if ( sscanf(mLine, " %*s = %127s", outName) != 1 )
				return E_ST_NO_XLT_OUTNAME;
			trans->mOutName = outName;
			continue;
		}
		if ( LLStringUtil::compareInsensitive(token, "frame")==0 )
		{
			LLMatrix3 fm;
			if ( sscanf(mLine, " %*s = %f %f %f, %f %f %f, %f %f %f",
					&fm.mMatrix[0][0], &fm.mMatrix[0][1], &fm.mMatrix[0][2],
					&fm.mMatrix[1][0], &fm.mMatrix[1][1], &fm.mMatrix[1][2],
					&fm.mMatrix[2][0], &fm.mMatrix[2][1], &fm.mMatrix[2][2]	) != 9 )
				return E_ST_NO_XLT_MATRIX;
			trans->mFrameMatrix = fm;
			continue;
		}
		if ( LLStringUtil::compareInsensitive(token, "offset")==0 )
		{
			LLMatrix3 om;
			if ( sscanf(mLine, " %*s = %f %f %f, %f %f %f, %f %f %f",
					&om.mMatrix[0][0], &om.mMatrix[0][1], &om.mMatrix[0][2],
					&om.mMatrix[1][0], &om.mMatrix[1][1], &om.mMatrix[1][2],
					&om.mMatrix[2][0], &om.mMatrix[2][1], &om.mMatrix[2][2]	) != 9 )
				return E_ST_NO_XLT_MATRIX;
			trans->mOffsetMatrix = om;
			continue;
		}
		if ( LLStringUtil::compareInsensitive(token, "mergeparent")==0 )
		{
			char mergeParentName[128];
			if ( sscanf(mLine, " %*s = %127s", mergeParentName) != 1 )
				return E_ST_NO_XLT_MERGEPARENT;
			trans->mMergeParentName = mergeParentName;
			continue;
		}
		if ( LLStringUtil::compareInsensitive(token, "mergechild")==0 )
		{
			char mergeChildName[128];
			if ( sscanf(mLine, " %*s = %127s", mergeChildName) != 1 )
				return E_ST_NO_XLT_MERGECHILD;
			trans->mMergeChildName = mergeChildName;
			continue;
		}
		if ( LLStringUtil::compareInsensitive(token, "priority")==0 )
		{
			S32 priority;
			if ( sscanf(mLine, " %*s = %d", &priority) != 1 )
				return E_ST_NO_XLT_PRIORITY;
			trans->mPriorityModifier = priority;
			continue;
		}
	}
	infstream.close() ;
	return E_ST_OK;
}
ELoadStatus LLBVHLoader::loadBVHFile(const char *buffer, char* error_text, S32 &err_line)
{
	std::string line;
	err_line = 0;
	error_text[127] = '\0';
	std::string str(buffer);
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("\r\n");
	tokenizer tokens(str, sep);
	tokenizer::iterator iter = tokens.begin();
	mLineNumber = 0;
	mJoints.clear();
	std::vector<S32> parent_joints;
	if (iter == tokens.end())
		return E_ST_EOF;
	line = (*(iter++));
	err_line++;
	if ( !strstr(line.c_str(), "HIERARCHY") )
	{
		return E_ST_NO_HIER;
	}
	while (TRUE)
	{
		if (iter == tokens.end())
			return E_ST_EOF;
		line = (*(iter++));
		err_line++;
		if ( strstr(line.c_str(), "}") )
		{
			if (parent_joints.size() > 0)
			{
				parent_joints.pop_back();
			}
			continue;
		}
		if ( strstr(line.c_str(), "MOTION") )
			break;
		if ( strstr(line.c_str(), "ROOT") )
		{
		}
		else if ( strstr(line.c_str(), "JOINT") )
		{
		}
		else if ( strstr(line.c_str(), "End Site") )
		{
			iter++;
			iter++;
			S32 depth = 0;
			for (S32 j = (S32)parent_joints.size() - 1; j >= 0; j--)
			{
				Joint *joint = mJoints[parent_joints[j]];
				if (depth > joint->mChildTreeMaxDepth)
				{
					joint->mChildTreeMaxDepth = depth;
				}
				depth++;
			}
			continue;
		}
		else
		{
			strncpy(error_text, line.c_str(), 127);
			return E_ST_NO_JOINT;
		}
		char jointName[80];
		if ( sscanf(line.c_str(), "%*s %79s", jointName) != 1 )
		{
			strncpy(error_text, line.c_str(), 127);
			return E_ST_NO_NAME;
		}
		const char* FORCED_ROOT_NAME = "hip";
		if ( (mJoints.size() == 0 ) && ( !strstr(jointName, FORCED_ROOT_NAME) ) )
		{
			strncpy(error_text, line.c_str(), 127);
			return E_ST_BAD_ROOT;
		}
		mJoints.push_back( new Joint( jointName ) );
		Joint *joint = mJoints.back();
		S32 depth = 1;
		for (S32 j = (S32)parent_joints.size() - 1; j >= 0; j--)
		{
			Joint *pjoint = mJoints[parent_joints[j]];
			if (depth > pjoint->mChildTreeMaxDepth)
			{
				pjoint->mChildTreeMaxDepth = depth;
			}
			depth++;
		}
		if (iter == tokens.end())
		{
			return E_ST_EOF;
		}
		line = (*(iter++));
		err_line++;
		if ( !strstr(line.c_str(), "{") )
		{
			strncpy(error_text, line.c_str(), 127);
			return E_ST_NO_OFFSET;
		}
		else
		{
			parent_joints.push_back((S32)mJoints.size() - 1);
		}
		if (iter == tokens.end())
		{
			return E_ST_EOF;
		}
		line = (*(iter++));
		err_line++;
		if ( !strstr(line.c_str(), "OFFSET") )
		{
			strncpy(error_text, line.c_str(), 127);
			return E_ST_NO_OFFSET;
		}
		if (iter == tokens.end())
		{
			return E_ST_EOF;
		}
		line = (*(iter++));
		err_line++;
		if ( !strstr(line.c_str(), "CHANNELS") )
		{
			strncpy(error_text, line.c_str(), 127);
			return E_ST_NO_CHANNELS;
		}
		const char *p = line.c_str();
		for (S32 i=0; i<3; i++)
		{
			p = strstr(p, "rotation");
			if (!p)
			{
				strncpy(error_text, line.c_str(), 127);
				return E_ST_NO_ROTATION;
			}
			const char axis = *(p - 1);
			if ((axis != 'X') && (axis != 'Y') && (axis != 'Z'))
			{
				strncpy(error_text, line.c_str(), 127);
				return E_ST_NO_AXIS;
			}
			joint->mOrder[i] = axis;
			p++;
		}
	}
	if ( !strstr(line.c_str(), "MOTION") )
	{
		strncpy(error_text, line.c_str(), 127);
		return E_ST_NO_MOTION;
	}
	if (iter == tokens.end())
	{
		return E_ST_EOF;
	}
	line = (*(iter++));
	err_line++;
	if ( !strstr(line.c_str(), "Frames:") )
	{
		strncpy(error_text, line.c_str(), 127);
		return E_ST_NO_FRAMES;
	}
	if ( sscanf(line.c_str(), "Frames: %d", &mNumFrames) != 1 )
	{
		strncpy(error_text, line.c_str(), 127);
		return E_ST_NO_FRAMES;
	}
	if (iter == tokens.end())
	{
		return E_ST_EOF;
	}
	line = (*(iter++));
	err_line++;
	if ( !strstr(line.c_str(), "Frame Time:") )
	{
		strncpy(error_text, line.c_str(), 127);
		return E_ST_NO_FRAME_TIME;
	}
	if ( sscanf(line.c_str(), "Frame Time: %f", &mFrameTime) != 1 )
	{
		strncpy(error_text, line.c_str(), 127);
		return E_ST_NO_FRAME_TIME;
	}
	mDuration = (F32)mNumFrames * mFrameTime;
	if (!mLoop)
	{
		mLoopOutPoint = mDuration;
	}
	for (S32 i=0; i<mNumFrames; i++)
	{
		if (iter == tokens.end())
		{
			return E_ST_EOF;
		}
		line = (*(iter++));
		err_line++;
		const char *p = line.c_str();
		for (U32 j=0; j<mJoints.size(); j++)
		{
			Joint *joint = mJoints[j];
			joint->mKeys.push_back( Key() );
			Key &key = joint->mKeys.back();
			if (j==0)
			{
				if ( sscanf(p, "%f %f %f", key.mPos, key.mPos+1, key.mPos+2) != 3 )
				{
					strncpy(error_text, line.c_str(), 127);
					return E_ST_NO_POS;
				}
			}
			p = find_next_whitespace(p);
			if (!p)
			{
				strncpy(error_text, line.c_str(), 127);
				return E_ST_NO_ROT;
			}
			p = find_next_whitespace(++p);
			if (!p)
			{
				strncpy(error_text, line.c_str(), 127);
				return E_ST_NO_ROT;
			}
			p = find_next_whitespace(++p);
			if (!p)
			{
				strncpy(error_text, line.c_str(), 127);
				return E_ST_NO_ROT;
			}
			F32 rot[3];
			if ( sscanf(p, " %f %f %f", rot, rot+1, rot+2) != 3 )
			{
				strncpy(error_text, line.c_str(), 127);
				return E_ST_NO_ROT;
			}
			p++;
			key.mRot[ joint->mOrder[0]-'X' ] = rot[0];
			key.mRot[ joint->mOrder[1]-'X' ] = rot[1];
			key.mRot[ joint->mOrder[2]-'X' ] = rot[2];
		}
	}
	return E_ST_OK;
}
void LLBVHLoader::applyTranslations()
{
	JointVector::iterator ji;
	for (ji = mJoints.begin(); ji != mJoints.end(); ++ji )
	{
		Joint *joint = *ji;
		TranslationMap::iterator ti = mTranslations.find( joint->mName );
		if ( ti == mTranslations.end() )
		{
			continue;
		}
		Translation &trans = ti->second;
		if ( trans.mIgnore )
		{
			joint->mIgnore = TRUE;
			continue;
		}
		if ( ! trans.mOutName.empty() )
		{
			joint->mOutName = trans.mOutName;
		}
		if ( joint->mOutName == std::string("mPelvis") )
		{
			joint->mIgnorePositions = FALSE;
		}
		if ( trans.mRelativePositionKey )
		{
			joint->mRelativePositionKey = TRUE;
		}
		if ( trans.mRelativeRotationKey )
		{
			joint->mRelativeRotationKey = TRUE;
		}
		if ( trans.mRelativePosition.magVec() > 0.0f )
		{
			joint->mRelativePosition = trans.mRelativePosition;
		}
		joint->mFrameMatrix = trans.mFrameMatrix;
		joint->mOffsetMatrix = trans.mOffsetMatrix;
		if ( ! trans.mMergeParentName.empty() )
		{
			joint->mMergeParentName = trans.mMergeParentName;
		}
		if ( ! trans.mMergeChildName.empty() )
		{
			joint->mMergeChildName = trans.mMergeChildName;
		}
		joint->mPriority = mPriority + trans.mPriorityModifier;
	}
}
void LLBVHLoader::optimize()
{
	if (!mLoop && mEaseIn + mEaseOut > mDuration && mDuration != 0.f)
	{
		F32 factor = mDuration / (mEaseIn + mEaseOut);
		mEaseIn *= factor;
		mEaseOut *= factor;
	}
	JointVector::iterator ji;
	for (ji = mJoints.begin(); ji != mJoints.end(); ++ji)
	{
		Joint *joint = *ji;
		BOOL pos_changed = FALSE;
		BOOL rot_changed = FALSE;
		if ( ! joint->mIgnore )
		{
			joint->mNumPosKeys = 0;
			joint->mNumRotKeys = 0;
			LLQuaternion::Order order = bvhStringToOrder( joint->mOrder );
			KeyVector::iterator first_key = joint->mKeys.begin();
			if (first_key == joint->mKeys.end())
			{
				joint->mIgnore = TRUE;
				continue;
			}
			LLVector3 first_frame_pos(first_key->mPos);
			LLQuaternion first_frame_rot = mayaQ( first_key->mRot[0], first_key->mRot[1], first_key->mRot[2], order);
			KeyVector::iterator ki = joint->mKeys.begin();
			if (joint->mKeys.size() == 1)
			{
				rot_changed = TRUE;
			}
			else
			{
				first_key->mIgnorePos = TRUE;
				first_key->mIgnoreRot = TRUE;
				++ki;
			}
			KeyVector::iterator ki_prev = ki;
			KeyVector::iterator ki_last_good_pos = ki;
			KeyVector::iterator ki_last_good_rot = ki;
			S32 numPosFramesConsidered = 2;
			S32 numRotFramesConsidered = 2;
			F32 rot_threshold = ROTATION_KEYFRAME_THRESHOLD / llmax((F32)joint->mChildTreeMaxDepth * 0.33f, 1.f);
			double diff_max = 0;
			KeyVector::iterator ki_max = ki;
			for (; ki != joint->mKeys.end(); ++ki)
			{
				if (ki_prev == ki_last_good_pos)
				{
					joint->mNumPosKeys++;
					if (dist_vec_squared(LLVector3(ki_prev->mPos), first_frame_pos) > POSITION_MOTION_THRESHOLD_SQUARED)
					{
						pos_changed = TRUE;
					}
				}
				else
				{
					LLVector3 test_pos(ki_prev->mPos);
					LLVector3 last_good_pos(ki_last_good_pos->mPos);
					LLVector3 current_pos(ki->mPos);
					LLVector3 interp_pos = lerp(current_pos, last_good_pos, 1.f / (F32)numPosFramesConsidered);
					if (dist_vec_squared(current_pos, first_frame_pos) > POSITION_MOTION_THRESHOLD_SQUARED)
					{
						pos_changed = TRUE;
					}
					if (dist_vec_squared(interp_pos, test_pos) < POSITION_KEYFRAME_THRESHOLD_SQUARED)
					{
						ki_prev->mIgnorePos = TRUE;
						numPosFramesConsidered++;
					}
					else
					{
						numPosFramesConsidered = 2;
						ki_last_good_pos = ki_prev;
						joint->mNumPosKeys++;
					}
				}
				if (ki_prev == ki_last_good_rot)
				{
					joint->mNumRotKeys++;
					LLQuaternion test_rot = mayaQ( ki_prev->mRot[0], ki_prev->mRot[1], ki_prev->mRot[2], order);
					F32 x_delta = dist_vec(LLVector3::x_axis * first_frame_rot, LLVector3::x_axis * test_rot);
					F32 y_delta = dist_vec(LLVector3::y_axis * first_frame_rot, LLVector3::y_axis * test_rot);
					F32 rot_test = x_delta + y_delta;
					if (rot_test > ROTATION_MOTION_THRESHOLD)
					{
						rot_changed = TRUE;
					}
				}
				else
				{
					LLQuaternion test_rot = mayaQ( ki_prev->mRot[0], ki_prev->mRot[1], ki_prev->mRot[2], order);
					LLQuaternion last_good_rot = mayaQ( ki_last_good_rot->mRot[0], ki_last_good_rot->mRot[1], ki_last_good_rot->mRot[2], order);
					LLQuaternion current_rot = mayaQ( ki->mRot[0], ki->mRot[1], ki->mRot[2], order);
					LLQuaternion interp_rot = lerp(1.f / (F32)numRotFramesConsidered, current_rot, last_good_rot);
					F32 x_delta;
					F32 y_delta;
					F32 rot_test;
					x_delta = dist_vec(LLVector3::x_axis * first_frame_rot, LLVector3::x_axis * test_rot);
					y_delta = dist_vec(LLVector3::y_axis * first_frame_rot, LLVector3::y_axis * test_rot);
					rot_test = x_delta + y_delta;
					if (rot_test > ROTATION_MOTION_THRESHOLD)
					{
						rot_changed = TRUE;
					}
					x_delta = dist_vec(LLVector3::x_axis * interp_rot, LLVector3::x_axis * test_rot);
					y_delta = dist_vec(LLVector3::y_axis * interp_rot, LLVector3::y_axis * test_rot);
					rot_test = x_delta + y_delta;
					if (rot_test >= rot_threshold ||
						(ki+1 == joint->mKeys.end() && numRotFramesConsidered > 2))
					{
						numRotFramesConsidered = 2;
						ki_last_good_rot = ki_prev;
						joint->mNumRotKeys++;
						if (diff_max > 0)
						{
							if (ki_max->mIgnoreRot == TRUE)
							{
								ki_max->mIgnoreRot = FALSE;
								joint->mNumRotKeys++;
							}
							diff_max = 0;
						}
					}
					else
					{
						ki_prev->mIgnoreRot = TRUE;
						numRotFramesConsidered++;
						if (rot_test > diff_max)
						{
							diff_max = rot_test;
							ki_max = ki;
						}
					}
				}
				ki_prev = ki;
			}
		}
		if (!(pos_changed || rot_changed))
		{
			joint->mIgnore = TRUE;
		}
	}
}
void LLBVHLoader::reset()
{
	mLineNumber = 0;
	mNumFrames = 0;
	mFrameTime = 0.0f;
	mDuration = 0.0f;
	mPriority = 2;
	mLoop = FALSE;
	mLoopInPoint = 0.f;
	mLoopOutPoint = 0.f;
	mEaseIn = 0.3f;
	mEaseOut = 0.3f;
	mHand = 1;
	mInitialized = FALSE;
	mEmoteName = "";
}
BOOL LLBVHLoader::getLine(llifstream& stream)
{
	if (stream.getline(mLine, BVH_PARSER_LINE_SIZE))
	{
		mLineNumber++;
		return TRUE;
	}
	return FALSE;
}
U32 LLBVHLoader::getOutputSize()
{
	LLDataPackerBinaryBuffer dp;
	serialize(dp);
	return dp.getCurrentSize();
}
BOOL LLBVHLoader::serialize(LLDataPacker& dp)
{
	JointVector::iterator ji;
	KeyVector::iterator ki;
	F32 time;
	S32 numJoints = 0;
	for (ji=mJoints.begin(); ji!=mJoints.end(); ++ji)
	{
		Joint *joint = *ji;
		if ( ! joint->mIgnore )
			numJoints++;
	}
	dp.packU16(KEYFRAME_MOTION_VERSION, "version");
	dp.packU16(KEYFRAME_MOTION_SUBVERSION, "sub_version");
	dp.packS32(mPriority, "base_priority");
	dp.packF32(mDuration, "duration");
	dp.packString(mEmoteName, "emote_name");
	dp.packF32(mLoopInPoint, "loop_in_point");
	dp.packF32(mLoopOutPoint, "loop_out_point");
	dp.packS32(mLoop, "loop");
	dp.packF32(mEaseIn, "ease_in_duration");
	dp.packF32(mEaseOut, "ease_out_duration");
	dp.packU32(mHand, "hand_pose");
	dp.packU32(numJoints, "num_joints");
	for (	ji = mJoints.begin();
			ji != mJoints.end();
			++ji )
	{
		Joint *joint = *ji;
		if ( joint->mIgnore )
			continue;
		LLQuaternion first_frame_rot;
		LLQuaternion fixup_rot;
		dp.packString(joint->mOutName, "joint_name");
		dp.packS32(joint->mPriority, "joint_priority");
		LLQuaternion frameRot( joint->mFrameMatrix );
		LLQuaternion frameRotInv = ~frameRot;
		LLQuaternion offsetRot( joint->mOffsetMatrix );
		LLQuaternion mergeParentRot;
		LLQuaternion mergeChildRot;
		Joint *mergeParent = NULL;
		Joint *mergeChild = NULL;
		JointVector::iterator mji;
		for (mji=mJoints.begin(); mji!=mJoints.end(); ++mji)
		{
			Joint *mjoint = *mji;
			if ( !joint->mMergeParentName.empty() && (mjoint->mName == joint->mMergeParentName) )
			{
				mergeParent = *mji;
			}
			if ( !joint->mMergeChildName.empty() && (mjoint->mName == joint->mMergeChildName) )
			{
				mergeChild = *mji;
			}
		}
		dp.packS32(joint->mNumRotKeys, "num_rot_keys");
		LLQuaternion::Order order = bvhStringToOrder( joint->mOrder );
		S32 outcount = 0;
		S32 frame = 1;
		for (	ki = joint->mKeys.begin();
				ki != joint->mKeys.end();
				++ki )
		{
			if ((frame == 1) && joint->mRelativeRotationKey)
			{
				first_frame_rot = mayaQ( ki->mRot[0], ki->mRot[1], ki->mRot[2], order);
				fixup_rot.shortestArc(LLVector3::z_axis * first_frame_rot * frameRot, LLVector3::z_axis);
			}
			if (ki->mIgnoreRot)
			{
				frame++;
				continue;
			}
			time = (F32)frame * mFrameTime;
			if (mergeParent)
			{
				mergeParentRot = mayaQ(	mergeParent->mKeys[frame-1].mRot[0],
										mergeParent->mKeys[frame-1].mRot[1],
										mergeParent->mKeys[frame-1].mRot[2],
										bvhStringToOrder(mergeParent->mOrder) );
				LLQuaternion parentFrameRot( mergeParent->mFrameMatrix );
				LLQuaternion parentOffsetRot( mergeParent->mOffsetMatrix );
				mergeParentRot = ~parentFrameRot * mergeParentRot * parentFrameRot * parentOffsetRot;
			}
			else
			{
				mergeParentRot.loadIdentity();
			}
			if (mergeChild)
			{
				mergeChildRot = mayaQ(	mergeChild->mKeys[frame-1].mRot[0],
										mergeChild->mKeys[frame-1].mRot[1],
										mergeChild->mKeys[frame-1].mRot[2],
										bvhStringToOrder(mergeChild->mOrder) );
				LLQuaternion childFrameRot( mergeChild->mFrameMatrix );
				LLQuaternion childOffsetRot( mergeChild->mOffsetMatrix );
				mergeChildRot = ~childFrameRot * mergeChildRot * childFrameRot * childOffsetRot;
			}
			else
			{
				mergeChildRot.loadIdentity();
			}
			LLQuaternion inRot = mayaQ( ki->mRot[0], ki->mRot[1], ki->mRot[2], order);
			LLQuaternion outRot =  frameRotInv* mergeChildRot * inRot * mergeParentRot * ~first_frame_rot * frameRot * offsetRot;
			U16 time_short = F32_to_U16(time, 0.f, mDuration);
			dp.packU16(time_short, "time");
			U16 x, y, z;
			LLVector3 rot_vec = outRot.packToVector3();
			rot_vec.quantize16(-1.f, 1.f, -1.f, 1.f);
			x = F32_to_U16(rot_vec.mV[VX], -1.f, 1.f);
			y = F32_to_U16(rot_vec.mV[VY], -1.f, 1.f);
			z = F32_to_U16(rot_vec.mV[VZ], -1.f, 1.f);
			dp.packU16(x, "rot_angle_x");
			dp.packU16(y, "rot_angle_y");
			dp.packU16(z, "rot_angle_z");
			outcount++;
			frame++;
		}
		if ( ji == mJoints.begin() && !joint->mIgnorePositions )
		{
			dp.packS32(joint->mNumPosKeys, "num_pos_keys");
			LLVector3 relPos = joint->mRelativePosition;
			LLVector3 relKey;
			frame = 1;
			for (	ki = joint->mKeys.begin();
					ki != joint->mKeys.end();
					++ki )
			{
				if ((frame == 1) && joint->mRelativePositionKey)
				{
					relKey.setVec(ki->mPos);
				}
				if (ki->mIgnorePos)
				{
					frame++;
					continue;
				}
				time = (F32)frame * mFrameTime;
				LLVector3 inPos = (LLVector3(ki->mPos) - relKey) * ~first_frame_rot;
				LLVector3 outPos = inPos * frameRot * offsetRot;
				outPos *= INCHES_TO_METERS;
				outPos -= relPos;
				outPos.clamp(-LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				U16 time_short = F32_to_U16(time, 0.f, mDuration);
				dp.packU16(time_short, "time");
				U16 x, y, z;
				outPos.quantize16(-LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				x = F32_to_U16(outPos.mV[VX], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				y = F32_to_U16(outPos.mV[VY], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				z = F32_to_U16(outPos.mV[VZ], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				dp.packU16(x, "pos_x");
				dp.packU16(y, "pos_y");
				dp.packU16(z, "pos_z");
				frame++;
			}
		}
		else
		{
			dp.packS32(0, "num_pos_keys");
		}
	}
	S32 num_constraints = (S32)mConstraints.size();
	dp.packS32(num_constraints, "num_constraints");
	for (ConstraintVector::iterator constraint_it = mConstraints.begin();
		constraint_it != mConstraints.end();
		constraint_it++)
		{
			U8 byte = constraint_it->mChainLength;
			dp.packU8(byte, "chain_length");
			byte = constraint_it->mConstraintType;
			dp.packU8(byte, "constraint_type");
			dp.packBinaryDataFixed((U8*)constraint_it->mSourceJointName, 16, "source_volume");
			dp.packVector3(constraint_it->mSourceOffset, "source_offset");
			dp.packBinaryDataFixed((U8*)constraint_it->mTargetJointName, 16, "target_volume");
			dp.packVector3(constraint_it->mTargetOffset, "target_offset");
			dp.packVector3(constraint_it->mTargetDir, "target_dir");
			dp.packF32(constraint_it->mEaseInStart,	"ease_in_start");
			dp.packF32(constraint_it->mEaseInStop,	"ease_in_stop");
			dp.packF32(constraint_it->mEaseOutStart,	"ease_out_start");
			dp.packF32(constraint_it->mEaseOutStop,	"ease_out_stop");
		}
	return TRUE;
}
