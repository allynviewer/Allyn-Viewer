/** 
 * @file llmd5.cpp
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
#include "llmd5.h"
#include <cassert>
#include <iostream>
const int LLMD5::BLOCK_LEN = 4096;
LLMD5::LLMD5()
{
  init();
}
void LLMD5::update (const uint1 *input, const uint4 input_length) {
  uint4 input_index, buffer_index;
  uint4 buffer_space;
  if (mFinalized){
	  std::cerr << "LLMD5::update:  Can't update a finalized digest!" << std::endl;
    return;
  }
  buffer_index = (unsigned int)((mCount[0] >> 3) & 0x3F);
  if (  (mCount[0] += ((uint4) input_length << 3))<((uint4) input_length << 3) )
	  mCount[1]++;
  mCount[1] += ((uint4)input_length >> 29);
  buffer_space = 64 - buffer_index;
  if (input == NULL || input_length == 0){
	  std::cerr << "LLMD5::update:  Invalid input!" << std::endl;
	  return;
  }
  if (input_length >= buffer_space) {
    memcpy(
		mBuffer + buffer_index,
		input,
		buffer_space);
    transform (mBuffer);
    for (input_index = buffer_space; input_index + 63 < input_length;
	 input_index += 64)
      transform (input+input_index);
    buffer_index = 0;
  }
  else
    input_index=0;
  memcpy(mBuffer+buffer_index, input+input_index, input_length-input_index);
}
void LLMD5::update(FILE* file){
  unsigned char buffer[BLOCK_LEN];
  int len;
  while ( (len=(int)fread(buffer, 1, BLOCK_LEN, file)) )
    update(buffer, len);
  fclose (file);
}
void LLMD5::update(std::istream& stream){
  unsigned char buffer[BLOCK_LEN];
  int len;
  while (stream.good()){
    stream.read( (char*)buffer, BLOCK_LEN);
    len=(int)stream.gcount();
    update(buffer, len);
  }
}
void  LLMD5::update(const std::string& s)
{
	update((unsigned char *)s.c_str(),s.length());
}
void LLMD5::finalize (){
  unsigned char bits[8];
  unsigned int index, padLen;
  static uint1 PADDING[64]={
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
  if (mFinalized){
    std::cerr << "LLMD5::finalize:  Already finalized this digest!" << std::endl;
    return;
  }
  encode (bits, mCount, 8);
  index = (uint4) ((mCount[0] >> 3) & 0x3f);
  padLen = (index < 56) ? (56 - index) : (120 - index);
  update (PADDING, padLen);
  update (bits, 8);
  encode (mDigest, mState, 16);
  memset (mBuffer, 0, sizeof(mBuffer));
  mFinalized=true;
}
LLMD5::LLMD5(FILE *file){
  init();
  update(file);
  finalize ();
}
LLMD5::LLMD5(std::istream& stream){
  init();
  update (stream);
  finalize();
}
LLMD5::LLMD5(const unsigned char *string, const unsigned int number)
{
	const char *colon = ":";
	char tbuf[16];
	init();
	update(string, (U32)strlen((const char *) string));
	update((const unsigned char *) colon, (U32)strlen(colon));
	snprintf(tbuf, sizeof(tbuf), "%i", number);
	update((const unsigned char *) tbuf, (U32)strlen(tbuf));
	finalize();
}
LLMD5::LLMD5(const unsigned char *s)
{
	init();
	update(s, (U32)strlen((const char *) s));
	finalize();
}
void LLMD5::raw_digest(unsigned char *s) const
{
	if (!mFinalized)
	{
		std::cerr << "LLMD5::raw_digest:  Can't get digest if you haven't "<<
			"finalized the digest!" << std::endl;
		s[0] = '\0';
		return;
	}
	memcpy(s, mDigest, 16);
	return;
}
void LLMD5::clone(unsigned char const* s)
{
	memcpy(mDigest, s, 16);
	mFinalized = true;
}
void LLMD5::hex_digest(char *s) const
{
	int i;
	if (!mFinalized)
	{
		std::cerr << "LLMD5::hex_digest:  Can't get digest if you haven't "<<
		  "finalized the digest!" <<std::endl;
		s[0] = '\0';
		return;
	}
	for (i=0; i<16; i++)
	{
		sprintf(s+i*2, "%02x", mDigest[i]);
	}
	s[32]='\0';
	return;
}
void LLMD5::clone(std::string const& hash_str)
{
  for (int i = 0; i < 16; ++i)
  {
	unsigned char byte = 0;
	for (int j = 0; j < 2; ++j)
	{
	  char c = hash_str[i * 2 + j];
	  unsigned char  nibble = (c >= '0' && c <= '9') ? c - '0' : c - 'a' + 10;
	  byte += nibble << ((1 - j) << 2);
	}
	mDigest[i] = byte;
  }
  mFinalized = 1;
}
std::ostream& operator<<(std::ostream &stream, LLMD5 const& context)
{
	char s[33];
	context.hex_digest(s);
	stream << s;
	return stream;
}
void LLMD5::init(){
  mFinalized=false;
  mCount[0] = 0;
  mCount[1] = 0;
  mState[0] = 0x67452301;
  mState[1] = 0xefcdab89;
  mState[2] = 0x98badcfe;
  mState[3] = 0x10325476;
}
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#define FF(a, b, c, d, x, s, ac) { \
 (a) += F ((b), (c), (d)) + (x) + (U32)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) { \
 (a) += G ((b), (c), (d)) + (x) + (U32)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) { \
 (a) += H ((b), (c), (d)) + (x) + (U32)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) { \
 (a) += I ((b), (c), (d)) + (x) + (U32)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
void LLMD5::transform (const U8 block[64]){
  uint4 a = mState[0], b = mState[1], c = mState[2], d = mState[3], x[16];
  decode (x, block, 64);
  assert(!mFinalized);
  FF (a, b, c, d, x[ 0], S11, 0xd76aa478);
  FF (d, a, b, c, x[ 1], S12, 0xe8c7b756);
  FF (c, d, a, b, x[ 2], S13, 0x242070db);
  FF (b, c, d, a, x[ 3], S14, 0xc1bdceee);
  FF (a, b, c, d, x[ 4], S11, 0xf57c0faf);
  FF (d, a, b, c, x[ 5], S12, 0x4787c62a);
  FF (c, d, a, b, x[ 6], S13, 0xa8304613);
  FF (b, c, d, a, x[ 7], S14, 0xfd469501);
  FF (a, b, c, d, x[ 8], S11, 0x698098d8);
  FF (d, a, b, c, x[ 9], S12, 0x8b44f7af);
  FF (c, d, a, b, x[10], S13, 0xffff5bb1);
  FF (b, c, d, a, x[11], S14, 0x895cd7be);
  FF (a, b, c, d, x[12], S11, 0x6b901122);
  FF (d, a, b, c, x[13], S12, 0xfd987193);
  FF (c, d, a, b, x[14], S13, 0xa679438e);
  FF (b, c, d, a, x[15], S14, 0x49b40821);
  GG (a, b, c, d, x[ 1], S21, 0xf61e2562);
  GG (d, a, b, c, x[ 6], S22, 0xc040b340);
  GG (c, d, a, b, x[11], S23, 0x265e5a51);
  GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa);
  GG (a, b, c, d, x[ 5], S21, 0xd62f105d);
  GG (d, a, b, c, x[10], S22,  0x2441453);
  GG (c, d, a, b, x[15], S23, 0xd8a1e681);
  GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8);
  GG (a, b, c, d, x[ 9], S21, 0x21e1cde6);
  GG (d, a, b, c, x[14], S22, 0xc33707d6);
  GG (c, d, a, b, x[ 3], S23, 0xf4d50d87);
  GG (b, c, d, a, x[ 8], S24, 0x455a14ed);
  GG (a, b, c, d, x[13], S21, 0xa9e3e905);
  GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8);
  GG (c, d, a, b, x[ 7], S23, 0x676f02d9);
  GG (b, c, d, a, x[12], S24, 0x8d2a4c8a);
  HH (a, b, c, d, x[ 5], S31, 0xfffa3942);
  HH (d, a, b, c, x[ 8], S32, 0x8771f681);
  HH (c, d, a, b, x[11], S33, 0x6d9d6122);
  HH (b, c, d, a, x[14], S34, 0xfde5380c);
  HH (a, b, c, d, x[ 1], S31, 0xa4beea44);
  HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9);
  HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60);
  HH (b, c, d, a, x[10], S34, 0xbebfbc70);
  HH (a, b, c, d, x[13], S31, 0x289b7ec6);
  HH (d, a, b, c, x[ 0], S32, 0xeaa127fa);
  HH (c, d, a, b, x[ 3], S33, 0xd4ef3085);
  HH (b, c, d, a, x[ 6], S34,  0x4881d05);
  HH (a, b, c, d, x[ 9], S31, 0xd9d4d039);
  HH (d, a, b, c, x[12], S32, 0xe6db99e5);
  HH (c, d, a, b, x[15], S33, 0x1fa27cf8);
  HH (b, c, d, a, x[ 2], S34, 0xc4ac5665);
  II (a, b, c, d, x[ 0], S41, 0xf4292244);
  II (d, a, b, c, x[ 7], S42, 0x432aff97);
  II (c, d, a, b, x[14], S43, 0xab9423a7);
  II (b, c, d, a, x[ 5], S44, 0xfc93a039);
  II (a, b, c, d, x[12], S41, 0x655b59c3);
  II (d, a, b, c, x[ 3], S42, 0x8f0ccc92);
  II (c, d, a, b, x[10], S43, 0xffeff47d);
  II (b, c, d, a, x[ 1], S44, 0x85845dd1);
  II (a, b, c, d, x[ 8], S41, 0x6fa87e4f);
  II (d, a, b, c, x[15], S42, 0xfe2ce6e0);
  II (c, d, a, b, x[ 6], S43, 0xa3014314);
  II (b, c, d, a, x[13], S44, 0x4e0811a1);
  II (a, b, c, d, x[ 4], S41, 0xf7537e82);
  II (d, a, b, c, x[11], S42, 0xbd3af235);
  II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb);
  II (b, c, d, a, x[ 9], S44, 0xeb86d391);
  mState[0] += a;
  mState[1] += b;
  mState[2] += c;
  mState[3] += d;
  memset ( (uint1 *) x, 0, sizeof(x));
}
void LLMD5::encode (uint1 *output, const uint4 *input, const uint4 len) {
  unsigned int i, j;
  for (i = 0, j = 0; j < len; i++, j += 4) {
    output[j]   = (uint1)  (input[i] & 0xff);
    output[j+1] = (uint1) ((input[i] >> 8) & 0xff);
    output[j+2] = (uint1) ((input[i] >> 16) & 0xff);
    output[j+3] = (uint1) ((input[i] >> 24) & 0xff);
  }
}
void LLMD5::decode (uint4 *output, const uint1 *input, const uint4 len){
  unsigned int i, j;
  for (i = 0, j = 0; j < len; i++, j += 4)
    output[i] = ((uint4)input[j]) | (((uint4)input[j+1]) << 8) |
      (((uint4)input[j+2]) << 16) | (((uint4)input[j+3]) << 24);
}
