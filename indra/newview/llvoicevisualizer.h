/** 
 * @file llvoicevisualizer.h
 * @brief Draws in-world speaking indicators.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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
#ifndef LL_VOICE_VISUALIZER_H
#define LL_VOICE_VISUALIZER_H
#include "llhudeffect.h"
enum VoiceGesticulationLevel
{
	VOICE_GESTICULATION_LEVEL_OFF = -1,
	VOICE_GESTICULATION_LEVEL_LOW = 0,
	VOICE_GESTICULATION_LEVEL_MEDIUM,
	VOICE_GESTICULATION_LEVEL_HIGH,
	NUM_VOICE_GESTICULATION_LEVELS
};
const static int NUM_VOICE_SYMBOL_WAVES = 7;
class LLVoiceVisualizer : public LLHUDEffect
{
	public:
		LLVoiceVisualizer ( const U8 type );
		~LLVoiceVisualizer();
		friend class LLHUDObject;
		void					setVoiceSourceWorldPosition( const LLVector3 &p );
		void					setMinGesticulationAmplitude( F32 );
		void					setMaxGesticulationAmplitude( F32 );
		void					setStartSpeaking();
		void					setVoiceEnabled( bool );
		void					setSpeakingAmplitude( F32 );
		void					setStopSpeaking();
		bool					getCurrentlySpeaking();
		VoiceGesticulationLevel	getCurrentGesticulationLevel();
		static void				setPreferences( );
		static void				lipStringToF32s ( std::string& in_string, F32*& out_F32s, U32& count_F32s );
		void					lipSyncOohAah( F32& ooh, F32& aah );
		void					render();
		void 					packData(LLMessageSystem *mesgsys);
		void 					unpackData(LLMessageSystem *mesgsys, S32 blocknum);
		void					markDead();
		void setMaxGesticulationAmplitude();
		void setMinGesticulationAmplitude();
	private:
		struct SoundSymbol
		{
			F32						mWaveExpansion			[ NUM_VOICE_SYMBOL_WAVES ];
			bool					mWaveActive				[ NUM_VOICE_SYMBOL_WAVES ];
			F64						mWaveFadeOutStartTime	[ NUM_VOICE_SYMBOL_WAVES ];
			F32						mWaveOpacity			[ NUM_VOICE_SYMBOL_WAVES ];
			LLPointer<LLViewerTexture>	mTexture				[ NUM_VOICE_SYMBOL_WAVES ];
			bool					mActive;
			LLVector3				mPosition;
		};
		LLFrameTimer			mTimer;
		F64						mStartTime;
		F64						mCurrentTime;
		F64						mPreviousTime;
		SoundSymbol				mSoundSymbol;
		bool					mVoiceEnabled;
		bool					mCurrentlySpeaking;
		LLVector3				mVoiceSourceWorldPosition;
		F32						mSpeakingAmplitude;
		F32						mMaxGesticulationAmplitude;
		F32						mMinGesticulationAmplitude;
		static BOOL	  sLipSyncEnabled;
		static bool	  sPrefsInitialized;
		static F32*	  sOoh;
		static F32*	  sAah;
		static U32	  sOohs;
		static U32	  sAahs;
		static F32	  sOohAahRate;
		static F32*	  sOohPowerTransfer;
		static U32	  sOohPowerTransfers;
		static F32	  sOohPowerTransfersf;
		static F32*	  sAahPowerTransfer;
		static U32	  sAahPowerTransfers;
		static F32	  sAahPowerTransfersf;
};
#endif
