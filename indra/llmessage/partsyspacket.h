/** 
 * @file partsyspacket.h
 * @brief Object for packing particle system initialization parameters
 * before sending them over the network
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#ifndef LL_PARTSYSPACKET_H
#define LL_PARTSYSPACKET_H
#include "lluuid.h"
const U64 PART_SYS_MAX_TIME_IN_USEC = 1000000;
struct LLPartInitData {
	F32 bounce_b;
	F32 scale_range[4];
	F32 alpha_range[4];
	F32 vel_offset[3];
	F32 mDistBeginFadeout;
	F32 mDistEndFadeout;
	LLUUID mImageUuid;
	U8 mFlags[8];
	U8 createMe;
	F32 diffEqAlpha[3];
	F32 diffEqScale[3];
	U8	maxParticles;
	U8	initialParticles;
	F32	killPlaneZ;
	F32 killPlaneNormal[3];
	F32	bouncePlaneZ;
	F32 bouncePlaneNormal[3];
	F32	spawnRange;
	F32	spawnFrequency;
	F32	spawnFreqencyRange;
	F32	spawnDirection[3];
	F32	spawnDirectionRange;
	F32	spawnVelocity;
	F32	spawnVelocityRange;
	F32	speedLimit;
	F32	windWeight;
	F32	currentGravity[3];
	F32	gravityWeight;
	F32	globalLifetime;
	F32	individualLifetime;
	F32	individualLifetimeRange;
	F32	alphaDecay;
	F32	scaleDecay;
	F32	distanceDeath;
	F32	dampMotionFactor;
	F32 windDiffusionFactor[3];
};
const int PART_SYS_NO_Z_BUFFER_BYTE = 0;
const int PART_SYS_NO_Z_BUFFER_BIT = 2;
const int PART_SYS_SLOW_ANIM_BYTE = 0;
const int PART_SYS_SLOW_ANIM_BIT = 1;
const int PART_SYS_FOLLOW_VEL_BYTE = 0;
const int PART_SYS_FOLLOW_VEL_BIT = 4;
const int PART_SYS_IS_LIGHT_BYTE = 0;
const int PART_SYS_IS_LIGHT_BIT = 8;
const int PART_SYS_SPAWN_COPY_BYTE = 0;
const int PART_SYS_SPAWN_COPY_BIT = 0x10;
const int PART_SYS_COPY_VEL_BYTE = 0;
const int PART_SYS_COPY_VEL_BIT = 0x20;
const int PART_SYS_INVISIBLE_BYTE = 0;
const int PART_SYS_INVISIBLE_BIT = 0x40;
const int PART_SYS_ADAPT_TO_FRAMERATE_BYTE = 0;
const int PART_SYS_ADAPT_TO_FRAMERATE_BIT = 0x80;
const U16 MAX_PART_SYS_PACKET_SIZE = 256;
const U8 PART_SYS_KILL_P_MASK			= 0x02;
const U8 PART_SYS_BOUNCE_P_MASK			= 0x04;
const U8 PART_SYS_BOUNCE_B_MASK			= 0x08;
const U8 PART_SYS_VEL_OFFSET_MASK		= 0x10;
const U8 PART_SYS_ALPHA_SCALE_DIFF_MASK = 0x20;
const U8 PART_SYS_SCALE_RANGE_MASK		= 0x40;
const U8 PART_SYS_M_IMAGE_UUID_MASK		= 0x80;
const U8 PART_SYS_BYTE_3_ALPHA_MASK		= 0x01;
const U8 PART_SYS_BYTE_SPAWN_MASK		= 0x01;
const U8 PART_SYS_BYTE_ENVIRONMENT_MASK	= 0x02;
const U8 PART_SYS_BYTE_LIFESPAN_MASK	= 0x04;
const U8 PART_SYS_BYTE_DECAY_DAMP_MASK	= 0x08;
const U8 PART_SYS_BYTE_WIND_DIFF_MASK	= 0x10;
const int PART_SYS_ACTION_BYTE = 1;
const U8 PART_SYS_SPAWN 						= 0x01;
const U8 PART_SYS_BOUNCE 						= 0x02;
const U8 PART_SYS_AFFECTED_BY_WIND 				= 0x04;
const U8 PART_SYS_AFFECTED_BY_GRAVITY			= 0x08;
const U8 PART_SYS_EVALUATE_WIND_PER_PARTICLE 	= 0x10;
const U8 PART_SYS_DAMP_MOTION 					= 0x20;
const U8 PART_SYS_WIND_DIFFUSION 				= 0x40;
const int PART_SYS_KILL_BYTE = 2;
const U8 PART_SYS_KILL_PLANE					= 0x01;
const U8 PART_SYS_GLOBAL_DIE 					= 0x02;
const U8 PART_SYS_DISTANCE_DEATH 				= 0x04;
const U8 PART_SYS_TIME_DEATH 					= 0x08;
void gSetInitDataDefaults(LLPartInitData *setMe);
class LLPartSysCompressedPacket
{
public:
	LLPartSysCompressedPacket();
	~LLPartSysCompressedPacket();
	BOOL	fromLLPartInitData(LLPartInitData *in, U32 &bytesUsed);
	BOOL	toLLPartInitData(LLPartInitData *out, U32 *bytesUsed);
	BOOL	fromUnsignedBytes(U8 *in, U32 bytesUsed);
	BOOL	toUnsignedBytes(U8 *out);
	U32		bufferSize();
	U8		*getBytePtr();
protected:
	U8 mData[MAX_PART_SYS_PACKET_SIZE];
	U32 mNumBytes;
	LLPartInitData mDefaults;
	LLPartInitData mWorkingCopy;
protected:
	void	writeFlagByte(LLPartInitData *in);
	U32		writeKill_p(LLPartInitData *in, U32 startByte);
	U32		writeBounce_p(LLPartInitData *in, U32 startByte);
	U32		writeBounce_b(LLPartInitData *in, U32 startByte);
	U32		writeAlphaScaleDiffEqn_range(LLPartInitData *in, U32 startByte);
	U32		writeScale_range(LLPartInitData *in, U32 startByte);
	U32		writeAlpha_range(LLPartInitData *in, U32 startByte);
	U32		writeUUID(LLPartInitData *in, U32 startByte);
	U32		writeVelocityOffset(LLPartInitData *in, U32 startByte);
	U32		writeSpawn(LLPartInitData *in, U32 startByte);
	U32		writeEnvironment(LLPartInitData *in, U32 startByte);
	U32		writeLifespan(LLPartInitData *in, U32 startByte);
	U32		writeDecayDamp(LLPartInitData *in, U32 startByte);
	U32		writeWindDiffusionFactor(LLPartInitData *in, U32 startByte);
	U32		readKill_p(LLPartInitData *in, U32 startByte);
	U32		readBounce_p(LLPartInitData *in, U32 startByte);
	U32		readBounce_b(LLPartInitData *in, U32 startByte);
	U32		readAlphaScaleDiffEqn_range(LLPartInitData *in, U32 startByte);
	U32		readScale_range(LLPartInitData *in, U32 startByte);
	U32		readAlpha_range(LLPartInitData *in, U32 startByte);
	U32		readUUID(LLPartInitData *in, U32 startByte);
	U32		readVelocityOffset(LLPartInitData *in, U32 startByte);
	U32		readSpawn(LLPartInitData *in, U32 startByte);
	U32		readEnvironment(LLPartInitData *in, U32 startByte);
	U32		readLifespan(LLPartInitData *in, U32 startByte);
	U32		readDecayDamp(LLPartInitData *in, U32 startByte);
	U32		readWindDiffusionFactor(LLPartInitData *in, U32 startByte);
};
#endif
