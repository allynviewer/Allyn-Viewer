/** 
 * @file llpostprocess.h
 * @brief LLPostProcess class definition
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
#ifndef LL_POSTPROCESS_H
#define LL_POSTPROCESS_H
#include <map>
#include <boost/signals2.hpp>
#include "llsd.h"
#include "llrendertarget.h"
class LLSD;
class LLGLSLShader;
typedef enum _QuadType {
	QUAD_NORMAL,
	QUAD_NOISE
} QuadType;
class IPostProcessShader
{
protected:
	struct IShaderSettingBase
	{
		virtual ~IShaderSettingBase() {}
		virtual const std::string& getName() const = 0;
		virtual LLSD getDefaultValue() const = 0;
		virtual void setValue(const LLSD& value) = 0;
	};
public:
	virtual ~IPostProcessShader() {}
	virtual bool isEnabled()		const = 0;
	virtual S32 getColorChannel()	const = 0;
	virtual S32 getDepthChannel()	const = 0;
	virtual void bindShader() = 0;
	virtual void unbindShader() = 0;
	virtual LLGLSLShader& getShader() = 0;
	virtual QuadType preDraw() = 0;
	virtual bool draw(U32 pass) = 0;
	virtual void postDraw() = 0;
	virtual LLSD getDefaults() = 0;
	virtual void loadSettings(const LLSD& settings) = 0;
	virtual void addSetting(IShaderSettingBase& setting) = 0;
};
class LLPostProcessShader;
class LLPostProcess : public LLSingleton<LLPostProcess>
{
private:
	std::list<LLPointer<LLPostProcessShader> > mShaders;
	LLPointer<LLVertexBuffer> mVBO;
	U32 mNextDrawTarget;
	LLRenderTarget mRenderTarget[2];
	LLImageGL::GLTextureName mDepthTexture;
	LLImageGL::GLTextureName mNoiseTexture ;
	U32 mScreenWidth;
	U32 mScreenHeight;
	F32 mNoiseTextureScale;
	std::string mSelectedEffectName;
	LLSD mSelectedEffectInfo;
	LLSD mAllEffectInfo;
	typedef boost::signals2::signal<void(const std::string&)> selected_effect_changed_signal;
	selected_effect_changed_signal mSelectedEffectChanged;
public:
	LLPostProcess(void);
	~LLPostProcess(void);
private:
	void initialize(unsigned int width, unsigned int height);
	void createScreenTextures();
	void createNoiseTexture();
public:
	void destroyGL();
	static void cleanupClass();
	void copyFrameBuffer();
	void bindNoise(U32 channel);
	void renderEffects(unsigned int width, unsigned int height);
private:
	void doEffects(void);
	void applyShaders(void);
	void drawOrthoQuad(QuadType type);
public:
	LLVector2 getDimensions() { return LLVector2(mScreenWidth,mScreenHeight); }
	inline LLSD const & getAllEffectInfo(void) const				{ return mAllEffectInfo; }
	inline std::string const & getSelectedEffectName(void) const	{ return mSelectedEffectName; }
	inline LLSD const & getSelectedEffectInfo(void) const			{ return mSelectedEffectInfo; }
	void setSelectedEffect(std::string const & effectName);
	void setSelectedEffectValue(std::string const & setting, LLSD value);
	auto setSelectedEffectChangeCallback(const selected_effect_changed_signal::slot_type& func) { return mSelectedEffectChanged.connect(func); }
	void resetSelectedEffect();
	void saveEffectAs(std::string const & effectName);
};
#endif
