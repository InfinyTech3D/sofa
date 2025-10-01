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
#include <sofa/component/io/mesh/MeshVTKLoader.h>
#include <sofa/core/ObjectFactory.h>
#include <sofa/core/visual/VisualParams.h>

#include <cstdio>
#include <iostream>
#include <regex>
#include <sstream>
using sofa::component::io::mesh::BaseVTKReader;

/// This is needed for template specialization.
#include <tinyxml2.h>

#include <sofa/component/io/mesh/BaseVTKReader.inl>

// XML VTK Loader
#define checkError(A) \
    if (!A)           \
    {                 \
        return false; \
    }
#define checkErrorPtr(A) \
    if (!A)              \
    {                    \
        return nullptr;  \
    }
#define checkErrorMsg(A, B)       \
    if (!A)                       \
    {                             \
        msg_error() << B << "\n"; \
        return false;             \
    }
#define checkErrorMsgAuto(A)                                                                 \
    if (A != tinyxml2::XML_SUCCESS)                                                          \
    {                                                                                        \
        msg_error() << "TinyXML error message : " << tinyxml2::XMLDocument::ErrorIDToName(A) \
                    << "\n";                                                                 \
        return false;                                                                        \
    }

namespace sofa::component::io::mesh
{

using namespace sofa::type;
using namespace sofa::defaulttype;
using namespace sofa::helper;
using sofa::core::objectmodel::BaseData;
using sofa::core::objectmodel::BaseObject;
using sofa::core::objectmodel::ComponentState;
using sofa::type::Vec;
using sofa::type::Vec3;
using std::istream;
using std::istringstream;
using std::ofstream;
using std::string;
using type::vector;

class LegacyVTKReader : public BaseVTKReader
{
   public:
    bool readFile(const char* filename) override;
};

class XMLVTKReader : public BaseVTKReader
{
   public:
    bool readFile(const char* filename) override;

   protected:
    bool loadUnstructuredGrid(tinyxml2::XMLHandle datasetFormatHandle);
    bool loadPolydata(tinyxml2::XMLHandle datasetFormatHandle);
    bool loadRectilinearGrid(tinyxml2::XMLHandle datasetFormatHandle);
    bool loadStructuredGrid(tinyxml2::XMLHandle datasetFormatHandle);
    bool loadStructuredPoints(tinyxml2::XMLHandle datasetFormatHandle);
    bool loadImageData(tinyxml2::XMLHandle datasetFormatHandle);
    BaseVTKDataIO* loadDataArray(tinyxml2::XMLElement* dataArrayElement, int size, string type);
    BaseVTKDataIO* loadDataArray(tinyxml2::XMLElement* dataArrayElement, int size);
    BaseVTKDataIO* loadDataArray(tinyxml2::XMLElement* dataArrayElement);
};

////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// MeshVTKLoader IMPLEMENTATION //////////////////////////////////
MeshVTKLoader::MeshVTKLoader() : MeshLoader(), reader(nullptr) {}

MeshVTKLoader::VTKFileType MeshVTKLoader::detectFileType(const char* filename)
{
    std::ifstream inVTKFile(filename, std::ifstream::in | std::ifstream::binary);

    if (!inVTKFile.is_open())
    {
        return MeshVTKLoader::NONE;
    }

    string line;
    std::getline(inVTKFile, line);

    if (line.find("<?xml") != string::npos)
    {
        std::getline(inVTKFile, line);

        if (line.find("<VTKFile") != string::npos)
        {
            return MeshVTKLoader::XML;
        }
        else
        {
            return MeshVTKLoader::NONE;
        }
    }
    else if (line.find("<VTKFile") != string::npos)  //... not xml-compliant
    {
        return MeshVTKLoader::XML;
    }
    else if (line.find("# vtk DataFile") != string::npos)
    {
        std::regex pattern(R"(# vtk DataFile Version (\d+)\.\d+)");
        std::smatch match;

        if (std::regex_search(line, match, pattern) && match.size() == 2)
        {
            std::string version = match[1].str();
            if (stod(version) >= 5)
                msg_warning() << "VTK5.+ format might not be well supported, see issue "
                                 "https://github.com/sofa-framework/sofa/issues/3405";
            else
                msg_info() << "Extracted version: " << version;
        }
        else
        {
            msg_warning() << "Could not read the version of VTK";
        }
        return MeshVTKLoader::LEGACY;
    }
    else  // default behavior if the first line is not correct ?
    {
        return MeshVTKLoader::NONE;
    }
}

bool MeshVTKLoader::doLoad()
{
    msg_info() << "Loading VTK file: " << d_filename;

    bool fileRead = false;

    // -- Loading file
    const char* filename = d_filename.getFullPath().c_str();

    // Detect file type (legacy or vtk)
    const MeshVTKLoader::VTKFileType type = detectFileType(filename);
    switch (type)
    {
        case XML:
            reader = new XMLVTKReader();
            break;
        case LEGACY:
            reader = new LegacyVTKReader();
            break;
        default:
            msg_error() << "Header not recognized";
            reader = nullptr;
            break;
    }

    if (!reader)
    {
        return false;
    }

    // -- Reading file
    if (!canLoad())
    {
        return false;
    }

    fileRead = reader->readVTK(filename);
    this->setInputsMesh();
    this->setInputsData();

    delete reader;

    return fileRead;
}

bool MeshVTKLoader::setInputsMesh()
{
    auto my_positions = getWriteOnlyAccessor(d_positions);
    if (reader->m_inputPoints)
    {
        BaseVTKReader::VTKDataIO<double>* vtkpd =
            dynamic_cast<BaseVTKReader::VTKDataIO<double>*>(reader->m_inputPoints);
        BaseVTKReader::VTKDataIO<float>* vtkpf =
            dynamic_cast<BaseVTKReader::VTKDataIO<float>*>(reader->m_inputPoints);
        if (vtkpd)
        {
            const double* inPoints = (vtkpd->m_data);
            if (inPoints)
                for (int i = 0; i < vtkpd->m_dataSize; i += 3)
                {
                    my_positions.push_back(Vec3d(double(inPoints[i + 0]), double(inPoints[i + 1]),
                                                 double(inPoints[i + 2])));
                }
            else
            {
                return false;
            }
        }
        else if (vtkpf)
        {
            const float* inPoints = (vtkpf->m_data);
            if (inPoints)
                for (int i = 0; i < vtkpf->m_dataSize; i += 3)
                {
                    my_positions.push_back(
                        Vec3f(inPoints[i + 0], inPoints[i + 1], inPoints[i + 2]));
                }
            else
            {
                return false;
            }
        }
        else
        {
            msg_info() << "Type of coordinate (X,Y,Z) not supported";
            return false;
        }
    }
    else
    {
        return false;
    }

    auto my_normals = getWriteOnlyAccessor(d_normals);
    if (reader->m_inputNormals)
    {
        BaseVTKReader::VTKDataIO<double>* vtkpd =
            dynamic_cast<BaseVTKReader::VTKDataIO<double>*>(reader->m_inputNormals);
        BaseVTKReader::VTKDataIO<float>* vtkpf =
            dynamic_cast<BaseVTKReader::VTKDataIO<float>*>(reader->m_inputNormals);

        if (vtkpd)
        {
            const double* inNormals = (vtkpd->m_data);
            if (inNormals)
                for (int i = 0; i < vtkpd->m_dataSize; i += 3)
                {
                    my_normals.push_back(Vec3(double(inNormals[i + 0]), double(inNormals[i + 1]),
                                              double(inNormals[i + 2])));
                }
            else
            {
                return false;
            }
        }
        else if (vtkpf)
        {
            const float* inNormals = (vtkpf->m_data);
            if (inNormals)
                for (int i = 0; i < vtkpf->m_dataSize; i += 3)
                {
                    my_normals.push_back(Vec3 (inNormals[i + 0], inNormals[i + 1], inNormals[i + 2]));
                }
            else
            {
                return false;
            }
        }
        else
        {
            msg_info() << "Type of coordinate (X,Y,Z) not supported";
            return false;
        }
    }

    auto my_polylines = getWriteOnlyAccessor(d_polylines);
    auto my_edges = getWriteOnlyAccessor(d_edges);
    auto my_triangles = getWriteOnlyAccessor(d_triangles);
    auto my_quads = getWriteOnlyAccessor(d_quads);
    auto my_tetrahedra = getWriteOnlyAccessor(d_tetrahedra);
    auto my_hexahedra = getWriteOnlyAccessor(d_hexahedra);

    auto my_highOrderEdgePositions = getWriteOnlyAccessor(d_highOrderEdgePositions);

    int errorcount = 0;
    if (reader->m_inputPolygons)
    {
        const int* inFP = (const int*)reader->m_inputPolygons->getData();
        int poly = 0;
        for (int i = 0; i < reader->m_inputPolygons->m_dataSize;)
        {
            int nv = inFP[i];
            ++i;
            bool valid = true;
            if (reader->m_inputPoints)
            {
                for (int j = 0; j < nv; ++j)
                {
                    if (unsigned(inFP[i + j]) >= unsigned(reader->m_inputPoints->m_dataSize / 3))
                    {
                        /// More user friendly error message to avoid flooding him
                        /// in case of severely broken file.
                        errorcount++;

                        if (errorcount < 20)
                        {
                            msg_error() << "invalid point at " << i + j << " in polygon " << poly;
                        }
                        if (errorcount == 20)
                        {
                            msg_error() << "too much invalid points in polygon '" << poly
                                        << "' ...now hiding others error message.";
                        }
                        valid = false;
                    }
                }
            }
            if (valid)
            {
                if (nv == 4)
                {
                    addQuad(my_quads.wref(), unsigned(inFP[i + 0]), unsigned(inFP[i + 1]),
                            unsigned(inFP[i + 2]), unsigned(inFP[i + 3]));
                }
                else if (nv >= 3)
                {
                    int f[3];
                    f[0] = inFP[i + 0];
                    f[1] = inFP[i + 1];
                    for (int j = 2; j < nv; j++)
                    {
                        f[2] = inFP[i + j];
                        addTriangle(my_triangles.wref(), unsigned(f[0]), unsigned(f[1]),
                                    unsigned(f[2]));
                        f[1] = f[2];
                    }
                }
                i += nv;
            }
            ++poly;
        }
    }
    else if (reader->m_inputCells && reader->m_inputCellTypes)
    {
        const int* inFP = (const int*)reader->m_inputCells->getData();
        // std::cout
        //     << "inFP: "
        //     << inFP[0] << " " << inFP[1] << " " << inFP[2]
        //     << " "
        //     << inFP[3] << " " << inFP[4] << " " << inFP[5]
        //     << std::endl
        //     << "inFP: "
        //     << inFP[6] << " " << inFP[7] << " " << inFP[8]
        //     << " "
        //     << inFP[9] << " " << inFP[10] << " " << inFP[11]
        //     << std::endl
        //     << "inFP: "
        //     << inFP[12] << " " << inFP[13] << " " << inFP[14]
        //     << " "
        //     << inFP[15] << " " << inFP[16] << " " << inFP[17]
        //     << std::endl;

        // offsets are not used if we have parsed with the legacy method
        const int* offsets = (reader->m_inputCellOffsets == nullptr)
                                 ? nullptr
                                 : (const int*)reader->m_inputCellOffsets->getData();

        const int* dataT = (int*)(reader->m_inputCellTypes->getData());

        type::vector<int> numSubPolyLines;

        const unsigned int edgesInQuadraticTriangle[3][2] = {{0, 1}, {1, 2}, {2, 0}};
        const unsigned int edgesInQuadraticTetrahedron[6][2] = {{0, 1}, {1, 2}, {0, 2},
                                                                {0, 3}, {1, 3}, {2, 3}};
        std::set<topology::Edge> edgeSet;
        size_t j;
        int nbf = reader->m_numberOfCells;
        int i = 0;
        for (int c = 0; c < nbf; ++c)
        {
            int t = dataT[c];  // - 48; //ASCII
            int nv;
            if (offsets)
            {
                i = (c == 0) ? 0 : offsets[c - 1];
                nv = inFP[i];
                // std::cout << "inFP: "
                //     << inFP[i + 0] << " " << inFP[i + 1] << " " << inFP[i + 2]
                //     << std::endl;
                // std::cout << "i : " << i << std::endl;
            }
            else
            {
                nv = inFP[i];
                ++i;
            }

            switch (t)
            {
                case 0:  // EMPTY_CELL
                    break;
                case 1:  // VERTEX
                    break;
                case 2:  // POLY_VERTEX
                    break;
                case 3:  // LINE
                    addEdge(my_edges.wref(), unsigned(inFP[i + 0]), unsigned(inFP[i + 1]));
                    break;
                case 4:  // POLY_LINE
                {
                    numSubPolyLines.push_back(nv);
                    std::vector<PointID> points;
                    for (int v = 0; v < nv; ++v)
                    {
                        points.push_back(unsigned(inFP[i + v]));
                    }
                    addPolyline(my_polylines.wref(), points);
                }
                break;
                case 5:  // TRIANGLE
                    addTriangle(my_triangles.wref(), unsigned(inFP[i + 0]), unsigned(inFP[i + 1]),
                                unsigned(inFP[i + 2]));
                    break;
                case 6:  // TRIANGLE_STRIP
                    for (int j = 0; j < nv - 2; j++)
                        if (j & 1)
                        {
                            addTriangle(my_triangles.wref(), unsigned(inFP[i + j + 0]),
                                        unsigned(inFP[i + j + 1]), unsigned(inFP[i + j + 2]));
                        }
                        else
                        {
                            addTriangle(my_triangles.wref(), unsigned(inFP[i + j + 0]),
                                        unsigned(inFP[i + j + 2]), unsigned(inFP[i + j + 1]));
                        }
                    break;
                case 7:  // POLYGON
                    for (int j = 2; j < nv; j++)
                    {
                        addTriangle(my_triangles.wref(), unsigned(inFP[i + 0]),
                                    unsigned(inFP[i + j - 1]), unsigned(inFP[i + j]));
                    }
                    break;
                case 8:  // PIXEL
                    addQuad(my_quads.wref(), unsigned(inFP[i + 0]), unsigned(inFP[i + 1]),
                            unsigned(inFP[i + 3]), unsigned(inFP[i + 2]));
                    break;
                case 9:  // QUAD
                    addQuad(my_quads.wref(), unsigned(inFP[i + 0]), unsigned(inFP[i + 1]),
                            unsigned(inFP[i + 2]), unsigned(inFP[i + 3]));
                    break;
                case 10:  // TETRA
                    // std::cout << inFP[i + 0] << " " << inFP[i + 1] << " " << inFP[i + 2] << " "
                    //           << inFP[i + 3] << std::endl;
                    addTetrahedron(my_tetrahedra.wref(), unsigned(inFP[i + 0]),
                                   unsigned(inFP[i + 1]), unsigned(inFP[i + 2]),
                                   unsigned(inFP[i + 3]));
                    break;
                case 11:  // VOXEL
                    addHexahedron(my_hexahedra.wref(), unsigned(inFP[i + 0]), unsigned(inFP[i + 1]),
                                  unsigned(inFP[i + 3]), unsigned(inFP[i + 2]),
                                  unsigned(inFP[i + 4]), unsigned(inFP[i + 5]),
                                  unsigned(inFP[i + 7]), unsigned(inFP[i + 6]));
                    break;
                case 12:  // HEXAHEDRON
                    addHexahedron(my_hexahedra.wref(), unsigned(inFP[i + 0]), unsigned(inFP[i + 1]),
                                  unsigned(inFP[i + 2]), unsigned(inFP[i + 3]),
                                  unsigned(inFP[i + 4]), unsigned(inFP[i + 5]),
                                  unsigned(inFP[i + 6]), unsigned(inFP[i + 7]));
                    break;
                case 21:  // QUADRATIC Edge
                    addEdge(my_edges.wref(), unsigned(inFP[i + 0]), unsigned(inFP[i + 1]));
                    {
                        HighOrderEdgePosition hoep;
                        hoep[0] = unsigned(inFP[i + 2]);
                        hoep[1] = unsigned(my_edges.size() - 1);
                        hoep[2] = 1;
                        hoep[3] = 1;
                        my_highOrderEdgePositions.push_back(hoep);
                    }
                    break;
                case 22:  // QUADRATIC Triangle
                    addTriangle(my_triangles.wref(), unsigned(inFP[i + 0]), unsigned(inFP[i + 1]),
                                unsigned(inFP[i + 2]));
                    {
                        HighOrderEdgePosition hoep;
                        for (j = 0; j < 3; ++j)
                        {
                            sofa::Index v0 = std::min(inFP[i + edgesInQuadraticTriangle[j][0]],
                                                      inFP[i + edgesInQuadraticTriangle[j][1]]);
                            sofa::Index v1 = std::max(inFP[i + edgesInQuadraticTriangle[j][0]],
                                                      inFP[i + edgesInQuadraticTriangle[j][1]]);
                            topology::Edge e(v0, v1);
                            if (!edgeSet.contains(e))
                            {
                                edgeSet.insert(e);
                                addEdge(my_edges.wref(), v0, v1);
                                hoep[0] = inFP[i + j + 3];
                                hoep[1] = sofa::Size(my_edges.size()) - 1;
                                hoep[2] = 1;
                                hoep[3] = 1;
                                my_highOrderEdgePositions.push_back(hoep);
                            }
                        }
                    }
                    break;
                case 24:  // QUADRATIC Tetrahedron
                    addTetrahedron(my_tetrahedra.wref(), inFP[i + 0], inFP[i + 1], inFP[i + 2],
                                   inFP[i + 3]);
                    {
                        HighOrderEdgePosition hoep;
                        for (j = 0; j < 6; ++j)
                        {
                            sofa::Index v0 = std::min(inFP[i + edgesInQuadraticTetrahedron[j][0]],
                                                      inFP[i + edgesInQuadraticTetrahedron[j][1]]);
                            sofa::Index v1 = std::max(inFP[i + edgesInQuadraticTetrahedron[j][0]],
                                                      inFP[i + edgesInQuadraticTetrahedron[j][1]]);
                            topology::Edge e(v0, v1);
                            if (!edgeSet.contains(e))
                            {
                                edgeSet.insert(e);
                                addEdge(my_edges.wref(), v0, v1);
                                hoep[0] = inFP[i + j + 4];
                                hoep[1] = sofa::Size(my_edges.size()) - 1;
                                hoep[2] = 1;
                                hoep[3] = 1;
                                my_highOrderEdgePositions.push_back(hoep);
                            }
                        }
                    }
                    break;
                    // more types are defined in vtkCellType.h in libvtk
                default:
                    msg_error() << "Unsupported cell type " << t;
            }

            if (!offsets)
            {
                i += nv;
            }
        }

        if (numSubPolyLines.size() > 0)
        {
            size_t sz = reader->m_inputCellDataVector.size();
            reader->m_inputCellDataVector.resize(sz + 1);
            reader->m_inputCellDataVector[sz] = reader->newVTKDataIO("int");

            BaseVTKReader::VTKDataIO<int>* cellData =
                dynamic_cast<BaseVTKReader::VTKDataIO<int>*>(reader->m_inputCellDataVector[sz]);

            if (cellData == nullptr)
            {
                return false;
            }

            cellData->resize((int)numSubPolyLines.size());

            for (size_t ii = 0; ii < numSubPolyLines.size(); ii++)
            {
                cellData->m_data[ii] = numSubPolyLines[ii];
            }

            cellData->m_name = "PolyLineSubEdges";
        }
    }
    if (reader->m_inputPoints)
    {
        delete reader->m_inputPoints;
    }
    if (reader->m_inputNormals)
    {
        delete reader->m_inputNormals;
    }
    if (reader->m_inputPolygons)
    {
        delete reader->m_inputPolygons;
    }
    if (reader->m_inputCells)
    {
        delete reader->m_inputCells;
    }
    if (reader->m_inputCellTypes)
    {
        delete reader->m_inputCellTypes;
    }

    return true;
}

bool MeshVTKLoader::setInputsData()
{
    /// Point Data
    for (const auto& inputPointData : reader->m_inputPointDataVector)
    {
        const char* dataname = inputPointData->m_name.c_str();

        BaseData* basedata = inputPointData->createSofaData();
        this->addData(basedata, dataname);
        addOutputsToCallback("filename", {basedata});
    }

    /// Cell Data
    for (const auto& inputCellData : reader->m_inputCellDataVector)
    {
        const char* dataname = inputCellData->m_name.c_str();
        BaseData* basedata = inputCellData->createSofaData();
        this->addData(basedata, dataname);
        addOutputsToCallback("filename", {basedata});
    }

    return true;
}

void MeshVTKLoader::doClearBuffers()
{
    // Should clear data fields added by setInputsData(), Preferably without using
    // abstractTypeInfo...
}

// Legacy VTK Loader
bool LegacyVTKReader::readFile(const char* filename)
{
    std::ifstream inVTKFile(filename, std::ifstream::in | std::ifstream::binary);
    if (!inVTKFile.is_open())
    {
        return false;
    }

    string line;

    // Part 1
    std::getline(inVTKFile, line);
    if (string(line, 0, 23) != "# vtk DataFile Version ")
    {
        msg_error() << "Unrecognized header in file '" << filename << "'.";
        return false;
    }
    string version(line, 23);

    // Part 2
    string header;
    std::getline(inVTKFile, header);

    // Part 3
    std::getline(inVTKFile, line);

    int binary;
    if (line == "BINARY" || line == "BINARY\r")
    {
        binary = 1;
    }
    else if (line == "ASCII" || line == "ASCII\r")
    {
        binary = 0;
    }
    else
    {
        msg_error() << "Unrecognized format in file '" << filename << "'.";
        return false;
    }

    if (binary && strlen(filename) > 9 && !strcmp(filename + strlen(filename) - 9, ".vtk_swap"))
    {
        binary = 2;  // bytes will be swapped
    }

    // Part 4
    do
    {
        std::getline(inVTKFile, line);
    } while (line.empty());
    if (line != "DATASET POLYDATA" && line != "DATASET UNSTRUCTURED_GRID" &&
        line != "DATASET POLYDATA\r" && line != "DATASET UNSTRUCTURED_GRID\r")
    {
        msg_error() << "Unsupported data type in file '" << filename << "'.";
        return false;
    }

    msg_info() << (binary == 0     ? "Text"
                   : (binary == 1) ? "Binary"
                                   : "Swapped Binary")
               << " VTK File (version " << version << "): " << header;
    VTKDataIO<int>* m_inputPolygonsInt = nullptr;
    VTKDataIO<int>* m_inputCellsInt = nullptr;
    VTKDataIO<int>* m_inputCellTypesInt = nullptr;
    m_inputCellOffsets = nullptr;

    while (!inVTKFile.eof())
    {
        do
        {
            std::getline(inVTKFile, line);
        } while (!inVTKFile.eof() && line.empty());

        istringstream ln(line);
        string kw;
        ln >> kw;
        if (kw == "POINTS")
        {
            int n;
            string typestr;
            ln >> n >> typestr;
            msg_info() << "Found " << n << " " << typestr << " points";
            m_inputPoints = newVTKDataIO(typestr);
            if (m_inputPoints == nullptr)
            {
                return false;
            }
            if (!m_inputPoints->read(inVTKFile, 3 * n, binary))
            {
                return false;
            }
            // nbp = n;
        }
        else if (kw == "POLYGONS")
        {
            int n, ni;
            ln >> n >> ni;
            msg_info() << n << " polygons ( " << (ni - 3 * n) << " triangles )";
            m_inputPolygons = new VTKDataIO<int>;
            m_inputPolygonsInt = dynamic_cast<VTKDataIO<int>*>(m_inputPolygons);
            if (!m_inputPolygons->read(inVTKFile, ni, binary))
            {
                return false;
            }
        }
        else if (kw == "CELLS")
        {
            int n, ni;
            ln >> n >> ni;
            msg_info() << "Found " << n << " cells";
            m_inputCells = new VTKDataIO<int>;
            m_inputCellsInt = dynamic_cast<VTKDataIO<int>*>(m_inputCells);
            if (!m_inputCells->read(inVTKFile, ni, binary))
            {
                return false;
            }
            m_numberOfCells = n;
        }
        else if (kw == "LINES")
        {
            int n, ni;
            ln >> n >> ni;
            msg_info() << "Found " << n << " lines";
            m_inputCells = new VTKDataIO<int>;
            m_inputCellsInt = dynamic_cast<VTKDataIO<int>*>(m_inputCellsInt);
            if (!m_inputCells->read(inVTKFile, ni, binary))
            {
                return false;
            }
            m_numberOfCells = n;

            m_inputCellTypes = new VTKDataIO<int>;
            m_inputCellTypesInt = dynamic_cast<VTKDataIO<int>*>(m_inputCellTypes);
            m_inputCellTypesInt->resize(n);
            for (int i = 0; i < n; i++)
            {
                m_inputCellTypesInt->m_data[i] = 4;
            }
        }
        else if (kw == "CELL_TYPES")
        {
            int n;
            ln >> n;
            m_inputCellTypes = new VTKDataIO<int>;
            m_inputCellTypesInt = dynamic_cast<VTKDataIO<int>*>(m_inputCellTypes);
            if (!m_inputCellTypes->read(inVTKFile, n, binary))
            {
                return false;
            }
        }
        else if (kw == "CELL_DATA" || kw == "POINT_DATA")
        {
            const bool cellData = (kw == "CELL_DATA");
            type::vector<BaseVTKDataIO*>& m_inputDataVector = cellData ? m_inputCellDataVector
                                                                     : m_inputPointDataVector;
            int nb_ele;
            ln >> nb_ele;
            while (!inVTKFile.eof())
            {
                std::ifstream::pos_type previousPos = inVTKFile.tellg();
                /// line defines the type and name such as SCALAR dataset
                do
                {
                    std::getline(inVTKFile, line);
                } while (!inVTKFile.eof() && line.empty());

                if (line.empty())
                {
                    break;
                }
                istringstream lnData(line);
                string dataStructure;
                lnData >> dataStructure;

                msg_info() << "Data structure: " << dataStructure;

                if (dataStructure == "SCALARS")
                {
                    string dataName, dataType;
                    lnData >> dataName >> dataType;
                    BaseVTKDataIO* data = newVTKDataIO(dataType);
                    if (data != nullptr)
                    {
                        {
                            // skip lookup_table if present
                            const std::ifstream::pos_type positionBeforeLookupTable =
                                inVTKFile.tellg();
                            std::string lookupTable;
                            std::string lookupTableName;
                            std::getline(inVTKFile, line);
                            istringstream lnDataLookup(line);
                            lnDataLookup >> lookupTable >> lookupTableName;
                            if (lookupTable == "LOOKUP_TABLE")
                            {
                                msg_info()
                                    << "Ignoring lookup table named \"" << lookupTableName << "\".";
                            }
                            else
                            {
                                inVTKFile.seekg(positionBeforeLookupTable);
                            }
                        }
                        if (data->read(inVTKFile, nb_ele, binary))
                        {
                            m_inputDataVector.push_back(data);
                            data->name = dataName;
                            if (kw == "CELL_DATA")
                            {
                                msg_info() << "Read cell data: " << data->name;
                            }
                            else
                            {
                                msg_info() << "Read point data: " << data->name;
                            }
                        }
                        else
                        {
                            delete data;
                        }
                    }
                }
                else if (dataStructure == "NORMALS")
                {
                    string dataName, dataType;
                    lnData >> dataName >> dataType;
                    msg_info() << "Reading normals named \"" << dataName << "\" of type \""
                               << dataType << "\".";
                    m_inputNormals = newVTKDataIO(dataType);
                    if (m_inputNormals == nullptr)
                    {
                        return false;
                    }
                    if (!m_inputNormals->read(inVTKFile, 3 * nb_ele, binary))
                    {
                        return false;
                    }
                }
                else if (dataStructure == "VECTORS")
                {
                    string dataName, dataType;
                    lnData >> dataName >> dataType;
                    BaseVTKDataIO* data = newVTKDataIO(dataType, 3);
                    if (data != nullptr)
                    {
                        if (data->read(inVTKFile, nb_ele, binary))
                        {
                            m_inputDataVector.push_back(data);
                            data->name = dataName;
                            if (kw == "CELL_DATA")
                            {
                                msg_info() << "Read cell data: " << data->name;
                            }
                            else
                            {
                                msg_info() << "Read point data: " << data->name;
                            }
                        }
                        else
                        {
                            delete data;
                        }
                    }
                }
                else if (dataStructure == "FIELD")
                {
                    std::string fieldName;
                    unsigned int nb_arrays = 0u;
                    lnData >> fieldName >> nb_arrays;
                    msg_info() << "Reading field \"" << fieldName << "\" with " << nb_arrays
                               << " arrays.";
                    for (unsigned field = 0u; field < nb_arrays; ++field)
                    {
                        do
                        {
                            std::getline(inVTKFile, line);
                        } while (line.empty());
                        istringstream lnData(line);
                        std::string dataName;
                        int nbData;
                        int nbComponents;
                        std::string dataType;
                        lnData >> dataName >> nbComponents >> nbData >> dataType;
                        msg_info() << "Reading field data named \"" << dataName << "\" of type \""
                                   << dataType << "\" with " << nbComponents << " components.";
                        BaseVTKDataIO* data = newVTKDataIO(dataType, nbComponents);
                        if (data != nullptr)
                        {
                            if (data->read(inVTKFile, nbData, binary))
                            {
                                m_inputDataVector.push_back(data);
                                data->name = dataName;
                            }
                            else
                            {
                                delete data;
                            }
                        }
                    }
                }
                else if (dataStructure == "LOOKUP_TABLE")
                {
                    std::string tableName;
                    lnData >> tableName >> nb_ele;
                    msg_info() << "Ignoring the definition of the lookup table named \""
                               << tableName << "\".";
                    if (binary)
                    {
                        BaseVTKDataIO* data =
                            newVTKDataIO("UInt8", 4);  // in the binary case there will be 4
                                                       // unsigned chars per table entry
                        if (data)
                        {
                            data->read(inVTKFile, nb_ele, binary);
                        }
                        delete data;
                    }
                    else
                    {
                        BaseVTKDataIO* data = newVTKDataIO("Float32", 4);
                        if (data)
                        {
                            data->read(inVTKFile, nb_ele, binary);  // in the ascii case there will
                                                                    // be 4 float32 per table entry
                        }
                        delete data;
                    }
                }
                else  /// TODO
                {
                    inVTKFile.seekg(previousPos);
                    break;
                }
            }
            continue;
        }
        else if (!kw.empty())
        {
            msg_warning() << "Unknown keyword " << kw;
        }

        msg_info() << "LNG: " << m_inputCellDataVector.size();

        if (m_inputPoints && m_inputPolygons)
        {
            break;  // already found the mesh description, skip the rest
        }
        if (m_inputPoints && m_inputCells && m_inputCellTypes && m_inputCellDataVector.size() > 0)
        {
            break;  // already found the mesh description, skip the rest
        }
    }

    if (binary)
    {
        // detect swapped data
        bool swapped = false;
        if (m_inputPolygons)
        {
            if ((unsigned)m_inputPolygonsInt->m_data[0] >
                (unsigned)m_inputPolygonsInt->swapT(m_inputPolygonsInt->m_data[0], 1))
            {
                swapped = true;
            }
        }
        else if (m_inputCells && m_inputCellTypes)
        {
            if ((unsigned)m_inputCellTypesInt->m_data[0] >
                (unsigned)m_inputCellTypesInt->swapT(m_inputCellTypesInt->m_data[0], 1))
            {
                swapped = true;
            }
        }
        if (swapped)
        {
            msg_info() << "Binary data is byte-swapped.";
            if (m_inputPoints)
            {
                m_inputPoints->swap();
            }
            if (m_inputPolygons)
            {
                m_inputPolygons->swap();
            }
            if (m_inputCells)
            {
                m_inputCells->swap();
            }
            if (m_inputCellTypes)
            {
                m_inputCellTypes->swap();
            }
        }
    }

    return true;
}

bool XMLVTKReader::readFile(const char* filename)
{
    tinyxml2::XMLDocument vtkDoc(true, tinyxml2::COLLAPSE_WHITESPACE);
    // quick check
    checkErrorMsgAuto(vtkDoc.LoadFile(filename))

        tinyxml2::XMLHandle hVTKDoc(&vtkDoc);
    tinyxml2::XMLElement* pElem;
    tinyxml2::XMLHandle hVTKDocRoot(nullptr);

    // block VTKFile
    pElem = hVTKDoc.FirstChildElement().ToElement();
    checkErrorMsg(pElem, "VTKFile Node not found");

    hVTKDocRoot = tinyxml2::XMLHandle(pElem);

    // Endianness
    const char* endiannessStrTemp = pElem->Attribute("byte_order");
    m_isLittleEndian = (string(endiannessStrTemp).compare("LittleEndian") == 0);

    // read VTK data format type
    const char* datasetFormatStrTemp = pElem->Attribute("type");
    checkErrorMsg(datasetFormatStrTemp, "Dataset format not defined");
    const string datasetFormatStr = string(datasetFormatStrTemp);
    VTKDatasetFormat datasetFormat;

    if (datasetFormatStr.compare("UnstructuredGrid") == 0)
    {
        datasetFormat = VTKDatasetFormat::UNSTRUCTURED_GRID;
    }
    else if (datasetFormatStr.compare("PolyData") == 0)
    {
        datasetFormat = VTKDatasetFormat::POLYDATA;
    }
    else if (datasetFormatStr.compare("RectilinearGrid") == 0)
    {
        datasetFormat = VTKDatasetFormat::RECTILINEAR_GRID;
    }
    else if (datasetFormatStr.compare("StructuredGrid") == 0)
    {
        datasetFormat = VTKDatasetFormat::STRUCTURED_GRID;
    }
    else if (datasetFormatStr.compare("StructuredPoints") == 0)
    {
        datasetFormat = VTKDatasetFormat::STRUCTURED_POINTS;
    }
    else if (datasetFormatStr.compare("ImageData") == 0)
    {
        datasetFormat = VTKDatasetFormat::IMAGE_DATA;
    }
    else
    {
        checkErrorMsg(false, "Dataset format " << datasetFormatStr << " not recognized");
    }

    const tinyxml2::XMLHandle datasetFormatHandle =
        tinyxml2::XMLHandle(hVTKDocRoot.FirstChildElement(datasetFormatStr.c_str()));

    bool stateLoading = false;
    switch (datasetFormat)
    {
        case VTKDatasetFormat::UNSTRUCTURED_GRID:
            stateLoading = loadUnstructuredGrid(datasetFormatHandle);
            break;
        case VTKDatasetFormat::POLYDATA:
            stateLoading = loadPolydata(datasetFormatHandle);
            break;
        case VTKDatasetFormat::RECTILINEAR_GRID:
            stateLoading = loadRectilinearGrid(datasetFormatHandle);
            break;
        case VTKDatasetFormat::STRUCTURED_GRID:
            stateLoading = loadStructuredGrid(datasetFormatHandle);
            break;
        case VTKDatasetFormat::STRUCTURED_POINTS:
            stateLoading = loadStructuredPoints(datasetFormatHandle);
            break;
        case VTKDatasetFormat::IMAGE_DATA:
            stateLoading = loadImageData(datasetFormatHandle);
            break;
        default:
            checkErrorMsg(false, "Dataset format not implemented");
            break;
    }
    checkErrorMsg(stateLoading, "Unable to parse XML");

    return true;
}

BaseVTKReader::BaseVTKDataIO* XMLVTKReader::loadDataArray(tinyxml2::XMLElement* dataArrayElement)
{
    return loadDataArray(dataArrayElement, 0);
}

BaseVTKReader::BaseVTKDataIO* XMLVTKReader::loadDataArray(tinyxml2::XMLElement* dataArrayElement,
                                                          int size)
{
    return loadDataArray(dataArrayElement, size, "");
}

BaseVTKReader::BaseVTKDataIO* XMLVTKReader::loadDataArray(tinyxml2::XMLElement* dataArrayElement,
                                                          int size, string type)
{
    // Type
    const char* typeStrTemp;
    if (type.empty())
    {
        typeStrTemp = dataArrayElement->Attribute("type");
        checkErrorPtr(typeStrTemp);
    }
    else
    {
        typeStrTemp = type.c_str();
    }

    // Format
    const char* formatStrTemp = dataArrayElement->Attribute("format");

    if (formatStrTemp == nullptr)
    {
        formatStrTemp = dataArrayElement->Attribute("Format");
    }

    checkErrorPtr(formatStrTemp);

    int binary = 0;
    if (string(formatStrTemp).compare("ascii") == 0)
    {
        binary = 0;
    }
    else if (m_isLittleEndian)
    {
        binary = 1;
    }
    else
    {
        binary = 2;
    }

    // NumberOfComponents
    int numberOfComponents;
    if (dataArrayElement->QueryIntAttribute("NumberOfComponents", &numberOfComponents) !=
        tinyxml2::XML_SUCCESS)
    {
        numberOfComponents = 1;
    }

    // Values
    const char* listValuesStrTemp = dataArrayElement->GetText();

    bool state = false;

    if (!listValuesStrTemp)
    {
        return nullptr;
    }
    if (string(listValuesStrTemp).size() < 1)
    {
        return nullptr;
    }

    BaseVTKDataIO* d = BaseVTKReader::newVTKDataIO(string(typeStrTemp));

    if (!d)
    {
        return nullptr;
    }

    if (size > 0)
    {
        state = (d->read(string(listValuesStrTemp), numberOfComponents * size, binary));
    }
    else
    {
        state = (d->read(string(listValuesStrTemp), binary));
    }
    checkErrorPtr(state);

    return d;
}

bool XMLVTKReader::loadUnstructuredGrid(tinyxml2::XMLHandle datasetFormatHandle)
{
    tinyxml2::XMLElement* pieceElem = datasetFormatHandle.FirstChildElement("Piece").ToElement();

    checkError(pieceElem);
    for (; pieceElem; pieceElem = pieceElem->NextSiblingElement())
    {
        pieceElem->QueryIntAttribute("NumberOfPoints", &m_numberOfPoints);
        pieceElem->QueryIntAttribute("NumberOfCells", &m_numberOfCells);

        tinyxml2::XMLNode* dataArrayNode;
        tinyxml2::XMLElement* dataArrayElement;
        tinyxml2::XMLNode* node = pieceElem->FirstChild();

        for (; node; node = node->NextSibling())
        {
            string currentNodeName = string(node->Value());

            if (currentNodeName.compare("Points") == 0)
            {
                /* Points */
                dataArrayNode = node->FirstChildElement("DataArray");
                checkError(dataArrayNode);
                dataArrayElement = dataArrayNode->ToElement();
                checkError(dataArrayElement);
                //Force the points coordinates to be stocked as double
                m_inputPoints = loadDataArray(dataArrayElement, m_numberOfPoints, "Float64");
                checkError(m_inputPoints);
            }

            if (currentNodeName.compare("Cells") == 0)
            {
                /* Cells */
                dataArrayNode = node->FirstChildElement("DataArray");
                for (; dataArrayNode;
                     dataArrayNode = dataArrayNode->NextSiblingElement("DataArray"))
                {
                    dataArrayElement = dataArrayNode->ToElement();
                    checkError(dataArrayElement);
                    string currentDataArrayName = string(dataArrayElement->Attribute("Name"));
                    /// DA - connectivity
                    if (currentDataArrayName.compare("connectivity") == 0)
                    {
                        // number of elements in values is not known ; have to guess it
                        m_inputCells = loadDataArray(dataArrayElement, 0, "Int32");
                        checkError(m_inputCells);
                    }
                    /// DA - offsets
                    if (currentDataArrayName.compare("offsets") == 0)
                    {
                        m_inputCellOffsets =
                            loadDataArray(dataArrayElement, m_numberOfCells, "Int32");  // - 1);
                        // const int* offsets = (m_inputCellOffsets == nullptr) ? nullptr : (const
                        // int*) m_inputCellOffsets->getData(); std::cout << "offsets: " << std::endl
                        //     << offsets[0] << std::endl
                        //     << offsets[1] << std::endl
                        //     << offsets[2]
                        //     << std::endl;
                        checkError(m_inputCellOffsets);
                    }
                    /// DA - types
                    if (currentDataArrayName.compare("types") == 0)
                    {
                        m_inputCellTypes = loadDataArray(dataArrayElement, m_numberOfCells, "Int32");
                        checkError(m_inputCellTypes);
                    }
                }
            }

            if (currentNodeName.compare("PointData") == 0)
            {
                dataArrayNode = node->FirstChildElement("DataArray");
                for (; dataArrayNode;
                     dataArrayNode = dataArrayNode->NextSiblingElement("DataArray"))
                {
                    dataArrayElement = dataArrayNode->ToElement();
                    checkError(dataArrayElement);

                    const string currentDataArrayName = string(dataArrayElement->Attribute("Name"));

                    BaseVTKDataIO* pointdata = loadDataArray(dataArrayElement, m_numberOfPoints);
                    checkError(pointdata);
                    pointdata->name = currentDataArrayName;
                    m_inputPointDataVector.push_back(pointdata);
                }
            }
            if (currentNodeName.compare("CellData") == 0)
            {
                dataArrayNode = node->FirstChildElement("DataArray");
                for (; dataArrayNode;
                     dataArrayNode = dataArrayNode->NextSiblingElement("DataArray"))
                {
                    dataArrayElement = dataArrayNode->ToElement();
                    checkError(dataArrayElement);
                    const string currentDataArrayName = string(dataArrayElement->Attribute("Name"));
                    BaseVTKDataIO* celldata = loadDataArray(dataArrayElement, m_numberOfCells);
                    checkError(celldata);
                    celldata->name = currentDataArrayName;
                    m_inputCellDataVector.push_back(celldata);
                }
            }
        }
    }

    return true;
}

bool XMLVTKReader::loadPolydata(tinyxml2::XMLHandle datasetFormatHandle)
{
    SOFA_UNUSED(datasetFormatHandle);
    msg_error() << "Polydata dataset not implemented yet";
    return false;
}

bool XMLVTKReader::loadRectilinearGrid(tinyxml2::XMLHandle datasetFormatHandle)
{
    SOFA_UNUSED(datasetFormatHandle);
    msg_error() << "RectilinearGrid dataset not implemented yet";
    return false;
}

bool XMLVTKReader::loadStructuredGrid(tinyxml2::XMLHandle datasetFormatHandle)
{
    SOFA_UNUSED(datasetFormatHandle);
    msg_error() << "StructuredGrid dataset not implemented yet";
    return false;
}

bool XMLVTKReader::loadStructuredPoints(tinyxml2::XMLHandle datasetFormatHandle)
{
    SOFA_UNUSED(datasetFormatHandle);
    msg_error() << "StructuredPoints dataset not implemented yet";
    return false;
}

bool XMLVTKReader::loadImageData(tinyxml2::XMLHandle datasetFormatHandle)
{
    SOFA_UNUSED(datasetFormatHandle);
    msg_error() << "ImageData dataset not implemented yet";
    return false;
}

void registerMeshVTKLoader(sofa::core::ObjectFactory* factory)
{
    factory->registerObjects(
        core::ObjectRegistrationData("Mesh loader for the VTK/VTU file format.")
            .add<MeshVTKLoader>());
}

}  // namespace sofa::component::io::mesh
