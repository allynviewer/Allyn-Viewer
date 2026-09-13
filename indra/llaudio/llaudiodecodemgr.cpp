/** 
 * @file llaudiodecodemgr.cpp
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
#include "llaudiodecodemgr.h"
#include "llaudioengine.h"
#include "lllfsthread.h"
#include "llvfile.h"
#include "llstring.h"
#include "lldir.h"
#include "llendianswizzle.h"
#include "llassetstorage.h"
#include "llrefcount.h"
#include "llvorbisencode.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
#include <iterator>
#include <deque>
extern LLAudioEngine *gAudiop;
LLAudioDecodeMgr *gAudioDecodeMgrp = NULL;
static const S32 WAV_HEADER_SIZE = 44;
class LLVorbisDecodeState : public LLRefCount
{
public:
	class WriteResponder : public LLLFSThread::Responder
	{
	public:
		WriteResponder(LLVorbisDecodeState* decoder) : mDecoder(decoder) {}
		~WriteResponder() {}
		void completed(S32 bytes)
		{
			mDecoder->ioComplete(bytes);
		}
		LLPointer<LLVorbisDecodeState> mDecoder;
	};
	LLVorbisDecodeState(const LLUUID &uuid, const std::string &out_filename);
	BOOL initDecode();
	BOOL decodeSection();
	BOOL finishDecode();
	void flushBadFile();
	void ioComplete(S32 bytes)			{ mBytesRead = bytes; }
	BOOL isValid() const				{ return mValid; }
	BOOL isDone() const					{ return mDone; }
	const LLUUID &getUUID() const		{ return mUUID; }
protected:
	virtual ~LLVorbisDecodeState();
	BOOL mValid;
	BOOL mDone;
	LLAtomicS32 mBytesRead;
	LLUUID mUUID;
	std::vector<U8> mWAVBuffer;
#if !defined(USE_WAV_VFILE)
	std::string mOutFilename;
	LLLFSThread::handle_t mFileHandle;
#endif
	LLVFile *mInFilep;
	OggVorbis_File mVF;
	S32 mCurrentSection;
};
size_t vfs_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	LLVFile *file = (LLVFile *)datasource;
	if (file->read((U8*)ptr, (S32)(size * nmemb)))
	{
		S32 read = file->getLastBytesRead();
		return  read / size;
	}
	else
	{
		return 0;
	}
}
S32 vfs_seek(void *datasource, ogg_int64_t offset, S32 whence)
{
	LLVFile *file = (LLVFile *)datasource;
	if (offset > S32_MAX)
	{
		return -1;
	}
	S32 origin;
	switch (whence) {
	case SEEK_SET:
		origin = 0;
		break;
	case SEEK_END:
		origin = file->getSize();
		break;
	case SEEK_CUR:
		origin = -1;
		break;
	default:
		LL_ERRS("AudioEngine") << "Invalid whence argument to vfs_seek" << LL_ENDL;
		return -1;
	}
	if (file->seek((S32)offset, origin))
	{
		return 0;
	}
	else
	{
		return -1;
	}
}
S32 vfs_close (void *datasource)
{
	LLVFile *file = (LLVFile *)datasource;
	delete file;
	return 0;
}
long vfs_tell (void *datasource)
{
	LLVFile *file = (LLVFile *)datasource;
	return file->tell();
}
LLVorbisDecodeState::LLVorbisDecodeState(const LLUUID &uuid, const std::string &out_filename) :
	mValid(FALSE), mDone(FALSE), mBytesRead(-1), mUUID(uuid),
#if !defined(USE_WAV_VFILE)
	mOutFilename(out_filename), mFileHandle(LLLFSThread::nullHandle()),
#endif
	mInFilep(NULL), mCurrentSection(0)
{
}
LLVorbisDecodeState::~LLVorbisDecodeState()
{
	if (!mDone)
	{
		delete mInFilep;
		mInFilep = NULL;
	}
}
BOOL LLVorbisDecodeState::initDecode()
{
	ov_callbacks vfs_callbacks;
	vfs_callbacks.read_func = vfs_read;
	vfs_callbacks.seek_func = vfs_seek;
	vfs_callbacks.close_func = vfs_close;
	vfs_callbacks.tell_func = vfs_tell;
	LL_DEBUGS("AudioEngine") << "Initing decode from vfile: " << mUUID << LL_ENDL;
	mInFilep = new LLVFile(gVFS, mUUID, LLAssetType::AT_SOUND);
	if (!mInFilep || !mInFilep->getSize())
	{
		LL_WARNS("AudioEngine") << "unable to open vorbis source vfile for reading" << LL_ENDL;
		delete mInFilep;
		mInFilep = NULL;
		return FALSE;
	}
	S32 r = ov_open_callbacks(mInFilep, &mVF, NULL, 0, vfs_callbacks);
	if(r < 0)
	{
		LL_WARNS("AudioEngine") << r << " Input to vorbis decode does not appear to be an Ogg bitstream: " << mUUID << LL_ENDL;
		return(FALSE);
	}
	S32 sample_count = (S32)ov_pcm_total(&mVF, -1);
	size_t size_guess = (size_t)sample_count;
	vorbis_info* vi = ov_info(&mVF, -1);
	size_guess *= (vi? vi->channels : 1);
	size_guess *= 2;
	size_guess += 2048;
	bool abort_decode = false;
	if (vi)
	{
		if( vi->channels < 1 || vi->channels > LLVORBIS_CLIP_MAX_CHANNELS )
		{
			abort_decode = true;
			LL_WARNS("AudioEngine") << "Bad channel count: " << vi->channels << LL_ENDL;
		}
	}
	else
	{
		abort_decode = true;
		LL_WARNS("AudioEngine") << "No default bitstream found" << LL_ENDL;
	}
	if(size_guess >= 157286400)
	{
		LL_WARNS() << "Bad sound caught by zmagic" << LL_ENDL;
		abort_decode = true;
	}
	else if(!gAudiop->getAllowLargeSounds())
	{
	if( (size_t)sample_count > LLVORBIS_CLIP_REJECT_SAMPLES ||
	    (size_t)sample_count <= 0)
	{
		abort_decode = true;
		LL_WARNS("AudioEngine") << "Illegal sample count: " << sample_count << LL_ENDL;
	}
	if( size_guess > LLVORBIS_CLIP_REJECT_SIZE ||
	    size_guess < 0)
	{
		abort_decode = true;
		LL_WARNS("AudioEngine") << "Illegal sample size: " << size_guess << LL_ENDL;
	}
	}
	if( abort_decode )
	{
		LL_WARNS("AudioEngine") << "Canceling initDecode. Bad asset: " << mUUID << LL_ENDL;
		vorbis_comment* comment = ov_comment(&mVF,-1);
		if (comment && comment->vendor)
		{
			LL_WARNS("AudioEngine") << "Bad asset encoded by: " << comment->vendor << LL_ENDL;
		}
		delete mInFilep;
		mInFilep = NULL;
		return FALSE;
	}
	try
	{
		mWAVBuffer.reserve(size_guess);
		mWAVBuffer.resize(WAV_HEADER_SIZE);
	}
	catch(std::bad_alloc)
	{
		LL_WARNS() << "bad_alloc" << LL_ENDL;
		if(mInFilep)
		{
			delete mInFilep;
			mInFilep = NULL;
		}
		return FALSE;
	}
	{
		mWAVBuffer[0] = 0x52;
		mWAVBuffer[1] = 0x49;
		mWAVBuffer[2] = 0x46;
		mWAVBuffer[3] = 0x46;
		mWAVBuffer[4] = 0x00;
		mWAVBuffer[5] = 0x00;
		mWAVBuffer[6] = 0x00;
		mWAVBuffer[7] = 0x00;
		mWAVBuffer[8] = 0x57;
		mWAVBuffer[9] = 0x41;
		mWAVBuffer[10] = 0x56;
		mWAVBuffer[11] = 0x45;
		mWAVBuffer[12] = 0x66;
		mWAVBuffer[13] = 0x6D;
		mWAVBuffer[14] = 0x74;
		mWAVBuffer[15] = 0x20;
		mWAVBuffer[16] = 0x10;
		mWAVBuffer[17] = 0x00;
		mWAVBuffer[18] = 0x00;
		mWAVBuffer[19] = 0x00;
		mWAVBuffer[20] = 0x01;
		mWAVBuffer[21] = 0x00;
		mWAVBuffer[22] = 0x01;
		mWAVBuffer[23] = 0x00;
		mWAVBuffer[24] = 0x44;
		mWAVBuffer[25] = 0xAC;
		mWAVBuffer[26] = 0x00;
		mWAVBuffer[27] = 0x00;
		mWAVBuffer[28] = 0x88;
		mWAVBuffer[29] = 0x58;
		mWAVBuffer[30] = 0x01;
		mWAVBuffer[31] = 0x00;
		mWAVBuffer[32] = 0x02;
		mWAVBuffer[33] = 0x00;
		mWAVBuffer[34] = 0x10;
		mWAVBuffer[35] = 0x00;
		mWAVBuffer[36] = 0x64;
		mWAVBuffer[37] = 0x61;
		mWAVBuffer[38] = 0x74;
		mWAVBuffer[39] = 0x61;
		mWAVBuffer[40] = 0x00;
		mWAVBuffer[41] = 0x00;
		mWAVBuffer[42] = 0x00;
		mWAVBuffer[43] = 0x00;
	}
	return TRUE;
}
BOOL LLVorbisDecodeState::decodeSection()
{
	if (!mInFilep)
	{
		LL_WARNS("AudioEngine") << "No VFS file to decode in vorbis!" << LL_ENDL;
		return TRUE;
	}
	if (mDone)
	{
		return TRUE;
	}
	char pcmout[4096];
	BOOL eof = FALSE;
	long ret=ov_read(&mVF, pcmout, sizeof(pcmout), 0, 2, 1, &mCurrentSection);
	if (ret == 0)
	{
		eof = TRUE;
		mDone = TRUE;
		mValid = TRUE;
	}
	else if (ret < 0)
	{
		LL_WARNS("AudioEngine") << "BAD vorbis decode in decodeSection." << LL_ENDL;
		mValid = FALSE;
		mDone = TRUE;
		return TRUE;
	}
	else
	{
		std::copy(pcmout, pcmout+ret, std::back_inserter(mWAVBuffer));
	}
	return eof;
}
BOOL LLVorbisDecodeState::finishDecode()
{
	if (!isValid())
	{
		LL_WARNS("AudioEngine") << "Bogus vorbis decode state for " << getUUID() << ", aborting!" << LL_ENDL;
		return TRUE;
	}
#if !defined(USE_WAV_VFILE)
	if (mFileHandle == LLLFSThread::nullHandle())
#endif
	{
		ov_clear(&mVF);
		S32 data_length = mWAVBuffer.size() - WAV_HEADER_SIZE;
		mWAVBuffer[40] = (data_length) & 0x000000FF;
		mWAVBuffer[41] = (data_length >> 8) & 0x000000FF;
		mWAVBuffer[42] = (data_length >> 16) & 0x000000FF;
		mWAVBuffer[43] = (data_length >> 24) & 0x000000FF;
		data_length += 36;
		mWAVBuffer[4] = (data_length) & 0x000000FF;
		mWAVBuffer[5] = (data_length >> 8) & 0x000000FF;
		mWAVBuffer[6] = (data_length >> 16) & 0x000000FF;
		mWAVBuffer[7] = (data_length >> 24) & 0x000000FF;
		{
			S16 *samplep;
			S32 i;
			S32 fade_length;
			char pcmout[4096];
			fade_length = llmin((S32)128,(S32)(data_length-36)/8);
			if((S32)mWAVBuffer.size() > (WAV_HEADER_SIZE + 2* fade_length))
			{
				memcpy(pcmout, &mWAVBuffer[WAV_HEADER_SIZE], (2 * fade_length));
			}
			llendianswizzle(&pcmout, 2, fade_length);
			samplep = (S16 *)pcmout;
			for (i = 0 ;i < fade_length; i++)
			{
				*samplep = llfloor((F32)*samplep * ((F32)i/(F32)fade_length));
				samplep++;
			}
			llendianswizzle(&pcmout, 2, fade_length);
			if((WAV_HEADER_SIZE+(2 * fade_length)) < (S32)mWAVBuffer.size())
			{
				memcpy(&mWAVBuffer[WAV_HEADER_SIZE], pcmout, (2 * fade_length));
			}
			S32 near_end = mWAVBuffer.size() - (2 * fade_length);
			if ((S32)mWAVBuffer.size() > ( near_end + 2* fade_length))
			{
				memcpy(pcmout, &mWAVBuffer[near_end], (2 * fade_length));
			}
			llendianswizzle(&pcmout, 2, fade_length);
			samplep = (S16 *)pcmout;
			for (i = fade_length-1 ; i >=  0; i--)
			{
				*samplep = llfloor((F32)*samplep * ((F32)i/(F32)fade_length));
				samplep++;
			}
			llendianswizzle(&pcmout, 2, fade_length);
			if (near_end + (2 * fade_length) < (S32)mWAVBuffer.size())
			{
				memcpy(&mWAVBuffer[near_end], pcmout, (2 * fade_length));
			}
		}
		if (36 == data_length)
		{
			LL_WARNS("AudioEngine") << "BAD Vorbis decode in finishDecode!" << LL_ENDL;
			mValid = FALSE;
			return TRUE;
		}
#if !defined(USE_WAV_VFILE)
		mBytesRead = -1;
		mFileHandle = LLLFSThread::sLocal->write(mOutFilename, &mWAVBuffer[0], 0, mWAVBuffer.size(),
							 new WriteResponder(this));
#endif
	}
	if (mFileHandle != LLLFSThread::nullHandle())
	{
		if (mBytesRead >= 0)
		{
			if (mBytesRead == 0)
			{
				LL_WARNS("AudioEngine") << "Unable to write file in LLVorbisDecodeState::finishDecode" << LL_ENDL;
				mValid = FALSE;
				return TRUE;
			}
		}
		else
		{
			return FALSE;
		}
	}
	mDone = TRUE;
#if defined(USE_WAV_VFILE)
	LLVFile output(gVFS, mUUID, LLAssetType::AT_SOUND_WAV);
	output.write(&mWAVBuffer[0], mWAVBuffer.size());
#endif
	LL_DEBUGS("AudioEngine") << "Finished decode for " << getUUID() << LL_ENDL;
	return TRUE;
}
void LLVorbisDecodeState::flushBadFile()
{
	if (mInFilep)
	{
		LL_WARNS("AudioEngine") << "Flushing bad vorbis file from VFS for " << mUUID << LL_ENDL;
		mInFilep->remove();
	}
}
class LLAudioDecodeMgr::Impl
{
	friend class LLAudioDecodeMgr;
public:
	Impl() {};
	~Impl() {};
	void processQueue(const F32 num_secs = 0.005);
protected:
	std::deque<LLUUID> mDecodeQueue;
	LLPointer<LLVorbisDecodeState> mCurrentDecodep;
};
void LLAudioDecodeMgr::Impl::processQueue(const F32 num_secs)
{
	LLUUID uuid;
	LLTimer decode_timer;
	BOOL done = FALSE;
	while (!done)
	{
		if (mCurrentDecodep)
		{
			BOOL res = false;
			try{
			while(!(res = mCurrentDecodep->decodeSection()) && (decode_timer.getElapsedTimeF32() < num_secs))
			{
			}
			}catch(std::bad_alloc){LL_ERRS() << "bad_alloc whilst decoding" << LL_ENDL;}
			if (mCurrentDecodep->isDone() && !mCurrentDecodep->isValid())
			{
				LL_WARNS("AudioEngine") << mCurrentDecodep->getUUID() << " has invalid vorbis data, aborting decode" << LL_ENDL;
				mCurrentDecodep->flushBadFile();
				if (gAudiop)
				{
					LLAudioData *adp = gAudiop->getAudioData(mCurrentDecodep->getUUID());
					if(adp)
					{
						adp->setLoadState(LLAudioData::STATE_LOAD_ERROR);
					}
				}
				mCurrentDecodep = NULL;
				done = TRUE;
			}
			if (!res)
			{
				done = TRUE;
			}
			else if (mCurrentDecodep)
			{
				if (gAudiop && mCurrentDecodep->finishDecode())
				{
					LLAudioData *adp = gAudiop->getAudioData(mCurrentDecodep->getUUID());
					if (!adp)
					{
						LL_WARNS("AudioEngine") << "Missing LLAudioData for decode of " << mCurrentDecodep->getUUID() << LL_ENDL;
					}
					else if (mCurrentDecodep->isValid() && mCurrentDecodep->isDone())
					{
						adp->setLoadState(LLAudioData::STATE_LOAD_READY);
					}
					else
					{
						adp->setLoadState(LLAudioData::STATE_LOAD_ERROR);
						LL_INFOS("AudioEngine") << "Vorbis decode failed for " << mCurrentDecodep->getUUID() << LL_ENDL;
					}
					mCurrentDecodep = NULL;
				}
				done = TRUE;
			}
		}
		if (!done)
		{
			if (mDecodeQueue.empty())
			{
				done = TRUE;
			}
			else
			{
				LLUUID uuid;
				uuid = mDecodeQueue.front();
				mDecodeQueue.pop_front();
				if (!gAudiop || gAudiop->hasDecodedFile(uuid))
				{
					continue;
				}
				LL_DEBUGS() << "Decoding " << uuid << " from audio queue!" << LL_ENDL;
				std::string uuid_str;
				std::string d_path;
				LLTimer timer;
				timer.reset();
				uuid.toString(uuid_str);
				d_path = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_str) + ".dsf";
				mCurrentDecodep = new LLVorbisDecodeState(uuid, d_path);
				if (!mCurrentDecodep->initDecode())
				{
					LLAudioData *adp = gAudiop->getAudioData(uuid);
					if(adp)
					{
						adp->setLoadState(LLAudioData::STATE_LOAD_ERROR);
					}
					mCurrentDecodep = NULL;
				}
			}
		}
	}
}
LLAudioDecodeMgr::LLAudioDecodeMgr()
{
	mImpl = new Impl;
}
LLAudioDecodeMgr::~LLAudioDecodeMgr()
{
	delete mImpl;
}
void LLAudioDecodeMgr::processQueue(const F32 num_secs)
{
	mImpl->processQueue(num_secs);
}
bool LLAudioDecodeMgr::addDecodeRequest(const LLUUID &uuid)
{
	if(uuid.isNull())
	{
		return true;
	}
	if (gAudiop && gAudiop->hasDecodedFile(uuid))
	{
		LL_DEBUGS("AudioEngine") << "addDecodeRequest for " << uuid << " has decoded file already" << LL_ENDL;
		return true;
	}
	if (gAssetStorage && gAssetStorage->hasLocalAsset(uuid, LLAssetType::AT_SOUND))
	{
		LL_DEBUGS("AudioEngine") << "addDecodeRequest for " << uuid << " has local asset file already" << LL_ENDL;
		mImpl->mDecodeQueue.push_back(uuid);
		return true;
	}
	LL_DEBUGS("AudioEngine") << "addDecodeRequest for " << uuid << " no file available" << LL_ENDL;
	return false;
}
