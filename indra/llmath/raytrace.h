/** 
 * @file raytrace.h
 * @brief Ray intersection tests for primitives.
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
#ifndef LL_RAYTRACE_H
#define LL_RAYTRACE_H
class LLVector3;
class LLQuaternion;
BOOL line_plane(const LLVector3 &line_point, const LLVector3 &line_direction,
				const LLVector3 &plane_point, const LLVector3 plane_normal,
				LLVector3 &intersection);
BOOL ray_plane(const LLVector3 &ray_point, const LLVector3 &ray_direction,
			   const LLVector3 &plane_point, const LLVector3 plane_normal,
			   LLVector3 &intersection);
BOOL ray_circle(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				const LLVector3 &circle_center, const LLVector3 plane_normal, F32 circle_radius,
				LLVector3 &intersection);
BOOL ray_triangle(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				  const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2,
				  LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL ray_quadrangle(const LLVector3 &ray_point, const LLVector3 &ray_direction,
					const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2,
					LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL ray_sphere(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				const LLVector3 &sphere_center, F32 sphere_radius,
				LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL ray_cylinder(const LLVector3 &ray_point, const LLVector3 &ray_direction,
		          const LLVector3 &cyl_center, const LLVector3 &cyl_scale, const LLQuaternion &cyl_rotation,
				  LLVector3 &intersection, LLVector3 &intersection_normal);
U32 ray_box(const LLVector3 &ray_point, const LLVector3 &ray_direction,
		    const LLVector3 &box_center, const LLVector3 &box_scale, const LLQuaternion &box_rotation,
			LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL ray_prism(const LLVector3 &ray_point, const LLVector3 &ray_direction,
			   const LLVector3 &prism_center, const LLVector3 &prism_scale, const LLQuaternion &prism_rotation,
			   LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL ray_tetrahedron(const LLVector3 &ray_point, const LLVector3 &ray_direction,
					 const LLVector3 &t_center, const LLVector3 &t_scale, const LLQuaternion &t_rotation,
					 LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL ray_pyramid(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				 const LLVector3 &p_center, const LLVector3 &p_scale, const LLQuaternion &p_rotation,
				 LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL linesegment_circle(const LLVector3 &point_a, const LLVector3 &point_b,
						const LLVector3 &circle_center, const LLVector3 plane_normal, F32 circle_radius,
						LLVector3 &intersection);
BOOL linesegment_triangle(const LLVector3 &point_a, const LLVector3 &point_b,
						  const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2,
						  LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL linesegment_quadrangle(const LLVector3 &point_a, const LLVector3 &point_b,
							const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2,
							LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL linesegment_sphere(const LLVector3 &point_a, const LLVector3 &point_b,
				const LLVector3 &sphere_center, F32 sphere_radius,
				LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL linesegment_cylinder(const LLVector3 &point_a, const LLVector3 &point_b,
						  const LLVector3 &cyl_center, const LLVector3 &cyl_scale, const LLQuaternion &cyl_rotation,
						  LLVector3 &intersection, LLVector3 &intersection_normal);
U32 linesegment_box(const LLVector3 &point_a, const LLVector3 &point_b,
					const LLVector3 &box_center, const LLVector3 &box_scale, const LLQuaternion &box_rotation,
					LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL linesegment_prism(const LLVector3 &point_a, const LLVector3 &point_b,
					   const LLVector3 &prism_center, const LLVector3 &prism_scale, const LLQuaternion &prism_rotation,
					   LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL linesegment_tetrahedron(const LLVector3 &point_a, const LLVector3 &point_b,
							 const LLVector3 &t_center, const LLVector3 &t_scale, const LLQuaternion &t_rotation,
							 LLVector3 &intersection, LLVector3 &intersection_normal);
BOOL linesegment_pyramid(const LLVector3 &point_a, const LLVector3 &point_b,
						 const LLVector3 &p_center, const LLVector3 &p_scale, const LLQuaternion &p_rotation,
						 LLVector3 &intersection, LLVector3 &intersection_normal);
#endif
