/** 
 * @file linkability.cpp
 * @author andrew@lindenlab.com
 * @date 2007-04-23
 * @brief Tests for the LLPrimLinkInfo template which computes the linkability of prims
 *
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
#include "lltut.h"
#include "llprimlinkinfo.h"
#include "llrand.h"
void randomize_sphere(LLSphere& sphere, F32 center_range, F32 radius_range)
{
	F32 radius = ll_frand(2.f * radius_range) - radius_range;
	LLVector3 center;
	for (S32 i=0; i<3; ++i)
	{
		center.mV[i] = ll_frand(2.f * center_range) - center_range;
	}
	sphere.setRadius(radius);
	sphere.setCenter(center);
}
void randomize_sphere(LLSphere& sphere, F32 center_range, F32 minimum_radius, F32 maximum_radius)
{
	F32 radius = ll_frand(maximum_radius - minimum_radius) + minimum_radius;
	LLVector3 center;
	for (S32 i=0; i<3; ++i)
	{
		center.mV[i] = ll_frand(2.f * center_range) - center_range;
	}
	sphere.setRadius(radius);
	sphere.setCenter(center);
}
bool random_sort( const LLPrimLinkInfo< S32 >&, const LLPrimLinkInfo< S32 >& b)
{
	return (ll_rand(64) < 32);
}
namespace tut
{
	struct linkable_data
	{
		LLPrimLinkInfo<S32> info;
	};
	typedef test_group<linkable_data> linkable_test;
	typedef linkable_test::object linkable_object;
	tut::linkable_test wtf("prim linkability");
	template<> template<>
	void linkable_object::test<1>()
	{
		S32 number_of_tests = 100;
		for (S32 test = 0; test < number_of_tests; ++test)
		{
			F32 first_radius = 0.f;
			F32 second_radius = 0.f;
			F32 max_link_span = ll_frand(MAX_OBJECT_SPAN);
			if (max_link_span < OBJECT_SPAN_BONUS)
			{
				max_link_span += OBJECT_SPAN_BONUS;
			}
			LLVector3 first_center(
					ll_frand(2.f * max_link_span) - max_link_span,
					ll_frand(2.f * max_link_span) - max_link_span,
					ll_frand(2.f * max_link_span) - max_link_span);
			LLVector3 direction(ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
			direction.normalize();
			F32 half_milimeter = 0.0005f;
			LLVector3 second_center;
			{
				second_center = first_center + (OBJECT_SPAN_BONUS - half_milimeter) * direction;
				LLPrimLinkInfo<S32> first_info(0, LLSphere(first_center, first_radius) );
				LLPrimLinkInfo<S32> second_info(1, LLSphere(second_center, second_radius) );
				ensure("these nearby objects should link", first_info.canLink(second_info) );
			}
			{
				second_center = first_center + (OBJECT_SPAN_BONUS + half_milimeter) * direction;
				LLPrimLinkInfo<S32> first_info(0, LLSphere(first_center, first_radius) );
				LLPrimLinkInfo<S32> second_info(1, LLSphere(second_center, second_radius) );
				ensure("these nearby objects should NOT link", !first_info.canLink(second_info) );
			}
			{
				first_radius = 0.3f * ll_frand(max_link_span - OBJECT_SPAN_BONUS);
				second_radius = ((max_link_span - OBJECT_SPAN_BONUS) / 3.f) - first_radius;
				second_center = first_center + (max_link_span - first_radius - second_radius - half_milimeter) * direction;
				LLPrimLinkInfo<S32> first_info(0, LLSphere(first_center, first_radius) );
				LLPrimLinkInfo<S32> second_info(1, LLSphere(second_center, second_radius) );
				ensure("these objects should link", first_info.canLink(second_info) );
			}
			{
				second_center += (2.f * half_milimeter) * direction;
				LLPrimLinkInfo<S32> first_info(0, LLSphere(first_center, first_radius) );
				LLPrimLinkInfo<S32> second_info(1, LLSphere(second_center, second_radius) );
				ensure("these objects should NOT link", !first_info.canLink(second_info) );
			}
			{
				second_center = first_center + (MAX_OBJECT_SPAN + 2.f * half_milimeter) * direction;
				second_radius = 0.3f * MAX_OBJECT_SPAN;
				LLPrimLinkInfo<S32> first_info(0, LLSphere(first_center, first_radius) );
				LLPrimLinkInfo<S32> second_info(1, LLSphere(second_center, second_radius) );
				ensure("these objects should NOT link", !first_info.canLink(second_info) );
			}
		}
	}
	template<> template<>
	void linkable_object::test<2>()
	{
		F32 radius = 5.f;
		F32 spacing = 10.f;
		LLVector3 line_direction(ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
		line_direction.normalize();
		LLVector3 first_center(ll_frand(2.f * spacing) -spacing, ll_frand(2.f * spacing) - spacing, ll_frand(2.f * spacing) - spacing);
		LLPrimLinkInfo<S32> infos[8];
		for (S32 index = 0; index < 8; ++index)
		{
			LLVector3 center = first_center + ((F32)(index) * spacing) * line_direction;
			infos[index].set(index, LLSphere(center, radius));
		}
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			info_list.push_back(infos[2]);
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("0&2 prim count should be 2", prim_count, 2);
			ensure_equals("0&2 unlinkable list should have length 0", (S32) info_list.size(), 0);
		}
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			info_list.push_back(infos[3]);
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("0&4 prim count should be 1", prim_count, 1);
			ensure_equals("0&4 unlinkable list should have length 1", (S32) info_list.size(), 1);
		}
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			for (S32 index = 1; index < 5; ++index)
			{
				info_list.push_back(infos[index]);
			}
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("01234 prim count should be 5", prim_count, 5);
			ensure_equals("01234 unlinkable list should have length 0", (S32) info_list.size(), 0);
		}
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			for (S32 index = 4; index > 0; --index)
			{
				info_list.push_back(infos[index]);
			}
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("04321 prim count should be 5", prim_count, 5);
			ensure_equals("04321 unlinkable list should have length 0", (S32) info_list.size(), 0);
		}
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			info_list.push_back(infos[1]);
			info_list.push_back(infos[4]);
			info_list.push_back(infos[2]);
			info_list.push_back(infos[3]);
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("01423 prim count should be 5", prim_count, 5);
			ensure_equals("01423 unlinkable list should have length 0", (S32) info_list.size(), 0);
		}
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			for (S32 index = 1; index < 6; ++index)
			{
				info_list.push_back(infos[index]);
			}
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("012345 prim count should be 5", prim_count, 5);
			ensure_equals("012345 unlinkable list should have length 1", (S32) info_list.size(), 1);
			std::list< LLPrimLinkInfo<S32> >::iterator info_itr = info_list.begin();
			if (info_itr != info_list.end())
			{
				std::list<S32> unlinked_indecies;
				info_itr->getData(unlinked_indecies);
				ensure_equals("012345 unlinkable index count should be 1", (S32) unlinked_indecies.size(), 1);
				std::list<S32>::iterator unlinked_index_itr = unlinked_indecies.begin();
				S32 unlinkable_index = *unlinked_index_itr;
				ensure_equals("012345 unlinkable index should be 5", (S32) unlinkable_index, 5);
			}
		}
		{
			LLPrimLinkInfo<S32> root_info = infos[0];
			std::list< LLPrimLinkInfo<S32> > info_list;
			for (S32 index = 1; index < 8; ++index)
			{
				info_list.push_back(infos[index]);
			}
			root_info.mergeLinkableSet(info_list);
			S32 prim_count = root_info.getPrimCount();
			ensure_equals("01234567 prim count should be 5", prim_count, 5);
			ensure_equals("01234567 unlinkable list should have length 1", (S32) info_list.size(), 1);
			std::list< LLPrimLinkInfo<S32> >::iterator info_itr = info_list.begin();
			if (info_itr != info_list.end())
			{
				std::list<S32> unlinked_indecies;
				info_itr->getData(unlinked_indecies);
				ensure_equals("0123456 unlinkable index count should be 3", (S32) unlinked_indecies.size(), 3);
				std::list<S32>::iterator unlinked_index_itr = unlinked_indecies.begin();
				S32 unlinkable_index = *unlinked_index_itr;
				ensure_equals("0123456 first unlinkable index should be 5", (S32) unlinkable_index, 5);
				++unlinked_index_itr;
				unlinkable_index = *unlinked_index_itr;
				ensure_equals("0123456 second unlinkable index should be 6", (S32) unlinkable_index, 6);
				++unlinked_index_itr;
				unlinkable_index = *unlinked_index_itr;
				ensure_equals("0123456 third unlinkable index should be 7", (S32) unlinkable_index, 7);
			}
		}
	}
	template<> template<>
	void linkable_object::test<3>()
	{
		S32 number_of_tests = 5;
		for (S32 test = 0; test < number_of_tests; ++test)
		{
			F32 first_radius = 1.f;
			F32 second_radius = 2.f;
			F32 third_radius = 3.f;
			F32 half_milimeter = 0.0005f;
			F32 max_first_second_span = 3.f * (first_radius + second_radius) + OBJECT_SPAN_BONUS;
			F32 linkable_distance = max_first_second_span - first_radius - second_radius - half_milimeter;
			F32 max_full_span = 3.f * (0.5f * max_first_second_span + third_radius) + OBJECT_SPAN_BONUS;
			F32 unlinkable_distance = max_full_span - 0.5f * linkable_distance - third_radius + half_milimeter;
			LLVector3 first_direction(ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
			first_direction.normalize();
			LLVector3 second_direction(ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
			second_direction.normalize();
			LLVector3 third_direction(ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f, ll_frand(2.f) - 1.f);
			third_direction.normalize();
			LLVector3 first_center = ll_frand(10.f) * first_direction;
			LLVector3 second_center = first_center + ll_frand(linkable_distance) * second_direction;
			LLVector3 first_join_center = 0.5f * (first_center + second_center);
			LLVector3 third_center = first_join_center + unlinkable_distance * third_direction;
			{
				S32 index = 0;
				LLPrimLinkInfo<S32> first_info(index++, LLSphere(first_center, first_radius));
				LLPrimLinkInfo<S32> second_info(index++, LLSphere(second_center, second_radius));
				LLPrimLinkInfo<S32> third_info(index++, LLSphere(third_center, third_radius));
				std::list< LLPrimLinkInfo<S32> > info_list;
				info_list.push_back(second_info);
				info_list.push_back(third_info);
				first_info.mergeLinkableSet(info_list);
				S32 prim_count = first_info.getPrimCount();
				ensure_equals("prim count should be 2", prim_count, 2);
				ensure_equals("unlinkable list should have length 1", (S32) info_list.size(), 1);
			}
			{
				S32 index = 0;
				LLPrimLinkInfo<S32> first_info(index++, LLSphere(first_center, first_radius));
				LLPrimLinkInfo<S32> second_info(index++, LLSphere(second_center, second_radius));
				LLPrimLinkInfo<S32> third_info(index++, LLSphere(third_center, third_radius));
				std::list< LLPrimLinkInfo<S32> > info_list;
				info_list.push_back(third_info);
				info_list.push_back(second_info);
				first_info.mergeLinkableSet(info_list);
				S32 prim_count = first_info.getPrimCount();
				ensure_equals("prim count should be 2", prim_count, 2);
				ensure_equals("unlinkable list should have length 1", (S32) info_list.size(), 1);
			}
		}
	}
	template<> template<>
	void linkable_object::test<4>()
	{
		F32 root_center_range = 0.f;
		F32 min_prim_radius = 0.1f;
		F32 max_prim_radius = 2.f;
		F32 child_center_range = 0.45f * ( (6*min_prim_radius) + OBJECT_SPAN_BONUS );
		S32 number_of_tests = 100;
		S32 number_of_spheres = 10;
		S32 number_of_scrambles = 10;
		S32 number_of_random_bubble_sorts = 10;
		for (S32 test = 0; test < number_of_tests; ++test)
		{
			LLSphere sphere;
			S32 sphere_index = 0;
			randomize_sphere(sphere, root_center_range, min_prim_radius, max_prim_radius);
			info.set( sphere_index++, sphere );
			std::list< LLPrimLinkInfo<S32> > info_list;
			for (; sphere_index < number_of_spheres; ++sphere_index)
			{
				randomize_sphere(sphere, child_center_range, min_prim_radius, max_prim_radius);
				LLPrimLinkInfo<S32> child_info( sphere_index, sphere );
				info_list.push_back(child_info);
			}
			std::list<S32> first_linked_list;
			{
				LLPrimLinkInfo<S32> test_info( 0, LLSphere(info.getCenter(), 0.5f * info.getDiameter()) );
				std::list< LLPrimLinkInfo<S32> > test_list;
				test_list.assign(info_list.begin(), info_list.end());
				test_info.mergeLinkableSet(test_list);
				ensure("All prims should link, but did not.",test_list.empty());
				test_info.getData(first_linked_list);
				first_linked_list.sort();
			}
			for (S32 scramble = 0; scramble < number_of_scrambles; ++scramble)
			{
				LLPrimLinkInfo<S32> test_info(0, LLSphere(info.getCenter(), 0.5f * info.getDiameter()) );
				std::list< LLPrimLinkInfo<S32> > test_list;
				test_list.assign(info_list.begin(), info_list.end());
				for (S32 i = 0; i < number_of_random_bubble_sorts; i++)
				{
					test_list.sort(random_sort);
				}
				test_info.mergeLinkableSet(test_list);
				std::list<S32> linked_list;
				test_info.getData(linked_list);
				linked_list.sort();
				ensure_equals("linked set size should be order independent",linked_list.size(),first_linked_list.size());
			}
		}
	}
}
