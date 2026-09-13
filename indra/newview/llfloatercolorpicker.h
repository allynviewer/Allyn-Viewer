/** 
 * @file llfloatercolorpicker.h
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
#ifndef LL_LLFLOATERCOLORPICKER_H
#define LL_LLFLOATERCOLORPICKER_H
#include <vector>
#include "llfloater.h"
#include "llmemory.h"
#include "llcolorswatch.h"
#include "llspinctrl.h"
#include "lltextureentry.h"
class LLButton;
class LLLineEditor;
class LLCheckBoxCtrl;
class LLFloaterColorPicker
	: public LLFloater
{
	public:
		LLFloaterColorPicker (LLColorSwatchCtrl* swatch, BOOL show_apply_immediate = FALSE);
		virtual ~LLFloaterColorPicker ();
		virtual BOOL postBuild ();
		virtual void draw ();
		virtual BOOL handleMouseDown ( S32 x, S32 y, MASK mask );
		virtual BOOL handleMouseUp ( S32 x, S32 y, MASK mask );
		virtual BOOL handleHover ( S32 x, S32 y, MASK mask );
		virtual void onMouseCaptureLost();
		virtual void onClose(bool app_quitting);
		void createUI ();
		void initUI ( F32 rValIn, F32 gValIn, F32 bValIn );
		void showUI ();
		void destroyUI ();
		void cancelSelection ();
		LLColorSwatchCtrl* getSwatch () { return mSwatch; };
		void setOrigRgb ( F32 origRIn, F32 origGIn, F32 origBIn );
		void getOrigRgb ( F32& origROut, F32& origGOut, F32& origBOut );
		F32 getOrigR () { return origR; };
		F32 getOrigG () { return origG; };
		F32 getOrigB () { return origB; };
		void setCurRgb ( F32 curRIn, F32 curGIn, F32 curBIn );
		void getCurRgb ( F32& curROut, F32& curGOut, F32& curBOut );
		F32	 getCurR () { return curR; };
		F32	 getCurG () { return curG; };
		F32	 getCurB () { return curB; };
		void setCurHsl ( F32 curHIn, F32 curSIn, F32 curLIn );
		void getCurHsl ( F32& curHOut, F32& curSOut, F32& curLOut );
		F32	 getCurH () { return curH; };
		F32	 getCurS () { return  curS; };
		F32	 getCurL () { return curL; };
		BOOL updateRgbHslFromPoint ( S32 xPosIn, S32 yPosIn );
		void updateTextEntry ();
		void stopUsingPipette();
		void setMouseDownInHueRegion ( BOOL mouse_down_in_region );
		BOOL getMouseDownInHueRegion () { return mMouseDownInHueRegion; };
		void setMouseDownInLumRegion ( BOOL mouse_down_in_region );
		BOOL getMouseDownInLumRegion () { return mMouseDownInLumRegion; };
		void setMouseDownInSwatch (BOOL mouse_down_in_swatch);
		BOOL getMouseDownInSwatch () { return mMouseDownInSwatch; }
		void onTextEntryChanged ( LLUICtrl* ctrl );
		void hslToRgb ( F32 hValIn, F32 sValIn, F32 lValIn, F32& rValOut, F32& gValOut, F32& bValOut );
		F32	 hueToRgb ( F32 val1In, F32 val2In, F32 valHUeIn );
		void setActive(BOOL active);
	protected:
		static void onClickCancel ( void* data );
		static void onClickSelect ( void* data );
			   void onClickPipette ( );
		static void onTextCommit ( LLUICtrl* ctrl, void* data );
		static void onImmediateCheck ( LLUICtrl* ctrl, void* data );
			   void onHexCommit(const LLSD& value);
			   void onColorSelect( const LLTextureEntry& te );
	private:
		void drawPalette ();
		const LLColor4& getComplimentaryColor ( const LLColor4& backgroundColor );
		F32 origR, origG, origB;
		F32 curR, curG, curB;
		F32 curH, curS, curL;
		const S32 mComponents;
		BOOL mMouseDownInLumRegion;
		BOOL mMouseDownInHueRegion;
		BOOL mMouseDownInSwatch;
		const S32 mRGBViewerImageLeft;
		const S32 mRGBViewerImageTop;
		const S32 mRGBViewerImageWidth;
		const S32 mRGBViewerImageHeight;
		const S32 mLumRegionLeft;
		const S32 mLumRegionTop;
		const S32 mLumRegionWidth;
		const S32 mLumRegionHeight;
		const S32 mLumMarkerSize;
		const S32 mSwatchRegionLeft;
		const S32 mSwatchRegionTop;
		const S32 mSwatchRegionWidth;
		const S32 mSwatchRegionHeight;
		LLView* mSwatchView;
		const S32 numPaletteColumns;
		const S32 numPaletteRows;
        std::vector < LLColor4* > mPalette;
		S32 highlightEntry;
		const S32 mPaletteRegionLeft;
		const S32 mPaletteRegionTop;
		const S32 mPaletteRegionWidth;
		const S32 mPaletteRegionHeight;
		LLPointer<LLViewerTexture> mRGBImage;
		LLColorSwatchCtrl* mSwatch;
		BOOL	mActive;
		LLCheckBoxCtrl* mApplyImmediateCheck;
		BOOL mCanApplyImmediately;
		LLButton* mSelectBtn;
		LLButton* mCancelBtn;
		LLButton* mPipetteBtn;
		F32		  mContextConeOpacity;
};
#endif
