/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob
	
	v2 - Optimized by Hugo Mateo and Juan Lorente, MSoA 2024

    Nori is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Nori is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <nori/mesh.h>
#include <nori/timer.h>
#include <filesystem/resolver.h>
#include <unordered_map>
#include <fstream>
#include <functional>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <random>
#include <execution>

NORI_NAMESPACE_BEGIN

/**
 * \brief Loader for Wavefront OBJ triangle meshes
 */
class WavefrontOBJ : public Mesh 
{
    protected:

     /// Vertex indices used by the OBJ format
    struct OBJVertex {
        uint32_t p = (uint32_t) -1;
        uint32_t n = (uint32_t) -1;
        uint32_t uv = (uint32_t) -1;

        inline OBJVertex() { }

        inline OBJVertex(const std::string &string) {
            std::vector<std::string> tokens = tokenize(string, "/", true);

            if (tokens.size() < 1 || tokens.size() > 3)
                throw NoriException("Invalid vertex data: \"%s\"", string);

            p = toUInt(tokens[0]);

            if (tokens.size() >= 2 && !tokens[1].empty())
                uv = toUInt(tokens[1]);

            if (tokens.size() >= 3 && !tokens[2].empty())
                n = toUInt(tokens[2]);
        }

        inline bool operator==(const OBJVertex &v) const {
            return v.p == p && v.n == n && v.uv == uv;
        }
    };

    struct Face {
        OBJVertex v[6];
        int nVertices;
    };


    uint64_t float_to_morton(float value, int max_bits = 10) 
    {
        uint64_t morton_code = 0;
        uint32_t value_scaled = static_cast<uint32_t>(value * ((1 << max_bits) - 1)); // Scale to the number of bits
        
        for (int i = 0; i < max_bits; ++i) {
            morton_code |= ((value_scaled >> i) & 1) << (3 * i); // 3D interleaving for Morton code
        }
        
        return morton_code;
    }


    // Function to compute the Morton code for a 3D point in [0, 1]
    uint64_t morton3D(Point3f p, int bits = 10) 
    {
        uint64_t x_morton = float_to_morton(p.x(), bits);
        uint64_t y_morton = float_to_morton(p.y(), bits);
        uint64_t z_morton = float_to_morton(p.z(), bits);  

        uint64_t morton_code = 0;
        for (int i = 0; i < bits; ++i) {
            morton_code |= ((x_morton >> (3 * i)) & 1) << (3 * i);
            morton_code |= ((y_morton >> (3 * i)) & 1) << (3 * i + 1);
            morton_code |= ((z_morton >> (3 * i)) & 1) << (3 * i + 2);
        }

        return morton_code;
    }

    void order_faces_morton(std::vector<Face> &faces,
            std::vector<Vector3f> &vertices)
    {
        Vector3f bbox_size = m_bbox.max - m_bbox.min;
        for (int i = 0; i < 3; ++i)
            if (bbox_size[i] == 0) bbox_size[i] = 1.0f;


        auto compute_morton = [&](const Face &f) -> uint64_t
            {
                Vector3f p0 = vertices[f.v[0].p - 1];
                Vector3f p1 = vertices[f.v[1].p - 1];
                Vector3f p2 = vertices[f.v[2].p - 1];

                Vector3f centroid = (p0 + p1 + p2) / 3.0f;
                Vector3f normalized_centroid = (centroid - m_bbox.min);
                
                normalized_centroid = normalized_centroid.cwiseQuotient(bbox_size);
                return morton3D(normalized_centroid);
            };

        auto policy = std::execution::par_unseq;

        // Generate data to sort
        std::vector<uint32_t> indices(faces.size());
        std::vector<uint64_t> morton_codes(faces.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::transform(policy, faces.begin(), faces.end(), morton_codes.begin(), compute_morton);

        // Sort face indices by Morton code
        //  An indexed sort is needed (instead of directly sorting the faces) 
        //  since the index is required to access the precomputed morton codes
        //  Computing morton in-place is significantly slower, 
        //  as each access in the sorting requires recomputing
        std::sort(policy, indices.begin(), indices.end(), [&](uint32_t i, uint32_t j) { return morton_codes[i] < morton_codes[j]; });

        // Sort the faces data
        std::vector<Face> tmp_faces(faces.size());
        std::transform(policy, indices.begin(), indices.end(), tmp_faces.begin(), [&](uint32_t idx) { return faces[idx]; });
        faces = std::move(tmp_faces);
    }
    

    void order_vertices_morton(
        std::vector<Face> &faces,
        std::vector<Vector3f> &vertices)
    {
        Vector3f bbox_size = m_bbox.max - m_bbox.min;
        for (int i = 0; i < 3; ++i)
            if (bbox_size[i] == 0) bbox_size[i] = 1.0f;


        auto compute_morton = [&](const Vector3f &vertex) -> uint64_t
            {
                Vector3f norm = (vertex - m_bbox.min);
                norm = norm.cwiseQuotient(bbox_size);
                return morton3D(norm);
            };

        auto policy = std::execution::par_unseq;

        // Generate data to sort
        std::vector<uint32_t> indices(vertices.size());
        std::vector<uint64_t> morton_codes(vertices.size());      
        std::iota(indices.begin(), indices.end(), 0);  
        std::transform(policy, vertices.begin(), vertices.end(), morton_codes.begin(), compute_morton);

        // Sort vertex indices by Morton code
        std::sort(policy, indices.begin(), indices.end(), [&](uint32_t i, uint32_t j) { return morton_codes[i] < morton_codes[j]; });

        // Sort the vertices data
        std::vector<Vector3f> tmp_vertices(vertices.size());
        std::transform(policy, indices.begin(), indices.end(), tmp_vertices.begin(), [&](uint32_t idx) { return vertices[idx]; });
        vertices = std::move(tmp_vertices);

        // Make a lookup table to update face indices
        std::vector<uint32_t> inverse_indices(vertices.size());
        for (uint32_t i = 0; i < indices.size(); ++i)
            inverse_indices[indices[i]] = i;

        // Update face indices
        std::for_each(policy, faces.begin(), faces.end(), [&](Face &face) 
        {
            for (int i = 0; i < face.nVertices; ++i) {
                OBJVertex &v = face.v[i];
                v.p = inverse_indices[v.p - 1] + 1;
            }
        });
    }


    void order_by_morton(
            std::vector<Face> &loaded_faces,
            std::vector<Vector3f> &positions)
    {
        auto init = std::chrono::high_resolution_clock::now();
        order_vertices_morton(loaded_faces, positions);
        order_faces_morton(loaded_faces, positions); 

        //order_vertices_randomly(loaded_faces, positions); 
        //order_faces_randomly(loaded_faces, positions);  

        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> elapsed = end - init;
        //std::cout << "Elapsed time: " << elapsed.count() << "s\n";
    }

    void order_vertices_randomly(
            std::vector<Face> &loaded_faces,
            std::vector<Vector3f> &vertices)
    {
        auto device = std::random_device();
        std::mt19937 g(987654321);
        std::vector<uint32_t> indices(vertices.size());
        std::iota(indices.begin(), indices.end(), 0);

        std::vector<Vector3f> tmp_vertices(vertices.size());
        std::shuffle(indices.begin(), indices.end(), g);
        std::transform(indices.begin(), indices.end(), tmp_vertices.begin(), [&](uint32_t idx) { return vertices[idx]; });
        vertices.swap(tmp_vertices);

        std::vector<uint32_t> inverse_indices(vertices.size());
        for (uint32_t i = 0; i < indices.size(); ++i)
            inverse_indices[indices[i]] = i;

        for (Face &face : loaded_faces)
        {
            for (int i = 0; i < face.nVertices; ++i)
            {
                OBJVertex &v = face.v[i];
                v.p = inverse_indices[v.p - 1] + 1;
            }
        }
    }

    void order_faces_randomly( 
            std::vector<Face> &loaded_faces,
            const std::vector<Vector3f> &positions)
    {
        auto device = std::random_device();
        std::mt19937 g(987654321);
        std::shuffle(loaded_faces.begin(), loaded_faces.end(), g);
    }

public:

    WavefrontOBJ(const PropertyList &propList) {
        typedef std::unordered_map<OBJVertex, uint32_t, OBJVertexHash> VertexMap;

        filesystem::path filename =
            getFileResolver()->resolve(propList.getString("filename"));

        std::ifstream is(filename.str());
        if (is.fail())
            throw NoriException("Unable to open OBJ file \"%s\"!", filename);
        Transform trafo = propList.getTransform("toWorld", Transform());

        //cout << "Loading \"" << filename << "\" .. ";
        cout.flush();
        Timer timer;

        std::vector<Vector3f>   positions;
        std::vector<Vector2f>   texcoords;
        std::vector<Vector3f>   normals;
        std::vector<uint32_t>   indices;
        std::vector<OBJVertex>  vertices;
        VertexMap vertexMap;

        std::vector<Face> loaded_faces;

        std::string line_str;
        while (std::getline(is, line_str)) {
            std::istringstream line(line_str);

            std::string prefix;
            line >> prefix;

            if (prefix == "v") {
                Point3f p;
                line >> p.x() >> p.y() >> p.z();
                p = trafo * p;
                m_bbox.expandBy(p);
                positions.push_back(p);
            } else if (prefix == "vt") {
                Point2f tc;
                line >> tc.x() >> tc.y();
                texcoords.push_back(tc);
            } else if (prefix == "vn") {
                Normal3f n;
                line >> n.x() >> n.y() >> n.z();
                normals.push_back((trafo * n).normalized());
            } else if (prefix == "f") {
                std::string v1, v2, v3, v4;
                line >> v1 >> v2 >> v3 >> v4;
                OBJVertex verts[6];
                int nVertices = 3;

                verts[0] = OBJVertex(v1);
                verts[1] = OBJVertex(v2);
                verts[2] = OBJVertex(v3);

                if (!v4.empty()) {
                    /* This is a quad, split into two triangles */
                    verts[3] = OBJVertex(v4);
                    verts[4] = verts[0];
                    verts[5] = verts[2];
                    nVertices = 6;
                }

                Face face;
                face.nVertices = nVertices;
                for (int i=0; i<nVertices; ++i)
                    face.v[i] = verts[i];

                loaded_faces.push_back(face);
            }
        }
        //cout << "End reading \"" << filename << "\" .. ";

        order_by_morton(loaded_faces, positions);

        for (const Face &face : loaded_faces) 
        {
            /* Convert to an indexed vertex list */
            for (int i=0; i < face.nVertices; ++i) 
            {
                const OBJVertex &v = face.v[i];
                VertexMap::const_iterator it = vertexMap.find(v);
                if (it == vertexMap.end()) {
                    vertexMap[v] = (uint32_t) vertices.size();
                    indices.push_back((uint32_t) vertices.size());
                    vertices.push_back(v);
                } else {
                    indices.push_back(it->second);
                }
            }
        }


        m_F.resize(3, indices.size()/3);
        memcpy(m_F.data(), indices.data(), sizeof(uint32_t)*indices.size());

        m_V.resize(3, vertices.size());
        for (uint32_t i = 0; i < vertices.size(); ++i)
        {
            m_V.col(i) = positions.at(vertices[i].p - 1);
        }

        //cout << "Passed with vertices \"" << filename << "\" .. ";

        if (!normals.empty()) {
            m_N.resize(3, vertices.size());
            for (uint32_t i=0; i<vertices.size(); ++i)
                m_N.col(i) = normals.at(vertices[i].n-1);
        }

        //cout << "Passed with normals \"" << filename << "\" .. ";

        if (!texcoords.empty()) {
            m_UV.resize(2, vertices.size());
            for (uint32_t i=0; i<vertices.size(); ++i)
                m_UV.col(i) = texcoords.at(vertices[i].uv-1);
        }

        //cout << "Passed with UVs \"" << filename << "\" .. ";

        m_name = filename.str();
        //cout << "done. (V=" << m_V.cols() << ", F=" << m_F.cols() << ", took "
        //     << timer.elapsedString() << " and "
        //     << memString(m_F.size() * sizeof(uint32_t) +
        //                  sizeof(float) * (m_V.size() + m_N.size() + m_UV.size()))
        //     << ")" << endl;
    }

protected:

    /// Hash function for OBJVertex
    struct OBJVertexHash {
        std::size_t operator()(const OBJVertex &v) const {
            std::size_t hash = std::hash<uint32_t>()(v.p);
            hash = hash * 37 + std::hash<uint32_t>()(v.uv);
            hash = hash * 37 + std::hash<uint32_t>()(v.n);
            return hash;
        }
    };
};

NORI_REGISTER_CLASS(WavefrontOBJ, "obj");
NORI_NAMESPACE_END
