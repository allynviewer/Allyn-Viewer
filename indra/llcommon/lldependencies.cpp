/**
 * @file   lldependencies.cpp
 * @author Nat Goodspeed
 * @date   2008-09-17
 * @brief  Implementation for lldependencies.
 * 
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
#include "lldependencies.h"
#include <map>
#include <sstream>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/exception.hpp>
LLDependenciesBase::VertexList LLDependenciesBase::topo_sort(std::size_t vertices, const EdgeList& edges) const
{
    typedef boost::adjacency_list<boost::setS, boost::vecS, boost::directedS,
                                  boost::no_property> Graph;
    Graph g(edges.begin(), edges.end(), vertices);
    typedef boost::graph_traits<Graph>::vertex_descriptor VertexDesc;
    typedef std::vector<VertexDesc> SortedList;
    SortedList sorted;
    try
    {
        boost::topological_sort(g, std::back_inserter(sorted));
    }
    catch (const boost::not_a_dag& e)
    {
        std::ostringstream out;
        out << "LLDependencies cycle: " << e.what() << '\n';
        describe(out, false);
        throw Cycle(out.str());
    }
    return VertexList(sorted.rbegin(), sorted.rend());
}
std::ostream& LLDependenciesBase::describe(std::ostream& out, bool full) const
{
    out << "<no description available>";
    return out;
}
std::string LLDependenciesBase::describe(bool full) const
{
    std::ostringstream out;
    describe(out, full);
    return out.str();
}
