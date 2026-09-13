/** 
 * @file llxmlparser.cpp
 * @brief LLXmlParser implementation
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
#include "llxmlparser.h"
#include "llerror.h"
LLXmlParser::LLXmlParser()
	:
	mParser( NULL ),
	mDepth( 0 )
{
	mAuxErrorString = "no error";
	mParser = XML_ParserCreate(NULL);
	XML_SetUserData(mParser, this);
	XML_SetElementHandler(					mParser,	startElementHandler, endElementHandler);
	XML_SetCharacterDataHandler(			mParser,	characterDataHandler);
	XML_SetProcessingInstructionHandler(	mParser,	processingInstructionHandler);
	XML_SetCommentHandler(					mParser,	commentHandler);
	XML_SetCdataSectionHandler(				mParser,	startCdataSectionHandler, endCdataSectionHandler);
	XML_SetDefaultHandlerExpand(			mParser,	defaultDataHandler);
	XML_SetUnparsedEntityDeclHandler(		mParser,	unparsedEntityDeclHandler);
}
LLXmlParser::~LLXmlParser()
{
	XML_ParserFree( mParser );
}
BOOL LLXmlParser::parseFile(const std::string &path)
{
	llassert( !mDepth );
	BOOL success = TRUE;
	LLFILE* file = LLFile::fopen(path, "rb");
	if( !file )
	{
		mAuxErrorString = llformat( "Couldn't open file %s", path.c_str());
		success = FALSE;
	}
	else
	{
		S32 bytes_read = 0;
		fseek(file, 0L, SEEK_END);
		size_t buffer_size = ftell(file);
		fseek(file, 0L, SEEK_SET);
		void* buffer = XML_GetBuffer(mParser, buffer_size);
		if( !buffer )
		{
			mAuxErrorString = llformat( "Unable to allocate XML buffer while reading file %s", path.c_str() );
			success = FALSE;
			goto exit_label;
		}
		bytes_read = (S32)fread(buffer, 1, buffer_size, file);
		if( bytes_read <= 0 )
		{
			mAuxErrorString = llformat( "Error while reading file  %s", path.c_str() );
			success = FALSE;
			goto exit_label;
		}
		if( !XML_ParseBuffer(mParser, bytes_read, TRUE ) )
		{
			mAuxErrorString = llformat( "Error while parsing file  %s", path.c_str() );
			success = FALSE;
		}
exit_label:
		fclose( file );
	}
	if( success )
	{
		llassert( !mDepth );
	}
	mDepth = 0;
	if( !success )
	{
		LL_WARNS() << mAuxErrorString << LL_ENDL;
	}
	return success;
}
S32 LLXmlParser::parse( const char* buf, int len, int isFinal )
{
	return XML_Parse(mParser, buf, len, isFinal);
}
const char* LLXmlParser::getErrorString()
{
	const char* error_string = XML_ErrorString(XML_GetErrorCode( mParser ));
	if( !error_string )
	{
		error_string = mAuxErrorString.c_str();
	}
	return error_string;
}
S32 LLXmlParser::getCurrentLineNumber()
{
	return XML_GetCurrentLineNumber( mParser );
}
S32 LLXmlParser::getCurrentColumnNumber()
{
	return XML_GetCurrentColumnNumber(mParser);
}
void LLXmlParser::startElementHandler(
	 void *userData,
	 const XML_Char *name,
	 const XML_Char **atts)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->startElement( name, atts );
	self->mDepth++;
}
void LLXmlParser::endElementHandler(
	void *userData,
	const XML_Char *name)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->mDepth--;
	self->endElement( name );
}
void LLXmlParser::characterDataHandler(
	void *userData,
	const XML_Char *s,
	int len)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->characterData( s, len );
}
void LLXmlParser::processingInstructionHandler(
	void *userData,
	const XML_Char *target,
	const XML_Char *data)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->processingInstruction( target, data );
}
void LLXmlParser::commentHandler(void *userData, const XML_Char *data)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->comment( data );
}
void LLXmlParser::startCdataSectionHandler(void *userData)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->mDepth++;
	self->startCdataSection();
}
void LLXmlParser::endCdataSectionHandler(void *userData)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->endCdataSection();
	self->mDepth++;
}
void LLXmlParser::defaultDataHandler(
	void *userData,
	const XML_Char *s,
	int len)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->defaultData( s, len );
}
void LLXmlParser::unparsedEntityDeclHandler(
	void *userData,
	const XML_Char *entityName,
	const XML_Char *base,
	const XML_Char *systemId,
	const XML_Char *publicId,
	const XML_Char *notationName)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->unparsedEntityDecl( entityName, base, systemId, publicId, notationName );
}
