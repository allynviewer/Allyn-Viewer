/**
 * @file audioengine_openal.cpp
 * @brief implementation of audio engine using OpenAL
 * support as a OpenAL 3D implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "lldir.h"
#include "llaudioengine_openal.h"
#include "lllistener_openal.h"
const float LLAudioEngine_OpenAL::WIND_BUFFER_SIZE_SEC = 0.05f;
LLAudioEngine_OpenAL::LLAudioEngine_OpenAL()
	:
	mWindGen(NULL),
	mWindBuf(NULL),
	mWindBufFreq(0),
	mWindBufSamples(0),
	mWindBufBytes(0),
	mWindSource(AL_NONE),
	mNumEmptyWindALBuffers(MAX_NUM_WIND_BUFFERS)
{
}
LLAudioEngine_OpenAL::~LLAudioEngine_OpenAL()
{
}
bool LLAudioEngine_OpenAL::init(const S32 num_channels, void* userdata)
{
	mWindGen = NULL;
	LLAudioEngine::init(num_channels, userdata);
	if(!alutInit(NULL, NULL))
	{
		LL_WARNS() << "LLAudioEngine_OpenAL::init() ALUT initialization failed: " << alutGetErrorString (alutGetError ()) << LL_ENDL;
		return false;
	}
	LL_INFOS() << "LLAudioEngine_OpenAL::init() OpenAL successfully initialized" << LL_ENDL;
	LL_INFOS() << "OpenAL version: "
		<< ll_safe_string(alGetString(AL_VERSION)) << LL_ENDL;
	LL_INFOS() << "OpenAL vendor: "
		<< ll_safe_string(alGetString(AL_VENDOR)) << LL_ENDL;
	LL_INFOS() << "OpenAL renderer: "
		<< ll_safe_string(alGetString(AL_RENDERER)) << LL_ENDL;
	ALint major = alutGetMajorVersion ();
	ALint minor = alutGetMinorVersion ();
	LL_INFOS() << "ALUT version: " << major << "." << minor << LL_ENDL;
	ALCdevice *device = alcGetContextsDevice(alcGetCurrentContext());
	alcGetIntegerv(device, ALC_MAJOR_VERSION, 1, &major);
	alcGetIntegerv(device, ALC_MAJOR_VERSION, 1, &minor);
	LL_INFOS() << "ALC version: " << major << "." << minor << LL_ENDL;
	LL_INFOS() << "ALC default device: "
		<< ll_safe_string(alcGetString(device,
					       ALC_DEFAULT_DEVICE_SPECIFIER))
		<< LL_ENDL;
	return true;
}
std::string LLAudioEngine_OpenAL::getDriverName(bool verbose)
{
	ALCdevice *device = alcGetContextsDevice(alcGetCurrentContext());
	std::ostringstream version;
	version <<
		"OpenAL";
	if (verbose)
	{
		version <<
			", version " <<
			ll_safe_string(alGetString(AL_VERSION)) <<
			" / " <<
			ll_safe_string(alGetString(AL_VENDOR)) <<
			" / " <<
			ll_safe_string(alGetString(AL_RENDERER));
		if (device)
			version <<
				": " <<
				ll_safe_string(alcGetString(device,
				    ALC_DEFAULT_DEVICE_SPECIFIER));
	}
	return version.str();
}
void LLAudioEngine_OpenAL::allocateListener()
{
	mListenerp = (LLListener *) new LLListener_OpenAL();
	if(!mListenerp)
	{
		LL_WARNS() << "LLAudioEngine_OpenAL::allocateListener() Listener creation failed" << LL_ENDL;
	}
}
void LLAudioEngine_OpenAL::shutdown()
{
	LL_INFOS() << "About to LLAudioEngine::shutdown()" << LL_ENDL;
	LLAudioEngine::shutdown();
	LL_INFOS() << "About to alutExit()" << LL_ENDL;
	if(!alutExit())
	{
		LL_WARNS() << "Nuts." << LL_ENDL;
		LL_WARNS() << "LLAudioEngine_OpenAL::shutdown() ALUT shutdown failed: " << alutGetErrorString (alutGetError ()) << LL_ENDL;
	}
	LL_INFOS() << "LLAudioEngine_OpenAL::shutdown() OpenAL successfully shut down" << LL_ENDL;
	delete mListenerp;
	mListenerp = NULL;
}
LLAudioBuffer *LLAudioEngine_OpenAL::createBuffer()
{
	return new LLAudioBufferOpenAL();
}
LLAudioChannel *LLAudioEngine_OpenAL::createChannel()
{
	return new LLAudioChannelOpenAL();
}
void LLAudioEngine_OpenAL::setInternalGain(F32 gain)
{
	alListenerf(AL_GAIN, gain);
}
LLAudioChannelOpenAL::LLAudioChannelOpenAL()
	:
	mALSource(AL_NONE),
	mLastSamplePos(0)
{
	alGenSources(1, &mALSource);
}
LLAudioChannelOpenAL::~LLAudioChannelOpenAL()
{
	cleanup();
	alDeleteSources(1, &mALSource);
}
void LLAudioChannelOpenAL::cleanup()
{
	LLAudioChannel::cleanup();
	alSourceStop(mALSource);
	alSourcei(mALSource, AL_BUFFER, AL_NONE);
	mLastSamplePos = 0;
}
void LLAudioChannelOpenAL::play()
{
	if (mALSource == AL_NONE)
	{
		LL_WARNS() << "Playing without a mALSource, aborting" << LL_ENDL;
		return;
	}
	if(isPlaying())
	{
		alSourceStop(mALSource);
	}
	alSourcePlay(mALSource);
	getSource()->setPlayedOnce(true);
}
void LLAudioChannelOpenAL::playSynced(LLAudioChannel *channelp)
{
	if (channelp)
	{
		LLAudioChannelOpenAL *masterchannelp =
			(LLAudioChannelOpenAL*)channelp;
		if (mALSource != AL_NONE &&
		    masterchannelp->mALSource != AL_NONE)
		{
			ALfloat master_offset;
			alGetSourcef(masterchannelp->mALSource, AL_SEC_OFFSET,
				     &master_offset);
			LL_INFOS() << "Syncing with master at " << master_offset
				<< "sec" << LL_ENDL;
			alSourcef(mALSource, AL_SEC_OFFSET, master_offset);
		}
	}
	play();
}
bool LLAudioChannelOpenAL::isPlaying()
{
	if (mALSource != AL_NONE)
	{
		ALint state;
		alGetSourcei(mALSource, AL_SOURCE_STATE, &state);
		if(state == AL_PLAYING)
		{
			return true;
		}
	}
	return false;
}
bool LLAudioChannelOpenAL::updateBuffer()
{
	if (LLAudioChannel::updateBuffer())
	{
		LLAudioBufferOpenAL *bufferp = (LLAudioBufferOpenAL *)mCurrentSourcep->getCurrentBuffer();
		ALuint buffer = bufferp->getBuffer();
		alSourcei(mALSource, AL_BUFFER, buffer);
		mLastSamplePos = 0;
	}
	if (mCurrentSourcep)
	{
		alSourcef(mALSource, AL_GAIN,
			  mCurrentSourcep->getGain() * getSecondaryGain());
		alSourcei(mALSource, AL_LOOPING,
			  mCurrentSourcep->isLoop() ? AL_TRUE : AL_FALSE);
		alSourcef(mALSource, AL_ROLLOFF_FACTOR,
			  gAudiop->mListenerp->getRolloffFactor());
	}
	return true;
}
void LLAudioChannelOpenAL::updateLoop()
{
	if (mALSource == AL_NONE)
	{
		return;
	}
	ALint cur_pos;
	alGetSourcei(mALSource, AL_SAMPLE_OFFSET, &cur_pos);
	if (cur_pos < mLastSamplePos)
	{
		mLoopedThisFrame = true;
	}
	mLastSamplePos = cur_pos;
}
void LLAudioChannelOpenAL::update3DPosition()
{
	if(!mCurrentSourcep)
	{
		return;
	}
	if (mCurrentSourcep->isAmbient())
	{
		alSource3f(mALSource, AL_POSITION, 0.0, 0.0, 0.0);
		alSource3f(mALSource, AL_VELOCITY, 0.0, 0.0, 0.0);
		alSourcei (mALSource, AL_SOURCE_RELATIVE, AL_TRUE);
	} else {
		LLVector3 float_pos;
		float_pos.setVec(mCurrentSourcep->getPositionGlobal());
		alSourcefv(mALSource, AL_POSITION, float_pos.mV);
		alSourcefv(mALSource, AL_VELOCITY, mCurrentSourcep->getVelocity().mV);
		alSourcei (mALSource, AL_SOURCE_RELATIVE, AL_FALSE);
	}
	alSourcef(mALSource, AL_GAIN, mCurrentSourcep->getGain() * getSecondaryGain());
}
LLAudioBufferOpenAL::LLAudioBufferOpenAL() : LLAudioBuffer()
{
	mALBuffer = AL_NONE;
}
LLAudioBufferOpenAL::~LLAudioBufferOpenAL()
{
	cleanup();
}
void LLAudioBufferOpenAL::cleanup()
{
	if(mALBuffer != AL_NONE)
	{
		alGetError();
		alDeleteBuffers(1, &mALBuffer);
		ALenum error = alutGetError();
		if(ALC_NO_ERROR != error)
		{
			LL_WARNS("OpenAL") << "Error: " << alutGetErrorString( error ) << " when cleaning up a buffer" << LL_ENDL;
		}
		mALBuffer = AL_NONE;
	}
}
bool LLAudioBufferOpenAL::loadWAV(const std::string& filename)
{
	cleanup();
	mALBuffer = alutCreateBufferFromFile(filename.c_str());
	if(mALBuffer == AL_NONE)
	{
		ALenum error = alutGetError();
		if (gDirUtilp->fileExists(filename))
		{
			LL_WARNS() <<
				"LLAudioBufferOpenAL::loadWAV() Error loading "
				<< filename
				<< " " << alutGetErrorString(error) << LL_ENDL;
		}
		else
		{
			LL_DEBUGS() <<
				"LLAudioBufferOpenAL::loadWAV() Error loading "
				 << filename
				 << " " << alutGetErrorString(error) << LL_ENDL;
		}
		return false;
	}
	return true;
}
U32 LLAudioBufferOpenAL::getLength()
{
	if(mALBuffer == AL_NONE)
	{
		return 0;
	}
	ALint length;
	alGetBufferi(mALBuffer, AL_SIZE, &length);
	return length / 2;
}
bool LLAudioEngine_OpenAL::initWind()
{
	ALenum error;
	LL_INFOS() << "LLAudioEngine_OpenAL::initWind() start" << LL_ENDL;
	mNumEmptyWindALBuffers = MAX_NUM_WIND_BUFFERS;
	alGetError();
	alGenSources(1,&mWindSource);
	if((error=alGetError()) != AL_NO_ERROR)
	{
		LL_WARNS() << "LLAudioEngine_OpenAL::initWind() Error creating wind sources: "<<error<<LL_ENDL;
	}
	mWindGen = new LLWindGen<WIND_SAMPLE_T>;
	mWindBufFreq = mWindGen->getInputSamplingRate();
	mWindBufSamples = llceil(mWindBufFreq * WIND_BUFFER_SIZE_SEC);
	mWindBufBytes = mWindBufSamples * 2 * sizeof(WIND_SAMPLE_T);
	mWindBuf = new WIND_SAMPLE_T [mWindBufSamples * 2 ];
	if(mWindBuf==NULL)
	{
		LL_ERRS() << "LLAudioEngine_OpenAL::initWind() Error creating wind memory buffer" << LL_ENDL;
		return false;
	}
	LL_INFOS() << "LLAudioEngine_OpenAL::initWind() done" << LL_ENDL;
	return true;
}
void LLAudioEngine_OpenAL::cleanupWind()
{
	LL_INFOS() << "LLAudioEngine_OpenAL::cleanupWind()" << LL_ENDL;
	if (mWindSource != AL_NONE)
	{
		alSourceStop(mWindSource);
		ALint processed;
		alGetSourcei(mWindSource, AL_BUFFERS_PROCESSED, &processed);
		while (processed--)
		{
			ALuint buffer = AL_NONE;
			alSourceUnqueueBuffers(mWindSource, 1, &buffer);
			alDeleteBuffers(1, &buffer);
		}
		alDeleteSources(1, &mWindSource);
		mWindSource = AL_NONE;
	}
	delete[] mWindBuf;
	mWindBuf = NULL;
	delete mWindGen;
	mWindGen = NULL;
}
void LLAudioEngine_OpenAL::updateWind(LLVector3 wind_vec, F32 camera_altitude)
{
	LLVector3 wind_pos;
	F64 pitch;
	F64 center_freq;
	ALenum error;
	ALuint *buffers = NULL;
	if (!mEnableWind)
		return;
	if(!mWindBuf)
		return;
	if (mWindUpdateTimer.checkExpirationAndReset(LL_WIND_UPDATE_INTERVAL))
	{
		wind_vec.setVec(-wind_vec.mV[1], wind_vec.mV[2], -wind_vec.mV[0]);
		pitch = 1.0 + mapWindVecToPitch(wind_vec);
		center_freq = 80.0 * pow(pitch,2.5*(mapWindVecToGain(wind_vec)+1.0));
		mWindGen->mTargetFreq = (F32)center_freq;
		mWindGen->mTargetGain = (F32)mapWindVecToGain(wind_vec) * mMaxWindGain;
		mWindGen->mTargetPanGainR = (F32)mapWindVecToPan(wind_vec);
		alSourcei(mWindSource, AL_LOOPING, AL_FALSE);
		alSource3f(mWindSource, AL_POSITION, 0.0, 0.0, 0.0);
		alSource3f(mWindSource, AL_VELOCITY, 0.0, 0.0, 0.0);
		alSourcef(mWindSource, AL_ROLLOFF_FACTOR, 0.0);
		alSourcei(mWindSource, AL_SOURCE_RELATIVE, AL_TRUE);
	}
	ALint processed, queued, unprocessed;
	alGetSourcei(mWindSource, AL_BUFFERS_PROCESSED, &processed);
	alGetSourcei(mWindSource, AL_BUFFERS_QUEUED, &queued);
	unprocessed = queued - processed;
	mNumEmptyWindALBuffers = llmin(mNumEmptyWindALBuffers + processed * 3 - unprocessed, MAX_NUM_WIND_BUFFERS-unprocessed);
	mNumEmptyWindALBuffers = llmax(mNumEmptyWindALBuffers, 0);
	buffers = new ALuint[processed];
	alGetError();
	alSourceUnqueueBuffers(mWindSource, processed, &buffers[0]);
	error = alGetError();
	if(error != AL_NO_ERROR)
	{
		LL_WARNS() << "LLAudioEngine_OpenAL::updateWind() error swapping (unqueuing) buffers" << LL_ENDL;
	}
	else
	{
		alDeleteBuffers(processed, &buffers[0]);
	}
	delete[] buffers;
	buffers = NULL;
	unprocessed += mNumEmptyWindALBuffers;
	buffers = new ALuint[mNumEmptyWindALBuffers];
	alGetError();
	alGenBuffers(mNumEmptyWindALBuffers,&buffers[0]);
	if((error=alGetError()) != AL_NO_ERROR)
	{
		LL_WARNS() << "LLAudioEngine_OpenAL::updateWind() Error creating wind buffer: " << error << LL_ENDL;
	}
	int errors = 0;
	for(int i = 0; i < mNumEmptyWindALBuffers; i++)
	{
		alBufferData(buffers[i],
					AL_FORMAT_STEREO16,
					mWindGen->windGenerate(mWindBuf,
							mWindBufSamples),
					mWindBufBytes,
					mWindBufFreq);
		error = alGetError();
		if(error != AL_NO_ERROR)
		{
			LL_WARNS() << "LLAudioEngine_OpenAL::updateWind() error swapping (bufferdata) buffers" << LL_ENDL;
			errors++;
		}
	}
	alSourceQueueBuffers(mWindSource, mNumEmptyWindALBuffers, &buffers[0]);
	error = alGetError();
	if(error != AL_NO_ERROR)
	{
		LL_WARNS() << "LLAudioEngine_OpenAL::updateWind() error swapping (queuing) buffers" << LL_ENDL;
	}
	mNumEmptyWindALBuffers = errors;
	delete[] buffers;
	buffers = NULL;
	ALint playing;
	alGetSourcei(mWindSource, AL_SOURCE_STATE, &playing);
	if(playing != AL_PLAYING)
	{
		alSourcePlay(mWindSource);
		LL_DEBUGS() << "Wind had stopped - probably ran out of buffers - restarting: " << (unprocessed+mNumEmptyWindALBuffers) << " now queued." << LL_ENDL;
	}
}
