/** 
 * @file llsphere.cpp
 * @author Andrew Meadows
 * @brief Simple line class that can compute nearest approach between two lines
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#include "llsphere.h"
LLSphere::LLSphere()
:	mCenter(0.f, 0.f, 0.f),
	mRadius(0.f)
{ }
LLSphere::LLSphere( const LLVector3& center, F32 radius)
{
	set(center, radius);
}
void LLSphere::set( const LLVector3& center, F32 radius )
{
	mCenter = center;
	setRadius(radius);
}
void LLSphere::setCenter( const LLVector3& center)
{
	mCenter = center;
}
void LLSphere::setRadius( F32 radius)
{
	if (radius < 0.f)
	{
		radius = -radius;
	}
	mRadius = radius;
}
const LLVector3& LLSphere::getCenter() const
{
	return mCenter;
}
F32 LLSphere::getRadius() const
{
	return mRadius;
}
BOOL LLSphere::contains(const LLSphere& other_sphere) const
{
	F32 separation = (mCenter - other_sphere.mCenter).length();
	return (mRadius >= separation + other_sphere.mRadius) ? TRUE : FALSE;
}
BOOL LLSphere::overlaps(const LLSphere& other_sphere) const
{
	F32 separation = (mCenter - other_sphere.mCenter).length();
	return (separation <= mRadius + other_sphere.mRadius) ? TRUE : FALSE;
}
F32 LLSphere::getOverlap(const LLSphere& other_sphere) const
{
	return (mCenter - other_sphere.mCenter).length() - mRadius - other_sphere.mRadius;
}
bool LLSphere::operator==(const LLSphere& rhs) const
{
	return (mRadius == rhs.mRadius
			&& mCenter == rhs.mCenter);
}
std::ostream& operator<<( std::ostream& output_stream, const LLSphere& sphere)
{
	output_stream << "{center=" << sphere.mCenter << "," << "radius=" << sphere.mRadius << "}";
	return output_stream;
}
void LLSphere::collapse(std::vector<LLSphere>& sphere_list)
{
	std::vector<LLSphere>::iterator first_itr = sphere_list.begin();
	while (first_itr != sphere_list.end())
	{
		bool delete_from_front = false;
		std::vector<LLSphere>::iterator second_itr = first_itr;
		++second_itr;
		while (second_itr != sphere_list.end())
		{
			if (second_itr->contains(*first_itr))
			{
				delete_from_front = true;
				break;
			}
			else if (first_itr->contains(*second_itr))
			{
				sphere_list.erase(second_itr++);
			}
			else
			{
				++second_itr;
			}
		}
		if (delete_from_front)
		{
			sphere_list.erase(first_itr++);
		}
		else
		{
			++first_itr;
		}
	}
}
LLSphere LLSphere::getBoundingSphere(const LLSphere& first_sphere, const LLSphere& second_sphere)
{
	LLVector3 direction = second_sphere.mCenter - first_sphere.mCenter;
	F32 half_milimeter = 0.0005f;
	F32 distance = direction.length();
	if (0.f == distance)
	{
		direction.setVec(1.f, 0.f, 0.f);
	}
	else
	{
		direction.normVec();
	}
	F32 max_edge = 0.f;
	F32 min_edge = 0.f;
	max_edge = llmax(max_edge + first_sphere.getRadius(), max_edge + distance + second_sphere.getRadius() + half_milimeter);
	min_edge = llmin(min_edge - first_sphere.getRadius(), min_edge + distance - second_sphere.getRadius() - half_milimeter);
	F32 radius = 0.5f * (max_edge - min_edge);
	LLVector3 center = first_sphere.mCenter + (0.5f * (max_edge + min_edge)) * direction;
	return LLSphere(center, radius);
}
LLSphere LLSphere::getBoundingSphere(const std::vector<LLSphere>& sphere_list)
{
	LLSphere bounding_sphere( LLVector3(0.f, 0.f, 0.f), 0.f );
	S32 sphere_count = sphere_list.size();
	if (1 == sphere_count)
	{
		std::vector<LLSphere>::const_iterator sphere_itr = sphere_list.begin();
		bounding_sphere = *sphere_itr;
	}
	else if (2 == sphere_count)
	{
		std::vector<LLSphere>::const_iterator first_sphere = sphere_list.begin();
		std::vector<LLSphere>::const_iterator second_sphere = first_sphere;
		++second_sphere;
		bounding_sphere = LLSphere::getBoundingSphere(*first_sphere, *second_sphere);
	}
	else if (sphere_count > 0)
	{
		std::vector<LLSphere>::const_iterator first_itr = sphere_list.begin();
		LLVector3 max_corner = first_itr->getCenter() + first_itr->getRadius() * LLVector3(1.f, 1.f, 1.f);
		LLVector3 min_corner = first_itr->getCenter() - first_itr->getRadius() * LLVector3(1.f, 1.f, 1.f);
		{
			std::vector<LLSphere>::const_iterator sphere_itr = sphere_list.begin();
			for (++sphere_itr; sphere_itr != sphere_list.end(); ++sphere_itr)
			{
				LLVector3 center = sphere_itr->getCenter();
				F32 radius = sphere_itr->getRadius();
				for (S32 i=0; i<3; ++i)
				{
					if (center.mV[i] + radius > max_corner.mV[i])
					{
						max_corner.mV[i] = center.mV[i] + radius;
					}
					if (center.mV[i] - radius < min_corner.mV[i])
					{
						min_corner.mV[i] = center.mV[i] - radius;
					}
				}
			}
		}
		LLVector3 diagonal = max_corner - min_corner;
		F32 bounding_radius = 0.5f * diagonal.length();
		LLVector3 bounding_center = 0.5f * (max_corner + min_corner);
		F32 minimum_radius = 0.5f * llmin(diagonal.mV[VX], llmin(diagonal.mV[VY], diagonal.mV[VZ]));
		F32 step_length = bounding_radius - minimum_radius;
		S32 step_count = 0;
		S32 max_step_count = 12;
		F32 half_milimeter = 0.0005f;
		S32 last_dx = 2;
		S32 last_dy = 2;
		S32 last_dz = 2;
		while (step_length > half_milimeter
				&& step_count < max_step_count)
		{
			S32 best_dx = 0;
			S32 best_dy = 0;
			S32 best_dz = 0;
			bool found_better_center = false;
			for (S32 dx = -1; dx < 2; ++dx)
			{
				for (S32 dy = -1; dy < 2; ++dy)
				{
					for (S32 dz = -1; dz < 2; ++dz)
					{
						if (dx == 0 && dy == 0 && dz == 0)
						{
							continue;
						}
						S32 match_count = 0;
						if (last_dx == dx) ++match_count;
						if (last_dy == dy) ++match_count;
						if (last_dz == dz) ++match_count;
						if (match_count == 2)
						{
							continue;
						}
						LLVector3 center = bounding_center;
						center.mV[VX] += (F32) dx * step_length;
						center.mV[VY] += (F32) dy * step_length;
						center.mV[VZ] += (F32) dz * step_length;
						F32 max_radius = 0.f;
						std::vector<LLSphere>::const_iterator sphere_itr;
						for (sphere_itr = sphere_list.begin(); sphere_itr != sphere_list.end(); ++sphere_itr)
						{
							F32 radius = (sphere_itr->getCenter() - center).length() + sphere_itr->getRadius();
							if (radius > max_radius)
							{
								max_radius = radius;
							}
						}
						if (max_radius < bounding_radius)
						{
							best_dx = dx;
							best_dy = dy;
							best_dz = dz;
							bounding_center = center;
							bounding_radius = max_radius;
							found_better_center = true;
						}
					}
				}
			}
			if (found_better_center)
			{
				last_dx = -best_dx;
				last_dy = -best_dy;
				last_dz = -best_dz;
			}
			else
			{
				step_length *= 0.5f;
				last_dx = 2;
				last_dy = 2;
				last_dz = 2;
			}
		}
		bounding_radius += half_milimeter;
		bounding_sphere.set(bounding_center, bounding_radius);
	}
	return bounding_sphere;
}
