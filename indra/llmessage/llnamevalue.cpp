/** 
 * @file llnamevalue.cpp
 * @brief class for defining name value pairs.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llnamevalue.h"
#include "llstring.h"
#include "llstringtable.h"
enum
{
	NV_BUFFER_LEN = 2048,
	U64_BUFFER_LEN = 64
};
LLStringTable	gNVNameTable(256);
char NameValueTypeStrings[NVT_EOF][NAME_VALUE_TYPE_STRING_LENGTH] =
{
	"NULL",
	"STRING",
	"F32",
	"S32",
	"VEC3",
	"U32",
	"CAMERA",
	"ASSET",
	"U64"
};
char NameValueClassStrings[NVC_EOF][NAME_VALUE_CLASS_STRING_LENGTH] =
{
	"NULL",
	"R",
	"RW"
};
char NameValueSendtoStrings[NVS_EOF][NAME_VALUE_SENDTO_STRING_LENGTH] =
{
	"NULL",
	"S",
	"DS",
	"SV",
	"DSV"
};
LLNameValue::LLNameValue()
{
	baseInit();
}
void LLNameValue::baseInit()
{
	mNVNameTable = &gNVNameTable;
	mName = NULL;
	mNameValueReference.string = NULL;
	mType = NVT_NULL;
	mStringType = NameValueTypeStrings[NVT_NULL];
	mClass = NVC_NULL;
	mStringClass = NameValueClassStrings[NVC_NULL];
	mSendto = NVS_NULL;
	mStringSendto = NameValueSendtoStrings[NVS_NULL];
}
void LLNameValue::init(const char *name, const char *data, const char *type, const char *nvclass, const char *nvsendto)
{
	mNVNameTable = &gNVNameTable;
	mName = mNVNameTable->addString(name);
	mStringType = mNVNameTable->addString(type);
	if (!strcmp(mStringType, "STRING"))
	{
		S32 string_length = (S32)strlen(data);
		mType = NVT_STRING;
		delete[] mNameValueReference.string;
		mNameValueReference.string = new char[string_length + 1];;
		strncpy(mNameValueReference.string, data, string_length);
		mNameValueReference.string[string_length] = 0;
	}
	else if (!strcmp(mStringType, "F32"))
	{
		mType = NVT_F32;
		mNameValueReference.f32 = new F32((F32)atof(data));
	}
	else if (!strcmp(mStringType, "S32"))
	{
		mType = NVT_S32;
		mNameValueReference.s32 = new S32(atoi(data));
	}
	else if (!strcmp(mStringType, "U64"))
	{
		mType = NVT_U64;
		mNameValueReference.u64 = new U64(std::stoull(data));
	}
	else if (!strcmp(mStringType, "VEC3"))
	{
		mType = NVT_VEC3;
		F32 t1, t2, t3;
		if (strchr(data, '<'))
		{
			sscanf(data, "<%f, %f, %f>", &t1, &t2, &t3);
		}
		else
		{
			sscanf(data, "%f, %f, %f", &t1, &t2, &t3);
		}
		if (!std::isfinite(t1) || !std::isfinite(t2) || !std::isfinite(t3))
		{
			t1 = 0.f;
			t2 = 0.f;
			t3 = 0.f;
		}
		mNameValueReference.vec3 = new LLVector3(t1, t2, t3);
	}
	else if (!strcmp(mStringType, "U32"))
	{
		mType = NVT_U32;
		mNameValueReference.u32 = new U32(atoi(data));
	}
	else if(!strcmp(mStringType, (const char*)NameValueTypeStrings[NVT_ASSET]))
	{
		S32 string_length = (S32)strlen(data);
		mType = NVT_ASSET;
		mNameValueReference.string = new char[string_length + 1];;
		strncpy(mNameValueReference.string, data, string_length);
		mNameValueReference.string[string_length] = 0;
	}
	else
	{
		LL_WARNS() << "Unknown name value type string " << mStringType << " for " << mName << LL_ENDL;
		mType = NVT_NULL;
	}
	if (!strcmp(nvclass, "R") ||
		!strcmp(nvclass, "READ_ONLY"))
	{
		mClass = NVC_READ_ONLY;
		mStringClass = mNVNameTable->addString("R");
	}
	else if (!strcmp(nvclass, "RW") ||
			!strcmp(nvclass, "READ_WRITE"))
	{
		mClass = NVC_READ_WRITE;
		mStringClass = mNVNameTable->addString("RW");
	}
	else
	{
		mClass = NVC_NULL;
		mStringClass = mNVNameTable->addString(nvclass);
	}
	if (!strcmp(nvsendto, "S") ||
		!strcmp(nvsendto, "SIM"))
	{
		mSendto = NVS_SIM;
		mStringSendto = mNVNameTable->addString("S");
	}
	else if (!strcmp(nvsendto, "DS") ||
		!strcmp(nvsendto, "SIM_SPACE"))
	{
		mSendto = NVS_DATA_SIM;
		mStringSendto = mNVNameTable->addString("DS");
	}
	else if (!strcmp(nvsendto, "SV") ||
			!strcmp(nvsendto, "SIM_VIEWER"))
	{
		mSendto = NVS_SIM_VIEWER;
		mStringSendto = mNVNameTable->addString("SV");
	}
	else if (!strcmp(nvsendto, "DSV") ||
			!strcmp(nvsendto, "SIM_SPACE_VIEWER"))
	{
		mSendto = NVS_DATA_SIM_VIEWER;
		mStringSendto = mNVNameTable->addString("DSV");
	}
	else
	{
		LL_WARNS() << "LLNameValue::init() - unknown sendto field "
				<< nvsendto << " for NV " << mName << LL_ENDL;
		mSendto = NVS_NULL;
		mStringSendto = mNVNameTable->addString("S");
	}
}
LLNameValue::LLNameValue(const char *name, const char *data, const char *type, const char *nvclass)
{
	baseInit();
	init(name, data, type, nvclass, "SIM");
}
LLNameValue::LLNameValue(const char *name, const char *data, const char *type, const char *nvclass, const char *nvsendto)
{
	baseInit();
	init(name, data, type, nvclass, nvsendto);
}
LLNameValue::LLNameValue(const char *name, const char *type, const char *nvclass)
{
	baseInit();
	mName = mNVNameTable->addString(name);
	mStringType = mNVNameTable->addString(type);
	if (!strcmp(mStringType, "STRING"))
	{
		mType = NVT_STRING;
		mNameValueReference.string = NULL;
	}
	else if (!strcmp(mStringType, "F32"))
	{
		mType = NVT_F32;
		mNameValueReference.f32 = NULL;
	}
	else if (!strcmp(mStringType, "S32"))
	{
		mType = NVT_S32;
		mNameValueReference.s32 = NULL;
	}
	else if (!strcmp(mStringType, "VEC3"))
	{
		mType = NVT_VEC3;
		mNameValueReference.vec3 = NULL;
	}
	else if (!strcmp(mStringType, "U32"))
	{
		mType = NVT_U32;
		mNameValueReference.u32 = NULL;
	}
	else if (!strcmp(mStringType, "U64"))
	{
		mType = NVT_U64;
		mNameValueReference.u64 = NULL;
	}
	else if(!strcmp(mStringType, (const char*)NameValueTypeStrings[NVT_ASSET]))
	{
		mType = NVT_ASSET;
		mNameValueReference.string = NULL;
	}
	else
	{
		mType = NVT_NULL;
		LL_INFOS() << "Unknown name-value type " << mStringType << LL_ENDL;
	}
	mStringClass = mNVNameTable->addString(nvclass);
	if (!strcmp(mStringClass, "READ_ONLY"))
	{
		mClass = NVC_READ_ONLY;
	}
	else if (!strcmp(mStringClass, "READ_WRITE"))
	{
		mClass = NVC_READ_WRITE;
	}
	else
	{
		mClass = NVC_NULL;
	}
	mStringSendto = mNVNameTable->addString("SIM");
	mSendto = NVS_SIM;
}
LLNameValue::LLNameValue(const char *data)
{
	baseInit();
	static char name[NV_BUFFER_LEN];
	static char type[NV_BUFFER_LEN];
	static char nvclass[NV_BUFFER_LEN];
	static char nvsendto[NV_BUFFER_LEN];
	static char nvdata[NV_BUFFER_LEN];
	S32 i;
	S32	character_count = 0;
	S32	length = 0;
	while (1)
	{
		if (  (*(data + character_count) == ' ')
			||(*(data + character_count) == '\n')
			||(*(data + character_count) == '\t')
			||(*(data + character_count) == '\r'))
		{
			character_count++;
		}
		else
		{
			break;
		}
	}
	sscanf((data + character_count), "%2047s", name);
	length = (S32)strlen(name);
	name[length] = 0;
	character_count += length;
	while (1)
	{
		if (  (*(data + character_count) == ' ')
			||(*(data + character_count) == '\n')
			||(*(data + character_count) == '\t')
			||(*(data + character_count) == '\r'))
		{
			character_count++;
		}
		else
		{
			break;
		}
	}
	sscanf((data + character_count), "%2047s", type);
	length = (S32)strlen(type);
	type[length] = 0;
	character_count += length;
	while (1)
	{
		if (  (*(data + character_count) == ' ')
			||(*(data + character_count) == '\n')
			||(*(data + character_count) == '\t')
			||(*(data + character_count) == '\r'))
		{
			character_count++;
		}
		else
		{
			break;
		}
	}
	for (i = NVC_READ_ONLY; i < NVC_EOF; i++)
	{
		if (!strncmp(NameValueClassStrings[i], data + character_count, strlen(NameValueClassStrings[i])))
		{
			break;
		}
	}
	if (i != NVC_EOF)
	{
		sscanf((data + character_count), "%2047s", nvclass);
		length = (S32)strlen(nvclass);
		nvclass[length] = 0;
		character_count += length;
		while (1)
		{
			if (  (*(data + character_count) == ' ')
				||(*(data + character_count) == '\n')
				||(*(data + character_count) == '\t')
				||(*(data + character_count) == '\r'))
			{
				character_count++;
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		strncpy(nvclass, "READ_WRITE", sizeof(nvclass) -1);
		nvclass[sizeof(nvclass) -1] = '\0';
	}
	for (i = NVS_SIM; i < NVS_EOF; i++)
	{
		if (!strncmp(NameValueSendtoStrings[i], data + character_count, strlen(NameValueSendtoStrings[i])))
		{
			break;
		}
	}
	if (i != NVS_EOF)
	{
		sscanf((data + character_count), "%2047s", nvsendto);
		length = (S32)strlen(nvsendto);
		nvsendto[length] = 0;
		character_count += length;
		while (1)
		{
			if (  (*(data + character_count) == ' ')
				||(*(data + character_count) == '\n')
				||(*(data + character_count) == '\t')
				||(*(data + character_count) == '\r'))
			{
				character_count++;
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		strncpy(nvsendto, "SIM", sizeof(nvsendto) -1);
		nvsendto[sizeof(nvsendto) -1] ='\0';
	}
	length = 0;
	while ( (*(nvdata + length++) = *(data + character_count++)) )
		;
	init(name, nvdata, type, nvclass, nvsendto);
}
LLNameValue::~LLNameValue()
{
	mNVNameTable->removeString(mName);
	mName = NULL;
	switch(mType)
	{
	case NVT_STRING:
	case NVT_ASSET:
		delete [] mNameValueReference.string;
		mNameValueReference.string = NULL;
		break;
	case NVT_F32:
		delete mNameValueReference.f32;
		mNameValueReference.string = NULL;
		break;
	case NVT_S32:
		delete mNameValueReference.s32;
		mNameValueReference.string = NULL;
		break;
	case NVT_VEC3:
		delete mNameValueReference.vec3;
		mNameValueReference.string = NULL;
		break;
	case NVT_U32:
		delete mNameValueReference.u32;
		mNameValueReference.u32 = NULL;
		break;
	case NVT_U64:
		delete mNameValueReference.u64;
		mNameValueReference.u64 = NULL;
		break;
	default:
		break;
	}
	delete[] mNameValueReference.string;
	mNameValueReference.string = NULL;
}
char	*LLNameValue::getString()
{
	if (mType == NVT_STRING)
	{
		return mNameValueReference.string;
	}
	else
	{
		LL_ERRS() << mName << " not a string!" << LL_ENDL;
		return NULL;
	}
}
const char *LLNameValue::getAsset() const
{
	if (mType == NVT_ASSET)
	{
		return mNameValueReference.string;
	}
	else
	{
		LL_ERRS() << mName << " not an asset!" << LL_ENDL;
		return NULL;
	}
}
F32		*LLNameValue::getF32()
{
	if (mType == NVT_F32)
	{
		return mNameValueReference.f32;
	}
	else
	{
		LL_ERRS() << mName << " not a F32!" << LL_ENDL;
		return NULL;
	}
}
S32		*LLNameValue::getS32()
{
	if (mType == NVT_S32)
	{
		return mNameValueReference.s32;
	}
	else
	{
		LL_ERRS() << mName << " not a S32!" << LL_ENDL;
		return NULL;
	}
}
U32		*LLNameValue::getU32()
{
	if (mType == NVT_U32)
	{
		return mNameValueReference.u32;
	}
	else
	{
		LL_ERRS() << mName << " not a U32!" << LL_ENDL;
		return NULL;
	}
}
U64		*LLNameValue::getU64()
{
	if (mType == NVT_U64)
	{
		return mNameValueReference.u64;
	}
	else
	{
		LL_ERRS() << mName << " not a U64!" << LL_ENDL;
		return NULL;
	}
}
void	LLNameValue::getVec3(LLVector3 &vec)
{
	if (mType == NVT_VEC3)
	{
		vec = *mNameValueReference.vec3;
	}
	else
	{
		LL_ERRS() << mName << " not a Vec3!" << LL_ENDL;
	}
}
LLVector3	*LLNameValue::getVec3()
{
	if (mType == NVT_VEC3)
	{
		 return (mNameValueReference.vec3);
	}
	else
	{
		LL_ERRS() << mName << " not a Vec3!" << LL_ENDL;
		return NULL;
	}
}
BOOL LLNameValue::sendToData() const
{
	return (mSendto == NVS_DATA_SIM || mSendto == NVS_DATA_SIM_VIEWER);
}
BOOL LLNameValue::sendToViewer() const
{
	return (mSendto == NVS_SIM_VIEWER || mSendto == NVS_DATA_SIM_VIEWER);
}
LLNameValue &LLNameValue::operator=(const LLNameValue &a)
{
	if (mType != a.mType)
	{
		return *this;
	}
	if (mClass == NVC_READ_ONLY)
		return *this;
	switch(a.mType)
	{
	case NVT_STRING:
	case NVT_ASSET:
		if (mNameValueReference.string)
			delete [] mNameValueReference.string;
		mNameValueReference.string = new char [strlen(a.mNameValueReference.string) + 1];
		if(mNameValueReference.string != NULL)
		{
			strcpy(mNameValueReference.string, a.mNameValueReference.string);
		}
		break;
	case NVT_F32:
		*mNameValueReference.f32 = *a.mNameValueReference.f32;
		break;
	case NVT_S32:
		*mNameValueReference.s32 = *a.mNameValueReference.s32;
		break;
	case NVT_VEC3:
		*mNameValueReference.vec3 = *a.mNameValueReference.vec3;
		break;
	case NVT_U32:
		*mNameValueReference.u32 = *a.mNameValueReference.u32;
		break;
	case NVT_U64:
		*mNameValueReference.u64 = *a.mNameValueReference.u64;
		break;
	default:
		LL_ERRS() << "Unknown Name value type " << (U32)a.mType << LL_ENDL;
		break;
	}
	return *this;
}
void LLNameValue::setString(const char *a)
{
	if (mClass == NVC_READ_ONLY)
		return;
	switch(mType)
	{
	case NVT_STRING:
		if (a)
		{
			if (mNameValueReference.string)
			{
				delete [] mNameValueReference.string;
			}
			mNameValueReference.string = new char [strlen(a) + 1];
			if(mNameValueReference.string != NULL)
			{
				strcpy(mNameValueReference.string,  a);
			}
		}
		else
		{
			if (mNameValueReference.string)
				delete [] mNameValueReference.string;
			mNameValueReference.string = new char [1];
			mNameValueReference.string[0] = 0;
		}
		break;
	default:
		break;
	}
	return;
}
void LLNameValue::setAsset(const char *a)
{
	if (mClass == NVC_READ_ONLY)
		return;
	switch(mType)
	{
	case NVT_ASSET:
		if (a)
		{
			if (mNameValueReference.string)
			{
				delete [] mNameValueReference.string;
			}
			mNameValueReference.string = new char [strlen(a) + 1];
			if(mNameValueReference.string != NULL)
			{
				strcpy(mNameValueReference.string,  a);
			}
		}
		else
		{
			if (mNameValueReference.string)
				delete [] mNameValueReference.string;
			mNameValueReference.string = new char [1];
			mNameValueReference.string[0] = 0;
		}
		break;
	default:
		break;
	}
}
void LLNameValue::setF32(const F32 a)
{
	if (mClass == NVC_READ_ONLY)
		return;
	switch(mType)
	{
	case NVT_F32:
		*mNameValueReference.f32 = a;
		break;
	default:
		break;
	}
	return;
}
void LLNameValue::setS32(const S32 a)
{
	if (mClass == NVC_READ_ONLY)
		return;
	switch(mType)
	{
	case NVT_S32:
		*mNameValueReference.s32 = a;
		break;
	case NVT_U32:
		*mNameValueReference.u32 = a;
		break;
	case NVT_F32:
		*mNameValueReference.f32 = (F32)a;
		break;
	default:
		break;
	}
	return;
}
void LLNameValue::setU32(const U32 a)
{
	if (mClass == NVC_READ_ONLY)
		return;
	switch(mType)
	{
	case NVT_S32:
		*mNameValueReference.s32 = a;
		break;
	case NVT_U32:
		*mNameValueReference.u32 = a;
		break;
	case NVT_F32:
		*mNameValueReference.f32 = (F32)a;
		break;
	default:
		LL_ERRS() << "NameValue: Trying to set U32 into a " << mStringType << ", unknown conversion" << LL_ENDL;
		break;
	}
	return;
}
void LLNameValue::setVec3(const LLVector3 &a)
{
	if (mClass == NVC_READ_ONLY)
		return;
	switch(mType)
	{
	case NVT_VEC3:
		*mNameValueReference.vec3 = a;
		break;
	default:
		LL_ERRS() << "NameValue: Trying to set LLVector3 into a " << mStringType << ", unknown conversion" << LL_ENDL;
		break;
	}
	return;
}
std::string LLNameValue::printNameValue() const
{
	std::string buffer;
	buffer = llformat("%s %s %s %s ", mName, mStringType, mStringClass, mStringSendto);
	buffer += printData();
	return buffer;
}
std::string LLNameValue::printData() const
{
	std::string buffer;
	switch(mType)
	{
	case NVT_STRING:
	case NVT_ASSET:
		buffer = mNameValueReference.string;
		break;
	case NVT_F32:
		buffer = llformat("%f", *mNameValueReference.f32);
		break;
	case NVT_S32:
		buffer = llformat("%d", *mNameValueReference.s32);
		break;
	case NVT_U32:
	  	buffer = llformat("%u", *mNameValueReference.u32);
		break;
	case NVT_U64:
		{
			buffer = fmt::to_string(*mNameValueReference.u64);
		}
		break;
	case NVT_VEC3:
	  	buffer = llformat( "%f, %f, %f", mNameValueReference.vec3->mV[VX], mNameValueReference.vec3->mV[VY], mNameValueReference.vec3->mV[VZ]);
		break;
	default:
		LL_ERRS() << "Trying to print unknown NameValue type " << mStringType << LL_ENDL;
		break;
	}
	return buffer;
}
std::ostream&		operator<<(std::ostream& s, const LLNameValue &a)
{
	switch(a.mType)
	{
	case NVT_STRING:
	case NVT_ASSET:
		s << a.mNameValueReference.string;
		break;
	case NVT_F32:
		s << (*a.mNameValueReference.f32);
		break;
	case NVT_S32:
		s << *(a.mNameValueReference.s32);
		break;
	case NVT_U32:
		s << *(a.mNameValueReference.u32);
		break;
	case NVT_U64:
		s << (*a.mNameValueReference.u64);
		break;
	case NVT_VEC3:
		s << *(a.mNameValueReference.vec3);
		break;
	default:
		LL_ERRS() << "Trying to print unknown NameValue type " << a.mStringType << LL_ENDL;
		break;
	}
	return s;
}
