/**
 * @file llline.cpp
 * @author Andrew Meadows
 * @brief Simple line for computing nearest approach between two infinite lines
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
#ifndef LL_LINE_H
#define LL_LINE_H
#include <iostream>
#include "stdtypes.h"
#include "v3math.h"
const F32 DEFAULT_INTERSECTION_ERROR = 0.000001f;
class LLLine
{
public:
	LLLine();
	LLLine( const LLVector3& first_point, const LLVector3& second_point );
	virtual ~LLLine() {};
	void setPointDirection( const LLVector3& first_point, const LLVector3& second_point );
	void setPoints( const LLVector3& first_point, const LLVector3& second_point );
	bool intersects( const LLVector3& point, F32 radius = DEFAULT_INTERSECTION_ERROR ) const;
	LLVector3 nearestApproach( const LLVector3& some_point ) const;
	LLVector3 nearestApproach( const LLLine& other_line ) const;
	friend std::ostream& operator<<( std::ostream& output_stream, const LLLine& line );
	bool intersectsPlane( LLVector3& result, const LLLine& plane ) const;
	static bool getIntersectionBetweenTwoPlanes( LLLine& result, const LLLine& first_plane, const LLLine& second_plane );
	const LLVector3& getPoint() const { return mPoint; }
	const LLVector3& getDirection() const { return mDirection; }
protected:
	LLVector3 mPoint;
	LLVector3 mDirection;
};
#endif
