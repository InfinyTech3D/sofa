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
#include <sofa/component/io/mesh/BaseVTKReader.h>

#include <cstdint>
#include <sofa/component/io/mesh/BaseVTKReader.inl>

namespace sofa::component::io::mesh::basevtkreader
{

BaseVTKReader::BaseVTKReader()
    : m_inputPoints(nullptr),
      m_inputNormals(nullptr),
      m_inputPolygons(nullptr),
      m_inputCells(nullptr),
      m_inputCellOffsets(nullptr),
      m_inputCellTypes(nullptr),
      m_inputPointDataVector(),
      m_inputCellDataVector(),
      m_isLittleEndian(true),
      m_numberOfPoints(0),
      m_numberOfCells(0),
      m_numberOfLines(0)
{
}

BaseVTKReader::BaseVTKDataIO* BaseVTKReader::newVTKDataIO(const string& typestr)
{
    return newVTKDataIO(typestr, 1);
}

BaseVTKReader::BaseVTKDataIO* BaseVTKReader::newVTKDataIO(const string& typestr, int numComponents)
{
    BaseVTKDataIO* result = nullptr;

    if (isEqual(typestr, "char"))
    {
        msg_error("BaseVTKReader") << "encountered legacy type 'char', mapping to Int8 (int8_t). "
                                      "Signedness of 'char' is platform-dependent.";
        result = makeDataIO<std::int8_t>(numComponents);
    }
    else if (isEqual(typestr, "Int8"))
        result = makeDataIO<std::int8_t>(numComponents);
    else if (isEqual(typestr, "unsigned_char") || isEqual(typestr, "UInt8"))
        result = makeDataIO<std::uint8_t>(numComponents);
    else if (isEqual(typestr, "short") || isEqual(typestr, "Int16"))
        result = makeDataIO<std::int16_t>(numComponents);
    else if (isEqual(typestr, "unsigned_short") || isEqual(typestr, "UInt16"))
        result = makeDataIO<std::uint16_t>(numComponents);
    else if (isEqual(typestr, "int") || isEqual(typestr, "Int32"))
        result = makeDataIO<std::int32_t>(numComponents);
    else if (isEqual(typestr, "unsigned_int") || isEqual(typestr, "UInt32"))
        result = makeDataIO<std::uint32_t>(numComponents);
    else if (isEqual(typestr, "long"))
    {
        msg_error("BaseVTKReader") << "encountered legacy type 'long', mapping to Int64 (int64_t). "
                                      "Size of 'long' is platform-dependent.";
        result = makeDataIO<std::int64_t>(numComponents);
    }
    else if (isEqual(typestr, "Int64"))
        result = makeDataIO<std::int64_t>(numComponents);
    else if (isEqual(typestr, "unsigned_long"))
    {
        msg_error("BaseVTKReader")
            << "encountered legacy type 'unsigned_long', mapping to UInt64 (uint64_t). "
               "Size of 'unsigned long' is platform-dependent.";
        result = makeDataIO<std::uint64_t>(numComponents);
    }
    else if (isEqual(typestr, "UInt64"))
        result = makeDataIO<std::uint64_t>(numComponents);
    else if (isEqual(typestr, "float") || isEqual(typestr, "Float32"))
        result = makeDataIO<float>(numComponents);
    else if (isEqual(typestr, "double") || isEqual(typestr, "Float64"))
        result = makeDataIO<double>(numComponents);

    if (result) result->m_nestedDataSize = numComponents;

    return result;
}

bool BaseVTKReader::readVTK(const char* filename) { return readFile(filename); }

}  // namespace sofa::component::io::mesh::basevtkreader
