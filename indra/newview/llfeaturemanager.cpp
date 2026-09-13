/** 
 * @file llfeaturemanager.cpp
 * @brief LLFeatureManager class implementation
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
#include "llviewerprecompiledheaders.h"
#include <iostream>
#include <fstream>
#if LL_MSVC
#pragma warning( disable       : 4265 )
#endif
#include <boost/regex.hpp>
#include "llfeaturemanager.h"
#include "lldir.h"
#include "llsys.h"
#include "llgl.h"
#include "llsecondlifeurls.h"
#include "llviewercontrol.h"
#include "llworld.h"
#include "lldrawpoolterrain.h"
#include "llviewertexturelist.h"
#include "llwindow.h"
#include "llui.h"
#include "llcontrol.h"
#include "llboost.h"
#include "llweb.h"
#include "llviewershadermgr.h"
#if LL_WINDOWS
#include "lldxhardware.h"
#endif
extern LLMemoryInfo gSysMemory;
const char FEATURE_TABLE_FILENAME[] = "featuretable.txt";
const char GPU_TABLE_FILENAME[] = "gpu_table.txt";
LLFeatureInfo::LLFeatureInfo(const std::string& name, const BOOL available, const F32 level)
	: mValid(TRUE), mName(name), mAvailable(available), mRecommendedLevel(level)
{
}
LLFeatureList::LLFeatureList(const std::string& name)
	: mName(name)
{
}
LLFeatureList::~LLFeatureList()
{
}
void LLFeatureList::addFeature(const std::string& name, const BOOL available, const F32 level)
{
	LLFeatureInfo fi(name, available, level);
	feature_map_t::iterator iter = mFeatures.find(name);
	if (iter != mFeatures.end())
	{
		LL_WARNS("RenderInit") << "LLFeatureList::Attempting to add preexisting feature " << name << LL_ENDL;
		iter->second = fi;
	}
	else
	{
		mFeatures.insert(std::make_pair(name, fi));
	}
}
BOOL LLFeatureList::isFeatureAvailable(const std::string& name)
{
	feature_map_t::iterator iter = mFeatures.find(name);
	if (iter != mFeatures.end())
	{
		return iter->second.mAvailable;
	}
	LL_WARNS_ONCE("RenderInit") << "Feature " << name << " not on feature list!" << LL_ENDL;
	return TRUE;
}
BOOL LLFeatureList::maskList(LLFeatureList &mask)
{
	LLFeatureInfo mask_fi;
	feature_map_t::iterator feature_it;
	for (feature_it = mask.mFeatures.begin(); feature_it != mask.mFeatures.end(); ++feature_it)
	{
		mask_fi = feature_it->second;
		feature_map_t::iterator iter = mFeatures.find(mask_fi.mName);
		if (iter == mFeatures.end())
		{
			LL_WARNS("RenderInit") << "Feature " << mask_fi.mName << " in mask not in top level!" << LL_ENDL;
			continue;
		}
		LLFeatureInfo &cur_fi = iter->second;
		if (mask_fi.mAvailable && !cur_fi.mAvailable)
		{
			LL_WARNS("RenderInit") << "Mask attempting to reenabling disabled feature, ignoring " << cur_fi.mName << LL_ENDL;
			continue;
		}
		cur_fi.mAvailable = mask_fi.mAvailable;
		cur_fi.mRecommendedLevel = llmin(cur_fi.mRecommendedLevel, mask_fi.mRecommendedLevel);
		LL_DEBUGS("RenderInit") << "Feature mask " << mask.mName
				<< " Feature " << mask_fi.mName
				<< " Mask: " << mask_fi.mRecommendedLevel
				<< " Now: " << cur_fi.mRecommendedLevel << LL_ENDL;
	}
	LL_DEBUGS("RenderInit") << "After applying mask " << mask.mName << std::endl;
		dump();
	LL_CONT << LL_ENDL;
	return TRUE;
}
void LLFeatureList::dump()
{
	LL_DEBUGS("RenderInit") << "Feature list: " << mName << LL_ENDL;
	LL_DEBUGS("RenderInit") << "--------------" << LL_ENDL;
	LLFeatureInfo fi;
	feature_map_t::iterator feature_it;
	for (feature_it = mFeatures.begin(); feature_it != mFeatures.end(); ++feature_it)
	{
		fi = feature_it->second;
		LL_DEBUGS("RenderInit") << fi.mName << "\t\t" << fi.mAvailable << ":" << fi.mRecommendedLevel << LL_ENDL;
	}
	LL_DEBUGS("RenderInit") << LL_ENDL;
}
LLFeatureList *LLFeatureManager::findMask(const std::string& name)
{
	if (mMaskList.count(name))
	{
		return mMaskList[name];
	}
	return NULL;
}
BOOL LLFeatureManager::maskFeatures(const std::string& name)
{
	LLFeatureList *maskp = findMask(name);
	if (!maskp)
	{
 		LL_DEBUGS("RenderInit") << "Unknown feature mask " << name << LL_ENDL;
		return FALSE;
	}
	LL_INFOS("RenderInit") << "Applying GPU Feature list: " << name << LL_ENDL;
	return maskList(*maskp);
}
BOOL LLFeatureManager::loadFeatureTables()
{
	mSkippedFeatures.insert("RenderAnisotropic");
	mSkippedFeatures.insert("RenderGamma");
	mSkippedFeatures.insert("RenderVBOEnable");
	mSkippedFeatures.insert("RenderFogRatio");
	std::string app_path = gDirUtilp->getAppRODataDir();
	app_path += gDirUtilp->getDirDelimiter();
	std::string filename = FEATURE_TABLE_FILENAME;
	app_path += filename;
	return parseFeatureTable(app_path);
}
BOOL LLFeatureManager::parseFeatureTable(std::string filename)
{
	LL_INFOS() << "Looking for feature table in " << filename << LL_ENDL;
	llifstream file;
	std::string name;
	U32		version;
	file.open(filename);
	if (!file)
	{
		LL_WARNS("RenderInit") << "Unable to open feature table " << filename << LL_ENDL;
		return FALSE;
	}
	file >> name;
	file >> version;
	if (name != "version")
	{
		LL_WARNS("RenderInit") << filename << " does not appear to be a valid feature table!" << LL_ENDL;
		return FALSE;
	}
	mTableVersion = version;
	LLFeatureList *flp = NULL;
	while (file >> name)
	{
		char buffer[MAX_STRING];
		if (name.substr(0,2) == "//")
		{
			file.getline(buffer, MAX_STRING);
			continue;
		}
		if (name == "list")
		{
			if (flp)
			{
			}
			file >> name;
			if (mMaskList.count(name))
			{
				LL_ERRS("RenderInit") << "Overriding mask " << name << ", this is invalid!" << LL_ENDL;
			}
			flp = new LLFeatureList(name);
			mMaskList[name] = flp;
		}
		else
		{
			if (!flp)
			{
				LL_ERRS("RenderInit") << "Specified parameter before <list> keyword!" << LL_ENDL;
				return FALSE;
			}
			S32 available;
			F32 recommended;
			file >> available >> recommended;
			flp->addFeature(name, available, recommended);
		}
	}
	file.close();
	return TRUE;
}
void LLFeatureManager::loadGPUClass()
{
	mGPUClass = GPU_CLASS_UNKNOWN;
	mGPUString = gGLManager.getRawGLString();
	mGPUSupported = FALSE;
	std::string app_path = gDirUtilp->getAppRODataDir();
	app_path += gDirUtilp->getDirDelimiter();
	app_path += GPU_TABLE_FILENAME;
	parseGPUTable(app_path);
}
void LLFeatureManager::parseGPUTable(std::string filename)
{
	llifstream file;
	file.open(filename);
	if (!file)
	{
		LL_WARNS("RenderInit") << "Unable to open GPU table: " << filename << "!" << LL_ENDL;
		return;
	}
	std::string rawRenderer = gGLManager.getRawGLString();
	std::string renderer = rawRenderer;
	for (std::string::iterator i = renderer.begin(); i != renderer.end(); ++i)
	{
		*i = tolower(*i);
	}
	bool gpuFound;
	U32 lineNumber;
	for (gpuFound = false, lineNumber = 0; !gpuFound && !file.eof(); lineNumber++)
	{
		char buffer[MAX_STRING];
		buffer[0] = 0;
		file.getline(buffer, MAX_STRING);
		if (strlen(buffer) >= 2 &&
			buffer[0] == '/' &&
			buffer[1] == '/')
		{
			continue;
		}
		if (strlen(buffer) == 0)
		{
			continue;
		}
		std::string buf(buffer);
		std::string cls, label, expr, supported;
		boost_tokenizer tokens(buf, boost::char_separator<char>("\t\n"));
		boost_tokenizer::iterator token_iter = tokens.begin();
		if(token_iter != tokens.end())
		{
			label = *token_iter++;
		}
		if(token_iter != tokens.end())
		{
			expr = *token_iter++;
		}
		if(token_iter != tokens.end())
		{
			cls = *token_iter++;
		}
		if(token_iter != tokens.end())
		{
			supported = *token_iter++;
		}
		if (label.empty() || expr.empty() || cls.empty() || supported.empty())
		{
			LL_WARNS("RenderInit") << "invald gpu_table.txt:" << lineNumber << ": '" << buffer << "'" << LL_ENDL;
			continue;
		}
		for (U32 i = 0; i < expr.length(); i++)
		{
			expr[i] = tolower(expr[i]);
		}
		boost::regex re(expr.c_str());
		if(boost::regex_search(renderer, re))
		{
			gpuFound = true;
			mGPUString = label;
			mGPUClass = (EGPUClass) strtol(cls.c_str(), NULL, 10);
			mGPUSupported = (BOOL) strtol(supported.c_str(), NULL, 10);
		}
	}
	file.close();
	if ( gpuFound )
	{
		LL_INFOS("RenderInit") << "GPU '" << rawRenderer << "' recognized as '" << mGPUString << "'" << LL_ENDL;
		if (!mGPUSupported)
		{
			LL_INFOS("RenderInit") << "GPU '" << mGPUString << "' is not supported." << LL_ENDL;
		}
	}
	else
	{
		LL_WARNS("RenderInit") << "GPU '" << rawRenderer << "' not recognized" << LL_ENDL;
		mGPUString = rawRenderer;
		mGPUClass = EGPUClass::GPU_CLASS_3;
		mGPUSupported = true;
	}
}
void LLFeatureManager::cleanupFeatureTables()
{
	std::for_each(mMaskList.begin(), mMaskList.end(), DeletePairedPointer());
	mMaskList.clear();
}
void LLFeatureManager::init()
{
	loadFeatureTables();
	loadGPUClass();
	applyBaseMasks();
}
void LLFeatureManager::applyRecommendedSettings()
{
	S32 level = llmax(GPU_CLASS_0, llmin(mGPUClass, GPU_CLASS_2));
	LL_INFOS() << "Applying Recommended Features" << LL_ENDL;
	setGraphicsLevel(level, false);
	gSavedSettings.setU32("RenderQualityPerformance", level);
	gSavedSettings.setBOOL("RenderCustomSettings", FALSE);
	if(!gSavedSettings.getBOOL("Disregard96DefaultDrawDistance"))
	{
		gSavedSettings.setF32("RenderFarClip", 96.0f);
	}
	else if(!gSavedSettings.getBOOL("Disregard128DefaultDrawDistance"))
	{
		gSavedSettings.setF32("RenderFarClip", 128.0f);
	}
}
void LLFeatureManager::applyFeatures(bool skipFeatures)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	dump();
#endif
	for(feature_map_t::iterator mIt = mFeatures.begin();
		mIt != mFeatures.end();
		++mIt)
	{
		if(skipFeatures)
		{
			if(mSkippedFeatures.find(mIt->first) != mSkippedFeatures.end())
			{
				continue;
			}
		}
		LLControlVariable* ctrl = gSavedSettings.getControl(mIt->first);
		if(ctrl == NULL)
		{
			LL_WARNS() << "AHHH! Control setting " << mIt->first << " does not exist!" << LL_ENDL;
			continue;
		}
		if (!mIt->second.mAvailable)
		{
			LL_WARNS_ONCE("RenderInit") << "Feature " << mIt->first << " not available!" << LL_ENDL;
		}
		F32 val = mIt->second.mAvailable ? mIt->second.mRecommendedLevel : 0.f;
		if(ctrl->isType(TYPE_BOOLEAN))
		{
			gSavedSettings.setBOOL(mIt->first, (BOOL)val);
		}
		else if (ctrl->isType(TYPE_S32))
		{
			gSavedSettings.setS32(mIt->first, (S32)val);
		}
		else if (ctrl->isType(TYPE_U32))
		{
			gSavedSettings.setU32(mIt->first, (U32)val);
		}
		else if (ctrl->isType(TYPE_F32))
		{
			gSavedSettings.setF32(mIt->first, (F32)val);
		}
		else
		{
			LL_WARNS() << "AHHH! Control variable is not a numeric type!" << LL_ENDL;
		}
	}
}
void LLFeatureManager::setGraphicsLevel(S32 level, bool skipFeatures)
{
	LLViewerShaderMgr::sSkipReload = true;
	applyBaseMasks();
	switch (level)
	{
		case 0:
			maskFeatures("Low");
			break;
		case 1:
			maskFeatures("Mid");
			break;
		case 2:
			maskFeatures("High");
			break;
		case 3:
			maskFeatures("Ultra");
			break;
		default:
			maskFeatures("Low");
			break;
	}
	applyFeatures(skipFeatures);
	LLViewerShaderMgr::sSkipReload = false;
	LLViewerShaderMgr::instance()->setShaders();
}
void LLFeatureManager::applyBaseMasks()
{
	mFeatures.clear();
	LLFeatureList* maskp = findMask("all");
	if(maskp == NULL)
	{
		LL_WARNS("RenderInit") << "AHH! No \"all\" in feature table!" << LL_ENDL;
		return;
	}
	mFeatures = maskp->getFeatures();
	if (mGPUClass >= 0 && mGPUClass < 4)
	{
		const char* class_table[] =
		{
			"Class0",
			"Class1",
			"Class2",
			"Class3"
		};
		LL_INFOS("RenderInit") << "Setting GPU Class to " << class_table[mGPUClass] << LL_ENDL;
		maskFeatures(class_table[mGPUClass]);
	}
	else
	{
		LL_INFOS("RenderInit") << "Setting GPU Class to Unknown" << LL_ENDL;
		maskFeatures("Unknown");
	}
	if (!gGLManager.mHasFragmentShader)
	{
		maskFeatures("NoPixelShaders");
	}
	if (!gGLManager.mHasVertexShader || !mGPUSupported)
	{
		maskFeatures("NoVertexShaders");
	}
	if (gGLManager.mIsNVIDIA)
	{
		maskFeatures("NVIDIA");
	}
	if (gGLManager.mIsGF2or4MX)
	{
		maskFeatures("GeForce2");
	}
	if (gGLManager.mIsATI)
	{
		maskFeatures("ATI");
	}
	if (gGLManager.mHasATIMemInfo && gGLManager.mVRAM < 256)
	{
		maskFeatures("ATIVramLT256");
	}
	if (gGLManager.mATIOldDriver)
	{
		maskFeatures("ATIOldDriver");
	}
	if (gGLManager.mIsGFFX)
	{
		maskFeatures("GeForceFX");
	}
	if (gGLManager.mIsIntel && gGLManager.mGLVersion<3.0f)
	{
		maskFeatures("IntelPre30");
	}
	if (gGLManager.mIsIntel)
	{
		maskFeatures("Intel");
	}
	if (gGLManager.mGLVersion < 3.f)
	{
		maskFeatures("OpenGLPre30");
		if(gGLManager.mGLVersion < 2.1f || glUniformMatrix3x4fv == NULL)
		{
			maskFeatures("OpenGLPre21");
			if (gGLManager.mGLVersion < 1.5f)
			{
				maskFeatures("OpenGLPre15");
			}
		}
	}
	if (gGLManager.mNumTextureImageUnits <= 8)
	{
		maskFeatures("TexUnit8orLess");
	}
	if (gGLManager.mHasMapBufferRange)
	{
		maskFeatures("MapBufferRange");
	}
	if (gGLManager.mGLMaxVertexUniformComponents < 1024)
	{
		maskFeatures("VertexUniformsLT1024");
	}
	std::string gpustr = mGPUString;
	for (std::string::iterator iter = gpustr.begin(); iter != gpustr.end(); ++iter)
	{
		if (*iter == ' ')
		{
			*iter = '_';
		}
	}
	maskFeatures(gpustr);
	if (gSysMemory.getPhysicalMemoryKB() <= U32Megabytes(256))
	{
		maskFeatures("RAM256MB");
	}
	if (gSysCPU.getMHz() < 1100)
	{
		maskFeatures("CPUSlow");
	}
	if (isSafe())
	{
		maskFeatures("safe");
	}
}
