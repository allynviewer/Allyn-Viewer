/** 
 * @file LLAvatarJointMesh.cpp
 * @brief Implementation of LLAvatarJointMesh class
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
#include "imageids.h"
#include "llfasttimer.h"
#include "llrender.h"
#include "llavatarjointmesh.h"
#include "llavatarappearance.h"
#include "lltexlayer.h"
#include "llmath.h"
#include "v4math.h"
#include "m3math.h"
#include "m4math.h"
#include "llmatrix4a.h"
LLJoint *getBaseSkeletonAncestor(LLJoint* joint)
{
    LLJoint *ancestor = joint->getParent();
    while (ancestor->getParent() && (ancestor->getSupport() != LLJoint::SUPPORT_BASE))
    {
        LL_DEBUGS("Avatar") << "skipping non-base ancestor " << ancestor->getName() << LL_ENDL;
        ancestor = ancestor->getParent();
    }
    return ancestor;
}
LLVector3 totalSkinOffset(LLJoint *joint)
{
    LLVector3 totalOffset;
    while (joint)
    {
		if (joint->getSupport() == LLJoint::SUPPORT_BASE)
		{
			totalOffset += joint->getSkinOffset();
		}
		joint = joint->getParent();
    }
    return totalOffset;
}
LLSkinJoint::LLSkinJoint()
{
	mJoint       = NULL;
}
LLSkinJoint::~LLSkinJoint()
{
	mJoint = NULL;
}
BOOL LLSkinJoint::setupSkinJoint( LLJoint *joint)
{
	if (!(mJoint = dynamic_cast<LLAvatarJoint*>(joint)))
	{
		LL_INFOS() << "Can't find joint" << LL_ENDL;
		return FALSE;
	}
	mRootToJointSkinOffset = totalSkinOffset(mJoint);
    mRootToJointSkinOffset = -mRootToJointSkinOffset;
	mRootToParentJointSkinOffset = totalSkinOffset(getBaseSkeletonAncestor(mJoint));
	mRootToParentJointSkinOffset = -mRootToParentJointSkinOffset;
	return TRUE;
}
BOOL LLAvatarJointMesh::sPipelineRender = FALSE;
EAvatarRenderPass LLAvatarJointMesh::sRenderPass = AVATAR_RENDER_PASS_SINGLE;
U32 LLAvatarJointMesh::sClothingMaskImageName = 0;
LLColor4 LLAvatarJointMesh::sClothingInnerColor;
LLAvatarJointMesh::LLAvatarJointMesh()
	:
	mTexture( NULL ),
	mLayerSet( NULL ),
	mTestImageName( 0 ),
	mFaceIndexCount(0)
{
	mColor[0] = 1.0f;
	mColor[1] = 1.0f;
	mColor[2] = 1.0f;
	mColor[3] = 1.0f;
	mShiny = 0.0f;
	mCullBackFaces = TRUE;
	mMesh = NULL;
	mNumSkinJoints = 0;
	mSkinJoints = NULL;
	mFace = NULL;
	mMeshID = 0;
	mUpdateXform = FALSE;
	mValid = FALSE;
	mIsTransparent = FALSE;
}
LLAvatarJointMesh::~LLAvatarJointMesh()
{
	mMesh = NULL;
	mTexture = NULL;
	freeSkinData();
}
BOOL LLAvatarJointMesh::allocateSkinData( U32 numSkinJoints )
{
	mSkinJoints = new LLSkinJoint[ numSkinJoints ];
	mNumSkinJoints = numSkinJoints;
	return TRUE;
}
void LLAvatarJointMesh::freeSkinData()
{
	mNumSkinJoints = 0;
	delete [] mSkinJoints;
	mSkinJoints = NULL;
}
void LLAvatarJointMesh::getColor( F32 *red, F32 *green, F32 *blue, F32 *alpha )
{
	*red   = mColor[0];
	*green = mColor[1];
	*blue  = mColor[2];
	*alpha = mColor[3];
}
void LLAvatarJointMesh::setColor( F32 red, F32 green, F32 blue, F32 alpha )
{
	mColor[0] = red;
	mColor[1] = green;
	mColor[2] = blue;
	mColor[3] = alpha;
}
void LLAvatarJointMesh::setColor( const LLColor4& color )
{
	mColor = color;
}
void LLAvatarJointMesh::setTexture( LLGLTexture *texture )
{
	mTexture = texture;
	if( texture )
	{
		mLayerSet = NULL;
	}
}
BOOL LLAvatarJointMesh::hasGLTexture() const
{
	return mTexture.notNull() && mTexture->hasGLTexture();
}
void LLAvatarJointMesh::setLayerSet( LLTexLayerSet* layer_set )
{
	mLayerSet = layer_set;
	if( layer_set )
	{
		mTexture = NULL;
	}
}
BOOL LLAvatarJointMesh::hasComposite() const
{
	return (mLayerSet && mLayerSet->hasComposite());
}
LLPolyMesh *LLAvatarJointMesh::getMesh()
{
	return mMesh;
}
void LLAvatarJointMesh::setMesh( LLPolyMesh *mesh )
{
	mMesh = mesh;
	freeSkinData();
	if ( mMesh == NULL )
	{
		return;
	}
	setPosition( mMesh->getPosition() );
	setRotation( mMesh->getRotation() );
	setScale( mMesh->getScale() );
	if ( mMesh->hasWeights() && !mMesh->isLOD())
	{
		U32 numJointNames = mMesh->getNumJointNames();
		allocateSkinData( numJointNames );
		std::string *jointNames = mMesh->getJointNames();
		U32 jn;
		for (jn = 0; jn < numJointNames; jn++)
		{
			mSkinJoints[jn].setupSkinJoint( getRoot()->findJoint(jointNames[jn]) );
		}
	}
	if (!mMesh->isLOD())
	{
		setupJoint(getRoot());
		LL_DEBUGS("Avatar") << getName() << " joint render entries: " << mMesh->mJointRenderData.size() << LL_ENDL;
	}
}
void LLAvatarJointMesh::setupJoint(LLJoint* current_joint)
{
	U32 sj;
	for (sj=0; sj<mNumSkinJoints; sj++)
	{
		LLSkinJoint &js = mSkinJoints[sj];
		if (js.mJoint != current_joint)
		{
			continue;
		}
        LL_DEBUGS("Avatar") << "Mesh: " << getName() << " joint " << current_joint->getName() << " matches skinjoint " << sj << LL_ENDL;
        std::vector<LLJointRenderData*>	&jrd = mMesh->mJointRenderData;
        LLJoint *ancestor = getBaseSkeletonAncestor(current_joint);
		if(jrd.size() && jrd.back()->mWorldMatrix == &ancestor->getWorldMatrix())
		{
			LLJoint* jointp = js.mJoint;
			jrd.push_back(new LLJointRenderData(&jointp->getWorldMatrix(), &js));
			LL_DEBUGS("Avatar") << "add joint[" << (jrd.size()-1) << "] = " << js.mJoint->getName() << LL_ENDL;
		}
		else
		{
			jrd.push_back(new LLJointRenderData(&ancestor->getWorldMatrix(), NULL));
			LL_DEBUGS("Avatar") << "add2 ancestor joint[" << (jrd.size()-1) << "] = " << ancestor->getName() << LL_ENDL;
			jrd.push_back(new LLJointRenderData(&current_joint->getWorldMatrix(), &js));
            LL_DEBUGS("Avatar") << "add2 joint[" << (jrd.size()-1) << "] = " << current_joint->getName() << LL_ENDL;
		}
	}
	for (LLJoint::child_list_t::iterator iter = current_joint->mChildren.begin();
		 iter != current_joint->mChildren.end(); ++iter)
	{
		LLAvatarJoint* child_joint = dynamic_cast<LLAvatarJoint*>(*iter);
		if(child_joint)
			setupJoint(child_joint);
	}
}
