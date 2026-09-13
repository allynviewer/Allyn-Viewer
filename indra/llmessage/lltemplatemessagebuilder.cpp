/** 
 * @file lltemplatemessagebuilder.cpp
 * @brief LLTemplateMessageBuilder class implementation.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#include "linden_common.h"
#include "lltemplatemessagebuilder.h"
#include "llmessagetemplate.h"
#include "llmath.h"
#include "llquaternion.h"
#include "u64.h"
#include "v3dmath.h"
#include "v3math.h"
#include "v4math.h"
LLTemplateMessageBuilder::LLTemplateMessageBuilder(const message_template_name_map_t& name_template_map) :
	mCurrentSMessageData(NULL),
	mCurrentSMessageTemplate(NULL),
	mCurrentSDataBlock(NULL),
	mCurrentSMessageName(NULL),
	mCurrentSBlockName(NULL),
	mbSBuilt(FALSE),
	mbSClear(TRUE),
	mCurrentSendTotal(0),
	mMessageTemplates(name_template_map)
{
}
LLTemplateMessageBuilder::~LLTemplateMessageBuilder()
{
	delete mCurrentSMessageData;
	mCurrentSMessageData = NULL;
}
void LLTemplateMessageBuilder::newMessage(const char *name)
{
	mbSBuilt = FALSE;
	mbSClear = FALSE;
	mCurrentSendTotal = 0;
	delete mCurrentSMessageData;
	mCurrentSMessageData = NULL;
	char* namep = (char*)name;
	if (mMessageTemplates.count(namep) > 0)
	{
		mCurrentSMessageTemplate = mMessageTemplates.find(name)->second;
		mCurrentSMessageData = new LLMsgData(namep);
		mCurrentSMessageName = namep;
		mCurrentSDataBlock = NULL;
		mCurrentSBlockName = NULL;
		const LLMessageTemplate* msg_template = mMessageTemplates.find(name)->second;
		if (msg_template->getDeprecation() != MD_NOTDEPRECATED)
		{
			LL_WARNS() << "Sending deprecated message " << namep << LL_ENDL;
		}
		LLMessageTemplate::message_block_map_t::const_iterator iter;
		for(iter = msg_template->mMemberBlocks.begin();
			iter != msg_template->mMemberBlocks.end();
			++iter)
		{
			const LLMessageBlock* ci = msg_template->mMemberBlocks.toValue(iter);
			LLMsgBlkData* tblockp = new LLMsgBlkData(ci->mName, 0);
			mCurrentSMessageData->addBlock(tblockp);
		}
	}
	else
	{
		LL_ERRS() << "newMessage - Message " << name << " not registered" << LL_ENDL;
	}
}
void LLTemplateMessageBuilder::clearMessage()
{
	mbSBuilt = FALSE;
	mbSClear = TRUE;
	mCurrentSendTotal = 0;
	mCurrentSMessageTemplate = NULL;
	delete mCurrentSMessageData;
	mCurrentSMessageData = NULL;
	mCurrentSMessageName = NULL;
	mCurrentSDataBlock = NULL;
	mCurrentSBlockName = NULL;
}
void LLTemplateMessageBuilder::nextBlock(const char* blockname)
{
	char *bnamep = (char *)blockname;
	if (!mCurrentSMessageTemplate)
	{
		LL_ERRS() << "newMessage not called prior to setBlock" << LL_ENDL;
		return;
	}
	const LLMessageBlock* template_data = mCurrentSMessageTemplate->getBlock(bnamep);
	if (!template_data)
	{
		LL_ERRS() << "LLTemplateMessageBuilder::nextBlock " << bnamep
			<< " not a block in " << mCurrentSMessageTemplate->mName << LL_ENDL;
		return;
	}
	LLMsgBlkData* block_data = mCurrentSMessageData->mMemberBlocks[bnamep];
	if (block_data->mBlockNumber == 0)
	{
		block_data->mBlockNumber = 1;
		mCurrentSDataBlock = block_data;
		mCurrentSBlockName = bnamep;
		for (LLMessageBlock::message_variable_map_t::const_iterator iter = template_data->mMemberVariables.begin();
			 iter != template_data->mMemberVariables.end(); iter++)
		{
			const LLMessageVariable* ci = template_data->mMemberVariables.toValue(iter);
			mCurrentSDataBlock->addVariable(ci->getName(), ci->getType());
		}
		return;
	}
	else
	{
		if (template_data->mType == MBT_SINGLE)
		{
			LL_ERRS() << "LLTemplateMessageBuilder::nextBlock called multiple times"
				<< " for " << bnamep << " but is type MBT_SINGLE" << LL_ENDL;
			return;
		}
		if (  (template_data->mType == MBT_MULTIPLE)
			&&(mCurrentSDataBlock->mBlockNumber == template_data->mNumber))
		{
			LL_ERRS() << "LLTemplateMessageBuilder::nextBlock called "
				<< mCurrentSDataBlock->mBlockNumber << " times for " << bnamep
				<< " exceeding " << template_data->mNumber
				<< " specified in type MBT_MULTIPLE." << LL_ENDL;
			return;
		}
		S32  count = block_data->mBlockNumber;
		block_data->mBlockNumber++;
		if (block_data->mBlockNumber > MAX_BLOCKS)
		{
			LL_ERRS() << "Trying to pack too many blocks into MBT_VARIABLE type "
				   << "(limited to " << MAX_BLOCKS << ")" << LL_ENDL;
		}
		char *nbnamep = bnamep + count;
		mCurrentSDataBlock = new LLMsgBlkData(bnamep, count);
		mCurrentSDataBlock->mName = nbnamep;
		mCurrentSMessageData->mMemberBlocks[nbnamep] = mCurrentSDataBlock;
		for (LLMessageBlock::message_variable_map_t::const_iterator
				 iter = template_data->mMemberVariables.begin(),
				 end = template_data->mMemberVariables.end();
			 iter != end; iter++)
		{
			const LLMessageVariable* ci = template_data->mMemberVariables.toValue(iter);
			mCurrentSDataBlock->addVariable(ci->getName(), ci->getType());
		}
		return;
	}
}
void LLTemplateMessageBuilder::addData(const char *varname, const void *data, EMsgVariableType type, S32 size)
{
	char *vnamep = (char *)varname;
	if (!mCurrentSMessageTemplate)
	{
		LL_ERRS() << "newMessage not called prior to addData" << LL_ENDL;
		return;
	}
	if (!mCurrentSDataBlock)
	{
		LL_ERRS() << "setBlock not called prior to addData" << LL_ENDL;
		return;
	}
	const LLMessageVariable* var_data = mCurrentSMessageTemplate->getBlock(mCurrentSBlockName)->getVariable(vnamep);
	if (!var_data || !var_data->getName())
	{
		LL_ERRS() << vnamep << " not a variable in block " << mCurrentSBlockName << " of " << mCurrentSMessageTemplate->mName << LL_ENDL;
		return;
	}
	if (var_data->getType() == MVT_VARIABLE)
	{
		if ((var_data->getSize() == 1) &&
			(size > 255))
		{
			LL_WARNS() << "Field " << varname << " is a Variable 1 but program "
			       << "attempted to stuff more than 255 bytes in "
			       << "(" << size << ").  Clamping size and truncating data." << LL_ENDL;
			size = 255;
			char *truncate = (char *)data;
			truncate[254] = 0;
		}
		mCurrentSDataBlock->addData(vnamep, data, size, type, var_data->getSize());
		mCurrentSendTotal += size;
	}
	else
	{
		if (size != var_data->getSize())
		{
			LL_ERRS() << varname << " is type MVT_FIXED but request size " << size << " doesn't match template size "
				   << var_data->getSize() << LL_ENDL;
			return;
		}
		mCurrentSDataBlock->addData(vnamep, data, size, type);
		mCurrentSendTotal += size;
	}
}
void LLTemplateMessageBuilder::addData(const char *varname, const void *data, EMsgVariableType type)
{
	char *vnamep = (char *)varname;
	if (!mCurrentSMessageTemplate)
	{
		LL_ERRS() << "newMessage not called prior to addData" << LL_ENDL;
		return;
	}
	if (!mCurrentSDataBlock)
	{
		LL_ERRS() << "setBlock not called prior to addData" << LL_ENDL;
		return;
	}
	const LLMessageVariable* var_data = mCurrentSMessageTemplate->getBlock(mCurrentSBlockName)->getVariable(vnamep);
	if (!var_data->getName())
	{
		LL_ERRS() << vnamep << " not a variable in block " << mCurrentSBlockName << " of " << mCurrentSMessageTemplate->mName << LL_ENDL;
		return;
	}
	if (var_data->getType() == MVT_VARIABLE)
	{
		LL_ERRS() << vnamep << " is type MVT_VARIABLE. Call using addData(name, data, size)" << LL_ENDL;
		return;
	}
	else
	{
		mCurrentSDataBlock->addData(vnamep, data, var_data->getSize(), type);
		mCurrentSendTotal += var_data->getSize();
	}
}
void LLTemplateMessageBuilder::addBinaryData(const char *varname,
											const void *data, S32 size)
{
	addData(varname, data, MVT_FIXED, size);
}
void LLTemplateMessageBuilder::addS8(const char *varname, S8 s)
{
	addData(varname, &s, MVT_S8, sizeof(s));
}
void LLTemplateMessageBuilder::addU8(const char *varname, U8 u)
{
	addData(varname, &u, MVT_U8, sizeof(u));
}
void LLTemplateMessageBuilder::addS16(const char *varname, S16 i)
{
	addData(varname, &i, MVT_S16, sizeof(i));
}
void LLTemplateMessageBuilder::addU16(const char *varname, U16 i)
{
	addData(varname, &i, MVT_U16, sizeof(i));
}
void LLTemplateMessageBuilder::addF32(const char *varname, F32 f)
{
	addData(varname, &f, MVT_F32, sizeof(f));
}
void LLTemplateMessageBuilder::addS32(const char *varname, S32 s)
{
	addData(varname, &s, MVT_S32, sizeof(s));
}
void LLTemplateMessageBuilder::addU32(const char *varname, U32 u)
{
	addData(varname, &u, MVT_U32, sizeof(u));
}
void LLTemplateMessageBuilder::addU64(const char *varname, U64 lu)
{
	addData(varname, &lu, MVT_U64, sizeof(lu));
}
void LLTemplateMessageBuilder::addF64(const char *varname, F64 d)
{
	addData(varname, &d, MVT_F64, sizeof(d));
}
void LLTemplateMessageBuilder::addIPAddr(const char *varname, U32 u)
{
	addData(varname, &u, MVT_IP_ADDR, sizeof(u));
}
void LLTemplateMessageBuilder::addIPPort(const char *varname, U16 u)
{
	u = htons(u);
	addData(varname, &u, MVT_IP_PORT, sizeof(u));
}
void LLTemplateMessageBuilder::addBOOL(const char* varname, BOOL b)
{
	U8 temp = (b != 0);
	addData(varname, &temp, MVT_BOOL, sizeof(temp));
}
void LLTemplateMessageBuilder::addString(const char* varname, const char* s)
{
	if (s)
		addData( varname, (void *)s, MVT_VARIABLE, (S32)strlen(s) + 1);
	else
		addData( varname, NULL, MVT_VARIABLE, 0);
}
void LLTemplateMessageBuilder::addString(const char* varname, const std::string& s)
{
	if (s.size())
		addData( varname, (void *)s.c_str(), MVT_VARIABLE, (S32)(s.size()) + 1);
	else
		addData( varname, NULL, MVT_VARIABLE, 0);
}
void LLTemplateMessageBuilder::addVector3(const char *varname, const LLVector3& vec)
{
	addData(varname, vec.mV, MVT_LLVector3, sizeof(vec.mV));
}
void LLTemplateMessageBuilder::addVector4(const char *varname, const LLVector4& vec)
{
	addData(varname, vec.mV, MVT_LLVector4, sizeof(vec.mV));
}
void LLTemplateMessageBuilder::addVector3d(const char *varname, const LLVector3d& vec)
{
	addData(varname, vec.mdV, MVT_LLVector3d, sizeof(vec.mdV));
}
void LLTemplateMessageBuilder::addQuat(const char *varname, const LLQuaternion& quat)
{
	addData(varname, quat.packToVector3().mV, MVT_LLQuaternion, sizeof(LLVector3));
}
void LLTemplateMessageBuilder::addUUID(const char *varname, const LLUUID& uuid)
{
	addData(varname, uuid.mData, MVT_LLUUID, sizeof(uuid.mData));
}
static S32 zero_code(U8 **data, U32 *data_size)
{
	static U8 encodedSendBuffer[2 * MAX_BUFFER_SIZE];
	S32 count = *data_size;
	S32 net_gain = 0;
	U8 num_zeroes = 0;
	U8 *inptr = (U8 *)*data;
	U8 *outptr = (U8 *)encodedSendBuffer;
	for (U32 ii = 0; ii < LL_PACKET_ID_SIZE ; ++ii)
	{
		count--;
		*outptr++ = *inptr++;
	}
	while (count--)
	{
		if (!(*inptr))
		{
			if (num_zeroes)
			{
				if (++num_zeroes > 254)
				{
					*outptr++ = num_zeroes;
					num_zeroes = 0;
				}
				net_gain--;
			}
			else
			{
				*outptr++ = 0;
				net_gain++;
				num_zeroes = 1;
			}
			inptr++;
		}
		else
		{
			if (num_zeroes)
			{
				*outptr++ = num_zeroes;
				num_zeroes = 0;
			}
			*outptr++ = *inptr++;
		}
	}
	if (num_zeroes)
	{
		*outptr++ = num_zeroes;
	}
	if (net_gain < 0)
	{
		*data = encodedSendBuffer;
		*data_size += net_gain;
		encodedSendBuffer[0] |= LL_ZERO_CODE_FLAG;
	}
	return(net_gain);
}
void LLTemplateMessageBuilder::compressMessage(U8*& buf_ptr, U32& buffer_length)
{
	if(ME_ZEROCODED == mCurrentSMessageTemplate->getEncoding())
	{
		zero_code(&buf_ptr, &buffer_length);
	}
}
BOOL LLTemplateMessageBuilder::isMessageFull(const char* blockname) const
{
	if(mCurrentSendTotal > MTUBYTES)
	{
		return TRUE;
	}
	if(!blockname)
	{
		return FALSE;
	}
	char* bnamep = (char*)blockname;
	S32 max;
	const LLMessageBlock* template_data = mCurrentSMessageTemplate->getBlock(bnamep);
	switch(template_data->mType)
	{
	case MBT_SINGLE:
		max = 1;
		break;
	case MBT_MULTIPLE:
		max = template_data->mNumber;
		break;
	case MBT_VARIABLE:
	default:
		max = MAX_BLOCKS;
		break;
	}
	if(mCurrentSMessageData->mMemberBlocks[bnamep]->mBlockNumber >= max)
	{
		return TRUE;
	}
	return FALSE;
}
static S32 buildBlock(U8* buffer, S32 buffer_size, const LLMessageBlock* template_data, LLMsgData* message_data)
{
	S32 result = 0;
	LLMsgData::msg_blk_data_map_t::const_iterator block_iter = message_data->mMemberBlocks.find(template_data->mName);
	const LLMsgBlkData* mbci = block_iter->second;
	S32 block_count = mbci->mBlockNumber;
	if (template_data->mType == MBT_VARIABLE)
	{
		U8 temp_block_number = (U8)mbci->mBlockNumber;
		if ((S32)(result + sizeof(U8)) < MAX_BUFFER_SIZE)
		{
			memcpy(&buffer[result], &temp_block_number, sizeof(U8));
			result += sizeof(U8);
		}
		else
		{
			LL_ERRS() << "buildBlock failed. Message excedding "
					<< "sendBuffersize." << LL_ENDL;
		}
	}
	else if (template_data->mType == MBT_MULTIPLE)
	{
		if (block_count != template_data->mNumber)
		{
			LL_ERRS() << "Block " << mbci->mName
				<< " is type MBT_MULTIPLE but only has data for "
				<< block_count << " out of its "
				<< template_data->mNumber << " blocks" << LL_ENDL;
		}
	}
	while(block_count > 0)
	{
		for (LLMsgBlkData::msg_var_data_map_t::const_iterator iter = mbci->mMemberVarData.begin();
			 iter != mbci->mMemberVarData.end(); iter++)
		{
			const LLMsgVarData& mvci = mbci->mMemberVarData.toValue(iter);
			if (mvci.getSize() == -1)
			{
				LL_ERRS() << "The variable " << mvci.getName() << " in block "
					<< mbci->mName << " of message "
					<< template_data->mName
					<< " wasn't set prior to buildMessage call" << LL_ENDL;
			}
			else
			{
				S32 data_size = mvci.getDataSize();
				if(data_size > 0)
				{
					S32 size = mvci.getSize();
					U8 sizeb;
					U16 sizeh;
					switch(data_size)
					{
					case 1:
						sizeb = size;
						htonmemcpy(&buffer[result], &sizeb, MVT_U8, 1);
						break;
					case 2:
						sizeh = size;
						htonmemcpy(&buffer[result], &sizeh, MVT_U16, 2);
						break;
					case 4:
						htonmemcpy(&buffer[result], &size, MVT_S32, 4);
						break;
					default:
						LL_ERRS() << "Attempting to build variable field with unknown size of " << size << LL_ENDL;
						break;
					}
					result += mvci.getDataSize();
				}
				if((mvci.getData() != NULL) && mvci.getSize())
				{
					if(result + mvci.getSize() < buffer_size)
					{
					    memcpy(
							&buffer[result],
							mvci.getData(),
							mvci.getSize());
					    result += mvci.getSize();
					}
					else
					{
						LL_ERRS() << "buildBlock failed. "
							<< "Attempted to pack "
							<< (result + mvci.getSize())
							<< " bytes into a buffer with size "
							<< buffer_size << "." << LL_ENDL;
					}
				}
			}
		}
		--block_count;
		if (block_iter != message_data->mMemberBlocks.end())
		{
			++block_iter;
			if (block_iter != message_data->mMemberBlocks.end())
			{
				mbci = block_iter->second;
			}
		}
	}
	return result;
}
U32 LLTemplateMessageBuilder::buildMessage(
	U8* buffer,
	U32 buffer_size,
	U8 offset_to_data)
{
	if (!mCurrentSMessageTemplate)
	{
		LL_ERRS() << "newMessage not called prior to buildMessage" << LL_ENDL;
		return 0;
	}
	buffer[PHL_OFFSET] = offset_to_data;
	U32 result = LL_PACKET_ID_SIZE;
	if (mCurrentSMessageTemplate->mFrequency == MFT_HIGH)
	{
		buffer[result] = (U8)mCurrentSMessageTemplate->mMessageNumber;
		result += sizeof(U8);
	}
	else if (mCurrentSMessageTemplate->mFrequency == MFT_MEDIUM)
	{
		U8 temp = 255;
		memcpy(&buffer[result], &temp, sizeof(U8));
		result += sizeof(U8);
		temp = mCurrentSMessageTemplate->mMessageNumber & 255;
		memcpy(&buffer[result], &temp, sizeof(U8));
		result += sizeof(U8);
	}
	else if (mCurrentSMessageTemplate->mFrequency == MFT_LOW)
	{
		U8 temp = 255;
		U16  message_num;
		memcpy(&buffer[result], &temp, sizeof(U8));
		result += sizeof(U8);
		memcpy(&buffer[result], &temp, sizeof(U8));
		result += sizeof(U8);
		message_num = mCurrentSMessageTemplate->mMessageNumber & 0xFFFF;
		message_num = htons(message_num);
		memcpy(&buffer[result], &message_num, sizeof(U16));
		result += sizeof(U16);
	}
	else
	{
		LL_ERRS() << "unexpected message frequency in buildMessage" << LL_ENDL;
		return 0;
	}
	result += offset_to_data;
	for(LLMessageTemplate::message_block_map_t::const_iterator
			iter = mCurrentSMessageTemplate->mMemberBlocks.begin(),
			end = mCurrentSMessageTemplate->mMemberBlocks.end();
		 iter != end;
		++iter)
	{
		const LLMessageBlock* block = mCurrentSMessageTemplate->mMemberBlocks.toValue(iter);
		result += buildBlock(buffer + result, buffer_size - result, block, mCurrentSMessageData);
	}
	mbSBuilt = TRUE;
	return result;
}
void LLTemplateMessageBuilder::copyFromMessageData(const LLMsgData& data)
{
	S32 block_count = 0;
    char *block_name = NULL;
	LLMsgData::msg_blk_data_map_t::const_iterator iter =
		data.mMemberBlocks.begin();
	LLMsgData::msg_blk_data_map_t::const_iterator end =
		data.mMemberBlocks.end();
	for(; iter != end; ++iter)
	{
		const LLMsgBlkData* mbci = iter->second;
		if(!mbci) continue;
		if (block_count == 0)
		{
			block_count = mbci->mBlockNumber;
			block_name = (char *)mbci->mName;
		}
		block_count--;
		nextBlock(block_name);
		LLMsgBlkData::msg_var_data_map_t::const_iterator dit = mbci->mMemberVarData.begin();
		LLMsgBlkData::msg_var_data_map_t::const_iterator dend = mbci->mMemberVarData.end();
		for(; dit != dend; ++dit)
		{
			const LLMsgVarData& mvci = mbci->mMemberVarData.toValue(dit);
			addData(mvci.getName(), mvci.getData(), mvci.getType(), mvci.getSize());
		}
	}
}
void LLTemplateMessageBuilder::copyFromLLSD(const LLSD&)
{
}
void LLTemplateMessageBuilder::setBuilt(BOOL b) { mbSBuilt = b; }
BOOL LLTemplateMessageBuilder::isBuilt() const {return mbSBuilt;}
BOOL LLTemplateMessageBuilder::isClear() const {return mbSClear;}
S32 LLTemplateMessageBuilder::getMessageSize() {return mCurrentSendTotal;}
const char* LLTemplateMessageBuilder::getMessageName() const
{
	return mCurrentSMessageName;
}
