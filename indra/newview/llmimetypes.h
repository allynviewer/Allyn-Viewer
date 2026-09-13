/**
 * @file llmimetypes.h
 * @brief Translates a MIME type like "video/quicktime" into a
 * localizable user-friendly string like "QuickTime Movie"
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
#ifndef LLMIMETYPES_H
#define LLMIMETYPES_H
class LLMIMETypes
{
public:
	static bool parseMIMETypes(const std::string& xml_file_path);
	static std::string translate(const std::string& mime_type);
	static std::string widgetType(const std::string& mime_type);
	static std::string implType(const std::string& mime_type);
	static std::string findIcon(const std::string& mime_type);
	static std::string findToolTip(const std::string& mime_type);
	static std::string findPlayTip(const std::string& mime_type);
	static std::string findDefaultMimeType(const std::string& widget_type);
	static const std::string& getDefaultMimeType();
	static const std::string& getDefaultMimeTypeTranslation();
	static bool findAllowResize(const std::string& mime_type);
	static bool findAllowLooping(const std::string& mime_type);
	static bool isTypeHandled(const std::string& mime_type);
	static void reload(void*);
public:
	struct LLMIMEInfo
	{
		std::string mLabel;
		std::string mWidgetType;
		std::string mImpl;
	};
	struct LLMIMEWidgetSet
	{
		std::string mLabel;
		std::string mIcon;
		std::string mDefaultMimeType;
		std::string mToolTip;
		std::string mPlayTip;
		BOOL mAllowResize;
		BOOL mAllowLooping;
	};
	typedef std::map< std::string, LLMIMEInfo > mime_info_map_t;
	typedef std::map< std::string, LLMIMEWidgetSet > mime_widget_set_map_t;
	static mime_info_map_t sMap;
	static mime_widget_set_map_t sWidgetMap;
private:
};
#endif
