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

#include <MultiThreading/config.h>

#include <sofa/component/solidmechanics/fem/elastic/HexahedronFEMForceField.h>
#include <sofa/simulation/CpuTask.h>

namespace sofa::component::forcefield
{

template<class DataTypes>
class AccumulateForceLargeTasks;

template<class DataTypes>
class AddDForceTask;

/**
 * Parallel implementation of HexahedronFEMForceField
 *
 * This implementation is the most efficient when:
 * 1) the number of hexahedron is large (> 1000)
 * 2) the global system matrix is not assembled. It is usually the case with a CGLinearSolver templated with GraphScattered types.
 * 3) the method is 'large'. If the method is 'polar' or 'small', addForce is executed sequentially, but addDForce in parallel.
 *
 * The following methods are executed in parallel:
 * - addForce for method 'large'.
 * - addDForce
 *
 * The method addKToMatrix is not executed in parallel. This method is called with an assembled system, usually with
 * a direct solver or a CGLinearSolver templated with types different from GraphScattered. In this case, the most
 * time-consumming step is to invert the matrix. This is where efforts should be put to accelerate the simulation.
 */
template<class DataTypes>
class SOFA_MULTITHREADING_PLUGIN_API ParallelHexahedronFEMForceField : virtual public sofa::component::solidmechanics::fem::elastic::HexahedronFEMForceField<DataTypes>
{
public:
    SOFA_CLASS(SOFA_TEMPLATE(ParallelHexahedronFEMForceField, DataTypes), SOFA_TEMPLATE(sofa::component::solidmechanics::fem::elastic::HexahedronFEMForceField, DataTypes));

    typedef typename DataTypes::Coord Coord;
    typedef typename DataTypes::Deriv Deriv;
    typedef typename DataTypes::VecCoord VecCoord;
    typedef typename DataTypes::VecDeriv VecDeriv;
    typedef core::objectmodel::Data<VecCoord> DataVecCoord;
    typedef core::objectmodel::Data<VecDeriv> DataVecDeriv;
    typedef typename Coord::value_type Real;
    typedef helper::ReadAccessor< Data< VecCoord > > RDataRefVecCoord;
    typedef helper::WriteAccessor< Data< VecDeriv > > WDataRefVecDeriv;
    typedef core::topology::BaseMeshTopology::Hexa Element;
    typedef core::topology::BaseMeshTopology::SeqHexahedra VecElement;
    typedef type::Mat<24, 24, Real> ElementStiffness;
    typedef helper::vector<ElementStiffness> VecElementStiffness;

    void init() override;

    void addForce (const core::MechanicalParams* mparams, DataVecDeriv& f,
                   const DataVecCoord& x, const DataVecDeriv& v) override;
    void addDForce (const core::MechanicalParams* mparams, DataVecDeriv& df,
                    const DataVecDeriv& dx) override;

protected:

    // code duplicated from HexahedronFEMForceField::accumulateForceLarge but adapted to be thread-safe
    void computeTaskForceLarge(RDataRefVecCoord& p, sofa::Index elementId, const Element& elem,
                               const VecElementStiffness& elementStiffnesses, SReal& OutPotentialEnery,
                               type::Vec<8, Deriv>& OutF);

    void initTaskScheduler();

private:
    bool updateStiffnessMatrices; /// cache to avoid calling 'getValue' on f_updateStiffnessMatrix
};

#if  !defined(SOFA_MULTITHREADING_PARALLELHEXAHEDRONFEMFORCEFIELD_CPP)
extern template class SOFA_MULTITHREADING_PLUGIN_API ParallelHexahedronFEMForceField<defaulttype::Vec3Types>;
#endif

} //namespace sofa::component::forcefield
