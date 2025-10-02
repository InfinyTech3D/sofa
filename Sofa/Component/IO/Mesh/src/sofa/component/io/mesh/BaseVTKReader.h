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
#include <sofa/component/io/mesh/config.h>
#include <sofa/core/objectmodel/BaseData.h>
#include <sofa/core/objectmodel/BaseObject.h>

#include <iosfwd>
#include <string>

namespace sofa::component::io::mesh::basevtkreader
{
/// Use a per-file namespace. The role of this per-file namespace contain the names to make
/// them private. Outside of this namespace the fully qualified name have to be used.
/// At the end of this namespace only a subset of the names are imported into the parent namespace.
/// So that you can access to BaseVTKReader with
/// sofa::component::loader::BaseVTKReader or sofa::component::loader::basevtkreader::BaseVTKReader

using sofa::core::objectmodel::BaseData;
using sofa::core::objectmodel::BaseObject;

using std::istream;
using std::ofstream;
using std::string;

enum class VTKDatasetFormat
{
    IMAGE_DATA,
    STRUCTURED_POINTS,
    STRUCTURED_GRID,
    RECTILINEAR_GRID,
    POLYDATA,
    UNSTRUCTURED_GRID
};

class BaseVTKReader : public BaseObject
{
   public:
    class BaseVTKDataIO : public BaseObject
    {
       public:
        string m_name;
        int m_dataSize;
        int m_nestedDataSize;
        BaseVTKDataIO() : m_dataSize(0), m_nestedDataSize(1) {}
        ~BaseVTKDataIO() override {}
        virtual void resize(int n) = 0;
        virtual bool read(istream& f, int n, int binary) = 0;
        virtual bool read(const string& s, int n, int binary) = 0;
        virtual bool read(const string& s, int binary) = 0;
        virtual bool write(ofstream& f, int n, int groups, int binary) = 0;
        virtual const void* getData() = 0;
        virtual void swap() = 0;

        virtual BaseData* createSofaData() = 0;
    };

    template <class T>
    class VTKDataIO : public BaseVTKDataIO
    {
       public:
        T* m_data;
        VTKDataIO() : m_data(nullptr) {}
        ~VTKDataIO() override
        {
            if (m_data)
            {
                delete[] m_data;
            }
        }
        const void* getData() override;
        void resize(int n) override;
        static T swapT(T t, int nestedDataSize);
        void swap() override;
        virtual bool read(const string& s, int n, int binary) override;
        virtual bool read(const string& s, int binary) override;
        virtual bool read(istream& in, int n, int binary) override;
        virtual bool write(ofstream& out, int n, int groups, int binary) override;
        BaseData* createSofaData() override;
    };

    BaseVTKDataIO* newVTKDataIO(const string&);
    BaseVTKDataIO* newVTKDataIO(const string&, int);

    BaseVTKDataIO* m_inputPoints;
    BaseVTKDataIO* m_inputNormals;
    BaseVTKDataIO* m_inputPolygons;
    BaseVTKDataIO* m_inputCells;
    BaseVTKDataIO* m_inputCellOffsets;
    BaseVTKDataIO* m_inputCellTypes;
    type::vector<BaseVTKDataIO*> m_inputPointDataVector;
    type::vector<BaseVTKDataIO*> m_inputCellDataVector;
    bool m_isLittleEndian;

    int m_numberOfPoints, m_numberOfCells, m_numberOfLines;

    BaseVTKReader();

    bool readVTK(const char* filename);

    virtual bool readFile(const char* filename) = 0;

   private:
    static inline bool isEqual(const string&, const string&);

    template <class T>
    BaseVTKDataIO* makeDataIO(int numComponents);
};

}  // namespace sofa::component::io::mesh::basevtkreader

namespace sofa::component::io::mesh
{
/// Importing the names defined in the per-file namespace into the classical
/// sofa namespace structure so that the classes are accessible with
/// sofa::component::loader::BaseVTKReader instead of
/// sofa::component::loader::basevtkreader::BaseVTKReader which is a bit longer to read and
/// write.
using basevtkreader::BaseVTKReader;
using basevtkreader::VTKDatasetFormat;

}  // namespace sofa::component::io::mesh
