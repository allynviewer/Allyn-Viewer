/** 
 * @file llfloatercolorpicker.cpp
 * @brief Generic system color picker
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"
#include <sstream>
#include <iomanip>
#include "llfloatercolorpicker.h"
#include "llfontgl.h"
#include "llsys.h"
#include "llgl.h"
#include "llrender.h"
#include "v3dmath.h"
#include "lldir.h"
#include "llui.h"
#include "lllineeditor.h"
#include "v4coloru.h"
#include "llbutton.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llgl.h"
#include "llmemory.h"
#include "llimage.h"
#include "llmousehandler.h"
#include "llimagegl.h"
#include "llglheaders.h"
#include "llcheckboxctrl.h"
#include "llworld.h"
#include "lltextbox.h"
#include "lluiconstants.h"
#include "llfocusmgr.h"
#include "lltoolmgr.h"
#include "lltoolpipette.h"
#include "lldraghandle.h"
const F32 CONTEXT_CONE_IN_ALPHA = 0.0f;
const F32 CONTEXT_CONE_OUT_ALPHA = 1.f;
const F32 CONTEXT_FADE_TIME = 0.08f;
LLFloaterColorPicker::LLFloaterColorPicker (LLColorSwatchCtrl* swatch, BOOL show_apply_immediate )
	: LLFloater (std::string("Color Picker Floater")),
	  origR(1.f),
	  origG(1.f),
	  origB(1.f),
	  curR(1.f),
	  curG(1.f),
	  curB(1.f),
	  curH(0.f),
	  curS(0.f),
	  curL(1.f),
	  mComponents			( 3 ),
	  mMouseDownInLumRegion	( FALSE ),
	  mMouseDownInHueRegion	( FALSE ),
	  mMouseDownInSwatch	( FALSE ),
	  mRGBViewerImageLeft	( 140 ),
	  mRGBViewerImageTop	( 356 ),
	  mRGBViewerImageWidth	( 256 ),
	  mRGBViewerImageHeight ( 256 ),
	  mLumRegionLeft		( mRGBViewerImageLeft + mRGBViewerImageWidth + 16 ),
	  mLumRegionTop			( mRGBViewerImageTop ),
	  mLumRegionWidth		( 16 ),
	  mLumRegionHeight		( mRGBViewerImageHeight ),
	  mLumMarkerSize		( 6 ),
	  mSwatchRegionLeft		( 12 ),
	  mSwatchRegionTop		( 170 ),
	  mSwatchRegionWidth	( 116 ),
	  mSwatchRegionHeight	( 60 ),
	  mSwatchView			( NULL ),
	  numPaletteColumns		( 16 ),
	  numPaletteRows		( 2 ),
	  highlightEntry		( -1 ),
	  mPaletteRegionLeft	( 11 ),
	  mPaletteRegionTop		( 92 ),
	  mPaletteRegionWidth	( mLumRegionLeft + mLumRegionWidth - 10 ),
	  mPaletteRegionHeight	( 40 ),
	  mSwatch				( swatch ),
	  mActive				( TRUE ),
	  mApplyImmediateCheck(nullptr),
	  mCanApplyImmediately	( show_apply_immediate ),
	  mSelectBtn(nullptr),
	  mCancelBtn(nullptr),
	  mPipetteBtn(nullptr),
	  mContextConeOpacity	( 0.f )
{
	LLUICtrlFactory::getInstance()->buildFloater ( this, "floater_color_picker.xml" );
	setVisible ( FALSE );
	createUI ();
	if (!mCanApplyImmediately)
	{
		mApplyImmediateCheck->setEnabled(FALSE);
		mApplyImmediateCheck->set(FALSE);
	}
}
LLFloaterColorPicker::~LLFloaterColorPicker()
{
	destroyUI ();
}
void LLFloaterColorPicker::createUI ()
{
	LLPointer<LLImageRaw> raw = new LLImageRaw ( mRGBViewerImageWidth, mRGBViewerImageHeight, mComponents );
	U8* bits = raw->getData();
	S32 linesize = mRGBViewerImageWidth * mComponents;
	for ( S32 y = 0; y < mRGBViewerImageHeight; ++y )
	{
		for ( S32 x = 0; x < linesize; x += mComponents )
		{
			F32 rVal, gVal, bVal;
			hslToRgb ( (F32)x / (F32) ( linesize - 1 ),
					   (F32)y / (F32) ( mRGBViewerImageHeight - 1 ),
					   0.5f,
					   rVal,
					   gVal,
					   bVal );
			* ( bits + x + y * linesize + 0 ) = ( U8 )( rVal * 255.0f );
			* ( bits + x + y * linesize + 1 ) = ( U8 )( gVal * 255.0f );
			* ( bits + x + y * linesize + 2 ) = ( U8 )( bVal * 255.0f );
		}
	}
	mRGBImage = LLViewerTextureManager::getLocalTexture( (LLImageRaw*)raw, FALSE );
	gGL.getTexUnit(0)->bind(mRGBImage);
	mRGBImage->setAddressMode(LLTexUnit::TAM_CLAMP);
	for ( S32 each = 0; each < numPaletteColumns * numPaletteRows; ++each )
	{
		std::ostringstream codec;
		codec << "ColorPaletteEntry" << std::setfill ( '0' ) << std::setw ( 2 ) << each + 1;
		const std::string s ( codec.str () );
		mPalette.push_back ( new LLColor4 ( gSavedSettings.getColor4 ( s )  ) );
	}
}
void LLFloaterColorPicker::showUI ()
{
	open();
	setVisible ( TRUE );
	setFocus ( TRUE );
	if ( gSavedSettings.getBOOL ( "UseDefaultColorPicker" ) )
	{
		LLColorSwatchCtrl* swatch = getSwatch ();
		setVisible ( FALSE );
		if ( swatch )
		{
			LLColor4 curCol = swatch->get ();
			send_agent_pause();
			gViewerWindow->getWindow()->dialogColorPicker( &curCol [ 0 ], &curCol [ 1 ], &curCol [ 2 ] );
			send_agent_resume();
			setOrigRgb ( curCol [ 0 ], curCol [ 1 ], curCol [ 2 ] );
			setCurRgb( curCol [ 0 ], curCol [ 1 ], curCol [ 2 ] );
			LLColorSwatchCtrl::onColorChanged ( swatch, LLColorSwatchCtrl::COLOR_CHANGE );
		}
		close();
	}
}
BOOL LLFloaterColorPicker::postBuild()
{
	mCancelBtn = getChild<LLButton>( "cancel_btn" );
    mCancelBtn->setClickedCallback ( onClickCancel, this );
	mSelectBtn = getChild<LLButton>( "select_btn");
    mSelectBtn->setClickedCallback ( onClickSelect, this );
	mSelectBtn->setFocus ( TRUE );
	mPipetteBtn = getChild<LLButton>("color_pipette" );
	mPipetteBtn->setImages(std::string("eye_button_inactive.tga"), std::string("eye_button_active.tga"));
	mPipetteBtn->setClickedCallback( boost::bind(&LLFloaterColorPicker::onClickPipette,this ));
	mApplyImmediateCheck = getChild<LLCheckBoxCtrl>("apply_immediate");
	mApplyImmediateCheck->set(gSavedSettings.getBOOL("ApplyColorImmediately"));
	mApplyImmediateCheck->setCommitCallback(onImmediateCheck, this);
	childSetCommitCallback("rspin", onTextCommit, (void*)this );
	childSetCommitCallback("gspin", onTextCommit, (void*)this );
	childSetCommitCallback("bspin", onTextCommit, (void*)this );
	childSetCommitCallback("hspin", onTextCommit, (void*)this );
	childSetCommitCallback("sspin", onTextCommit, (void*)this );
	childSetCommitCallback("lspin", onTextCommit, (void*)this );
	getChild<LLUICtrl>("hexval")->setCommitCallback(boost::bind(&LLFloaterColorPicker::onHexCommit, this, _2) );
	LLToolPipette::getInstance()->setToolSelectCallback(boost::bind(&LLFloaterColorPicker::onColorSelect, this, _1));
    return TRUE;
}
void LLFloaterColorPicker::initUI ( F32 rValIn, F32 gValIn, F32 bValIn )
{
	rValIn = llclamp ( rValIn, 0.0f, 1.0f );
	gValIn = llclamp ( gValIn, 0.0f, 1.0f );
	bValIn = llclamp ( bValIn, 0.0f, 1.0f );
	setOrigRgb ( rValIn, gValIn, bValIn );
	setCurRgb ( rValIn, gValIn, bValIn );
	updateTextEntry ();
}
void LLFloaterColorPicker::destroyUI ()
{
	stopUsingPipette();
	std::vector < LLColor4* >::iterator iter = mPalette.begin ();
	while ( iter != mPalette.end () )
	{
		delete ( *iter );
		++iter;
	}
	if ( mSwatchView )
	{
		this->removeChild ( mSwatchView );
		mSwatchView->die();;
		mSwatchView = NULL;
	}
}
F32 LLFloaterColorPicker::hueToRgb ( F32 val1In, F32 val2In, F32 valHUeIn )
{
	if ( valHUeIn < 0.0f ) valHUeIn += 1.0f;
	if ( valHUeIn > 1.0f ) valHUeIn -= 1.0f;
	if ( ( 6.0f * valHUeIn ) < 1.0f ) return ( val1In + ( val2In - val1In ) * 6.0f * valHUeIn );
	if ( ( 2.0f * valHUeIn ) < 1.0f ) return ( val2In );
	if ( ( 3.0f * valHUeIn ) < 2.0f ) return ( val1In + ( val2In - val1In ) * ( ( 2.0f / 3.0f ) - valHUeIn ) * 6.0f );
	return ( val1In );
}
void LLFloaterColorPicker::hslToRgb ( F32 hValIn, F32 sValIn, F32 lValIn, F32& rValOut, F32& gValOut, F32& bValOut )
{
	if ( sValIn < 0.00001f )
	{
		rValOut = lValIn;
		gValOut = lValIn;
		bValOut = lValIn;
	}
	else
	{
		F32 interVal1;
		F32 interVal2;
		if ( lValIn < 0.5f )
			interVal2 = lValIn * ( 1.0f + sValIn );
		else
			interVal2 = ( lValIn + sValIn ) - ( sValIn * lValIn );
		interVal1 = 2.0f * lValIn - interVal2;
		rValOut = hueToRgb ( interVal1, interVal2, hValIn + ( 1.f / 3.f ) );
		gValOut = hueToRgb ( interVal1, interVal2, hValIn );
		bValOut = hueToRgb ( interVal1, interVal2, hValIn - ( 1.f / 3.f ) );
	}
}
void LLFloaterColorPicker::setOrigRgb ( F32 origRIn, F32 origGIn, F32 origBIn )
{
	origR = origRIn;
	origG = origGIn;
	origB = origBIn;
}
void LLFloaterColorPicker::getOrigRgb ( F32& origROut, F32& origGOut, F32& origBOut )
{
	origROut = origR;
	origGOut = origG;
	origBOut = origB;
}
void LLFloaterColorPicker::setCurRgb ( F32 curRIn, F32 curGIn, F32 curBIn )
{
	curR = curRIn;
	curG = curGIn;
	curB = curBIn;
	LLColor3(curRIn, curGIn, curBIn).calcHSL(&curH, &curS, &curL);
    updateTextEntry();
}
void LLFloaterColorPicker::getCurRgb ( F32& curROut, F32& curGOut, F32& curBOut )
{
	curROut = curR;
	curGOut = curG;
	curBOut = curB;
}
void LLFloaterColorPicker::setCurHsl ( F32 curHIn, F32 curSIn, F32 curLIn )
{
	curH = curHIn;
	curS = curSIn;
	curL = curLIn;
	hslToRgb ( curH, curS, curL, curR, curG, curB );
}
void LLFloaterColorPicker::getCurHsl ( F32& curHOut, F32& curSOut, F32& curLOut )
{
	curHOut = curH;
	curSOut = curS;
	curLOut = curL;
}
void LLFloaterColorPicker::onClickCancel ( void* data )
{
	if (data)
	{
		LLFloaterColorPicker* self = ( LLFloaterColorPicker* )data;
		if ( self )
		{
			self->cancelSelection ();
			self->close();
		}
	}
}
void LLFloaterColorPicker::onClickSelect ( void* data )
{
	if (data)
	{
		LLFloaterColorPicker* self = ( LLFloaterColorPicker* )data;
		if ( self )
		{
			LLColorSwatchCtrl::onColorChanged ( self->getSwatch (), LLColorSwatchCtrl::COLOR_SELECT );
			self->close();
		}
	}
}
void LLFloaterColorPicker::onClickPipette()
{
	BOOL pipette_active = mPipetteBtn->getToggleState();
	pipette_active = !pipette_active;
	if (pipette_active)
	{
		LLToolMgr::getInstance()->setTransientTool(LLToolPipette::getInstance());
	}
	else
	{
		LLToolMgr::getInstance()->clearTransientTool();
	}
}
void LLFloaterColorPicker::onTextCommit ( LLUICtrl* ctrl, void* data )
{
	if ( data )
	{
		LLFloaterColorPicker* self = ( LLFloaterColorPicker* )data;
		if ( self )
		{
			self->onTextEntryChanged ( ctrl );
		}
	}
}
void LLFloaterColorPicker::onImmediateCheck( LLUICtrl* ctrl, void* data)
{
	LLFloaterColorPicker* self = ( LLFloaterColorPicker* )data;
	if (self)
	{
		gSavedSettings.setBOOL("ApplyColorImmediately", self->mApplyImmediateCheck->get());
		if (self->mApplyImmediateCheck->get())
		{
			LLColorSwatchCtrl::onColorChanged ( self->getSwatch (), LLColorSwatchCtrl::COLOR_CHANGE );
		}
	}
}
void LLFloaterColorPicker::onColorSelect( const LLTextureEntry& te)
{
	setCurRgb(te.getColor().mV[VRED], te.getColor().mV[VGREEN], te.getColor().mV[VBLUE]);
	if (mApplyImmediateCheck->get())
	{
		LLColorSwatchCtrl::onColorChanged ( getSwatch (), LLColorSwatchCtrl::COLOR_CHANGE );
	}
}
void LLFloaterColorPicker::onMouseCaptureLost()
{
	setMouseDownInHueRegion(FALSE);
	setMouseDownInLumRegion(FALSE);
}
void LLFloaterColorPicker::draw()
{
	LLRect swatch_rect;
	mSwatch->localRectToOtherView(mSwatch->getLocalRect(), &swatch_rect, this);
	LLRect local_rect = getLocalRect();
	if (hasFocus() && mSwatch->isInVisibleChain() && mContextConeOpacity > 0.001f)
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		LLGLEnable<GL_CULL_FACE> cull;
		gGL.begin(LLRender::TRIANGLE_STRIP);
		{
			gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			gGL.vertex2i(local_rect.mLeft, local_rect.mTop);
			gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			gGL.vertex2i(swatch_rect.mLeft, swatch_rect.mTop);
			gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			gGL.vertex2i(local_rect.mRight, local_rect.mTop);
			gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			gGL.vertex2i(swatch_rect.mRight, swatch_rect.mTop);
			gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			gGL.vertex2i(local_rect.mRight, local_rect.mBottom);
			gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			gGL.vertex2i(swatch_rect.mRight, swatch_rect.mBottom);
			gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			gGL.vertex2i(local_rect.mLeft, local_rect.mBottom);
			gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			gGL.vertex2i(swatch_rect.mLeft, swatch_rect.mBottom);
			gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			gGL.vertex2i(local_rect.mLeft, local_rect.mTop);
			gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			gGL.vertex2i(swatch_rect.mLeft, swatch_rect.mTop);
		}
		gGL.end();
	}
	if (gFocusMgr.childHasMouseCapture(getDragHandle()))
	{
		mContextConeOpacity = lerp(mContextConeOpacity, gSavedSettings.getF32("PickerContextOpacity"), LLSmoothInterpolation::getInterpolant(CONTEXT_FADE_TIME));
	}
	else
	{
		mContextConeOpacity = lerp(mContextConeOpacity, 0.f, LLSmoothInterpolation::getInterpolant(CONTEXT_FADE_TIME));
	}
	mPipetteBtn->setToggleState(LLToolMgr::getInstance()->getCurrentTool() == LLToolPipette::getInstance());
	mApplyImmediateCheck->setEnabled(mActive && mCanApplyImmediately);
	mSelectBtn->setEnabled(mActive);
	LLFloater::draw ();
	gl_draw_image ( mRGBViewerImageLeft, mRGBViewerImageTop - mRGBViewerImageHeight, mRGBImage, LLColor4::white );
	S32 xPos = ( S32 ) ( ( F32 )mRGBViewerImageWidth * getCurH () ) - 8;
	S32 yPos = ( S32 ) ( ( F32 )mRGBViewerImageHeight * getCurS () ) - 8;
	gl_line_2d ( mRGBViewerImageLeft + xPos,
				 mRGBViewerImageTop - mRGBViewerImageHeight + yPos + 8,
				 mRGBViewerImageLeft + xPos + 16,
				 mRGBViewerImageTop - mRGBViewerImageHeight + yPos + 8,
				 LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ) );
	gl_line_2d ( mRGBViewerImageLeft + xPos + 8,
				 mRGBViewerImageTop - mRGBViewerImageHeight + yPos,
				 mRGBViewerImageLeft + xPos + 8,
				 mRGBViewerImageTop - mRGBViewerImageHeight + yPos + 16,
				 LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ) );
	gl_rect_2d ( mRGBViewerImageLeft,
				 mRGBViewerImageTop - mRGBViewerImageHeight + 1,
				 mRGBViewerImageLeft + mRGBViewerImageWidth + 1,
				 mRGBViewerImageTop,
				 LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ),
				 FALSE );
	for ( S32 y = 0; y < mLumRegionHeight; ++y )
	{
		F32 rValSlider, gValSlider, bValSlider;
		hslToRgb ( getCurH (), getCurS (), ( F32 )y / ( F32 )mLumRegionHeight, rValSlider, gValSlider, bValSlider );
		gl_rect_2d( mLumRegionLeft,
			mLumRegionTop - mLumRegionHeight + y,
				mLumRegionLeft + mLumRegionWidth,
					mLumRegionTop - mLumRegionHeight + y - 1,
						LLColor4 ( rValSlider, gValSlider, bValSlider, 1.0f ) );
	}
	S32 startX = mLumRegionLeft + mLumRegionWidth;
	S32 startY = mLumRegionTop - mLumRegionHeight + ( S32 ) ( mLumRegionHeight * getCurL () );
	gl_triangle_2d ( startX, startY,
			startX + mLumMarkerSize, startY - mLumMarkerSize,
				startX + mLumMarkerSize, startY + mLumMarkerSize,
					LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ), TRUE );
	gl_triangle_2d ( startX+2, startY,
		startX + mLumMarkerSize - 1, startY - mLumMarkerSize + 2,
			startX + mLumMarkerSize - 1, startY + mLumMarkerSize - 2,
				LLColor4 ( 1.0f, 1.0f, 1.0f, 1.0f ), TRUE );
	gl_rect_2d ( mLumRegionLeft,
				 mLumRegionTop - mLumRegionHeight + 1,
				 mLumRegionLeft + mLumRegionWidth + 1,
				 mLumRegionTop,
				 LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ),
				 FALSE );
	gl_rect_2d ( mSwatchRegionLeft,
				 mSwatchRegionTop - mSwatchRegionHeight,
				 mSwatchRegionLeft + mSwatchRegionWidth,
				 mSwatchRegionTop,
				 LLColor4 ( getCurR (), getCurG (), getCurB (), 1.0f ),
				 TRUE );
	gl_rect_2d ( mSwatchRegionLeft,
				 mSwatchRegionTop - mSwatchRegionHeight + 1,
				 mSwatchRegionLeft + mSwatchRegionWidth + 1,
				 mSwatchRegionTop,
				 LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ),
				 FALSE );
	drawPalette ();
}
const LLColor4& LLFloaterColorPicker::getComplimentaryColor ( const LLColor4& backgroundColor )
{
	F32 hVal, sVal, lVal;
	backgroundColor.calcHSL(&hVal, &sVal, &lVal);
	hVal *= 360.f;
	sVal *= 100.f;
	lVal *= 100.f;
	if ( lVal < 0.5f )
	{
		return LLColor4::white;
	}
	return LLColor4::black;
}
void LLFloaterColorPicker::drawPalette ()
{
	S32 curEntry = 0;
	for ( S32 y = 0; y < numPaletteRows; ++y )
	{
		for ( S32 x = 0; x < numPaletteColumns; ++x )
		{
			S32 x1 = mPaletteRegionLeft + ( mPaletteRegionWidth * x ) / numPaletteColumns;
			S32 y1 = mPaletteRegionTop - ( mPaletteRegionHeight * y ) / numPaletteRows;
			S32 x2 = ( mPaletteRegionLeft + ( mPaletteRegionWidth * ( x + 1 ) ) / numPaletteColumns );
			S32 y2 = ( mPaletteRegionTop - ( mPaletteRegionHeight * ( y + 1 ) ) / numPaletteRows );
			if ( mPalette [ curEntry ] )
			{
				gl_rect_2d ( x1 + 1, y1 - 2, x2 - 2, y2 + 1, *mPalette [ curEntry++ ], TRUE );
				gl_rect_2d ( x1 + 1, y1 - 1, x2 - 1, y2 + 1, LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ), FALSE );
			}
		}
	}
	if ( highlightEntry >= 0 )
	{
		S32 entryColumn = highlightEntry % numPaletteColumns;
		S32 entryRow = highlightEntry / numPaletteColumns;
		S32 x1 = mPaletteRegionLeft + ( mPaletteRegionWidth * entryColumn ) / numPaletteColumns;
		S32 y1 = mPaletteRegionTop - ( mPaletteRegionHeight * entryRow ) / numPaletteRows;
		S32 x2 = ( mPaletteRegionLeft + ( mPaletteRegionWidth * ( entryColumn + 1 ) ) / numPaletteColumns );
		S32 y2 = ( mPaletteRegionTop - ( mPaletteRegionHeight * ( entryRow + 1 ) ) / numPaletteRows );
		S32 xCenter = x1 + ( x2 - x1 ) / 2;
		S32 yCenter = y1 - ( y1 - y2 ) / 2;
		LLColor4 hlColor ( getComplimentaryColor ( *mPalette [ highlightEntry ] ) );
		gl_line_2d ( xCenter - 4, yCenter - 4, xCenter + 4, yCenter + 4, hlColor );
		gl_line_2d ( xCenter + 4, yCenter - 4, xCenter - 4, yCenter + 4, hlColor );
	}
}
std::string RGBToHex(int rNum, int gNum, int bNum)
{
	std::string result;
	result.append(llformat("%.2X", rNum));
	result.append(llformat("%.2X", gNum));
	result.append(llformat("%.2X", bNum));
	return result;
}
void LLFloaterColorPicker::onHexCommit(const LLSD& value)
{
	char* pStop;
	int num = strtol(value.asString().c_str(), &pStop, 16);
	int r = (num & 0xFF0000) >> 16;
	int g = (num & 0xFF00) >> 8;
	int b = num & 0xFF;
	setCurRgb (r / 255.0f, g / 255.0f, b / 255.0f);
	updateTextEntry();
	if (mApplyImmediateCheck->get())
	{
		LLColorSwatchCtrl::onColorChanged(getSwatch(), LLColorSwatchCtrl::COLOR_CHANGE);
	}
}
void LLFloaterColorPicker::updateTextEntry ()
{
	getChild<LLUICtrl>("rspin")->setValue(( getCurR () * 255.0f ) );
	getChild<LLUICtrl>("gspin")->setValue(( getCurG () * 255.0f ) );
	getChild<LLUICtrl>("bspin")->setValue(( getCurB () * 255.0f ) );
	getChild<LLUICtrl>("hspin")->setValue(( getCurH () * 360.0f ) );
	getChild<LLUICtrl>("sspin")->setValue(( getCurS () * 100.0f ) );
	getChild<LLUICtrl>("lspin")->setValue(( getCurL () * 100.0f ) );
	getChild<LLUICtrl>("hexval")->setValue(RGBToHex(getCurR() * 255, getCurG() * 255, getCurB() * 255));
}
void LLFloaterColorPicker::onTextEntryChanged ( LLUICtrl* ctrl )
{
	std::string name = ctrl->getName();
	if ( ( name == "rspin" ) || ( name == "gspin" ) || ( name == "bspin" ) )
	{
		F32 rVal, gVal, bVal;
		getCurRgb ( rVal, gVal, bVal );
		if ( name == "rspin" )
		{
			rVal = (F32)ctrl->getValue().asReal() / 255.0f;
		}
		else
		if ( name == "gspin" )
		{
			gVal = (F32)ctrl->getValue().asReal() / 255.0f;
		}
		else
		if ( name == "bspin" )
		{
			bVal = (F32)ctrl->getValue().asReal() / 255.0f;
		}
		setCurRgb ( rVal, gVal, bVal );
		updateTextEntry ();
	}
	else
	if ( ( name == "hspin" ) || ( name == "sspin" ) || ( name == "lspin" ) )
	{
		F32 hVal, sVal, lVal;
		getCurHsl ( hVal, sVal, lVal );
		if ( name == "hspin" )
			hVal = (F32)ctrl->getValue().asReal() / 360.0f;
		else
		if ( name == "sspin" )
			sVal = (F32)ctrl->getValue().asReal() / 100.0f;
		else
		if ( name == "lspin" )
			lVal = (F32)ctrl->getValue().asReal() / 100.0f;
		setCurHsl ( hVal, sVal, lVal );
		updateTextEntry ();
	}
	if (mApplyImmediateCheck->get())
	{
		LLColorSwatchCtrl::onColorChanged ( getSwatch (), LLColorSwatchCtrl::COLOR_CHANGE );
	}
}
BOOL LLFloaterColorPicker::updateRgbHslFromPoint ( S32 xPosIn, S32 yPosIn )
{
	if ( xPosIn >= mRGBViewerImageLeft &&
		 xPosIn <= mRGBViewerImageLeft + mRGBViewerImageWidth &&
		 yPosIn <= mRGBViewerImageTop &&
		 yPosIn >= mRGBViewerImageTop - mRGBViewerImageHeight )
	{
		setCurHsl ( ( ( F32 )xPosIn - ( F32 )mRGBViewerImageLeft ) / ( F32 )mRGBViewerImageWidth,
					( ( F32 )yPosIn - ( ( F32 )mRGBViewerImageTop - ( F32 )mRGBViewerImageHeight ) ) / ( F32 )mRGBViewerImageHeight,
					getCurL () );
		return TRUE;
	}
	else
	if ( xPosIn >= mLumRegionLeft &&
		 xPosIn <= mLumRegionLeft + mLumRegionWidth &&
		 yPosIn <= mLumRegionTop &&
		 yPosIn >= mLumRegionTop - mLumRegionHeight )
	{
		 setCurHsl ( getCurH (),
					 getCurS (),
					( ( F32 )yPosIn - ( ( F32 )mRGBViewerImageTop - ( F32 )mRGBViewerImageHeight ) ) / ( F32 )mRGBViewerImageHeight );
		return TRUE;
	}
	return FALSE;
}
BOOL LLFloaterColorPicker::handleMouseDown ( S32 x, S32 y, MASK mask )
{
	gFloaterView->bringToFront(this);
	LLRect rgbAreaRect ( mRGBViewerImageLeft,
						 mRGBViewerImageTop,
						 mRGBViewerImageLeft + mRGBViewerImageWidth,
						 mRGBViewerImageTop - mRGBViewerImageHeight );
	if ( rgbAreaRect.pointInRect ( x, y ) )
	{
		gFocusMgr.setMouseCapture(this);
		setMouseDownInHueRegion ( TRUE );
		updateRgbHslFromPoint ( x, y );
		return TRUE;
	}
	LLRect lumAreaRect ( mLumRegionLeft,
						 mLumRegionTop,
						 mLumRegionLeft + mLumRegionWidth + mLumMarkerSize,
						 mLumRegionTop - mLumRegionHeight );
	if ( lumAreaRect.pointInRect ( x, y ) )
	{
		gFocusMgr.setMouseCapture(this);
		setMouseDownInLumRegion ( TRUE );
		return TRUE;
	}
	LLRect swatchRect ( mSwatchRegionLeft,
						mSwatchRegionTop,
						mSwatchRegionLeft + mSwatchRegionWidth,
						mSwatchRegionTop - mSwatchRegionHeight );
	setMouseDownInSwatch( FALSE );
	if ( swatchRect.pointInRect ( x, y ) )
	{
		setMouseDownInSwatch( TRUE );
		return TRUE;
	}
	LLRect paletteRect ( mPaletteRegionLeft,
						 mPaletteRegionTop,
						 mPaletteRegionLeft + mPaletteRegionWidth,
						 mPaletteRegionTop - mPaletteRegionHeight );
	if ( paletteRect.pointInRect ( x, y ) )
	{
		if (gFocusMgr.childHasKeyboardFocus(this))
		{
			mSelectBtn->setFocus(TRUE);
		}
		S32 c = ( ( x - mPaletteRegionLeft ) * numPaletteColumns ) / mPaletteRegionWidth;
		S32 r = ( ( y - ( mPaletteRegionTop - mPaletteRegionHeight ) ) * numPaletteRows ) / mPaletteRegionHeight;
		U32 index = ( numPaletteRows - r - 1 ) * numPaletteColumns + c;
		if ( index <= mPalette.size () )
		{
			LLColor4 selected = *mPalette [ index ];
			setCurRgb ( selected [ 0 ], selected [ 1 ], selected [ 2 ] );
			if (mApplyImmediateCheck->get())
			{
				LLColorSwatchCtrl::onColorChanged ( getSwatch (), LLColorSwatchCtrl::COLOR_CHANGE );
			}
			updateTextEntry ();
		}
		return TRUE;
	}
	return LLFloater::handleMouseDown ( x, y, mask );
}
BOOL LLFloaterColorPicker::handleHover ( S32 x, S32 y, MASK mask )
{
	if ( isFrontmost () )
	{
		if ( getMouseDownInHueRegion() || getMouseDownInLumRegion())
		{
			S32 clamped_x, clamped_y;
			if (getMouseDownInHueRegion())
			{
				clamped_x = llclamp(x, mRGBViewerImageLeft, mRGBViewerImageLeft + mRGBViewerImageWidth);
				clamped_y = llclamp(y, mRGBViewerImageTop - mRGBViewerImageHeight, mRGBViewerImageTop);
			}
			else
			{
				clamped_x = llclamp(x, mLumRegionLeft, mLumRegionLeft + mLumRegionWidth);
				clamped_y = llclamp(y, mLumRegionTop - mLumRegionHeight, mLumRegionTop);
			}
			if ( updateRgbHslFromPoint ( clamped_x, clamped_y ) )
			{
				updateTextEntry ();
			}
		}
		highlightEntry = -1;
		if ( mMouseDownInSwatch )
		{
			getWindow()->setCursor ( UI_CURSOR_ARROWDRAG );
			LLRect paletteRect ( mPaletteRegionLeft,
								mPaletteRegionTop,
								mPaletteRegionLeft + mPaletteRegionWidth,
								mPaletteRegionTop - mPaletteRegionHeight );
			if ( paletteRect.pointInRect ( x, y ) )
			{
				S32 xOffset = ( ( x - mPaletteRegionLeft ) * numPaletteColumns ) / mPaletteRegionWidth;
				S32 yOffset = ( ( mPaletteRegionTop - y - 1 ) * numPaletteRows ) / mPaletteRegionHeight;
				highlightEntry = xOffset + yOffset * numPaletteColumns;
			}
			return TRUE;
		}
	}
	return LLFloater::handleHover ( x, y, mask );
}
void LLFloaterColorPicker::onClose(bool app_quitting)
{
	LLFloater::onClose(app_quitting);
}
BOOL LLFloaterColorPicker::handleMouseUp ( S32 x, S32 y, MASK mask )
{
	getWindow()->setCursor ( UI_CURSOR_ARROW );
	if (getMouseDownInHueRegion() || getMouseDownInLumRegion())
	{
		if (mApplyImmediateCheck->get())
		{
			LLColorSwatchCtrl::onColorChanged ( getSwatch (), LLColorSwatchCtrl::COLOR_CHANGE );
		}
	}
	LLRect paletteRect ( mPaletteRegionLeft,
							mPaletteRegionTop,
							mPaletteRegionLeft + mPaletteRegionWidth,
							mPaletteRegionTop - mPaletteRegionHeight );
	if ( paletteRect.pointInRect ( x, y ) )
	{
		if ( mMouseDownInSwatch )
		{
			S32 curEntry = 0;
			for ( S32 row = 0; row < numPaletteRows; ++row )
			{
				for ( S32 column = 0; column < numPaletteColumns; ++column )
				{
					S32 left = mPaletteRegionLeft + ( mPaletteRegionWidth * column ) / numPaletteColumns;
					S32 top = mPaletteRegionTop - ( mPaletteRegionHeight * row ) / numPaletteRows;
					S32 right = ( mPaletteRegionLeft + ( mPaletteRegionWidth * ( column + 1 ) ) / numPaletteColumns );
					S32 bottom = ( mPaletteRegionTop - ( mPaletteRegionHeight * ( row + 1 ) ) / numPaletteRows );
					LLRect dropRect ( left, top, right, bottom );
					if ( dropRect.pointInRect ( x, y ) )
					{
						if ( mPalette [ curEntry ] )
						{
							delete mPalette [ curEntry ];
							mPalette [ curEntry ] = new LLColor4 ( getCurR (), getCurG (), getCurB (), 1.0f );
							std::ostringstream codec;
							codec << "ColorPaletteEntry" << std::setfill ( '0' ) << std::setw ( 2 ) << curEntry + 1;
							const std::string s ( codec.str () );
							gSavedSettings.setColor4( s, *mPalette [ curEntry ] );
						}
					}
					++curEntry;
				}
			}
		}
	}
	setMouseDownInHueRegion ( FALSE );
	setMouseDownInLumRegion ( FALSE );
	mMouseDownInSwatch = false;
	if (hasMouseCapture())
	{
		gFocusMgr.setMouseCapture(NULL);
	}
	return LLFloater::handleMouseUp ( x, y, mask );
}
void LLFloaterColorPicker::cancelSelection ()
{
	setCurRgb ( getOrigR (), getOrigG (), getOrigB () );
	LLColorSwatchCtrl::onColorChanged( getSwatch(), LLColorSwatchCtrl::COLOR_CANCEL );
	this->setVisible ( FALSE );
}
void LLFloaterColorPicker::setMouseDownInHueRegion ( BOOL mouse_down_in_region )
{
	mMouseDownInHueRegion = mouse_down_in_region;
	if (mouse_down_in_region)
	{
		if (gFocusMgr.childHasKeyboardFocus(this))
		{
			mSelectBtn->setFocus(TRUE);
		}
	}
}
void LLFloaterColorPicker::setMouseDownInLumRegion ( BOOL mouse_down_in_region )
{
	mMouseDownInLumRegion = mouse_down_in_region;
	if (mouse_down_in_region)
	{
		if (gFocusMgr.childHasKeyboardFocus(this))
		{
			mSelectBtn->setFocus(TRUE);
		}
	}
}
void LLFloaterColorPicker::setMouseDownInSwatch (BOOL mouse_down_in_swatch)
{
	mMouseDownInSwatch = mouse_down_in_swatch;
	if (mouse_down_in_swatch)
	{
		if (gFocusMgr.childHasKeyboardFocus(this))
		{
			mSelectBtn->setFocus(TRUE);
		}
	}
}
void LLFloaterColorPicker::setActive(BOOL active)
{
	if (!active && mPipetteBtn->getToggleState())
	{
		stopUsingPipette();
	}
	mActive = active;
}
void LLFloaterColorPicker::stopUsingPipette()
{
	if (LLToolMgr::getInstance()->getCurrentTool() == LLToolPipette::getInstance())
	{
		LLToolMgr::getInstance()->clearTransientTool();
	}
}
