/** 
 * @file llimagejpeg.cpp
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
#include "stdtypes.h"
#include "llimagejpeg.h"
#include "llerror.h"
jmp_buf	LLImageJPEG::sSetjmpBuffer ;
LLImageJPEG::LLImageJPEG(S32 quality)
	:
	LLImageFormatted(IMG_CODEC_JPEG),
	mOutputBuffer( NULL ),
	mOutputBufferSize( 0 ),
	mEncodeQuality( quality )
{
}
LLImageJPEG::~LLImageJPEG()
{
	llassert( !mOutputBuffer );
	delete[] mOutputBuffer;
}
BOOL LLImageJPEG::updateData()
{
	resetLastError();
	if (!getData() || (0 == getDataSize()))
	{
		setLastError("Uninitialized instance of LLImageJPEG");
		return FALSE;
	}
	struct jpeg_decompress_struct cinfo;
	cinfo.client_data = this;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jerr.error_exit =		&LLImageJPEG::errorExit;
	jerr.emit_message =		&LLImageJPEG::errorEmitMessage;
	jerr.output_message =	&LLImageJPEG::errorOutputMessage;
	if(setjmp(sSetjmpBuffer))
	{
		jpeg_destroy_decompress(&cinfo);
		return FALSE;
	}
	try
	{
		jpeg_create_decompress(&cinfo);
		if (cinfo.src == NULL)
		{
			cinfo.src = (struct jpeg_source_mgr *)
				(*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
				sizeof(struct jpeg_source_mgr));
		}
		cinfo.src->init_source	=		&LLImageJPEG::decodeInitSource;
		cinfo.src->fill_input_buffer =	&LLImageJPEG::decodeFillInputBuffer;
		cinfo.src->skip_input_data =	&LLImageJPEG::decodeSkipInputData;
		cinfo.src->resync_to_restart = jpeg_resync_to_restart;
		cinfo.src->term_source =		&LLImageJPEG::decodeTermSource;
		cinfo.src->bytes_in_buffer =	getDataSize();
		cinfo.src->next_input_byte =	getData();
		jpeg_read_header( &cinfo, TRUE );
		setSize(cinfo.image_width, cinfo.image_height, 3);
	}
	catch (int)
	{
		jpeg_destroy_decompress(&cinfo);
		return FALSE;
	}
	jpeg_destroy_decompress(&cinfo);
	return TRUE;
}
void LLImageJPEG::decodeInitSource( j_decompress_ptr cinfo )
{
}
boolean LLImageJPEG::decodeFillInputBuffer( j_decompress_ptr cinfo )
{
	ERREXIT(cinfo, JERR_INPUT_EMPTY);
	return TRUE;
}
void LLImageJPEG::decodeSkipInputData (j_decompress_ptr cinfo, long num_bytes)
{
	jpeg_source_mgr* src = cinfo->src;
    src->next_input_byte += (size_t) num_bytes;
    src->bytes_in_buffer -= (size_t) num_bytes;
}
void LLImageJPEG::decodeTermSource (j_decompress_ptr cinfo)
{
}
BOOL LLImageJPEG::decode(LLImageRaw* raw_image, F32 decode_time)
{
	llassert_always(raw_image);
	resetLastError();
	if (!getData() || (0 == getDataSize()))
	{
		setLastError("LLImageJPEG trying to decode an image with no data!");
		return TRUE;
	}
	S32 row_stride = 0;
	U8* raw_image_data = NULL;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jerr.error_exit =		&LLImageJPEG::errorExit;
	jerr.emit_message =		&LLImageJPEG::errorEmitMessage;
	jerr.output_message =	&LLImageJPEG::errorOutputMessage;
	if(setjmp(sSetjmpBuffer))
	{
		jpeg_destroy_decompress(&cinfo);
		return TRUE;
	}
	try
	{
		jpeg_create_decompress(&cinfo);
		if (cinfo.src == NULL)
		{
			cinfo.src = (struct jpeg_source_mgr *)
				(*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
				sizeof(struct jpeg_source_mgr));
		}
		cinfo.src->init_source	=		&LLImageJPEG::decodeInitSource;
		cinfo.src->fill_input_buffer =	&LLImageJPEG::decodeFillInputBuffer;
		cinfo.src->skip_input_data =	&LLImageJPEG::decodeSkipInputData;
		cinfo.src->resync_to_restart = jpeg_resync_to_restart;
		cinfo.src->term_source =		&LLImageJPEG::decodeTermSource;
		cinfo.src->bytes_in_buffer =	getDataSize();
		cinfo.src->next_input_byte =	getData();
		jpeg_read_header(&cinfo, TRUE);
		setSize(cinfo.image_width, cinfo.image_height, 3);
		raw_image->resize(getWidth(), getHeight(), getComponents());
		raw_image_data = raw_image->getData();
		cinfo.out_color_components = 3;
		cinfo.out_color_space = JCS_RGB;
		jpeg_start_decompress(&cinfo);
		row_stride = cinfo.output_width * cinfo.output_components;
		raw_image_data += row_stride * (cinfo.output_height - 1);
		while (cinfo.output_scanline < cinfo.output_height)
		{
			jpeg_read_scanlines(&cinfo, &raw_image_data, 1);
			raw_image_data -= row_stride;
		}
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
	}
	catch (int)
	{
		jpeg_destroy_decompress(&cinfo);
		return TRUE;
	}
	if( jerr.num_warnings != 0 )
	{
		setLastError( "Unable to decode JPEG image.");
		return TRUE;
	}
	return TRUE;
}
void LLImageJPEG::encodeInitDestination ( j_compress_ptr cinfo )
{
  LLImageJPEG* self = (LLImageJPEG*) cinfo->client_data;
  cinfo->dest->next_output_byte = self->mOutputBuffer;
  cinfo->dest->free_in_buffer = self->mOutputBufferSize;
}
boolean LLImageJPEG::encodeEmptyOutputBuffer( j_compress_ptr cinfo )
{
  LLImageJPEG* self = (LLImageJPEG*) cinfo->client_data;
  S32 new_buffer_size = self->mOutputBufferSize * 2;
  U8* new_buffer = new U8[ new_buffer_size ];
  if (!new_buffer)
  {
  	LL_ERRS() << "Out of memory in LLImageJPEG::encodeEmptyOutputBuffer( j_compress_ptr cinfo )" << LL_ENDL;
  	return FALSE;
  }
  memcpy( new_buffer, self->mOutputBuffer, self->mOutputBufferSize );
  delete[] self->mOutputBuffer;
  self->mOutputBuffer = new_buffer;
  cinfo->dest->next_output_byte = self->mOutputBuffer + self->mOutputBufferSize;
  cinfo->dest->free_in_buffer = self->mOutputBufferSize;
  self->mOutputBufferSize = new_buffer_size;
  return TRUE;
}
void LLImageJPEG::encodeTermDestination( j_compress_ptr cinfo )
{
	LLImageJPEG* self = (LLImageJPEG*) cinfo->client_data;
	S32 file_bytes = (S32)(self->mOutputBufferSize - cinfo->dest->free_in_buffer);
	self->allocateData(file_bytes);
	memcpy( self->getData(), self->mOutputBuffer, file_bytes );
}
void LLImageJPEG::errorExit( j_common_ptr cinfo )
{
	(*cinfo->err->output_message)(cinfo);
	jpeg_destroy(cinfo);
	longjmp(sSetjmpBuffer, 1) ;
}
void LLImageJPEG::errorEmitMessage( j_common_ptr cinfo, int msg_level )
{
  struct jpeg_error_mgr * err = cinfo->err;
  if (msg_level < 0)
  {
	  if (err->num_warnings == 0 || err->trace_level >= 3)
	  {
		  (*err->output_message) (cinfo);
	  }
	  err->num_warnings++;
  }
  else
  {
	  if (err->trace_level >= msg_level)
	  {
		  (*err->output_message) (cinfo);
	  }
  }
}
void LLImageJPEG::errorOutputMessage( j_common_ptr cinfo )
{
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message) (cinfo, buffer);
	std::string error = buffer ;
	LLImage::setLastError(error);
	BOOL is_decode = (cinfo->is_decompressor != 0);
	LL_WARNS() << "LLImageJPEG " << (is_decode ? "decode " : "encode ") << " failed: " << buffer << LL_ENDL;
}
BOOL LLImageJPEG::encode( const LLImageRaw* raw_image, F32 encode_time )
{
	llassert_always(raw_image);
	resetLastError();
	switch( raw_image->getComponents() )
	{
	case 1:
	case 3:
		break;
	default:
		setLastError("Unable to encode a JPEG image that doesn't have 1 or 3 components.");
		return FALSE;
	}
	setSize(raw_image->getWidth(), raw_image->getHeight(), raw_image->getComponents());
	delete[] mOutputBuffer;
	mOutputBufferSize = getWidth() * getHeight() * getComponents() + 1024;
	mOutputBuffer = new U8[ mOutputBufferSize ];
	const U8* raw_image_data = NULL;
	S32 row_stride = 0;
	struct jpeg_compress_struct cinfo;
	cinfo.client_data = this;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jerr.error_exit =		&LLImageJPEG::errorExit;
	jerr.emit_message =		&LLImageJPEG::errorEmitMessage;
	jerr.output_message =	&LLImageJPEG::errorOutputMessage;
	if( setjmp(sSetjmpBuffer) )
	{
		jpeg_destroy_compress(&cinfo);
		delete[] mOutputBuffer;
		mOutputBuffer = NULL;
		mOutputBufferSize = 0;
		return FALSE;
	}
	try
	{
		jpeg_create_compress(&cinfo);
		if( cinfo.dest == NULL)
		{
			cinfo.dest = (struct jpeg_destination_mgr *)
				(*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
				sizeof(struct jpeg_destination_mgr));
		}
		cinfo.dest->next_output_byte =		mOutputBuffer;
		cinfo.dest->free_in_buffer =		mOutputBufferSize;
		cinfo.dest->init_destination =		&LLImageJPEG::encodeInitDestination;
		cinfo.dest->empty_output_buffer =	&LLImageJPEG::encodeEmptyOutputBuffer;
		cinfo.dest->term_destination =		&LLImageJPEG::encodeTermDestination;
		cinfo.image_width = getWidth();
		cinfo.image_height = getHeight();
		switch( getComponents() )
		{
		case 1:
			cinfo.input_components = 1;
			cinfo.in_color_space = JCS_GRAYSCALE;
			break;
		case 3:
			cinfo.input_components = 3;
			cinfo.in_color_space = JCS_RGB;
			break;
		default:
			setLastError("Unable to encode a JPEG image that doesn't have 1 or 3 components.");
			return FALSE;
		}
		jpeg_set_defaults(&cinfo);
		jpeg_set_quality(&cinfo, mEncodeQuality, TRUE );
		jpeg_start_compress(&cinfo, TRUE);
		row_stride = getWidth() * getComponents();
		raw_image_data = raw_image->getData();
		const U8* last_row_data = raw_image_data + (getHeight()-1) * row_stride;
		JSAMPROW row_pointer[1];
		while (cinfo.next_scanline < cinfo.image_height)
		{
			row_pointer[0] = (JSAMPROW)(last_row_data - (cinfo.next_scanline * row_stride));
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
		jpeg_finish_compress(&cinfo);
		delete[] mOutputBuffer;
		mOutputBuffer = NULL;
		mOutputBufferSize = 0;
		jpeg_destroy_compress(&cinfo);
	}
	catch(int)
	{
		jpeg_destroy_compress(&cinfo);
		delete[] mOutputBuffer;
		mOutputBuffer = NULL;
		mOutputBufferSize = 0;
		return FALSE;
	}
	return TRUE;
}
