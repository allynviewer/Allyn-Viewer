/** 
 * @file llgl.h
 * @brief LLGL definition
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
#ifndef LL_LLGL_H
#define LL_LLGL_H
#include <string>
#include <vector>
#include <list>
#include "llerror.h"
#include "v4color.h"
#include "llstring.h"
#include "stdtypes.h"
#include "v4math.h"
#include "llmatrix4a.h"
#include "llplane.h"
#include "llgltypes.h"
#include "llrender.h"
#include "llinstancetracker.h"
#include "llglheaders.h"
extern BOOL gDebugGL;
extern BOOL gDebugSession;
extern std::ofstream gFailLog;
#define LL_GL_ERRS LL_ERRS("RenderState")
void ll_init_fail_log(std::string filename);
void ll_fail(std::string msg);
void ll_close_fail_log();
class LLSD;
class LLGLManager
{
public:
	LLGLManager();
	bool initGL();
	void shutdownGL();
	void initWGL();
	std::string getRawGLString();
	BOOL mInited;
	BOOL mIsDisabled;
	BOOL mHasMultitexture;
	BOOL mHasATIMemInfo;
	BOOL mHasNVXMemInfo;
	S32	 mNumTextureUnits;
	BOOL mHasMipMapGeneration;
	BOOL mHasCompressedTextures;
	BOOL mHasFramebufferObject;
	S32 mMaxSamples;
	BOOL mHasFramebufferMultisample;
	BOOL mHasBlendFuncSeparate;
	BOOL mHasVertexBufferObject;
	BOOL mHasVertexArrayObject;
	BOOL mHasSync;
	BOOL mHasMapBufferRange;
	BOOL mHasFlushBufferRange;
	BOOL mHasShaderObjects;
	BOOL mHasVertexShader;
	BOOL mHasFragmentShader;
	S32  mNumTextureImageUnits;
	BOOL mHasOcclusionQuery;
	BOOL mHasOcclusionQuery2;
	BOOL mHasPointParameters;
	BOOL mHasDrawBuffers;
	BOOL mHasDepthClamp;
	BOOL mHasTransformFeedback;
	S32 mMaxIntegerSamples;
	BOOL mHasAnisotropic;
	BOOL mHasARBEnvCombine;
	BOOL mHasCubeMap;
	BOOL mHasDebugOutput;
	BOOL mHasGpuShader5;
	BOOL mHasAdaptiveVsync;
	BOOL mHasTextureSwizzle;
	bool mHasTextureCompression;
	BOOL mIsATI;
	BOOL mIsNVIDIA;
	BOOL mIsIntel;
	BOOL mIsHD3K;
	BOOL mIsGF2or4MX;
	BOOL mIsGF3;
	BOOL mIsGFFX;
	BOOL mATIOffsetVerticalLines;
	BOOL mATIOldDriver;
	BOOL mHasRequirements;
	BOOL mDebugGPU;
	S32 mDriverVersionMajor;
	S32 mDriverVersionMinor;
	S32 mDriverVersionRelease;
	F32 mGLVersion;
	S32 mGLSLVersionMajor;
	S32 mGLSLVersionMinor;
	std::string mDriverVersionVendorString;
	std::string mGLVersionString;
	S32 mVRAM;
	S32 mGLMaxVertexRange;
	S32 mGLMaxIndexRange;
	S32 mGLMaxVertexUniformComponents;
	void getPixelFormat();
	std::string getGLInfoString();
	void printGLInfoString();
	void getGLInfo(LLSD& info);
	std::string mGLVendor;
	std::string mGLVendorShort;
	std::string mGLRenderer;
private:
	void initExtensions();
	void initGLStates();
	void initGLImages();
	void setToDebugGPU();
};
extern LLGLManager gGLManager;
class LLQuaternion;
class LLMatrix4;
void rotate_quat(LLQuaternion& rotation);
void flush_glerror();
void log_glerror();
void assert_glerror();
void clear_glerror();
# define stop_glerror() assert_glerror()
# define llglassertok() assert_glerror()
#define llglassertok_always() assert_glerror()
struct LLGLStateStaticData
{
	const char* stateStr;
	LLGLenum state;
	bool currentState;
	U32 depth;
	char* activeInstance;
	bool* disabler;
};
class LLGLStateValidator {
public:
	static void resetTextureStates();
	static void dumpStates();
	static void checkStates(const std::string& msg = "");
	static void checkTextureChannels(const std::string& msg = "");
	static void checkClientArrays(const std::string& msg = "", U32 data_mask = 0);
	static void checkState(LLGLStateStaticData& data);
	static void initClass();
	static void restoreGL();
	static bool registerStateData(LLGLStateStaticData& data);
private:
	static std::vector<LLGLStateStaticData*> sStateDataVec;
};
class LLGLStateIface {
public:
	enum { CURRENT_STATE = -2 };
	virtual ~LLGLStateIface() {}
	virtual void enable() = 0;
	virtual void disable() = 0;
};
template <LLGLenum state>
class LLGLState : public LLGLStateIface
{
public:
	LLGLState(S8 newState = CURRENT_STATE)
	{
		++staticData.depth;
		mPriorInstance = staticData.activeInstance;
		staticData.activeInstance = (char*)this;
		mPriorState = staticData.currentState;
		setEnabled(newState);
	}
	virtual ~LLGLState()
	{
		llassert_always(staticData.activeInstance == (char*)this);
		if (staticData.depth != 0)
		{
			staticData.activeInstance = mPriorInstance;
			--staticData.depth;
			if (gDebugGL) {
				LLGLStateValidator::checkState(staticData);
			}
			setState(mPriorState);
		}
		else
		{
			llassert_always(mPriorInstance == nullptr);
		}
	}
	virtual void enable() { setEnabled(true); }
	virtual void disable() { setEnabled(false); }
	static LLGLStateStaticData staticData;
	static bool isEnabled() { return staticData.currentState && (!staticData.disabler || !*staticData.disabler); }
	static bool checkEnabled() { return (!staticData.disabler || !*staticData.disabler) ? staticData.currentState : true; }
	static bool checkDisabled() { return (!staticData.disabler || !*staticData.disabler) ? !staticData.currentState : true; }
private:
	char *mPriorInstance;
	bool mPriorState;
	void setEnabled(S32 newState)
	{
		llassert_always(staticData.activeInstance == (char*)this);
		bool enabled = newState == CURRENT_STATE ? staticData.currentState : !!newState;
		setState(enabled);
	}
	static void setState(bool enabled)
	{
		if (staticData.currentState != enabled && (!staticData.disabler || !*staticData.disabler))
		{
			gGL.flush();
			staticData.currentState = enabled;
			staticData.currentState ? glEnable(state) : glDisable(state);
		}
	}
};
#define initLLGLState(state, value, disabler_ptr) \
	template <> \
    LLGLStateStaticData LLGLState<state>::staticData = {#state, state, value, 0, nullptr, disabler_ptr}; \
	bool registered_##state = LLGLStateValidator::registerStateData(LLGLState<state>::staticData);
template <>
class LLGLState<0> : public LLGLStateIface
{
	LLGLState(S8 newState = CURRENT_STATE) { }
	virtual ~LLGLState() { }
	virtual void enable() { }
	virtual void disable() { }
	static bool isEnabled() { return false; }
	static bool checkEnabled() { return true; }
	static bool checkDisabled() { return true; }
};
template <LLGLenum state>
struct LLGLEnable : public LLGLState<state>
{
	LLGLEnable(bool noskip = true) : LLGLState<state>(noskip ? TRUE : LLGLState<state>::CURRENT_STATE) {}
};
template <LLGLenum state>
struct LLGLDisable : public LLGLState<state>
{
	LLGLDisable(bool noskip = true) : LLGLState<state>(noskip ? FALSE : LLGLState<state>::CURRENT_STATE) {}
};
LL_ALIGN_PREFIX(16)
class LLGLUserClipPlane
{
public:
	LLGLUserClipPlane(const LLPlane& plane, const LLMatrix4a& modelview, const LLMatrix4a& projection, bool apply = true);
	~LLGLUserClipPlane();
	void setPlane(F32 a, F32 b, F32 c, F32 d);
private:
	LL_ALIGN_16(LLMatrix4a mProjection);
	LL_ALIGN_16(LLMatrix4a mModelview);
	bool mApply;
} LL_ALIGN_POSTFIX(16);
class LLGLSquashToFarClip
{
public:
	LLGLSquashToFarClip(const LLMatrix4a& projection, U32 layer = 0);
	~LLGLSquashToFarClip();
};
class LLGLNamePool : public LLInstanceTracker<LLGLNamePool>
{
public:
	typedef LLInstanceTracker<LLGLNamePool> tracker_t;
	struct NameEntry
	{
		GLuint name;
		BOOL used;
	};
	struct CompareUsed
	{
		bool operator()(const NameEntry& lhs, const NameEntry& rhs)
		{
			return lhs.used < rhs.used;
		}
	};
	typedef std::vector<NameEntry> name_list_t;
	name_list_t mNameList;
	LLGLNamePool();
	virtual ~LLGLNamePool();
	void upkeep();
	void cleanup();
	GLuint allocate();
	void release(GLuint name);
	static void upkeepPools();
	static void cleanupPools();
protected:
	typedef std::vector<LLGLNamePool*> pool_list_t;
	virtual GLuint allocateName() = 0;
	virtual void releaseName(GLuint name) = 0;
};
class LLGLUpdate
{
public:
	static std::list<LLGLUpdate*> sGLQ;
	BOOL mInQ;
	LLGLUpdate()
		: mInQ(FALSE)
	{
	}
	virtual ~LLGLUpdate()
	{
		if (mInQ)
		{
			std::list<LLGLUpdate*>::iterator iter = std::find(sGLQ.begin(), sGLQ.end(), this);
			if (iter != sGLQ.end())
			{
				sGLQ.erase(iter);
			}
		}
	}
	virtual void updateGL() = 0;
};
const U32 FENCE_WAIT_TIME_NANOSECONDS = 1000;
class LLGLFence
{
public:
	virtual ~LLGLFence()
	{
	}
	virtual void placeFence() = 0;
	virtual bool isCompleted() = 0;
	virtual void wait() = 0;
};
class LLGLSyncFence : public LLGLFence
{
public:
#ifdef GL_ARB_sync
	GLsync mSync;
#endif
	LLGLSyncFence();
	virtual ~LLGLSyncFence();
	void placeFence();
	bool isCompleted();
	void wait();
};
#include "llglstates.h"
void parse_gl_version( S32* major, S32* minor, S32* release, std::string* vendor_specific, std::string* version_string );
extern BOOL gClothRipple;
extern BOOL gNoRender;
extern BOOL gGLActive;
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT GL_DEPTH_ATTACHMENT_EXT
#endif
#ifndef GL_STENCIL_ATTACHMENT
#define GL_STENCIL_ATTACHMENT GL_STENCIL_ATTACHMENT_EXT
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER GL_FRAMEBUFFER_EXT
#define GL_DRAW_FRAMEBUFFER GL_DRAW_FRAMEBUFFER_EXT
#define GL_READ_FRAMEBUFFER GL_READ_FRAMEBUFFER_EXT
#define GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE_EXT
#define GL_FRAMEBUFFER_UNSUPPORTED GL_FRAMEBUFFER_UNSUPPORTED_EXT
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT
#define glGenFramebuffers glGenFramebuffersEXT
#define glBindFramebuffer glBindFramebufferEXT
#define glCheckFramebufferStatus glCheckFramebufferStatusEXT
#define glBlitFramebuffer glBlitFramebufferEXT
#define glDeleteFramebuffers glDeleteFramebuffersEXT
#define glFramebufferRenderbuffer glFramebufferRenderbufferEXT
#define glFramebufferTexture2D glFramebufferTexture2DEXT
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER GL_RENDERBUFFER_EXT
#define glGenRenderbuffers glGenRenderbuffersEXT
#define glBindRenderbuffer glBindRenderbufferEXT
#define glRenderbufferStorage glRenderbufferStorageEXT
#define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleEXT
#define glDeleteRenderbuffers glDeleteRenderbuffersEXT
#endif
#ifndef GL_COLOR_ATTACHMENT
#define GL_COLOR_ATTACHMENT GL_COLOR_ATTACHMENT_EXT
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0_EXT
#endif
#ifndef GL_COLOR_ATTACHMENT1
#define GL_COLOR_ATTACHMENT1 GL_COLOR_ATTACHMENT1_EXT
#endif
#ifndef GL_COLOR_ATTACHMENT2
#define GL_COLOR_ATTACHMENT2 GL_COLOR_ATTACHMENT2_EXT
#endif
#ifndef GL_COLOR_ATTACHMENT3
#define GL_COLOR_ATTACHMENT3 GL_COLOR_ATTACHMENT3_EXT
#endif
#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_EXT
#endif
#endif
