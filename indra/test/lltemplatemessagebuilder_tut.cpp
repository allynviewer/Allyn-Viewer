/**
 * @file lltemplatemessagebuilder_tut.cpp
 * @date 2007-04
 * @brief Tests for building messages.
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
#include <tut/tut.hpp>
#include "linden_common.h"
#include "lltut.h"
#include "llmessagetemplate.h"
#include "llquaternion.h"
#include "lltemplatemessagebuilder.h"
#include "lltemplatemessagereader.h"
#include "llversionserver.h"
#include "message_prehash.h"
#include "u64.h"
#include "v3dmath.h"
#include "v3math.h"
#include "v4math.h"
namespace tut
{
	static LLTemplateMessageBuilder::message_template_name_map_t nameMap;
	static LLTemplateMessageReader::message_template_number_map_t numberMap;
	struct LLTemplateMessageBuilderTestData
	{
		static LLMessageTemplate defaultTemplate()
		{
			static bool init = false;
			if(! init)
			{
				const F32 circuit_heartbeat_interval=5;
				const F32 circuit_timeout=100;
				start_messaging_system("notafile", 13035,
									   LL_VERSION_MAJOR,
									   LL_VERSION_MINOR,
									   LL_VERSION_PATCH,
									   FALSE,
									   "notasharedsecret",
									   NULL,
									   false,
									   circuit_heartbeat_interval,
									   circuit_timeout);
				init = true;
			}
			return LLMessageTemplate(_PREHASH_TestMessage, 1, MFT_HIGH);
		}
		static LLMessageBlock* defaultBlock(const EMsgVariableType type = MVT_NULL, const S32 size = 0, EMsgBlockType block = MBT_VARIABLE)
		{
			return createBlock(const_cast<char*>(_PREHASH_Test0), type, size, block);
		}
		static LLMessageBlock* createBlock(char* name, const EMsgVariableType type = MVT_NULL, const S32 size = 0, EMsgBlockType block = MBT_VARIABLE)
		{
			LLMessageBlock* result = new LLMessageBlock(name, block);
			if(type != MVT_NULL)
			{
				result->addVariable(const_cast<char*>(_PREHASH_Test0), type, size);
			}
			return result;
		}
		static LLTemplateMessageBuilder* defaultBuilder(LLMessageTemplate& messageTemplate, char* name = const_cast<char*>(_PREHASH_Test0))
		{
			nameMap[_PREHASH_TestMessage] = &messageTemplate;
			LLTemplateMessageBuilder* builder = new LLTemplateMessageBuilder(nameMap);
			builder->newMessage(_PREHASH_TestMessage);
			builder->nextBlock(name);
			return builder;
		}
		static LLTemplateMessageReader* setReader(
			LLMessageTemplate& messageTemplate,
			LLTemplateMessageBuilder* builder,
			U8 offset = 0)
		{
			numberMap[1] = &messageTemplate;
			const U32 bufferSize = 1024;
			U8 buffer[bufferSize];
			memset(buffer, 0, LL_PACKET_ID_SIZE);
			U32 builtSize = builder->buildMessage(buffer, bufferSize, offset);
			delete builder;
			LLTemplateMessageReader* reader = new LLTemplateMessageReader(numberMap);
			reader->validateMessage(buffer, builtSize, LLHost());
			reader->readMessage(buffer, LLHost());
			return reader;
		}
	};
	typedef test_group<LLTemplateMessageBuilderTestData>	LLTemplateMessageBuilderTestGroup;
	typedef LLTemplateMessageBuilderTestGroup::object		LLTemplateMessageBuilderTestObject;
	LLTemplateMessageBuilderTestGroup templateMessageBuilderTestGroup("LLTemplateMessageBuilder");
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<1>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock());
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<2>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_BOOL, 1));
		BOOL outValue, inValue = TRUE;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addBOOL(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getBOOL(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure BOOL", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<3>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_U8, 1));
		U8 outValue, inValue = 2;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU8(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getU8(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure U8", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<4>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_S16, 2));
		S16 outValue, inValue = 90;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addS16(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getS16(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure S16", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<5>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_U16, 2));
		U16 outValue, inValue = 3;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU16(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getU16(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure U16", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<6>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_S32, 4));
		S32 outValue, inValue = 44;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addS32(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getS32(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure S32", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<7>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_F32, 4));
		F32 outValue, inValue = 121.44f;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addF32(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getF32(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure F32", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<8>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_U32, 4));
		U32 outValue, inValue = 88;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU32(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getU32(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure U32", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<9>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_U64, 8));
		U64 outValue, inValue = 121;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU64(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getU64(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure U64", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<10>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_F64, 8));
		F64 outValue, inValue = 3232143.33;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addF64(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getF64(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure F64", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<11>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_LLVector3, 12));
		LLVector3 outValue, inValue = LLVector3(1,2,3);
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addVector3(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getVector3(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure LLVector3", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<12>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_LLVector4, 16));
		LLVector4 outValue, inValue = LLVector4(1,2,3,4);
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addVector4(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getVector4(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure LLVector4", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<13>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_LLVector3d, 24));
		LLVector3d outValue, inValue = LLVector3d(1,2,3);
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addVector3d(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getVector3d(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure LLVector3d", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<14>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_LLQuaternion, 12));
		LLQuaternion outValue, inValue = LLQuaternion(1,2,3,0);
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addQuat(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getQuat(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure LLQuaternion", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<15>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_LLUUID, 16));
		LLUUID outValue, inValue;
		inValue.generate();
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addUUID(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getUUID(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure UUID", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<16>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_IP_ADDR, 4));
		U32 outValue, inValue = 12344556;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addIPAddr(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getIPAddr(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure IPAddr", inValue, outValue);
		delete reader;
	}
	 template<> template<>
	void LLTemplateMessageBuilderTestObject::test<17>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_IP_PORT, 2));
		U16 outValue, inValue = 80;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addIPPort(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getIPPort(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure IPPort", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<18>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_VARIABLE, 1));
		std::string outValue, inValue = "testing";
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addString(_PREHASH_Test0, inValue.c_str());
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		char buffer[MAX_STRING];
		reader->getString(_PREHASH_Test0, _PREHASH_Test0, MAX_STRING, buffer);
		outValue = buffer;
		ensure_equals("Ensure String", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<19>()
	{
		U8 buffer1[MAX_BUFFER_SIZE];
		memset(buffer1, 0, MAX_BUFFER_SIZE);
		U8 buffer2[MAX_BUFFER_SIZE];
		memset(buffer2, 0, MAX_BUFFER_SIZE);
		U32 bufferSize1, bufferSize2;
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test0), MVT_U32, 4, MBT_SINGLE));
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test1), MVT_U32, 4, MBT_SINGLE));
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate, const_cast<char*>(_PREHASH_Test0));
		builder->addU32(_PREHASH_Test0, 0xaaaa);
		builder->nextBlock(_PREHASH_Test1);
		builder->addU32(_PREHASH_Test0, 0xbbbb);
		bufferSize1 = builder->buildMessage(buffer1, MAX_BUFFER_SIZE, 0);
		delete builder;
		messageTemplate = defaultTemplate();
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test1), MVT_U32, 4, MBT_SINGLE));
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test0), MVT_U32, 4, MBT_SINGLE));
		builder = defaultBuilder(messageTemplate, const_cast<char*>(_PREHASH_Test1));
		builder->addU32(_PREHASH_Test0, 0xaaaa);
		builder->nextBlock(_PREHASH_Test0);
		builder->addU32(_PREHASH_Test0, 0xbbbb);
		bufferSize2 = builder->buildMessage(buffer2, MAX_BUFFER_SIZE, 0);
		delete builder;
		ensure_equals("Ensure Buffer Sizes Equal", bufferSize1, bufferSize2);
		ensure_equals("Ensure Buffer Contents Equal", memcmp(buffer1, buffer2, bufferSize1), 0);
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<20>()
	{
		U8 buffer1[MAX_BUFFER_SIZE];
		memset(buffer1, 0, MAX_BUFFER_SIZE);
		U8 buffer2[MAX_BUFFER_SIZE];
		memset(buffer2, 0, MAX_BUFFER_SIZE);
		U32 bufferSize1, bufferSize2;
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test0), MVT_U32, 4, MBT_SINGLE));
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test1), MVT_U32, 4, MBT_SINGLE));
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate, const_cast<char*>(_PREHASH_Test0));
		builder->addU32(_PREHASH_Test0, 0xaaaa);
		builder->nextBlock(_PREHASH_Test1);
		builder->addU32(_PREHASH_Test0, 0xbbbb);
		bufferSize1 = builder->buildMessage(buffer1, MAX_BUFFER_SIZE, 0);
		delete builder;
		builder = defaultBuilder(messageTemplate, const_cast<char*>(_PREHASH_Test1));
		builder->addU32(_PREHASH_Test0, 0xbbbb);
		builder->nextBlock(_PREHASH_Test0);
		builder->addU32(_PREHASH_Test0, 0xaaaa);
		bufferSize2 = builder->buildMessage(buffer2, MAX_BUFFER_SIZE, 0);
		delete builder;
		ensure_equals("Ensure Buffer Sizes Equal", bufferSize1, bufferSize2);
		ensure_equals("Ensure Buffer Contents Equal", memcmp(buffer1, buffer2, bufferSize1), 0);
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<21>()
	{
		U8 buffer1[MAX_BUFFER_SIZE];
		memset(buffer1, 0, MAX_BUFFER_SIZE);
		U8 buffer2[MAX_BUFFER_SIZE];
		memset(buffer2, 0, MAX_BUFFER_SIZE);
		U32 bufferSize1, bufferSize2;
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test0), MVT_U32, 4, MBT_SINGLE));
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate, const_cast<char*>(_PREHASH_Test0));
		builder->addU32(_PREHASH_Test0, 0xaaaa);
		bufferSize1 = builder->buildMessage(buffer1, MAX_BUFFER_SIZE, 0);
		delete builder;
		messageTemplate = defaultTemplate();
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test0), MVT_U32, 4, MBT_SINGLE));
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test1), MVT_U32, 4, MBT_SINGLE));
		builder = defaultBuilder(messageTemplate, const_cast<char*>(_PREHASH_Test0));
		builder->addU32(_PREHASH_Test0, 0xaaaa);
		builder->nextBlock(_PREHASH_Test1);
		builder->addU32(_PREHASH_Test0, 0xbbbb);
		bufferSize2 = builder->buildMessage(buffer2, MAX_BUFFER_SIZE, 0);
		delete builder;
		ensure_not_equals("Ensure Buffer Sizes Not Equal", bufferSize1, bufferSize2);
		ensure_equals("Ensure Buffer Prefix Equal", memcmp(buffer1, buffer2, bufferSize1), 0);
		ensure_not_equals("Ensure Buffer Contents Not Equal", memcmp(buffer1, buffer2, bufferSize2), 0);
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<22>()
	{
		U32 inTest00 = 0, inTest01 = 1, inTest1 = 2;
		U32 outTest00, outTest01, outTest1;
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test0), MVT_U32, 4));
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test1), MVT_U32, 4));
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU32(_PREHASH_Test0, inTest00);
		builder->nextBlock(_PREHASH_Test0);
		builder->addU32(_PREHASH_Test0, inTest01);
		builder->nextBlock(_PREHASH_Test1);
		builder->addU32(_PREHASH_Test0, inTest1);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getU32(_PREHASH_Test0, _PREHASH_Test0, outTest00, 0);
		reader->getU32(_PREHASH_Test0, _PREHASH_Test0, outTest01, 1);
		reader->getU32(_PREHASH_Test1, _PREHASH_Test0, outTest1);
		ensure_equals("Ensure Test0[0]", inTest00, outTest00);
		ensure_equals("Ensure Test0[1]", inTest01, outTest01);
		ensure_equals("Ensure Test1", inTest1, outTest1);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<23>()
	{
		U32 inTest = 1, outTest;
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(
			createBlock(const_cast<char*>(_PREHASH_Test0), MVT_U32, 4, MBT_SINGLE));
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test1), MVT_U32, 4));
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU32(_PREHASH_Test0, inTest);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		reader->getU32(_PREHASH_Test0, _PREHASH_Test0, outTest);
		S32 blockCount = reader->getNumberOfBlocks(const_cast<char*>(_PREHASH_Test1));
		ensure_equals("Ensure block count", blockCount, 0);
		ensure_equals("Ensure Test0", inTest, outTest);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<24>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test0), MVT_U32, 4));
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU32(_PREHASH_Test0, 42);
		LLTemplateMessageReader* reader = setReader(messageTemplate, builder);
		builder = defaultBuilder(messageTemplate);
		builder->newMessage(_PREHASH_TestMessage);
		reader->copyToBuilder(*builder);
		U8 buffer[MAX_BUFFER_SIZE];
		builder->buildMessage(buffer, MAX_BUFFER_SIZE, 0);
		delete builder;
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<25>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock());
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 10);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<26>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_BOOL, 1));
		BOOL outValue, inValue = TRUE;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addBOOL(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 1);
		reader->getBOOL(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure BOOL", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<27>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_U8, 1));
		U8 outValue, inValue = 2;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU8(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 255);
		reader->getU8(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure U8", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<28>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_S16, 2));
		S16 outValue, inValue = 90;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addS16(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 2);
		reader->getS16(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure S16", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<29>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_U16, 2));
		U16 outValue, inValue = 3;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU16(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 4);
		reader->getU16(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure U16", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<30>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_S32, 4));
		S32 outValue, inValue = 44;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addS32(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 4);
		reader->getS32(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure S32", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<31>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_F32, 4));
		F32 outValue, inValue = 121.44f;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addF32(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 16);
		reader->getF32(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure F32", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<32>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_U32, 4));
		U32 outValue, inValue = 88;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU32(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 127);
		reader->getU32(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure U32", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<33>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_U64, 8));
		U64 outValue, inValue = 121;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU64(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 32);
		reader->getU64(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure U64", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<34>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_F64, 8));
		F64 outValue, inValue = 3232143.33;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addF64(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 128);
		reader->getF64(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure F64", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<35>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_LLVector3, 12));
		LLVector3 outValue, inValue = LLVector3(1,2,3);
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addVector3(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 63);
		reader->getVector3(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure LLVector3", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<36>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_LLVector4, 16));
		LLVector4 outValue, inValue = LLVector4(1,2,3,4);
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addVector4(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 64);
		reader->getVector4(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure LLVector4", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<37>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_LLVector3d, 24));
		LLVector3d outValue, inValue = LLVector3d(1,2,3);
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addVector3d(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 64);
		reader->getVector3d(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure LLVector3d", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<38>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_LLQuaternion, 12));
		LLQuaternion outValue, inValue = LLQuaternion(1,2,3,0);
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addQuat(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 12);
		reader->getQuat(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure LLQuaternion", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<39>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_LLUUID, 16));
		LLUUID outValue, inValue;
		inValue.generate();
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addUUID(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 31);
		reader->getUUID(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure UUID", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<40>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_IP_ADDR, 4));
		U32 outValue, inValue = 12344556;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addIPAddr(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 32);
		reader->getIPAddr(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure IPAddr", inValue, outValue);
		delete reader;
	}
	 template<> template<>
	void LLTemplateMessageBuilderTestObject::test<41>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_IP_PORT, 2));
		U16 outValue, inValue = 80;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addIPPort(_PREHASH_Test0, inValue);
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 6);
		reader->getIPPort(_PREHASH_Test0, _PREHASH_Test0, outValue);
		ensure_equals("Ensure IPPort", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<42>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_VARIABLE, 1));
		std::string outValue, inValue = "testing";
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addString(_PREHASH_Test0, inValue.c_str());
		LLTemplateMessageReader* reader = setReader(
			messageTemplate, builder, 255);
		char buffer[MAX_STRING];
		reader->getString(_PREHASH_Test0, _PREHASH_Test0, MAX_STRING, buffer);
		outValue = buffer;
		ensure_equals("Ensure String", inValue, outValue);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<43>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_U32, 4, MBT_SINGLE));
		U32 outValue, outValue2, inValue = 0xbbbbbbbb;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU32(_PREHASH_Test0, inValue);
		const U32 bufferSize = 1024;
		U8 buffer[bufferSize];
		memset(buffer, 0xaa, bufferSize);
		memset(buffer, 0, LL_PACKET_ID_SIZE);
		U32 builtSize = builder->buildMessage(buffer, bufferSize, 0);
		delete builder;
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test1), MVT_U32, 4, MBT_SINGLE));
		numberMap[1] = &messageTemplate;
		LLTemplateMessageReader* reader =
			new LLTemplateMessageReader(numberMap);
		reader->validateMessage(buffer, builtSize, LLHost());
		reader->readMessage(buffer, LLHost());
		reader->getU32(_PREHASH_Test0, _PREHASH_Test0, outValue);
		reader->getU32(_PREHASH_Test1, _PREHASH_Test0, outValue2);
		ensure_equals("Ensure present value ", outValue, inValue);
		ensure_equals("Ensure default value ", outValue2, 0);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<44>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_U32, 4, MBT_SINGLE));
		U32 outValue, outValue2, inValue = 0xbbbbbbbb;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU32(_PREHASH_Test0, inValue);
		const U32 bufferSize = 1024;
		U8 buffer[bufferSize];
		memset(buffer, 0xaa, bufferSize);
		memset(buffer, 0, LL_PACKET_ID_SIZE);
		U32 builtSize = builder->buildMessage(buffer, bufferSize, 0);
		delete builder;
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test1), MVT_U32, 4));
		numberMap[1] = &messageTemplate;
		LLTemplateMessageReader* reader =
			new LLTemplateMessageReader(numberMap);
		reader->validateMessage(buffer, builtSize, LLHost());
		reader->readMessage(buffer, LLHost());
		reader->getU32(_PREHASH_Test0, _PREHASH_Test0, outValue);
		outValue2 = reader->getNumberOfBlocks(_PREHASH_Test1);
		ensure_equals("Ensure present value ", outValue, inValue);
		ensure_equals("Ensure 0 repeats ", outValue2, 0);
		delete reader;
	}
	template<> template<>
	void LLTemplateMessageBuilderTestObject::test<45>()
	{
		LLMessageTemplate messageTemplate = defaultTemplate();
		messageTemplate.addBlock(defaultBlock(MVT_U32, 4, MBT_SINGLE));
		U32 outValue, outValue2, inValue = 0xbbbbbbbb;
		LLTemplateMessageBuilder* builder = defaultBuilder(messageTemplate);
		builder->addU32(_PREHASH_Test0, inValue);
		const U32 bufferSize = 1024;
		U8 buffer[bufferSize];
		memset(buffer, 0xaa, bufferSize);
		memset(buffer, 0, LL_PACKET_ID_SIZE);
		U32 builtSize = builder->buildMessage(buffer, bufferSize, 0);
		delete builder;
		messageTemplate.addBlock(createBlock(const_cast<char*>(_PREHASH_Test1), MVT_VARIABLE, 4,
											 MBT_SINGLE));
		numberMap[1] = &messageTemplate;
		LLTemplateMessageReader* reader =
			new LLTemplateMessageReader(numberMap);
		reader->validateMessage(buffer, builtSize, LLHost());
		reader->readMessage(buffer, LLHost());
		reader->getU32(_PREHASH_Test0, _PREHASH_Test0, outValue);
		char outBuffer[bufferSize];
		memset(buffer, 0xcc, bufferSize);
		reader->getString(_PREHASH_Test1, _PREHASH_Test0, bufferSize,
						  outBuffer);
		outValue2 = reader->getNumberOfBlocks(_PREHASH_Test1);
		ensure_equals("Ensure present value ", outValue, inValue);
		ensure_equals("Ensure unchanged buffer ", strlen(outBuffer), 0);
		delete reader;
	}
}
