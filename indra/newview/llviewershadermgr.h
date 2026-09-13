/** 
 * @file llviewershadermgr.h
 * @brief Viewer Shader Manager
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
#ifndef LL_VIEWER_SHADER_MGR_H
#define LL_VIEWER_SHADER_MGR_H
#include "llshadermgr.h"
#include "llmaterial.h"
#define LL_DEFERRED_MULTI_LIGHT_COUNT 16
enum
{
	SHD_ALPHA_MASK_BIT = 0,
	SHD_WATER_BIT,
	SHD_NO_INDEX_BIT,
	SHD_SKIN_BIT,
	SHD_SHINY_BIT,
	SHD_COUNT
};
class LLViewerShaderMgr: public LLShaderMgr
{
public:
	static BOOL sInitialized;
	static bool sSkipReload;
	LLViewerShaderMgr();
	~LLViewerShaderMgr();
	static LLViewerShaderMgr * instance();
	void initAttribsAndUniforms(void);
	void setShaders();
	void unloadShaders();
	S32 getVertexShaderLevel(S32 type);
	BOOL loadBasicShaders();
	BOOL loadShadersEffects();
	BOOL loadShadersDeferred();
	BOOL loadShadersObject();
	BOOL loadShadersAvatar();
	BOOL loadShadersEnvironment();
	BOOL loadShadersWater();
	BOOL loadShadersInterface();
	BOOL loadShadersWindLight();
	BOOL loadTransformShaders();
	std::vector<S32> mVertexShaderLevel;
	enum EShaderClass
	{
		SHADER_LIGHTING,
		SHADER_OBJECT,
		SHADER_AVATAR,
		SHADER_ENVIRONMENT,
		SHADER_INTERFACE,
		SHADER_EFFECT,
		SHADER_WINDLIGHT,
		SHADER_WATER,
		SHADER_DEFERRED,
		SHADER_TRANSFORM,
		SHADER_COUNT
	};
	class shader_iter
	{
	private:
		friend bool operator == (shader_iter const & a, shader_iter const & b);
		friend bool operator != (shader_iter const & a, shader_iter const & b);
		typedef std::vector<LLGLSLShader *>::const_iterator base_iter_t;
	public:
		shader_iter()
		{
		}
		shader_iter(base_iter_t iter) : mIter(iter)
		{
		}
		LLGLSLShader & operator * () const
		{
			return **mIter;
		}
		LLGLSLShader * operator -> () const
		{
			return *mIter;
		}
		shader_iter & operator++ ()
		{
			++mIter;
			return *this;
		}
		shader_iter operator++ (int)
		{
			return mIter++;
		}
	private:
		base_iter_t mIter;
	};
	shader_iter beginShaders() const
	{
		return (shader_iter)(getGlobalShaderList().begin());
	}
	shader_iter endShaders() const
	{
		return (shader_iter)(getGlobalShaderList().end());
	}
	std::string getShaderDirPrefix(void);
	void updateShaderUniforms(LLGLSLShader * shader);
	bool attachClassSharedShaders(LLGLSLShader& shader, S32 shader_class);
private:
	std::vector<std::string> mShinyUniforms;
	std::vector<std::string> mWaterUniforms;
	std::vector<std::string> mWLUniforms;
	std::vector<std::string> mTerrainUniforms;
	std::vector<std::string> mGlowUniforms;
	std::vector<std::string> mGlowExtractUniforms;
	std::vector<std::string> mAvatarUniforms;
};
inline bool operator == (LLViewerShaderMgr::shader_iter const & a, LLViewerShaderMgr::shader_iter const & b)
{
	return a.mIter == b.mIter;
}
inline bool operator != (LLViewerShaderMgr::shader_iter const & a, LLViewerShaderMgr::shader_iter const & b)
{
	return a.mIter != b.mIter;
}
extern LLVector4			gShinyOrigin;
template <LLViewerShaderMgr::EShaderClass type>
class LLGLSLShaderArray : public LLGLSLShader
{
public:
	LLGLSLShaderArray() : LLGLSLShader(type)	{};
};
extern LLGLSLShader			gTransformPositionProgram;
extern LLGLSLShader			gTransformTexCoordProgram;
extern LLGLSLShader			gTransformNormalProgram;
extern LLGLSLShader			gTransformColorProgram;
extern LLGLSLShader			gTransformTangentProgram;
extern LLGLSLShader			gOcclusionProgram;
extern LLGLSLShader			gOcclusionCubeProgram;
extern LLGLSLShader			gCustomAlphaProgram;
extern LLGLSLShader			gGlowCombineProgram;
extern LLGLSLShader			gSplatTextureRectProgram;
extern LLGLSLShader			gGlowCombineFXAAProgram;
extern LLGLSLShader			gDebugProgram;
extern LLGLSLShader			gClipProgram;
extern LLGLSLShader			gDownsampleDepthProgram;
extern LLGLSLShader			gTwoTextureAddProgram;
extern LLGLSLShader			gOneTextureNoColorProgram;
extern LLGLSLShaderArray<LLViewerShaderMgr::SHADER_OBJECT>	gObjectSimpleProgram[1<<SHD_COUNT];
extern LLGLSLShaderArray<LLViewerShaderMgr::SHADER_OBJECT>	gObjectFullbrightProgram[1<<SHD_COUNT];
extern LLGLSLShaderArray<LLViewerShaderMgr::SHADER_OBJECT>	gObjectEmissiveProgram[1<<SHD_SHINY_BIT];
extern LLGLSLShader			gObjectPreviewProgram;
extern LLGLSLShader			gObjectSimpleNonIndexedTexGenProgram;
extern LLGLSLShader			gObjectSimpleNonIndexedTexGenWaterProgram;
extern LLGLSLShader			gObjectAlphaMaskNoColorProgram;
extern LLGLSLShader			gObjectAlphaMaskNoColorWaterProgram;
extern LLGLSLShader			gObjectFullbrightNoColorProgram;
extern LLGLSLShader			gObjectFullbrightNoColorWaterProgram;
extern LLGLSLShader			gObjectBumpProgram;
extern LLGLSLShader			gTreeProgram;
extern LLGLSLShader			gTreeWaterProgram;
extern LLGLSLShader			gObjectSimpleLODProgram;
extern LLGLSLShader			gObjectFullbrightLODProgram;
extern LLGLSLShader			gTerrainProgram;
extern LLGLSLShader			gTerrainWaterProgram;
extern LLGLSLShader			gWaterProgram;
extern LLGLSLShader			gUnderWaterProgram;
extern LLGLSLShader			gGlowProgram;
extern LLGLSLShader			gGlowExtractProgram;
extern LLGLSLShader			gHighlightProgram;
extern LLGLSLShader			gHighlightNormalProgram;
extern LLGLSLShader			gHighlightSpecularProgram;
extern LLGLSLShader			gAvatarProgram;
extern LLGLSLShader			gAvatarWaterProgram;
extern LLGLSLShader			gImpostorProgram;
extern LLGLSLShader			gWLSkyProgram;
extern LLGLSLShader			gWLCloudProgram;
extern LLGLSLShader			gPostColorFilterProgram;
extern LLGLSLShader			gPostNightVisionProgram;
extern LLGLSLShader			gPostGaussianBlurProgram;
extern LLGLSLShader			gDeferredImpostorProgram;
extern LLGLSLShader			gDeferredWaterProgram;
extern LLGLSLShader			gDeferredUnderWaterProgram;
extern LLGLSLShader			gDeferredDiffuseProgram;
extern LLGLSLShader			gDeferredDiffuseAlphaMaskProgram;
extern LLGLSLShader			gDeferredNonIndexedDiffuseAlphaMaskProgram;
extern LLGLSLShader			gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram;
extern LLGLSLShader			gDeferredSkinnedDiffuseProgram;
extern LLGLSLShader			gDeferredSkinnedBumpProgram;
extern LLGLSLShader			gDeferredSkinnedAlphaProgram;
extern LLGLSLShader			gDeferredBumpProgram;
extern LLGLSLShader			gDeferredTerrainProgram;
extern LLGLSLShader			gDeferredTreeProgram;
extern LLGLSLShader			gDeferredTreeShadowProgram;
extern LLGLSLShader			gDeferredLightProgram;
extern LLGLSLShaderArray<LLViewerShaderMgr::SHADER_DEFERRED>			gDeferredMultiLightProgram[LL_DEFERRED_MULTI_LIGHT_COUNT];
extern LLGLSLShader			gDeferredSpotLightProgram;
extern LLGLSLShader			gDeferredMultiSpotLightProgram;
extern LLGLSLShader			gDeferredSunProgram;
extern LLGLSLShader			gDeferredSSAOProgram;
extern LLGLSLShader			gDeferredDownsampleDepthNearestProgram;
extern LLGLSLShader			gDeferredBlurLightProgram;
extern LLGLSLShader			gDeferredAvatarProgram;
extern LLGLSLShader			gDeferredSoftenProgram;
extern LLGLSLShader			gDeferredSoftenWaterProgram;
extern LLGLSLShader			gDeferredShadowProgram;
extern LLGLSLShader			gDeferredShadowCubeProgram;
extern LLGLSLShader			gDeferredShadowAlphaMaskProgram;
extern LLGLSLShader			gDeferredPostProgram;
extern LLGLSLShader			gDeferredCoFProgram;
extern LLGLSLShader			gDeferredDoFCombineProgram;
extern LLGLSLShader			gFXAAProgram;
extern LLGLSLShader			gDeferredPostNoDoFProgram;
extern LLGLSLShader			gDeferredPostGammaCorrectProgram;
extern LLGLSLShader			gDeferredAvatarShadowProgram;
extern LLGLSLShader			gDeferredAttachmentShadowProgram;
extern LLGLSLShader			gDeferredAlphaProgram;
extern LLGLSLShader			gDeferredAlphaImpostorProgram;
extern LLGLSLShader			gDeferredFullbrightProgram;
extern LLGLSLShader			gDeferredFullbrightAlphaMaskProgram;
extern LLGLSLShader			gDeferredAlphaWaterProgram;
extern LLGLSLShader			gDeferredFullbrightWaterProgram;
extern LLGLSLShader			gDeferredFullbrightAlphaMaskWaterProgram;
extern LLGLSLShader			gDeferredEmissiveProgram;
extern LLGLSLShader			gDeferredAvatarAlphaProgram;
extern LLGLSLShader			gDeferredWLSkyProgram;
extern LLGLSLShader			gDeferredWLCloudProgram;
extern LLGLSLShader			gDeferredStarProgram;
extern LLGLSLShader			gDeferredFullbrightShinyProgram;
extern LLGLSLShader			gDeferredSkinnedFullbrightShinyProgram;
extern LLGLSLShader			gDeferredSkinnedFullbrightProgram;
extern LLGLSLShader			gNormalMapGenProgram;
extern LLGLSLShaderArray<LLViewerShaderMgr::SHADER_DEFERRED>			gDeferredMaterialProgram[LLMaterial::SHADER_COUNT*2];
extern LLGLSLShaderArray<LLViewerShaderMgr::SHADER_DEFERRED>			gDeferredMaterialWaterProgram[LLMaterial::SHADER_COUNT*2];
#endif
