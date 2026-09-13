/** 
 * @file llvoicevisualizer.cpp
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
#include "llviewerprecompiledheaders.h"
#include "llviewercontrol.h"
#include "llglheaders.h"
#include "llsphere.h"
#include "llvoicevisualizer.h"
#include "llviewercamera.h"
#include "llviewerobject.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llvoiceclient.h"
#include "llrender.h"
const F32	HEIGHT_ABOVE_HEAD	= 0.3f;
const F32	RED_THRESHOLD		= LLVoiceClient::OVERDRIVEN_POWER_LEVEL;
const F32	GREEN_THRESHOLD		= 0.2f;
const F32	FADE_OUT_DURATION	= 0.4f;
const F32	EXPANSION_RATE		= 1.0f;
const F32	EXPANSION_MAX		= 1.5f;
const F32	WAVE_WIDTH_SCALE	= 0.03f;
const F32	WAVE_HEIGHT_SCALE	= 0.02f;
const F32	BASE_BRIGHTNESS		= 0.7f;
const F32	DOT_SIZE			= 0.05f;
const F32	DOT_OPACITY			= 0.7f;
const F32	WAVE_MOTION_RATE	= 1.5f;
const F32 DEFAULT_MINIMUM_GESTICULATION_AMPLITUDE	= 0.2f;
const F32 DEFAULT_MAXIMUM_GESTICULATION_AMPLITUDE	= 1.0f;
const F32 ONE_HALF = 1.0f;
const LLVector3 WORLD_UPWARD_DIRECTION = LLVector3( 0.0f, 0.0f, 1.0f );
static bool handleVoiceVisualizerPrefsChanged(const LLSD& newvalue)
{
	LLVoiceVisualizer::setPreferences();
	return true;
}
bool LLVoiceVisualizer::sPrefsInitialized	= false;
BOOL LLVoiceVisualizer::sLipSyncEnabled		= FALSE;
F32* LLVoiceVisualizer::sOoh				= NULL;
F32* LLVoiceVisualizer::sAah				= NULL;
U32	 LLVoiceVisualizer::sOohs				= 0;
U32	 LLVoiceVisualizer::sAahs				= 0;
F32	 LLVoiceVisualizer::sOohAahRate			= 0.0f;
F32* LLVoiceVisualizer::sOohPowerTransfer	= NULL;
U32	 LLVoiceVisualizer::sOohPowerTransfers	= 0;
F32	 LLVoiceVisualizer::sOohPowerTransfersf = 0.0f;
F32* LLVoiceVisualizer::sAahPowerTransfer	= NULL;
U32	 LLVoiceVisualizer::sAahPowerTransfers	= 0;
F32	 LLVoiceVisualizer::sAahPowerTransfersf = 0.0f;
LLVoiceVisualizer::LLVoiceVisualizer( const U8 type )
:LLHUDEffect( type )
{
	mCurrentTime					= mTimer.getTotalSeconds();
	mPreviousTime					= mCurrentTime;
	mStartTime						= mCurrentTime;
	mVoiceSourceWorldPosition		= LLVector3( 0.0f, 0.0f, 0.0f );
	mSpeakingAmplitude				= 0.0f;
	mCurrentlySpeaking				= false;
	mVoiceEnabled					= false;
	mMinGesticulationAmplitude		= DEFAULT_MINIMUM_GESTICULATION_AMPLITUDE;
	mMaxGesticulationAmplitude		= DEFAULT_MAXIMUM_GESTICULATION_AMPLITUDE;
	mSoundSymbol.mActive			= true;
	mSoundSymbol.mPosition			= LLVector3( 0.0f, 0.0f, 0.0f );
	mTimer.reset();
	const char* sound_level_img[] =
	{
		"voice_meter_dot.j2c",
		"voice_meter_rings.j2c",
		"voice_meter_rings.j2c",
		"voice_meter_rings.j2c",
		"voice_meter_rings.j2c",
		"voice_meter_rings.j2c",
		"voice_meter_rings.j2c"
	};
	for (int i=0; i<NUM_VOICE_SYMBOL_WAVES; i++)
	{
		mSoundSymbol.mWaveFadeOutStartTime	[i] = mCurrentTime;
		mSoundSymbol.mTexture				[i] = LLViewerTextureManager::getFetchedTextureFromFile(sound_level_img[i], FTT_LOCAL_FILE, FALSE, LLGLTexture::BOOST_UI);
		mSoundSymbol.mWaveActive			[i] = false;
		mSoundSymbol.mWaveOpacity			[i] = 1.0f;
		mSoundSymbol.mWaveExpansion			[i] = 1.0f;
	}
	mSoundSymbol.mTexture[0]->setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
	if (!sPrefsInitialized)
	{
		setPreferences();
		gSavedSettings.getControl("LipSyncEnabled")->getSignal()->connect(boost::bind(&handleVoiceVisualizerPrefsChanged, _2));
		gSavedSettings.getControl("LipSyncOohAahRate")->getSignal()->connect(boost::bind(&handleVoiceVisualizerPrefsChanged, _2));
		gSavedSettings.getControl("LipSyncOoh")->getSignal()->connect(boost::bind(&handleVoiceVisualizerPrefsChanged, _2));
		gSavedSettings.getControl("LipSyncAah")->getSignal()->connect(boost::bind(&handleVoiceVisualizerPrefsChanged, _2));
		gSavedSettings.getControl("LipSyncOohPowerTransfer")->getSignal()->connect(boost::bind(&handleVoiceVisualizerPrefsChanged, _2));
		gSavedSettings.getControl("LipSyncAahPowerTransfer")->getSignal()->connect(boost::bind(&handleVoiceVisualizerPrefsChanged, _2));
		sPrefsInitialized = true;
	}
}
void LLVoiceVisualizer::setMinGesticulationAmplitude( F32 m )
{
	mMinGesticulationAmplitude = m;
}
void LLVoiceVisualizer::setMaxGesticulationAmplitude( F32 m )
{
	mMaxGesticulationAmplitude = m;
}
void LLVoiceVisualizer::setVoiceEnabled( bool v )
{
	mVoiceEnabled = v;
}
void LLVoiceVisualizer::setStartSpeaking()
{
	mStartTime				= mTimer.getTotalSeconds();
	mCurrentlySpeaking		= true;
	mSoundSymbol.mActive	= true;
}
bool LLVoiceVisualizer::getCurrentlySpeaking()
{
	return mCurrentlySpeaking;
}
void LLVoiceVisualizer::setStopSpeaking()
{
	mCurrentlySpeaking = false;
	mSpeakingAmplitude = 0.0f;
}
void LLVoiceVisualizer::setSpeakingAmplitude( F32 a )
{
	mSpeakingAmplitude = a;
}
void LLVoiceVisualizer::setPreferences( )
{
	sLipSyncEnabled = gSavedSettings.getBOOL("LipSyncEnabled");
	sOohAahRate		= gSavedSettings.getF32("LipSyncOohAahRate");
	std::string oohString = gSavedSettings.getString("LipSyncOoh");
	lipStringToF32s (oohString, sOoh, sOohs);
	std::string aahString = gSavedSettings.getString("LipSyncAah");
	lipStringToF32s (aahString, sAah, sAahs);
	std::string oohPowerString = gSavedSettings.getString("LipSyncOohPowerTransfer");
	lipStringToF32s (oohPowerString, sOohPowerTransfer, sOohPowerTransfers);
	sOohPowerTransfersf = (F32) sOohPowerTransfers;
	std::string aahPowerString = gSavedSettings.getString("LipSyncAahPowerTransfer");
	lipStringToF32s (aahPowerString, sAahPowerTransfer, sAahPowerTransfers);
	sAahPowerTransfersf = (F32) sAahPowerTransfers;
}
void LLVoiceVisualizer::lipStringToF32s ( std::string& in_string, F32*& out_F32s, U32& count_F32s )
{
	delete[] out_F32s;
	count_F32s = in_string.length();
	if (count_F32s == 0)
	{
		count_F32s  = 1;
		out_F32s	   = new F32[1];
		out_F32s[0] = 0.0f;
	}
	else
	{
		out_F32s = new F32[count_F32s];
		for (U32 i=0; i<count_F32s; i++)
		{
		    U8 digit = in_string[i];
			U8 four_bits = digit % 16;
			if (four_bits > 9)
			{
				four_bits = 9;
			}
			out_F32s[i] = 0.11f * (F32) four_bits;
		}
	}
}
void LLVoiceVisualizer::lipSyncOohAah( F32& ooh, F32& aah )
{
	if( ( sLipSyncEnabled == TRUE ) && mCurrentlySpeaking )
	{
		U32 transfer_index = (U32) (sOohPowerTransfersf * mSpeakingAmplitude);
		if (transfer_index >= sOohPowerTransfers)
		{
		   transfer_index = sOohPowerTransfers - 1;
		}
		F32 transfer_ooh = sOohPowerTransfer[transfer_index];
		transfer_index = (U32) (sAahPowerTransfersf * mSpeakingAmplitude);
		if (transfer_index >= sAahPowerTransfers)
		{
		   transfer_index = sAahPowerTransfers - 1;
		}
		F32 transfer_aah = sAahPowerTransfer[transfer_index];
		F64 current_time   = mTimer.getTotalSeconds();
		F64 elapsed_time   = current_time - mStartTime;
		U32 elapsed_frames = (U32) (elapsed_time * sOohAahRate);
		U32 elapsed_oohs   = elapsed_frames % sOohs;
		U32 elapsed_aahs   = elapsed_frames % sAahs;
		ooh = transfer_ooh * sOoh[elapsed_oohs];
		aah = transfer_aah * sAah[elapsed_aahs];
	}
	else
	{
		ooh = 0.0f;
		aah = 0.0f;
	}
}
void LLVoiceVisualizer::render()
{
	if ( ! mVoiceEnabled )
	{
		return;
	}
	if ( mSoundSymbol.mActive )
	{
		mPreviousTime = mCurrentTime;
		mCurrentTime = mTimer.getTotalSeconds();
		mSoundSymbol.mPosition = mVoiceSourceWorldPosition + WORLD_UPWARD_DIRECTION * HEIGHT_ABOVE_HEAD;
		LLGLSPipelineAlpha alpha_blend;
		LLGLDepthTest depth(GL_TRUE, GL_FALSE);
		LLViewerCamera* camera = LLViewerCamera::getInstance();
		LLVector3 l	= camera->getLeftAxis() * DOT_SIZE;
		LLVector3 u	= camera->getUpAxis()   * DOT_SIZE;
		LLVector3 bottomLeft	= mSoundSymbol.mPosition + l - u;
		LLVector3 bottomRight	= mSoundSymbol.mPosition - l - u;
		LLVector3 topLeft		= mSoundSymbol.mPosition + l + u;
		LLVector3 topRight		= mSoundSymbol.mPosition - l + u;
		gGL.getTexUnit(0)->bind(mSoundSymbol.mTexture[0]);
		gGL.color4fv( LLColor4( 1.0f, 1.0f, 1.0f, DOT_OPACITY ).mV );
		gGL.begin( LLRender::TRIANGLE_STRIP );
			gGL.texCoord2i( 0,	0	); gGL.vertex3fv( bottomLeft.mV );
			gGL.texCoord2i( 1,	0	); gGL.vertex3fv( bottomRight.mV );
			gGL.texCoord2i( 0,	1	); gGL.vertex3fv( topLeft.mV );
		gGL.end();
		gGL.begin( LLRender::TRIANGLE_STRIP );
			gGL.texCoord2i( 1,	0	); gGL.vertex3fv( bottomRight.mV );
			gGL.texCoord2i( 1,	1	); gGL.vertex3fv( topRight.mV );
			gGL.texCoord2i( 0,	1	); gGL.vertex3fv( topLeft.mV );
		gGL.end();
		if ( mCurrentlySpeaking )
		{
			F32 min = 0.2f;
			F32 max = 0.7f;
			F32 fraction = ( mSpeakingAmplitude - min ) / ( max - min );
			if ( fraction > 1.0f )
			{
				fraction = 1.0f;
			}
			S32 level = 1 + (int)( fraction * ( NUM_VOICE_SYMBOL_WAVES - 2 ) );
			for (int i=0; i<level+1; i++)
			{
				mSoundSymbol.mWaveActive			[i] = true;
				mSoundSymbol.mWaveOpacity			[i] = 1.0f;
				mSoundSymbol.mWaveFadeOutStartTime	[i] = mCurrentTime;
			}
		}
		F32 red		= 0.0f;
		F32 green	= 0.0f;
		F32 blue	= 0.0f;
        if ( mSpeakingAmplitude < RED_THRESHOLD )
        {
			if ( mSpeakingAmplitude < GREEN_THRESHOLD )
			{
				red		= BASE_BRIGHTNESS;
				green	= BASE_BRIGHTNESS;
				blue	= BASE_BRIGHTNESS;
			}
			else
			{
				F32 fraction = ( mSpeakingAmplitude - GREEN_THRESHOLD ) / ( 1.0f - GREEN_THRESHOLD );
				red		= BASE_BRIGHTNESS - ( fraction * BASE_BRIGHTNESS );
				green	= BASE_BRIGHTNESS +   fraction * ( 1.0f - BASE_BRIGHTNESS );
				blue	= BASE_BRIGHTNESS - ( fraction * BASE_BRIGHTNESS );
			}
        }
        else
        {
			red		= 1.0f;
			green	= 0.2f;
			blue	= 0.2f;
        }
		for (int i=0; i<NUM_VOICE_SYMBOL_WAVES; i++)
		{
			if ( mSoundSymbol.mWaveActive[i] )
			{
				F32 fadeOutFraction = (F32)( mCurrentTime - mSoundSymbol.mWaveFadeOutStartTime[i] ) / FADE_OUT_DURATION;
				mSoundSymbol.mWaveOpacity[i] = 1.0f - fadeOutFraction;
				if ( mSoundSymbol.mWaveOpacity[i] < 0.0f )
				{
					mSoundSymbol.mWaveFadeOutStartTime	[i] = mCurrentTime;
					mSoundSymbol.mWaveOpacity			[i] = 0.0f;
					mSoundSymbol.mWaveActive			[i] = false;
				}
				F32 timeSlice = (F32)( mCurrentTime - mPreviousTime );
				F32 waveSpeed = mSpeakingAmplitude * WAVE_MOTION_RATE;
				mSoundSymbol.mWaveExpansion[i] *= ( 1.0f + EXPANSION_RATE * timeSlice * waveSpeed );
				if ( mSoundSymbol.mWaveExpansion[i] > EXPANSION_MAX )
				{
					mSoundSymbol.mWaveExpansion[i] = 1.0f;
				}
				F32 width	= i * WAVE_WIDTH_SCALE  * mSoundSymbol.mWaveExpansion[i];
				F32 height	= i * WAVE_HEIGHT_SCALE * mSoundSymbol.mWaveExpansion[i];
				LLVector3 l	= camera->getLeftAxis() * width;
				LLVector3 u	= camera->getUpAxis()   * height;
				LLVector3 bottomLeft	= mSoundSymbol.mPosition + l - u;
				LLVector3 bottomRight	= mSoundSymbol.mPosition - l - u;
				LLVector3 topLeft		= mSoundSymbol.mPosition + l + u;
				LLVector3 topRight		= mSoundSymbol.mPosition - l + u;
				gGL.color4fv( LLColor4( red, green, blue, mSoundSymbol.mWaveOpacity[i] ).mV );
				gGL.getTexUnit(0)->bind(mSoundSymbol.mTexture[i]);
				gGL.begin( LLRender::TRIANGLE_STRIP );
					gGL.texCoord2i( 0, 0 ); gGL.vertex3fv( bottomLeft.mV );
					gGL.texCoord2i( 1, 0 ); gGL.vertex3fv( bottomRight.mV );
					gGL.texCoord2i( 0, 1 ); gGL.vertex3fv( topLeft.mV );
				gGL.end();
				gGL.begin( LLRender::TRIANGLE_STRIP );
					gGL.texCoord2i( 1, 0 ); gGL.vertex3fv( bottomRight.mV );
					gGL.texCoord2i( 1, 1 ); gGL.vertex3fv( topRight.mV );
					gGL.texCoord2i( 0, 1 ); gGL.vertex3fv( topLeft.mV );
				gGL.end();
			}
		}
	}
}
void LLVoiceVisualizer::setVoiceSourceWorldPosition( const LLVector3 &p )
{
	mVoiceSourceWorldPosition	= p;
}
VoiceGesticulationLevel LLVoiceVisualizer::getCurrentGesticulationLevel()
{
	VoiceGesticulationLevel gesticulationLevel = VOICE_GESTICULATION_LEVEL_OFF;
	F32 range = mMaxGesticulationAmplitude - mMinGesticulationAmplitude;
			if ( mSpeakingAmplitude > mMinGesticulationAmplitude + range * 0.5f	)	{ gesticulationLevel = VOICE_GESTICULATION_LEVEL_HIGH;		}
	else	if ( mSpeakingAmplitude > mMinGesticulationAmplitude + range * 0.25f	)	{ gesticulationLevel = VOICE_GESTICULATION_LEVEL_MEDIUM;	}
	else	if ( mSpeakingAmplitude > mMinGesticulationAmplitude + range * 0.00000f	)	{ gesticulationLevel = VOICE_GESTICULATION_LEVEL_LOW;		}
	return gesticulationLevel;
}
LLVoiceVisualizer::~LLVoiceVisualizer()
{
}
void LLVoiceVisualizer::packData(LLMessageSystem *mesgsys)
{
	LLHUDEffect::packData(mesgsys);
	U8 packed_data = 0;
	mesgsys->addBinaryDataFast(_PREHASH_TypeData, &packed_data, 1);
}
void LLVoiceVisualizer::unpackData(LLMessageSystem *mesgsys, S32 blocknum)
{
}
void LLVoiceVisualizer::markDead()
{
	mCurrentlySpeaking		= false;
	mVoiceEnabled			= false;
	mSoundSymbol.mActive	= false;
	LLHUDEffect::markDead();
}
