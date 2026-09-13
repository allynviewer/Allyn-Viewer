/** 
 * @file llmd5.h
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
#ifndef LL_LLMD5_H
#define LL_LLMD5_H
#include "llpreprocessor.h"
#include <iosfwd>
#include <cstring>
const int MD5RAW_BYTES = 16;
const int MD5HEX_STR_SIZE = 33;
const int MD5HEX_STR_BYTES = 32;
class LL_COMMON_API LLMD5 {
  typedef unsigned       int uint4;
  typedef unsigned short int uint2;
  typedef unsigned      char uint1;
  static const int BLOCK_LEN;
public:
  LLMD5              ();
  void  update     (const uint1 *input, const uint4 input_length);
  void  update     (std::istream& stream);
  void  update     (FILE *file);
  void  update     (const std::string& str);
  void  finalize   ();
  bool isFinalized() const { return mFinalized; }
  LLMD5              (const unsigned char *string);
  LLMD5              (std::istream& stream);
  LLMD5              (FILE *file);
  LLMD5              (const unsigned char *string, const unsigned int number);
  void clone(unsigned char const* digest);
  void clone(std::string const& hash_str);
  void				raw_digest(unsigned char *array) const;
  void				hex_digest(char *string) const;
  friend LL_COMMON_API std::ostream& operator<< (std::ostream&, LLMD5 const& context);
  friend LL_COMMON_API bool operator==(const LLMD5& a, const LLMD5& b) { return std::memcmp(a.mDigest ,b.mDigest, 16) == 0; }
  friend LL_COMMON_API bool operator!=(const LLMD5& a, const LLMD5& b) { return std::memcmp(a.mDigest,b.mDigest, 16) != 0; }
  friend LL_COMMON_API bool  operator<(const LLMD5& a, const LLMD5& b) { return std::memcmp(a.mDigest,b.mDigest, 16) < 0; }
private:
  uint4 mState[4];
  uint4 mCount[2];
  uint1 mBuffer[64];
  uint1 mDigest[16];
  bool  mFinalized;
  void init             ();
  void transform        (const uint1 *buffer);
  static void encode    (uint1 *dest, const uint4 *src, const uint4 length);
  static void decode    (uint4 *dest, const uint1 *src, const uint4 length);
};
#endif
