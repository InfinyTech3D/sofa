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
      m_numberOfPoints(0),
      m_numberOfCells(0)
{
}

BaseVTKReader::BaseVTKDataIO* BaseVTKReader::newVTKDataIO(const string& typestr)
{
    if (isEqual(typestr, "char") || isEqual(typestr, "Int8"))
    {
        return new VTKDataIO<char>;
    }
    else if (isEqual(typestr, "unsigned_char") || isEqual(typestr, "UInt8"))
    {
        return new VTKDataIO<std::uint8_t>;
    }
    else if (isEqual(typestr, "short") || isEqual(typestr, "Int16"))
    {
        return new VTKDataIO<std::int16_t>;
    }
    else if (isEqual(typestr, "unsigned_short") || isEqual(typestr, "UInt16"))
    {
        return new VTKDataIO<std::uint16_t>;
    }
    else if (isEqual(typestr, "int") || isEqual(typestr, "Int32"))
    {
        return new VTKDataIO<std::int32_t>;
    }
    else if (isEqual(typestr, "unsigned_int") || isEqual(typestr, "UInt32"))
    {
        return new VTKDataIO<std::uint32_t>;
    }
    else if (isEqual(typestr, "long") || isEqual(typestr, "Int64"))
    {
        return new VTKDataIO<std::int64_t>;
    }
    else if (isEqual(typestr, "unsigned_long") || isEqual(typestr, "UInt64"))
    {
        return new VTKDataIO<std::uint64_t>;
    }
    else if (isEqual(typestr, "float") || isEqual(typestr, "Float32"))
    {
        return new VTKDataIO<float>;
    }
    else if (isEqual(typestr, "double") || isEqual(typestr, "Float64"))
    {
        return new VTKDataIO<double>;
    }
    else
    {
        return nullptr;
    }
}

BaseVTKReader::BaseVTKDataIO* BaseVTKReader::newVTKDataIO(const string& typestr, int num)
{
    BaseVTKDataIO* result = nullptr;

    if (num == 1)
    {
        result = newVTKDataIO(typestr);
    }
    else
    {
        if (isEqual(typestr, "char") || isEqual(typestr, "Int8"))
        {
            switch (num)
            {
                case 2:
                    result = new VTKDataIO<Vec<2, char> >;
                    break;
                case 3:
                    result = new VTKDataIO<Vec<3, char> >;
                    break;
                case 4:
                    result = new VTKDataIO<Vec<4, char> >;
                    break;
                default:
                    return nullptr;
            }
        }

        if (isEqual(typestr, "unsigned_char") || isEqual(typestr, "UInt8"))
        {
            switch (num)
            {
                case 2:
                    result = new VTKDataIO<Vec<2, std::uint8_t> >;
                    break;
                case 3:
                    result = new VTKDataIO<Vec<3, std::uint8_t> >;
                    break;
                case 4:
                    result = new VTKDataIO<Vec<4, std::uint8_t> >;
                    break;
                default:
                    return nullptr;
            }
        }

        if (isEqual(typestr, "short") || isEqual(typestr, "Int16"))
        {
            switch (num)
            {
                case 2:
                    result = new VTKDataIO<Vec<2, std::int16_t> >;
                    break;
                case 3:
                    result = new VTKDataIO<Vec<3, std::int16_t> >;
                    break;
                case 4:
                    result = new VTKDataIO<Vec<4, std::int16_t> >;
                    break;
                default:
                    return nullptr;
            }
        }

        if (isEqual(typestr, "unsigned_short") || isEqual(typestr, "UInt16"))
        {
            switch (num)
            {
                case 2:
                    result = new VTKDataIO<Vec<2, std::uint16_t> >;
                    break;
                case 3:
                    result = new VTKDataIO<Vec<3, std::uint16_t> >;
                    break;
                case 4:
                    result = new VTKDataIO<Vec<4, std::uint16_t> >;
                    break;
                default:
                    return nullptr;
            }
        }

        if (isEqual(typestr, "int") || isEqual(typestr, "Int32"))
        {
            switch (num)
            {
                case 2:
                    result = new VTKDataIO<Vec<2, std::int32_t> >;
                    break;
                case 3:
                    result = new VTKDataIO<Vec<3, std::int32_t> >;
                    break;
                case 4:
                    result = new VTKDataIO<Vec<4, std::int32_t> >;
                    break;
                default:
                    return nullptr;
            }
        }

        if (isEqual(typestr, "unsigned_int") || isEqual(typestr, "UInt32"))
        {
            switch (num)
            {
                case 2:
                    result = new VTKDataIO<Vec<2, std::uint32_t> >;
                    break;
                case 3:
                    result = new VTKDataIO<Vec<3, std::uint32_t> >;
                    break;
                case 4:
                    result = new VTKDataIO<Vec<4, std::uint32_t> >;
                    break;
                default:
                    return nullptr;
            }
        }

        if (isEqual(typestr, "long") || isEqual(typestr, "Int64"))
        {
            switch (num)
            {
                case 2:
                    result = new VTKDataIO<Vec<2, std::int64_t> >;
                    break;
                case 3:
                    result = new VTKDataIO<Vec<3, std::int64_t> >;
                    break;
                case 4:
                    result = new VTKDataIO<Vec<4, std::int64_t> >;
                    break;
                default:
                    return nullptr;
            }
        }

        if (isEqual(typestr, "unsigned_long") || isEqual(typestr, "UInt64"))
        {
            switch (num)
            {
                case 2:
                    result = new VTKDataIO<Vec<2, std::uint64_t> >;
                    break;
                case 3:
                    result = new VTKDataIO<Vec<3, std::uint64_t> >;
                    break;
                case 4:
                    result = new VTKDataIO<Vec<4, std::uint64_t> >;
                    break;
                default:
                    return nullptr;
            }
        }

        if (isEqual(typestr, "float") || isEqual(typestr, "Float32"))
        {
            switch (num)
            {
                case 2:
                    result = new VTKDataIO<Vec<2, float> >;
                    break;
                case 3:
                    result = new VTKDataIO<Vec<3, float> >;
                    break;
                case 4:
                    result = new VTKDataIO<Vec<4, float> >;
                    break;
                default:
                    return nullptr;
            }
        }
        if (isEqual(typestr, "double") || isEqual(typestr, "Float64"))
        {
            switch (num)
            {
                case 2:
                    result = new VTKDataIO<Vec<2, double> >;
                    break;
                case 3:
                    result = new VTKDataIO<Vec<3, double> >;
                    break;
                case 4:
                    result = new VTKDataIO<Vec<4, double> >;
                    break;
                default:
                    return nullptr;
            }
        }
    }
    result->m_nestedDataSize = num;
    return result;
}

bool BaseVTKReader::readVTK(const char* filename) { return readFile(filename); }

}  // namespace sofa::component::io::mesh::basevtkreader
