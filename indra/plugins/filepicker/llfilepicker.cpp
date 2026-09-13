/** 
 * @file llfilepicker.cpp
 * @brief OS-specific file picker
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#include "llfilepicker.h"
#include "llerror.h"
#include "basic_plugin_base.h"
LLFilePicker LLFilePicker::sInstance;
#if LL_WINDOWS
#define SOUND_FILTER L"Sounds (*.wav; *.ogg)\0*.wav;*.ogg\0"
#define IMAGE_FILTER L"Images (*.tga; *.bmp; *.jpg; *.jpeg; *.png; *.jp2; *.j2k; *.j2c)\0*.tga;*.bmp;*.jpg;*.jpeg;*.png;*.jp2;*.j2k;*.j2c\0"
#define INVGZ_FILTER L"Inv cache (*.inv; *.inv.gz)\0*.inv;*.inv.gz\0"
#define AO_FILTER L"Animation Override (*.ao)\0*.ao\0"
#define BLACKLIST_FILTER L"Asset Blacklist (*.blacklist)\0*.blacklist\0"
#define ANIM_FILTER L"Animations (*.bvh; *.anim)\0*.bvh;*.anim\0"
#define COLLADA_FILTER L"Scene (*.dae)\0*.dae\0"
#ifdef _CORY_TESTING
#define GEOMETRY_FILTER L"SL Geometry (*.slg)\0*.slg\0"
#endif
#define XML_FILTER L"XML files (*.xml)\0*.xml\0"
#define SLOBJECT_FILTER L"Objects (*.slobject)\0*.slobject\0"
#define RAW_FILTER L"RAW files (*.raw)\0*.raw\0"
#define MODEL_FILTER L"Model files (*.dae)\0*.dae\0"
#define SCRIPT_FILTER L"Script files (*.lsl)\0*.lsl\0"
#define DICTIONARY_FILTER L"Dictionary files (*.dic; *.xcu)\0*.dic;*.xcu\0"
#endif
LLFilePickerBase::LLFilePickerBase()
	: mCurrentFile(0),
	  mLocked(FALSE)
{
	reset();
#if LL_WINDOWS
	mOFN.lStructSize = sizeof(OPENFILENAMEW);
	mOFN.hwndOwner = NULL;
	mOFN.hInstance = NULL;
	mOFN.lpstrCustomFilter = NULL;
	mOFN.nMaxCustFilter = 0;
	mOFN.lpstrFile = NULL;
	mOFN.nMaxFile = LL_MAX_PATH;
	mOFN.lpstrFileTitle = NULL;
	mOFN.nMaxFileTitle = 0;
	mOFN.lpstrInitialDir = NULL;
	mOFN.lpstrTitle = NULL;
	mOFN.Flags = 0;
	mOFN.nFileOffset = 0;
	mOFN.nFileExtension = 0;
	mOFN.lpstrDefExt = NULL;
	mOFN.lCustData = 0L;
	mOFN.lpfnHook = NULL;
	mOFN.lpTemplateName = NULL;
#endif
#if LL_DARWIN
	memset(&mNavOptions, 0, sizeof(mNavOptions));
	OSStatus	error = NavGetDefaultDialogCreationOptions(&mNavOptions);
	if (error == noErr)
	{
		mNavOptions.modality = kWindowModalityAppModal;
	}
#endif
}
const std::string LLFilePickerBase::getFirstFile()
{
	mCurrentFile = 0;
	return getNextFile();
}
const std::string LLFilePickerBase::getNextFile()
{
	if (mCurrentFile >= getFileCount())
	{
		mLocked = FALSE;
		return std::string();
	}
	else
	{
		return mFiles[mCurrentFile++];
	}
}
const std::string LLFilePickerBase::getCurFile()
{
	if (mCurrentFile >= getFileCount())
	{
		mLocked = FALSE;
		return std::string();
	}
	else
	{
		return mFiles[mCurrentFile];
	}
}
void LLFilePickerBase::reset()
{
	mLocked = FALSE;
	mFiles.clear();
	mCurrentFile = 0;
}
#if LL_WINDOWS
bool LLFilePickerBase::setupFilter(ELoadFilter filter)
{
	bool res = TRUE;
	switch (filter)
	{
	case FFLOAD_ALL:
		mOFN.lpstrFilter = L"All Files (*.*)\0*.*\0" \
		SOUND_FILTER \
		IMAGE_FILTER \
		ANIM_FILTER \
		L"\0";
		break;
	case FFLOAD_WAV:
		mOFN.lpstrFilter = SOUND_FILTER \
			L"\0";
		break;
	case FFLOAD_IMAGE:
		mOFN.lpstrFilter = IMAGE_FILTER \
			L"\0";
		break;
	case FFLOAD_ANIM:
		mOFN.lpstrFilter = ANIM_FILTER \
			L"\0";
		break;
	case FFLOAD_COLLADA:
		mOFN.lpstrFilter = COLLADA_FILTER \
			L"\0";
		break;
#ifdef _CORY_TESTING
	case FFLOAD_GEOMETRY:
		mOFN.lpstrFilter = GEOMETRY_FILTER \
			L"\0";
		break;
#endif
	case FFLOAD_XML:
		mOFN.lpstrFilter = XML_FILTER \
			L"\0";
		break;
	case FFLOAD_SLOBJECT:
		mOFN.lpstrFilter = SLOBJECT_FILTER \
			L"\0";
		break;
	case FFLOAD_RAW:
		mOFN.lpstrFilter = RAW_FILTER \
			L"\0";
		break;
	case FFLOAD_MODEL:
		mOFN.lpstrFilter = MODEL_FILTER \
			L"\0";
		break;
	case FFLOAD_SCRIPT:
		mOFN.lpstrFilter = SCRIPT_FILTER \
			L"\0";
		break;
	case FFLOAD_DICTIONARY:
		mOFN.lpstrFilter = DICTIONARY_FILTER \
			L"\0";
		break;
	case FFLOAD_INVGZ:
		mOFN.lpstrFilter = INVGZ_FILTER \
			L"\0";
		break;
	case FFLOAD_AO:
		mOFN.lpstrFilter = AO_FILTER \
			L"\0";
		break;
	case FFLOAD_BLACKLIST:
		mOFN.lpstrFilter = BLACKLIST_FILTER \
			L"\0";
		break;
	default:
		res = FALSE;
		break;
	}
	return res;
}
bool LLFilePickerBase::getLoadFile(ELoadFilter filter, std::string const& folder)
{
	if( mLocked )
	{
		return FALSE;
	}
	bool success = FALSE;
	llutf16string tstring = utf8str_to_utf16str(folder);
	mOFN.lpstrInitialDir = (LPCTSTR)tstring.data();
	mFilesW[0] = '\0';
	mOFN.lpstrFile = mFilesW;
	mOFN.nMaxFile = SINGLE_FILENAME_BUFFER_SIZE;
	mOFN.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR ;
	mOFN.nFilterIndex = 1;
	setupFilter(filter);
	reset();
	success = GetOpenFileName(&mOFN);
	if (success)
	{
		std::string filename = utf16str_to_utf8str(llutf16string(mFilesW));
		mFiles.push_back(filename);
	}
	return success;
}
bool LLFilePickerBase::getMultipleLoadFiles(ELoadFilter filter, std::string const& folder)
{
	if( mLocked )
	{
		return FALSE;
	}
	bool success = FALSE;
	llutf16string tstring = utf8str_to_utf16str(folder);
	mOFN.lpstrInitialDir = (LPCTSTR)tstring.data();
	mFilesW[0] = '\0';
	mOFN.lpstrFile = mFilesW;
	mOFN.nFilterIndex = 1;
	mOFN.nMaxFile = FILENAME_BUFFER_SIZE;
	mOFN.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR |
		OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	setupFilter(filter);
	reset();
	success = GetOpenFileName(&mOFN);
	if( success )
	{
		if( wcslen(mOFN.lpstrFile) > mOFN.nFileOffset )
		{
			std::string filename = utf16str_to_utf8str(llutf16string(mFilesW));
			mFiles.push_back(filename);
		}
		else
		{
			mLocked = TRUE;
			WCHAR* tptrw = mFilesW;
			std::string dirname;
			while(1)
			{
				if (*tptrw == 0 && *(tptrw+1) == 0)
					break;
				if (*tptrw == 0)
					tptrw++;
				std::string filename = utf16str_to_utf8str(llutf16string(tptrw));
				if (dirname.empty())
					dirname = filename + "\\";
				else
					mFiles.push_back(dirname + filename);
				tptrw += filename.size();
			}
		}
	}
	return success;
}
bool LLFilePickerBase::getSaveFile(ESaveFilter filter, std::string const& filename, std::string const& folder)
{
	if( mLocked )
	{
		return FALSE;
	}
	bool success = FALSE;
	llutf16string tstring = utf8str_to_utf16str(folder);
	mOFN.lpstrInitialDir = (LPCTSTR)tstring.data();
	mOFN.lpstrFile = mFilesW;
	if (!filename.empty())
	{
		llutf16string tstring = utf8str_to_utf16str(filename);
		wcsncpy(mFilesW, tstring.data(), FILENAME_BUFFER_SIZE);	}
	else
	{
		mFilesW[0] = '\0';
	}
	switch( filter )
	{
	case FFSAVE_ALL:
		mOFN.lpstrDefExt = NULL;
		mOFN.lpstrFilter =
			L"All Files (*.*)\0*.*\0" \
			L"WAV Sounds (*.wav)\0*.wav\0" \
			L"Targa, Bitmap Images (*.tga; *.bmp)\0*.tga;*.bmp\0" \
			L"\0";
		break;
	case FFSAVE_WAV:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.wav", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"wav";
		mOFN.lpstrFilter =
			L"WAV Sounds (*.wav)\0*.wav\0" \
			L"\0";
		break;
	case FFSAVE_TGA:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.tga", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"tga";
		mOFN.lpstrFilter =
			L"Targa Images (*.tga)\0*.tga\0" \
			L"\0";
		break;
	case FFSAVE_BMP:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.bmp", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"bmp";
		mOFN.lpstrFilter =
			L"Bitmap Images (*.bmp)\0*.bmp\0" \
			L"\0";
		break;
	case FFSAVE_PNG:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.png", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"png";
		mOFN.lpstrFilter =
			L"PNG Images (*.png)\0*.png\0" \
			L"\0";
		break;
	case FFSAVE_JPEG:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.jpg", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"jpg";
		mOFN.lpstrFilter =
			L"JPEG Images (*.jpg *.jpeg)\0*.jpg;*.jpeg\0" \
			L"\0";
		break;
	case FFSAVE_AVI:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.avi", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"avi";
		mOFN.lpstrFilter =
			L"AVI Movie File (*.avi)\0*.avi\0" \
			L"\0";
		break;
	case FFSAVE_ANIM:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.xaf", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"xaf";
		mOFN.lpstrFilter =
			L"XAF Anim File (*.xaf)\0*.xaf\0" \
			L"\0";
		break;
#ifdef _CORY_TESTING
	case FFSAVE_GEOMETRY:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.slg", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"slg";
		mOFN.lpstrFilter =
			L"SLG SL Geometry File (*.slg)\0*.slg\0" \
			L"\0";
		break;
#endif
	case FFSAVE_XML:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.xml", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"xml";
		mOFN.lpstrFilter =
			L"XML File (*.xml)\0*.xml\0" \
			L"\0";
		break;
	case FFSAVE_COLLADA:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.collada", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"collada";
		mOFN.lpstrFilter =
			L"COLLADA File (*.collada)\0*.collada\0" \
			L"\0";
		break;
	case FFSAVE_RAW:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.raw", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"raw";
		mOFN.lpstrFilter =	RAW_FILTER \
							L"\0";
		break;
	case FFSAVE_J2C:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.j2c", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"j2c";
		mOFN.lpstrFilter =
			L"Compressed Images (*.j2c)\0*.j2c\0" \
			L"\0";
		break;
	case FFSAVE_ANIMATN:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.animatn", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"animatn";
		mOFN.lpstrFilter =
			L"SL Animations (*.animatn)\0*.animatn\0" \
			L"\0";
		break;
	case FFSAVE_OGG:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.ogg", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"ogg";
		mOFN.lpstrFilter =
			L"Ogg (*.ogg)\0*.ogg\0" \
			L"\0";
		break;
	case FFSAVE_NOTECARD:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.notecard", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"notecard";
		mOFN.lpstrFilter =
			L"Notecards (*.notecard)\0*.notecard\0" \
			L"\0";
		break;
	case FFSAVE_GESTURE:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.gesture", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"gesture";
		mOFN.lpstrFilter =
			L"Gestures (*.gesture)\0*.gesture\0" \
			L"\0";
		break;
	case FFSAVE_SCRIPT:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.lsl", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"lsl";
		mOFN.lpstrFilter = L"LSL Files (*.lsl)\0*.lsl\0" L"\0";
		break;
	case FFSAVE_SHAPE:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.shape", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"shape";
		mOFN.lpstrFilter =
			L"Shapes (*.shape)\0*.shape\0" \
			L"\0";
		break;
	case FFSAVE_SKIN:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.skin", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"skin";
		mOFN.lpstrFilter =
			L"Skins (*.skin)\0*.skin\0" \
			L"\0";
		break;
	case FFSAVE_HAIR:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.hair", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"hair";
		mOFN.lpstrFilter =
			L"Hair (*.hair)\0*.hair\0" \
			L"\0";
		break;
	case FFSAVE_EYES:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.eyes", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"eyes";
		mOFN.lpstrFilter =
			L"Eyes (*.eyes)\0*.eyes\0" \
			L"\0";
		break;
	case FFSAVE_SHIRT:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.shirt", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"shirt";
		mOFN.lpstrFilter =
			L"Shirts (*.shirt)\0*.shirt\0" \
			L"\0";
		break;
	case FFSAVE_PANTS:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.pants", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"pants";
		mOFN.lpstrFilter =
			L"Pants (*.pants)\0*.pants\0" \
			L"\0";
		break;
	case FFSAVE_SHOES:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.shoes", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"shoes";
		mOFN.lpstrFilter =
			L"Shoes (*.shoes)\0*.shoes\0" \
			L"\0";
		break;
	case FFSAVE_SOCKS:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.socks", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"socks";
		mOFN.lpstrFilter =
			L"Socks (*.socks)\0*.socks\0" \
			L"\0";
		break;
	case FFSAVE_JACKET:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.jacket", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"jacket";
		mOFN.lpstrFilter =
			L"Jackets (*.jacket)\0*.jacket\0" \
			L"\0";
		break;
	case FFSAVE_GLOVES:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.gloves", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"gloves";
		mOFN.lpstrFilter =
			L"Gloves (*.gloves)\0*.gloves\0" \
			L"\0";
		break;
	case FFSAVE_UNDERSHIRT:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.undershirt", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"undershirt";
		mOFN.lpstrFilter =
			L"Undershirts (*.undershirt)\0*.undershirt\0" \
			L"\0";
		break;
	case FFSAVE_UNDERPANTS:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.underpants", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"underpants";
		mOFN.lpstrFilter =
			L"Underpants (*.underpants)\0*.underpants\0" \
			L"\0";
		break;
	case FFSAVE_SKIRT:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.skirt", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"skirt";
		mOFN.lpstrFilter =
			L"Skirts (*.skirt)\0*.skirt\0" \
			L"\0";
		break;
	case FFSAVE_LANDMARK:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.landmark", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"landmark";
		mOFN.lpstrFilter =
			L"Landmarks (*.landmark)\0*.landmark\0" \
			L"\0";
		break;
	case FFSAVE_AO:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.ao", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"ao";
		mOFN.lpstrFilter =
			L"Animation overrides (*.ao)\0*.ao\0" \
			L"\0";
		break;
	case FFSAVE_INVGZ:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.inv", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L".inv";
		mOFN.lpstrFilter =
			L"InvCache (*.inv)\0*.inv\0" \
			L"\0";
		break;
	case FFSAVE_BLACKLIST:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.blacklist", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L".blacklist";
		mOFN.lpstrFilter =
			L"Asset Blacklists (*.blacklist)\0*.blacklist\0" \
			L"\0";
		break;
	case FFSAVE_PHYSICS:
		if(filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.phy", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"phy";
		mOFN.lpstrFilter =
			L"Landmarks (*.phy)\0*.phy\0" \
			L"\0";
		break;
	case FFSAVE_IMAGE:
		mOFN.lpstrDefExt = NULL;
		mOFN.lpstrFilter =
			L"Image (*.bmp *.dxt *.jpg *.jpeg *.j2c *.mip *.png *.tga)\0*.bmp;*.dxt;*.jpg;*.jpeg;*.j2c;*.mip;*.png;*.tga\0" \
			L"PNG Image (*.png)\0*.png\0" \
			L"Targa Image (*.tga)\0*.tga\0" \
			L"Bitmap Image (*.bmp)\0*.bmp\0" \
			L"JPEG Image (*.jpg *.jpeg)\0*.jpg;*.jpeg\0" \
			L"Compressed Image (*.j2c)\0*.j2c\0" \
			L"DXT Image (*.dxt *.mip)\0*.dxt;*.mip\0" \
			L"\0";
		break;
	default:
		return FALSE;
	}
	mOFN.nMaxFile = SINGLE_FILENAME_BUFFER_SIZE;
	mOFN.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
	reset();
	{
		success = GetSaveFileName(&mOFN);
		if (success)
		{
			std::string filename = utf16str_to_utf8str(llutf16string(mFilesW));
			mFiles.push_back(filename);
		}
	}
	return success;
}
#elif LL_DARWIN
Boolean LLFilePickerBase::navOpenFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode)
{
	LLFilePickerBase* picker = (LLFilePickerBase*)callBackUD;
	Boolean		result = true;
	ELoadFilter filter = picker->getLoadFilter();
	OSStatus	error = noErr;
	if (filterMode == kNavFilteringBrowserList && filter != FFLOAD_ALL && (theItem->descriptorType == typeFSRef || theItem->descriptorType == typeFSS))
	{
		NavFileOrFolderInfo	*navInfo = (NavFileOrFolderInfo*) info;
		if (!navInfo->isFolder)
		{
			AEDesc	desc;
			error = AECoerceDesc(theItem, typeFSRef, &desc);
			if (error == noErr)
			{
				FSRef	fileRef;
				error = AEGetDescData(&desc, &fileRef, sizeof(fileRef));
				if (error == noErr)
				{
					LSItemInfoRecord	fileInfo;
					error = LSCopyItemInfoForRef(&fileRef, kLSRequestExtension | kLSRequestTypeCreator, &fileInfo);
					if (error == noErr)
					{
						if (filter == FFLOAD_IMAGE)
						{
							if (fileInfo.filetype != 'JPEG' && fileInfo.filetype != 'JPG ' &&
								fileInfo.filetype != 'BMP ' && fileInfo.filetype != 'TGA ' &&
								fileInfo.filetype != 'BMPf' && fileInfo.filetype != 'TPIC' &&
								fileInfo.filetype != 'PNG ' && fileInfo.filetype != 'JP2 ' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("jpeg"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("jpg"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("bmp"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("tga"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("png"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("jp2"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("j2k"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("j2c"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
								)
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_WAV)
						{
							if (fileInfo.filetype != 'WAVE' && fileInfo.filetype != 'WAV ' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("wave"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("wav"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_ANIM)
						{
							if (fileInfo.filetype != 'BVH ' &&  fileInfo.filetype != 'ANIM' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("bvh"), kCFCompareCaseInsensitive) != kCFCompareEqualTo) &&
								 CFStringCompare(fileInfo.extension, CFSTR("anim"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_COLLADA)
						{
							if (fileInfo.filetype != 'DAE ' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("dae"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_XML)
						{
							if (fileInfo.filetype != 'XML ' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("xml"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
#ifdef _CORY_TESTING
						else if (filter == FFLOAD_GEOMETRY)
						{
							if (fileInfo.filetype != 'SLG ' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("slg"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
#endif
						else if (filter == FFLOAD_SLOBJECT)
						{
							PLS_WARNS << "*** navOpenFilterProc: FFLOAD_SLOBJECT NOT IMPLEMENTED ***" << PLS_ENDL;
						}
						else if (filter == FFLOAD_RAW)
						{
							if (fileInfo.filetype != '\?\?\?\?' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("raw"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_SCRIPT)
						{
							if (fileInfo.filetype != 'LSL ' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("lsl"), kCFCompareCaseInsensitive) != kCFCompareEqualTo)) )
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_DICTIONARY)
						{
							if (fileInfo.filetype != 'DIC ' &&
								fileInfo.filetype != 'XCU ' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("dic"), kCFCompareCaseInsensitive) != kCFCompareEqualTo) &&
								 fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("xcu"), kCFCompareCaseInsensitive) != kCFCompareEqualTo)))
							{
								result = false;
							}
						}
						if (fileInfo.extension)
						{
							CFRelease(fileInfo.extension);
						}
					}
				}
				AEDisposeDesc(&desc);
			}
		}
	}
	return result;
}
pascal void LLFilePickerBase::doNavCallbackEvent(NavEventCallbackMessage callBackSelector,
                                         NavCBRecPtr callBackParms, void* callBackUD)
{
    switch(callBackSelector)
    {
        case kNavCBStart:
        {
			LLFilePickerBase* picker = reinterpret_cast<LLFilePickerBase*>(callBackUD);
            if (picker->getFolder().empty()) break;
            OSStatus error = noErr;
            AEDesc theLocation = {typeNull, NULL};
            FSSpec outFSSpec;
            FSRef myFSRef;
            const char* folder = picker->getFolder().c_str();
            error = FSPathMakeRef ((UInt8*)folder,  &myFSRef,   NULL);
            if (error != noErr) break;
            error = FSGetCatalogInfo (&myFSRef, kFSCatInfoNone, NULL, NULL, &outFSSpec, NULL);
            if (error != noErr) break;
            error = AECreateDesc(typeFSS, &outFSSpec, sizeof(FSSpec), &theLocation);
            if (error != noErr) break;
            error = NavCustomControl(callBackParms->context,
                            kNavCtlSetLocation, (void*)&theLocation);
        }
    }
}
OSStatus	LLFilePickerBase::doNavChooseDialog(ELoadFilter filter, std::string const& folder)
{
	OSStatus		error = noErr;
	NavDialogRef	navRef = NULL;
	NavReplyRecord	navReply;
	memset(&navReply, 0, sizeof(navReply));
	mLoadFilter = filter;
	mFolder = folder;
	NavEventUPP eventProc = NewNavEventUPP(doNavCallbackEvent);
	error = NavCreateChooseFileDialog(&mNavOptions, NULL, eventProc, NULL, navOpenFilterProc, (void*)this, &navRef);
	if (error == noErr)
	{
		PLS_FLUSH;
		error = NavDialogRun(navRef);
	}
	if (error == noErr)
		error = NavDialogGetReply(navRef, &navReply);
	if (navRef)
		NavDialogDispose(navRef);
	if (error == noErr && navReply.validRecord)
	{
		SInt32	count = 0;
		SInt32	index;
		error = AECountItems(&navReply.selection, &count);
		for (index = 1; index <= count; index++)
		{
			FSRef		fsRef;
			AEKeyword	theAEKeyword;
			DescType	typeCode;
			Size		actualSize = 0;
			char		path[MAX_PATH];
			memset(&fsRef, 0, sizeof(fsRef));
			error = AEGetNthPtr(&navReply.selection, index, typeFSRef, &theAEKeyword, &typeCode, &fsRef, sizeof(fsRef), &actualSize);
			if (error == noErr)
				error = FSRefMakePath(&fsRef, (UInt8*) path, sizeof(path));
			if (error == noErr)
				mFiles.push_back(std::string(path));
		}
	}
	if (eventProc)
		DisposeNavEventUPP(eventProc);
	return error;
}
OSStatus	LLFilePickerBase::doNavSaveDialog(ESaveFilter filter, std::string const& filename, std::string const& folder)
{
	OSStatus		error = noErr;
	NavDialogRef	navRef = NULL;
	NavReplyRecord	navReply;
	memset(&navReply, 0, sizeof(navReply));
	OSType		type, creator;
	CFStringRef	extension = NULL;
	switch (filter)
	{
		case FFSAVE_WAV:
			type = 'WAVE';
			creator = 'TVOD';
			extension = CFSTR(".wav");
			break;
		case FFSAVE_TGA:
			type = 'TPIC';
			creator = 'prvw';
			extension = CFSTR(".tga");
			break;
		case FFSAVE_BMP:
			type = 'BMPf';
			creator = 'prvw';
			extension = CFSTR(".bmp");
			break;
		case FFSAVE_JPEG:
			type = 'JPEG';
			creator = 'prvw';
			extension = CFSTR(".jpeg");
			break;
		case FFSAVE_PNG:
			type = 'PNG ';
			creator = 'prvw';
			extension = CFSTR(".png");
			break;
		case FFSAVE_AVI:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR(".mov");
			break;
		case FFSAVE_ANIM:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR(".xaf");
			break;
#ifdef _CORY_TESTING
		case FFSAVE_GEOMETRY:
			type = L'\?\?\?\?';
			creator = L'\?\?\?\?';
			extension = CFSTR(".slg");
			break;
#endif
		case FFSAVE_RAW:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR(".raw");
			break;
		case FFSAVE_J2C:
			type = '\?\?\?\?';
			creator = 'prvw';
			extension = CFSTR(".j2c");
			break;
		case FFSAVE_SCRIPT:
			type = 'LSL ';
			creator = '\?\?\?\?';
			extension = CFSTR(".lsl");
			break;
		case FFSAVE_ALL:
		default:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR("");
			break;
	}
	NavEventUPP eventUPP = NewNavEventUPP(NULL);
	mFolder = folder;
	error = NavCreatePutFileDialog(&mNavOptions, type, creator, eventUPP, (void*)this, &navRef);
	if (error == noErr)
	{
		CFStringRef	nameString = NULL;
		bool		hasExtension = true;
		if (!filename.empty())
			nameString = CFStringCreateWithCString(NULL, filename.c_str(), kCFStringEncodingUTF8);
		else
			nameString = CFSTR("Untitled");
		if (nameString && !CFStringHasSuffix(nameString, extension))
		{
			CFStringRef	tempString = nameString;
			hasExtension = false;
			nameString = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@%@"), tempString, extension);
			CFRelease(tempString);
		}
		if (nameString)
		{
			error = NavDialogSetSaveFileName(navRef, nameString);
			CFRelease(nameString);
		}
		else
		{
			error = paramErr;
		}
	}
	if (error == noErr)
	{
		PLS_FLUSH;
		error = NavDialogRun(navRef);
	}
	if (error == noErr)
		error = NavDialogGetReply(navRef, &navReply);
	if (navRef)
		NavDialogDispose(navRef);
	if (error == noErr && navReply.validRecord)
	{
		SInt32	count = 0;
		error = AECountItems(&navReply.selection, &count);
		if (count > 0)
		{
			FSRef		fsRef;
			AEKeyword	theAEKeyword;
			DescType	typeCode;
			Size		actualSize = 0;
			memset(&fsRef, 0, sizeof(fsRef));
			error = AEGetNthPtr(&navReply.selection, 1, typeFSRef, &theAEKeyword, &typeCode, &fsRef, sizeof(fsRef), &actualSize);
			if (error == noErr)
			{
				char	path[PATH_MAX];
				char	newFileName[SINGLE_FILENAME_BUFFER_SIZE];
				error = FSRefMakePath(&fsRef, (UInt8*)path, PATH_MAX);
				if (error == noErr)
				{
					if (CFStringGetCString(navReply.saveFileName, newFileName, sizeof(newFileName), kCFStringEncodingUTF8))
					{
						mFiles.push_back(std::string(path) + "/" +  std::string(newFileName));
					}
					else
					{
						error = paramErr;
					}
				}
				else
				{
					error = paramErr;
				}
			}
		}
	}
	if (eventUPP)
		DisposeNavEventUPP(eventUPP);
	return error;
}
bool LLFilePickerBase::getLoadFile(ELoadFilter filter, std::string const& folder)
{
	if( mLocked )
		return FALSE;
	bool success = FALSE;
	OSStatus	error = noErr;
	reset();
	mNavOptions.optionFlags &= ~kNavAllowMultipleFiles;
	{
		error = doNavChooseDialog(filter, folder);
	}
	if (error == noErr)
	{
		if (getFileCount())
			success = true;
	}
	return success;
}
bool LLFilePickerBase::getMultipleLoadFiles(ELoadFilter filter, std::string const& folder)
{
	if( mLocked )
		return FALSE;
	bool success = FALSE;
	OSStatus	error = noErr;
	reset();
	mNavOptions.optionFlags |= kNavAllowMultipleFiles;
	{
		error = doNavChooseDialog(filter, folder);
	}
	if (error == noErr)
	{
		if (getFileCount())
			success = true;
		if (getFileCount() > 1)
			mLocked = TRUE;
	}
	return success;
}
bool LLFilePickerBase::getSaveFile(ESaveFilter filter, std::string const& filename, std::string const& folder)
{
	if( mLocked )
		return FALSE;
	bool success = FALSE;
	OSStatus	error = noErr;
	reset();
	mNavOptions.optionFlags &= ~kNavAllowMultipleFiles;
	{
		error = doNavSaveDialog(filter, filename, folder);
	}
	if (error == noErr)
	{
		if (getFileCount())
			success = true;
	}
	return success;
}
#else
bool LLFilePickerBase::getSaveFile(ESaveFilter filter, std::string const& filename, std::string const& folder)
{
	reset();
	return FALSE;
}
bool LLFilePickerBase::getLoadFile(ELoadFilter filter, std::string const& folder)
{
	reset();
	return FALSE;
}
bool LLFilePickerBase::getMultipleLoadFiles(ELoadFilter filter, std::string const& folder)
{
	reset();
	return FALSE;
}
#endif
