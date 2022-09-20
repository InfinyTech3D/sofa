/******************************************************************************
*                 SOFA, Simulation Open-Framework Architecture                *
*                    (c) 2006 INRIA, USTL, UJF, CNRS, MGH                     *
*                                                                             *
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this program. If not, see <http://www.gnu.org/licenses/>.        *
*******************************************************************************
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#pragma once
#include <sofa/core/config.h>

#include <sofa/core/topology/TopologyHandler.h>
#include <sofa/core/topology/BaseTopologyData.h>

#include <sofa/core/topology/BaseTopology.h>


namespace sofa::core::topology
{



////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////   Generic Topology Data Implementation   /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

template< class TopologyElementType, class VecT>
class TopologyDataHandler : public sofa::core::topology::TopologyHandler
{
public:
    typedef typename VecT::value_type value_type;
    using t_topologicalData = sofa::core::topology::BaseTopologyData<VecT>;

    using ElementInfo = sofa::geometry::ElementInfo<TopologyElementType>;
    using TopologyChangeElementInfo = core::topology::TopologyChangeElementInfo<TopologyElementType>;

    // Event types (EMoved* are not used for all element types, i.e. Point vs others)
    typedef typename TopologyChangeElementInfo::EIndicesSwap    ElementIndicesSwap;
    typedef typename TopologyChangeElementInfo::ERenumbering    ElementRenumbering;
    typedef typename TopologyChangeElementInfo::EAdded          ElementAdded;
    typedef typename TopologyChangeElementInfo::ERemoved        ElementRemoved;
    typedef typename TopologyChangeElementInfo::EMoved          ElementMoved;
    typedef typename TopologyChangeElementInfo::AncestorElem    AncestorElem;

    TopologyDataHandler(t_topologicalData* _topologicalData,
        sofa::core::topology::BaseMeshTopology* _topology, 
        value_type defaultValue = value_type());


    TopologyDataHandler(t_topologicalData* _topologicalData,
        value_type defaultValue = value_type());

    void init();

    void handleTopologyChange() override;

    void registerTopologicalData(t_topologicalData *topologicalData) {m_topologyData = topologicalData;}


    /// Function to link DataEngine with Data array from topology
    void linkToTopologyDataArray(sofa::geometry::ElementType elementType) override;
    void unlinkFromTopologyDataArray(sofa::geometry::ElementType elementType) override;


    using TopologyHandler::ApplyTopologyChange;

    /// Apply swap between indices elements.
    void ApplyTopologyChange(const ElementIndicesSwap* event) override;
    /// Apply adding elements.
    void ApplyTopologyChange(const ElementAdded* event) override;
    /// Apply removing elements.
    void ApplyTopologyChange(const ElementRemoved* event) override;
    /// Apply renumbering on elements.
    void ApplyTopologyChange(const ElementRenumbering* event) override;
    /// Apply moving elements.
    void ApplyTopologyChange(const ElementMoved* event) override;
    /// Apply adding function on moved elements.
    //virtual void ApplyTopologyChange(const EMoved_Adding* event) override;
    ///// Apply removing function on moved elements.
    //virtual void ApplyTopologyChange(const EMoved_Removing* event) override;


protected:
    t_topologicalData* m_topologyData;

};


} //namespace sofa::core::topology
