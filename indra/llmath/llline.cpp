/** 
 * @file llline.cpp
 * @author Andrew Meadows
 * @brief Simple line class that can compute nearest approach between two lines
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
#include "linden_common.h"
#include "llline.h"
#include "llrand.h"
const F32 SOME_SMALL_NUMBER = 1.0e-5f;
const F32 SOME_VERY_SMALL_NUMBER = 1.0e-8f;
LLLine::LLLine()
:	mPoint(0.f, 0.f, 0.f),
	mDirection(1.f, 0.f, 0.f)
{ }
LLLine::LLLine( const LLVector3& first_point, const LLVector3& second_point )
{
	setPoints(first_point, second_point);
}
void LLLine::setPoints( const LLVector3& first_point, const LLVector3& second_point )
{
	mPoint = first_point;
	mDirection = second_point - first_point;
	mDirection.normalize();
}
void LLLine::setPointDirection( const LLVector3& first_point, const LLVector3& second_point )
{
	setPoints(first_point, first_point + second_point);
}
bool LLLine::intersects( const LLVector3& point, F32 radius ) const
{
	LLVector3 other_direction = point - mPoint;
	LLVector3 nearest_point = mPoint + mDirection * (other_direction * mDirection);
	F32 nearest_approach = (nearest_point - point).length();
	return (nearest_approach <= radius);
}
LLVector3 LLLine::nearestApproach( const LLVector3& some_point ) const
{
	return (mPoint + mDirection * ((some_point - mPoint) * mDirection));
}
LLVector3 LLLine::nearestApproach( const LLLine& other_line ) const
{
	LLVector3 between_points = other_line.mPoint - mPoint;
	F32 dir_dot_dir = mDirection * other_line.mDirection;
	F32 one_minus_dir_dot_dir = 1.0f - fabs(dir_dot_dir);
	if ( one_minus_dir_dot_dir < SOME_VERY_SMALL_NUMBER )
	{
#ifdef LL_DEBUG
		LL_WARNS() << "LLLine::nearestApproach() was given two very "
			<< "nearly parallel lines dir1 = " << mDirection
			<< " dir2 = " << other_line.mDirection << " with 1-dot_product = "
			<< one_minus_dir_dot_dir << LL_ENDL;
#endif
		return 0.5f * (mPoint + other_line.mPoint);
	}
	F32 odir_dot_bp = other_line.mDirection * between_points;
	F32 numerator = 0;
	F32 denominator = 0;
	for (S32 i=0; i<3; i++)
	{
		F32 factor = dir_dot_dir * other_line.mDirection.mV[i] - mDirection.mV[i];
		numerator += ( between_points.mV[i] - odir_dot_bp * other_line.mDirection.mV[i] ) * factor;
		denominator -= factor * factor;
	}
	F32 length_to_nearest_approach = numerator / denominator;
	return mPoint + length_to_nearest_approach * mDirection;
}
std::ostream& operator<<( std::ostream& output_stream, const LLLine& line )
{
	output_stream << "{point=" << line.mPoint << "," << "dir=" << line.mDirection << "}";
	return output_stream;
}
F32 ALMOST_PARALLEL = 0.99f;
F32 TOO_SMALL_FOR_DIVISION = 0.0001f;
bool LLLine::intersectsPlane( LLVector3& result, const LLLine& plane ) const
{
	F32 dot = plane.mDirection * mDirection;
	if (fabs(dot) < TOO_SMALL_FOR_DIVISION)
	{
		return false;
	}
	F32 plane_dot = plane.mDirection * plane.mPoint;
	F32 length = ( plane_dot - (plane.mDirection * mPoint) ) / dot;
	result = mPoint + length * mDirection;
	return true;
}
bool LLLine::getIntersectionBetweenTwoPlanes( LLLine& result, const LLLine& first_plane, const LLLine& second_plane )
{
	F32 dot = fabs(first_plane.mDirection * second_plane.mDirection);
	if (dot > ALMOST_PARALLEL)
	{
		return false;
	}
	LLVector3 direction = first_plane.mDirection % second_plane.mDirection;
	direction.normalize();
	LLVector3 first_intersection;
	{
		LLLine intersection_line(first_plane);
		intersection_line.mDirection = direction % first_plane.mDirection;
		intersection_line.mDirection.normalize();
		intersection_line.intersectsPlane(first_intersection, second_plane);
	}
	result.mPoint = first_intersection;
	result.mDirection = direction;
	return true;
}
