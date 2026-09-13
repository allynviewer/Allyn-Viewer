/** 
 * @file llconsole.h
 * @brief a simple console-style output device
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
#ifndef LL_LLCONSOLE_H
#define LL_LLCONSOLE_H
#include "llerrorcontrol.h"
#include "llthread.h"
#include "llview.h"
#include "v4color.h"
#include <deque>
class LLFontGL;
class LLSD;
class LLConsole : public LLLineBuffer, public LLView
{
private:
	F32			mLinePersistTime;
	F32			mFadeTime;
	LLFontGL*	mFont;
	S32			mConsoleWidth;
	S32			mConsoleHeight;
	LLMutex 	mQueueMutex;
	LLTimer		mTimer;
public:
	struct ParagraphColorSegment
	{
		S32		mNumChars;
		LLColor4 mColor;
	};
	class LineColorSegment
	{
		public:
			LineColorSegment(LLWString text, LLColor4 color, F32 xpos) : mText(text), mColor(color), mXPosition(xpos) {}
		public:
			LLWString mText;
			LLColor4  mColor;
			F32		  mXPosition;
	};
	typedef std::list<LineColorSegment> line_color_segments_t;
	typedef std::list<line_color_segments_t> lines_t;
	typedef std::list<ParagraphColorSegment> paragraph_color_segments_t;
	class Paragraph
	{
		public:
			Paragraph (LLWString str, const LLColor4 &color, F32 add_time);
			void makeParagraphColorSegments ( const LLColor4 &color);
			void updateLines ( F32 screen_width,  LLFontGL* font, bool force_resize=false );
		public:
			LLWString mParagraphText;
			paragraph_color_segments_t	mParagraphColorSegments;
			F32 mAddTime;
			F32 mMaxWidth;
			lines_t	mLines;
	};
	typedef std::deque<Paragraph*> paragraph_t;
	paragraph_t mParagraphs;
	paragraph_t mNewParagraphs;
	LLConsole(const std::string& name, const LLRect &rect,
			  S32 font_size_index, F32 persist_time );
	~LLConsole();
	void setLinePersistTime(F32 seconds);
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	void setFontSize(S32 size_index);
	void clear();
	void addLine(const std::string& utf8line);
	void addConsoleLine(const std::string& utf8line, const LLColor4 &color);
	void addConsoleLine(const LLWString& wline, const LLColor4 &color);
	void	draw();
};
extern LLConsole* gConsole;
#endif
