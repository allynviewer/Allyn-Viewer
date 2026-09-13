/** 
 * @file llbase32.cpp
 * @brief base32 encoding that returns a std::string
 * @author James Cook
 *
 * Based on code from bitter
 * http://ghostwhitecrab.com/bitter/
 *
 * Some parts of this file are:
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
#include "linden_common.h"
#include "llbase32.h"
#include <string>
static const char base32_alphabet[32] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', '2', '3', '4', '5', '6', '7'
};
size_t
base32_encode(char *dst, size_t size, const void *data, size_t len)
{
  size_t i = 0;
  const U8 *p = (const U8*)data;
  const char *end = &dst[size];
  char *q = dst;
  do {
    size_t j, k;
    U8 x[5];
    char s[8];
    switch (len - i) {
    case 4: k = 7; break;
    case 3: k = 5; break;
    case 2: k = 3; break;
    case 1: k = 2; break;
    default:
      k = 8;
    }
    for (j = 0; j < 5; j++)
      x[j] = i < len ? p[i++] : 0;
    s[0] =  (x[0] >> 3);
    s[1] = ((x[0] & 0x07) << 2) | (x[1] >> 6);
    s[2] =  (x[1] >> 1) & 0x1f;
    s[3] = ((x[1] & 0x01) << 4) | (x[2] >> 4);
    s[4] = ((x[2] & 0x0f) << 1) | (x[3] >> 7);
    s[5] =  (x[3] >> 2) & 0x1f;
    s[6] = ((x[3] & 0x03) << 3) | (x[4] >> 5);
    s[7] =   x[4] & 0x1f;
    for (j = 0; j < k && q != end; j++)
      *q++ = base32_alphabet[(U8) s[j]];
  } while (i < len);
  return q - dst;
}
std::string LLBase32::encode(const U8* input, size_t input_size)
{
	std::string output;
	if (input)
	{
		size_t input_chunks = (input_size + 4) / 5;
		size_t output_size = input_chunks * 8;
		output.resize(output_size);
		size_t encoded = base32_encode(&output[0], output_size, input, input_size);
		LL_INFOS() << "encoded " << encoded << " into buffer of size "
			<< output_size << LL_ENDL;
	}
	return output;
}
