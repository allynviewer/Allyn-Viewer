/**
 * @file   is_approx_equal_fraction.h
 * @author Nat Goodspeed
 * @date   2009-01-28
 * @brief  lltut.h uses is_approx_equal_fraction(). Moved to this header
 *         file in llcommon so we can use lltut.h for llcommon tests without
 *         making llcommon depend on llmath.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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
#if ! defined(LL_IS_APPROX_EQUAL_FRACTION_H)
#define LL_IS_APPROX_EQUAL_FRACTION_H
#include "lldefs.h"
#include <cmath>
template <typename FTYPE>
inline bool is_approx_equal_fraction_impl(FTYPE x, FTYPE y, U32 frac_bits)
{
    FTYPE diff = (FTYPE) fabs(x - y);
    S32 diffInt = (S32) diff;
    S32 diffFracTolerance = (S32) ((diff - (FTYPE) diffInt) * (1 << frac_bits));
    if (diffInt != 0 || diffFracTolerance > 1)
    {
		return false;
    }
    return true;
}
inline bool is_approx_equal_fraction(F32 x, F32 y, U32 frac_bits)
{
    return is_approx_equal_fraction_impl<F32>(x, y, frac_bits);
}
inline bool is_approx_equal_fraction(F64 x, F64 y, U32 frac_bits)
{
    return is_approx_equal_fraction_impl<F64>(x, y, frac_bits);
}
#endif
