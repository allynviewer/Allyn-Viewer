/** 
 * @file llmultisldr.cpp
 * @brief LLMultiSlider base class
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
#include "linden_common.h"
#include "llmultislider.h"
#include "llui.h"
#include "llgl.h"
#include "llwindow.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"
#include "llcontrol.h"
#include "lluiimage.h"
#include <sstream>
static LLRegisterWidget<LLMultiSlider> r("multi_slider_bar");
const S32 MULTI_THUMB_WIDTH = 8;
const S32 MULTI_TRACK_HEIGHT = 6;
const F32 FLOAT_THRESHOLD = 0.00001f;
const S32 EXTRA_TRIANGLE_WIDTH = 2;
const S32 EXTRA_TRIANGLE_HEIGHT = -2;
S32 LLMultiSlider::mNameCounter = 0;
LLMultiSlider::LLMultiSlider(
	const std::string& name,
	const LLRect& rect,
	commit_callback_t commit_callback,
	F32 initial_value,
	F32 min_value,
	F32 max_value,
	F32 increment,
	S32 max_sliders,
	BOOL allow_overlap,
	BOOL draw_track,
	BOOL use_triangle,
	const std::string& control_name)
	:
	LLUICtrl( name, rect, TRUE,	commit_callback, FOLLOWS_LEFT | FOLLOWS_TOP),
	mInitialValue( initial_value ),
	mMinValue( min_value ),
	mMaxValue( max_value ),
	mIncrement( increment ),
	mMaxNumSliders(max_sliders),
	mAllowOverlap(allow_overlap),
	mDrawTrack(draw_track),
	mUseTriangle(use_triangle),
	mMouseOffset( 0 ),
	mDragStartThumbRect( 0, getRect().getHeight(), MULTI_THUMB_WIDTH, 0 ),
	mTrackColor(		LLUI::sColorsGroup->getColor( "MultiSliderTrackColor" ) ),
	mThumbOutlineColor(	LLUI::sColorsGroup->getColor( "MultiSliderThumbOutlineColor" ) ),
	mThumbCenterColor(	LLUI::sColorsGroup->getColor( "MultiSliderThumbCenterColor" ) ),
	mThumbCenterSelectedColor(	LLUI::sColorsGroup->getColor( "MultiSliderThumbCenterSelectedColor" ) ),
	mDisabledThumbColor(LLUI::sColorsGroup->getColor( "MultiSliderDisabledThumbColor" ) ),
	mTriangleColor(LLUI::sColorsGroup->getColor( "MultiSliderTriangleColor" ) ),
	mMouseDownSignal( NULL ),
	mMouseUpSignal( NULL ),
	mThumbWidth(MULTI_THUMB_WIDTH)
{
	mValue.emptyMap();
	mCurSlider = LLStringUtil::null;
	setControlName(control_name, NULL);
	setValue(getValue());
}
LLMultiSlider::~LLMultiSlider()
{
	delete mMouseDownSignal;
	delete mMouseUpSignal;
}
void LLMultiSlider::setSliderValue(const std::string& name, F32 value, BOOL from_event)
{
	if(!mValue.has(name)) {
		return;
	}
	value = llclamp( value, mMinValue, mMaxValue );
	value -= mMinValue;
	value += mIncrement/2.0001f;
	value -= fmod(value, mIncrement);
	F32 newValue = mMinValue + value;
	if(!mAllowOverlap) {
		bool hit = false;
		LLSD::map_iterator mIt = mValue.beginMap();
		for(;mIt != mValue.endMap(); mIt++) {
			F32 testVal = (F32)mIt->second.asReal() - newValue;
			if(testVal > -FLOAT_THRESHOLD && testVal < FLOAT_THRESHOLD &&
				mIt->first != name) {
				hit = true;
				break;
			}
		}
		if(hit) {
			return;
		}
	}
	mValue[name] = newValue;
	if (!from_event && name == mCurSlider)
	{
		setControlValue(mValue);
	}
	F32 t = (newValue - mMinValue) / (mMaxValue - mMinValue);
	S32 left_edge = mThumbWidth/2;
	S32 right_edge = getRect().getWidth() - (mThumbWidth/2);
	S32 x = left_edge + S32( t * (right_edge - left_edge) );
	mThumbRects[name].mLeft = x - (mThumbWidth/2);
	mThumbRects[name].mRight = x + (mThumbWidth/2);
}
void LLMultiSlider::setValue(const LLSD& value)
{
	if(value.isMap()) {
		LLSD::map_const_iterator mIt = value.beginMap();
		mCurSlider = mIt->first;
		for(; mIt != value.endMap(); mIt++) {
			setSliderValue(mIt->first, (F32)mIt->second.asReal(), TRUE);
		}
	}
}
F32 LLMultiSlider::getSliderValue(const std::string& name) const
{
	return (F32)mValue[name].asReal();
}
void LLMultiSlider::setCurSlider(const std::string& name)
{
	if(mValue.has(name)) {
		mCurSlider = name;
	}
}
const std::string& LLMultiSlider::addSlider()
{
	return addSlider(mInitialValue);
}
const std::string& LLMultiSlider::addSlider(F32 val)
{
	std::stringstream newName;
	F32 initVal = val;
	if(mValue.size() >= mMaxNumSliders) {
		return LLStringUtil::null;
	}
	newName << "sldr" << mNameCounter;
	mNameCounter++;
	bool foundOne = findUnusedValue(initVal);
	if(!foundOne) {
		return LLStringUtil::null;
	}
	mThumbRects[newName.str()] = LLRect( 0, getRect().getHeight(), mThumbWidth, 0 );
	mValue.insert(newName.str(), initVal);
	mCurSlider = newName.str();
	setSliderValue(mCurSlider, initVal, TRUE);
	return mCurSlider;
}
void LLMultiSlider::addSlider(F32 val, const std::string& name)
{
	F32 initVal = val;
	if(mValue.size() >= mMaxNumSliders) {
		return;
	}
	bool foundOne = findUnusedValue(initVal);
	if(!foundOne) {
		return;
	}
	mThumbRects[name] = LLRect( 0, getRect().getHeight(), mThumbWidth, 0 );
	mValue.insert(name, initVal);
	mCurSlider = name;
	setSliderValue(mCurSlider, initVal, TRUE);
}
bool LLMultiSlider::findUnusedValue(F32& initVal)
{
	bool firstTry = true;
	while(true) {
		bool hit = false;
		LLSD::map_iterator mIt = mValue.beginMap();
		for(;mIt != mValue.endMap(); mIt++) {
			F32 testVal = (F32)mIt->second.asReal() - initVal;
			if(testVal > -FLOAT_THRESHOLD && testVal < FLOAT_THRESHOLD) {
				hit = true;
				break;
			}
		}
		if(!hit) {
			break;
		}
		initVal += mIncrement;
		if(initVal > mMaxValue) {
			initVal = mMinValue;
		}
		if(initVal == mInitialValue && !firstTry) {
			LL_WARNS() << "Whoa! Too many multi slider elements to add one to" << LL_ENDL;
			return false;
		}
		firstTry = false;
		continue;
	}
	return true;
}
void LLMultiSlider::deleteSlider(const std::string& name)
{
	if(mValue.size() <= 0) {
		return;
	}
	mValue.erase(name);
	mThumbRects.erase(name);
	if(mValue.size() > 0) {
		std::map<std::string, LLRect>::iterator mIt = mThumbRects.end();
		mIt--;
		mCurSlider = mIt->first;
	}
}
void LLMultiSlider::clear()
{
	while(mThumbRects.size() > 0) {
		deleteCurSlider();
	}
	LLUICtrl::clear();
}
BOOL LLMultiSlider::handleHover(S32 x, S32 y, MASK mask)
{
	if( gFocusMgr.getMouseCapture() == this )
	{
		S32 left_edge = mThumbWidth/2;
		S32 right_edge = getRect().getWidth() - (mThumbWidth/2);
		x += mMouseOffset;
		x = llclamp( x, left_edge, right_edge );
		F32 t = F32(x - left_edge) / (right_edge - left_edge);
		setCurSliderValue(t * (mMaxValue - mMinValue) + mMinValue );
		onCommit();
		getWindow()->setCursor(UI_CURSOR_ARROW);
		LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (active)" << LL_ENDL;
	}
	else
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
		LL_DEBUGS("UserInput") << "hover handled by " << getName() << " (inactive)" << LL_ENDL;
	}
	return TRUE;
}
BOOL LLMultiSlider::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	if( gFocusMgr.getMouseCapture() == this )
	{
		gFocusMgr.setMouseCapture( NULL );
		if (mMouseUpSignal)
			(*mMouseUpSignal)( this, LLSD() );
		handled = TRUE;
		make_ui_sound("UISndClickRelease");
	}
	else
	{
		handled = TRUE;
	}
	return handled;
}
BOOL LLMultiSlider::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (!getIsChrome())
	{
		setFocus(TRUE);
	}
	if (mMouseDownSignal)
		(*mMouseDownSignal)( this, LLSD() );
	if (MASK_CONTROL & mask)
	{
		setCurSliderValue(mInitialValue);
		onCommit();
	}
	else
	{
		std::map<std::string, LLRect>::iterator mIt = mThumbRects.begin();
		for(; mIt != mThumbRects.end(); mIt++) {
			if(mIt->second.pointInRect(x,y)) {
				mCurSlider = mIt->first;
				break;
			}
		}
		if (mThumbRects[mCurSlider].pointInRect(x,y))
		{
			mMouseOffset = (mThumbRects[mCurSlider].mLeft + mThumbWidth/2) - x;
		}
		else
		{
			mMouseOffset = 0;
		}
		gFocusMgr.setMouseCapture( this );
		mDragStartThumbRect = mThumbRects[mCurSlider];
	}
	make_ui_sound("UISndClick");
	return TRUE;
}
BOOL	LLMultiSlider::handleKeyHere(KEY key, MASK mask)
{
	BOOL handled = FALSE;
	switch(key)
	{
	case KEY_UP:
	case KEY_DOWN:
		handled = TRUE;
		break;
	case KEY_LEFT:
		setCurSliderValue(getCurSliderValue() - getIncrement());
		onCommit();
		handled = TRUE;
		break;
	case KEY_RIGHT:
		setCurSliderValue(getCurSliderValue() + getIncrement());
		onCommit();
		handled = TRUE;
		break;
	default:
		break;
	}
	return handled;
}
void LLMultiSlider::draw()
{
	LLColor4 curThumbColor;
	std::map<std::string, LLRect>::iterator mIt;
	std::map<std::string, LLRect>::iterator curSldrIt;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLRect rect(mDragStartThumbRect);
	F32 opacity = getEnabled() ? 1.f : 0.3f;
	LLUIImagePtr thumb_imagep = LLUI::getUIImage("Rounded_Square");
	S32 height_offset = (getRect().getHeight() - MULTI_TRACK_HEIGHT) / 2;
	LLRect track_rect(0, getRect().getHeight() - height_offset, getRect().getWidth(), height_offset );
	if(mDrawTrack)
	{
		track_rect.stretch(-1);
		thumb_imagep->draw(track_rect, mTrackColor % opacity);
	}
	if(mUseTriangle) {
		for(mIt = mThumbRects.begin(); mIt != mThumbRects.end(); mIt++) {
			gl_triangle_2d(
				mIt->second.mLeft - EXTRA_TRIANGLE_WIDTH,
				mIt->second.mTop + EXTRA_TRIANGLE_HEIGHT,
				mIt->second.mRight + EXTRA_TRIANGLE_WIDTH,
				mIt->second.mTop + EXTRA_TRIANGLE_HEIGHT,
				mIt->second.mLeft + mIt->second.getWidth() / 2,
				mIt->second.mBottom - EXTRA_TRIANGLE_HEIGHT,
				mTriangleColor, TRUE);
		}
	}
	else if (!thumb_imagep)
	{
		curSldrIt = mThumbRects.end();
		for(mIt = mThumbRects.begin(); mIt != mThumbRects.end(); mIt++) {
			curThumbColor = mThumbCenterColor;
			if(mIt->first == mCurSlider) {
				curSldrIt = mIt;
				continue;
			}
			gl_rect_2d(mIt->second, curThumbColor, TRUE);
		}
		if(curSldrIt != mThumbRects.end()) {
			gl_rect_2d(curSldrIt->second, mThumbCenterSelectedColor, TRUE);
		}
		if (gFocusMgr.getMouseCapture() == this)
		{
			gl_rect_2d(mDragStartThumbRect, mThumbCenterColor % opacity, FALSE);
		}
	}
	else if( gFocusMgr.getMouseCapture() == this )
	{
		thumb_imagep->drawSolid(mDragStartThumbRect, mThumbCenterColor % 0.3f);
		if (hasFocus())
		{
			thumb_imagep->drawBorder(mThumbRects[mCurSlider], gFocusMgr.getFocusColor(), gFocusMgr.getFocusFlashWidth());
		}
		curSldrIt = mThumbRects.end();
		for(mIt = mThumbRects.begin(); mIt != mThumbRects.end(); mIt++)
		{
			curThumbColor = mThumbCenterColor;
			if(mIt->first == mCurSlider)
			{
				curSldrIt = mIt;
				continue;
			}
			thumb_imagep->drawSolid(mIt->second, curThumbColor);
		}
		if(curSldrIt != mThumbRects.end())
		{
			thumb_imagep->drawSolid(curSldrIt->second, mThumbCenterSelectedColor);
		}
	}
	else
	{
		if (hasFocus())
		{
			thumb_imagep->drawBorder(mThumbRects[mCurSlider], gFocusMgr.getFocusColor(), gFocusMgr.getFocusFlashWidth());
		}
		curSldrIt = mThumbRects.end();
		for(mIt = mThumbRects.begin(); mIt != mThumbRects.end(); mIt++)
		{
			curThumbColor = mThumbCenterColor;
			if(mIt->first == mCurSlider)
			{
				curSldrIt = mIt;
				continue;
			}
			thumb_imagep->drawSolid(mIt->second, curThumbColor % opacity);
		}
		if(curSldrIt != mThumbRects.end())
		{
			thumb_imagep->drawSolid(curSldrIt->second, mThumbCenterSelectedColor % opacity);
		}
	}
	LLUICtrl::draw();
}
boost::signals2::connection LLMultiSlider::setMouseDownCallback( const commit_signal_t::slot_type& cb )
{
	if (!mMouseDownSignal) mMouseDownSignal = new commit_signal_t();
	return mMouseDownSignal->connect(cb);
}
boost::signals2::connection LLMultiSlider::setMouseUpCallback(	const commit_signal_t::slot_type& cb )
{
	if (!mMouseUpSignal) mMouseUpSignal = new commit_signal_t();
	return mMouseUpSignal->connect(cb);
}
LLXMLNodePtr LLMultiSlider::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();
	node->setName(LL_MULTI_SLIDER_TAG);
	node->createChild("initial_val", TRUE)->setFloatValue(getInitialValue());
	node->createChild("min_val", TRUE)->setFloatValue(getMinValue());
	node->createChild("max_val", TRUE)->setFloatValue(getMaxValue());
	node->createChild("increment", TRUE)->setFloatValue(getIncrement());
	return node;
}
LLView* LLMultiSlider::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLRect rect;
	createRect(node, rect, parent, LLRect());
	F32 initial_value = 0.f;
	node->getAttributeF32("initial_val", initial_value);
	F32 min_value = 0.f;
	node->getAttributeF32("min_val", min_value);
	F32 max_value = 1.f;
	node->getAttributeF32("max_val", max_value);
	F32 increment = 0.1f;
	node->getAttributeF32("increment", increment);
	S32 max_sliders = 1;
	node->getAttributeS32("max_sliders", max_sliders);
	BOOL allow_overlap = FALSE;
	node->getAttributeBOOL("allow_overlap", allow_overlap);
	BOOL draw_track = TRUE;
	node->getAttributeBOOL("draw_track", draw_track);
	BOOL use_triangle = FALSE;
	node->getAttributeBOOL("use_triangle", use_triangle);
	LLMultiSlider* multiSlider = new LLMultiSlider("multi_slider_bar",
							rect,
							NULL,
							initial_value,
							min_value,
							max_value,
							increment,
							max_sliders,
							allow_overlap,
							draw_track,
							use_triangle);
	multiSlider->initFromXML(node, parent);
	return multiSlider;
}
