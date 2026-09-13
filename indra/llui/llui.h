/** 
 * @file llui.h
 * @brief General static UI services.
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
#ifndef LL_LLUI_H
#define LL_LLUI_H
#include "llrect.h"
#include "llcontrol.h"
#include "llcoord.h"
#include "v2math.h"
#include "llinitparam.h"
#include "llrender2dutils.h"
#include "llpointer.h"
#include "lluicolor.h"
#include "lluiimage.h"
#include <boost/signals2.hpp>
#include "llfontgl.h"
#include "llsd.h"
class LLHtmlHelp;
class LLUUID;
class LLWindow;
class LLView;
void make_ui_sound(const char* name);
extern BOOL gShowTextEditCursor;
class LLImageProviderInterface;
typedef	void (*LLUIAudioCallback)(const LLUUID& uuid);
class LLUI
{
	LOG_CLASS(LLUI);
public:
	static void initClass(LLControlGroup* config,
						  LLControlGroup* account,
						  LLControlGroup* ignores,
						  LLControlGroup* colors,
						  LLImageProviderInterface* image_provider,
						  LLUIAudioCallback audio_callback = NULL,
						  const LLVector2 *scale_factor = NULL,
						  const std::string& language = LLStringUtil::null);
	static void cleanupClass();
	static void pushMatrix() { LLRender2D::pushMatrix(); }
	static void popMatrix() { LLRender2D::popMatrix(); }
	static void loadIdentity() { LLRender2D::loadIdentity(); }
	static void translate(F32 x, F32 y, F32 z = 0.0f) { LLRender2D::translate(x, y, z); }
	static std::string getLanguage();
	static LLView* getRootView() { return sRootView; }
	static void setRootView(LLView* view) { sRootView = view; }
	static std::string locateSkin(const std::string& filename);
	static void setMousePositionScreen(S32 x, S32 y);
	static void getMousePositionScreen(S32 *x, S32 *y);
	static void setMousePositionLocal(const LLView* viewp, S32 x, S32 y);
	static void getMousePositionLocal(const LLView* viewp, S32 *x, S32 *y);
	static LLVector2& getScaleFactor() { return LLRender2D::sGLScaleFactor; }
	static void setScaleFactor(const LLVector2& scale_factor) { LLRender2D::setScaleFactor(scale_factor); }
	static void setLineWidth(F32 width) { LLRender2D::setLineWidth(width); }
	static LLPointer<LLUIImage> getUIImageByID(const LLUUID& image_id, S32 priority = 0)
		{ return LLRender2D::getUIImageByID(image_id, priority); }
	static LLPointer<LLUIImage> getUIImage(const std::string& name, S32 priority = 0)
		{ return LLRender2D::getUIImage(name, priority); }
	static LLVector2 getWindowSize();
	static void screenPointToGL(S32 screen_x, S32 screen_y, S32 *gl_x, S32 *gl_y);
	static void glPointToScreen(S32 gl_x, S32 gl_y, S32 *screen_x, S32 *screen_y);
	static void screenRectToGL(const LLRect& screen, LLRect *gl);
	static void glRectToScreen(const LLRect& gl, LLRect *screen);
	static LLControlGroup& getControlControlGroup (const std::string& controlname);
	static LLWindow* getWindow() { return sWindow; }
	static void setHtmlHelp(LLHtmlHelp* html_help);
	static LLControlGroup* sConfigGroup;
	static LLControlGroup* sAccountGroup;
	static LLControlGroup* sIgnoresGroup;
	static LLControlGroup* sColorsGroup;
	static LLUIAudioCallback sAudioCallback;
	static LLWindow*		sWindow;
	static LLView*			sRootView;
	static BOOL             sShowXUINames;
	static LLHtmlHelp*		sHtmlHelp;
	static void setQAMode(BOOL b);
	static BOOL sQAMode;
};
template <class T>
class FactoryPolicy
{
public:
	static T* findInstance(const LLSD& key);
	static T* createInstance(const LLSD& key);
};
template <class T>
class VisibilityPolicy
{
public:
	static bool visible(T* instance, const LLSD& key);
	static void show(T* instance, const LLSD& key);
	static void hide(T* instance, const LLSD& key);
};
template <class T, class FACTORY_POLICY = FactoryPolicy<T>, class VISIBILITY_POLICY = VisibilityPolicy<T> >
class LLUIFactory
{
public:
	typedef FACTORY_POLICY factory_policy_t;
	typedef VISIBILITY_POLICY visibility_policy_t;
	LLUIFactory()
	{
	}
	virtual ~LLUIFactory()
	{
	}
	static T* showInstance(const LLSD& key = LLSD())
	{
		T* instance = getInstance(key);
		if (instance != NULL)
		{
			VISIBILITY_POLICY::show(instance, key);
		}
		return instance;
	}
	static void hideInstance(const LLSD& key = LLSD())
	{
		T* instance = getInstance(key);
		if (instance != NULL)
		{
			VISIBILITY_POLICY::hide(instance, key);
		}
	}
	static void toggleInstance(const LLSD& key = LLSD())
	{
		if (instanceVisible(key))
		{
			hideInstance(key);
		}
		else
		{
			showInstance(key);
		}
	}
	static bool instanceVisible(const LLSD& key = LLSD())
	{
		T* instance = FACTORY_POLICY::findInstance(key);
		return instance != NULL && VISIBILITY_POLICY::visible(instance, key);
	}
	static T* getInstance(const LLSD& key = LLSD())
	{
		T* instance = FACTORY_POLICY::findInstance(key);
		if (instance == NULL)
		{
			instance = FACTORY_POLICY::createInstance(key);
		}
		return instance;
	}
};
template <class T, class VISIBILITY_POLICY = VisibilityPolicy<T> >
class LLUISingleton: public LLUIFactory<T, LLUISingleton<T, VISIBILITY_POLICY>, VISIBILITY_POLICY>
{
protected:
	LLUISingleton() { sInstance = static_cast<T*>(this); }
	~LLUISingleton() { sInstance = NULL; }
public:
	static T* findInstance(const LLSD& key = LLSD())
	{
		return sInstance;
	}
	static T* createInstance(const LLSD& key = LLSD())
	{
		if (sInstance == NULL)
		{
			sInstance = new T(key);
		}
		return sInstance;
	}
private:
	static T*	sInstance;
};
template <class T, class U> T* LLUISingleton<T,U>::sInstance = NULL;
class LLCallbackRegistry
{
public:
	typedef boost::signals2::signal<void()> callback_signal_t;
	void registerCallback(const callback_signal_t::slot_type& slot)
	{
		mCallbacks.connect(slot);
	}
	void fireCallbacks()
	{
		mCallbacks();
	}
private:
	callback_signal_t mCallbacks;
};
class LLInitClassList :
	public LLCallbackRegistry,
	public LLSingleton<LLInitClassList>
{
	friend class LLSingleton<LLInitClassList>;
private:
	LLInitClassList() {}
};
class LLDestroyClassList :
	public LLCallbackRegistry,
	public LLSingleton<LLDestroyClassList>
{
	friend class LLSingleton<LLDestroyClassList>;
private:
	LLDestroyClassList() {}
};
template<typename T>
class LLRegisterWith
{
public:
	LLRegisterWith(boost::function<void ()> func)
	{
		T::instance().registerCallback(func);
	}
	void reference()
	{
#if LL_WINDOWS
		S32 dummy;
		dummy = 0;
#endif
	}
};
template<typename T>
class LLInitClass
{
public:
	LLInitClass() { sRegister.reference(); }
	static LLRegisterWith<LLInitClassList> sRegister;
private:
	static void initClass()
	{
		LL_ERRS() << "No static initClass() method defined for " << typeid(T).name() << LL_ENDL;
	}
};
template<typename T>
class LLDestroyClass
{
public:
	LLDestroyClass() { sRegister.reference(); }
	static LLRegisterWith<LLDestroyClassList> sRegister;
private:
	static void destroyClass()
	{
		LL_ERRS() << "No static destroyClass() method defined for " << typeid(T).name() << LL_ENDL;
	}
};
template <typename T> LLRegisterWith<LLInitClassList> LLInitClass<T>::sRegister(&T::initClass);
template <typename T> LLRegisterWith<LLDestroyClassList> LLDestroyClass<T>::sRegister(&T::destroyClass);
template <class T>
class LLUICachedControl : public LLCachedControl<T>
{
public:
	LLUICachedControl(const std::string& name,
					  const T& default_value,
					  const std::string& comment = "Declared In Code")
	:	LLCachedControl<T>(LLUI::getControlControlGroup(name), name, default_value, comment)
	{}
	LLUICachedControl(const std::string& name)
	:	LLCachedControl<T>(LLUI::getControlControlGroup(name), name)
	{}
};
template <typename DERIVED>
class LLParamBlock
{
protected:
	LLParamBlock() { sBlock = (DERIVED*)this; }
	typedef typename boost::add_const<DERIVED>::type Tconst;
	template <typename T>
	class LLMandatoryParam
	{
	public:
		typedef typename boost::add_const<T>::type T_const;
		LLMandatoryParam(T_const initial_val) : mVal(initial_val), mBlock(sBlock) {}
		LLMandatoryParam(const LLMandatoryParam<T>& other) : mVal(other.mVal) {}
		DERIVED& operator ()(T_const set_value) { mVal = set_value; return *mBlock; }
		operator T() const { return mVal; }
		T operator=(T_const set_value) { mVal = set_value; return mVal; }
	private:
		T	mVal;
		DERIVED* mBlock;
	};
	template <typename T>
	class LLOptionalParam
	{
	public:
		typedef typename boost::add_const<T>::type T_const;
		LLOptionalParam(T_const initial_val) : mVal(initial_val), mBlock(sBlock) {}
		LLOptionalParam() : mBlock(sBlock) {}
		LLOptionalParam(const LLOptionalParam<T>& other) : mVal(other.mVal) {}
		DERIVED& operator ()(T_const set_value) { mVal = set_value; return *mBlock; }
		operator T() const { return mVal; }
		T operator=(T_const set_value) { mVal = set_value; return mVal; }
	private:
		T	mVal;
		DERIVED* mBlock;
	};
	template <typename T>
	class LLOptionalParam <T&>
	{
	public:
		typedef typename boost::add_const<T&>::type T_const;
		LLOptionalParam(T_const initial_val) : mVal(initial_val), mBlock(sBlock) {}
		LLOptionalParam(const LLOptionalParam<T&>& other) : mVal(other.mVal) {}
		DERIVED& operator ()(T_const set_value) { mVal = set_value; return *mBlock; }
		operator T&() const { return mVal; }
		T& operator=(T_const set_value) { mVal = set_value; return mVal; }
	private:
		T&	mVal;
		DERIVED* mBlock;
	};
	template<typename T>
	class LLOptionalParam<T*>
	{
	public:
		typedef typename boost::add_const<T*>::type T_const;
		LLOptionalParam(T_const initial_val) : mVal(initial_val), mBlock(sBlock) {}
		LLOptionalParam() : mVal((T*)NULL), mBlock(sBlock)  {}
		LLOptionalParam(const LLOptionalParam<T*>& other) : mVal(other.mVal) {}
		DERIVED& operator ()(T_const set_value) { mVal = set_value; return *mBlock; }
		operator T*() const { return mVal; }
		T* operator=(T_const set_value) { mVal = set_value; return mVal; }
	private:
		T*	mVal;
		DERIVED* mBlock;
	};
	static DERIVED* sBlock;
};
template <typename T> T* LLParamBlock<T>::sBlock = NULL;
namespace LLInitParam
{
	template<>
	class ParamValue<LLRect>
	:	public CustomParamValue<LLRect>
	{
		typedef CustomParamValue<LLRect> super_t;
	public:
		Optional<S32>	left,
						top,
						right,
						bottom,
						width,
						height;
		ParamValue(const LLRect& value);
		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};
	template<>
	class ParamValue<LLUIColor>
	:	public CustomParamValue<LLUIColor>
	{
		typedef CustomParamValue<LLUIColor> super_t;
	public:
		Optional<F32>			red,
								green,
								blue,
								alpha;
		Optional<std::string>	control;
		ParamValue(const LLUIColor& color);
		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};
	template<>
	class ParamValue<const LLFontGL*>
	:	public CustomParamValue<const LLFontGL* >
	{
		typedef CustomParamValue<const LLFontGL*> super_t;
	public:
		Optional<std::string>	name,
								size,
								style;
		ParamValue(const LLFontGL* value);
		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};
	template<>
	struct TypeValues<LLFontGL::HAlign> : public TypeValuesHelper<LLFontGL::HAlign>
	{
		static void declareValues();
	};
	template<>
	struct TypeValues<LLFontGL::VAlign> : public TypeValuesHelper<LLFontGL::VAlign>
	{
		static void declareValues();
	};
	template<>
	struct TypeValues<LLFontGL::ShadowType> : public TypeValuesHelper<LLFontGL::ShadowType>
	{
		static void declareValues();
	};
	template<>
	struct ParamCompare<const LLFontGL*, false>
	{
		static bool equals(const LLFontGL* a, const LLFontGL* b);
	};
	template<>
	class ParamValue<LLCoordGL>
	:	public CustomParamValue<LLCoordGL>
	{
		typedef CustomParamValue<LLCoordGL> super_t;
	public:
		Optional<S32>	x,
						y;
		ParamValue(const LLCoordGL& val);
		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};
}
#endif
