#include "io.h"

#include <fstream>
#include <limits>
#include <algorithm>
#include <cstdint>
#include <string>

void WriteOBJ(const char* path,const Mesh& m)
{
    std::ofstream out(path);

    for(auto& v:m.v)
        out<<"v "<<v.pos.x<<" "<<v.pos.y<<" "<<v.pos.z<<"\n";

    for(auto& v:m.v)
        out<<"vt "<<v.uv.x<<" "<<v.uv.y<<"\n";

    for(auto& v:m.v)
        out<<"vc "<<v.color.r<<" "<<v.color.g<<" "<<v.color.b<<" "<<v.color.a<<"\n";

    for(size_t k=0;k<m.i.size();k+=3){
        unsigned a=m.i[k]+1,b=m.i[k+1]+1,c=m.i[k+2]+1;
        out<<"f "<<a<<"/"<<a<<" "<<b<<"/"<<b<<" "<<c<<"/"<<c<<"\n";
    }
}

static void AppendAligned(std::vector<std::uint8_t>& buf)
{
    while(buf.size() % 4 != 0){
        buf.push_back(0);
    }
}

template <typename T>
static size_t AppendData(std::vector<std::uint8_t>& buf, const T* data, size_t count)
{
    AppendAligned(buf);
    size_t offset = buf.size();
    const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(data);
    buf.insert(buf.end(), bytes, bytes + sizeof(T) * count);
    return offset;
}

static std::string ReplaceExtension(const std::string& path, const char* ext)
{
    size_t dot = path.find_last_of('.');
    if(dot == std::string::npos){
        return path + ext;
    }
    return path.substr(0, dot) + ext;
}

static std::string EscapeJsonString(const std::string& text)
{
    std::string out;
    out.reserve(text.size());
    for(char c : text){
        switch(c){
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
            break;
        }
    }
    return out;
}

void WriteGLTF(const char* path, const Mesh& m)
{
    std::vector<TreeInstance> empty;
    std::vector<PropInstance> emptyProps;
    WriteGLTF(path, m, empty, emptyProps);
}

void WriteGLTF(const char* path, const Mesh& m, const std::vector<TreeInstance>& trees)
{
    std::vector<PropInstance> emptyProps;
    WriteGLTF(path, m, trees, emptyProps);
}

void WriteGLTF(const char* path, const Mesh& m, const std::vector<TreeInstance>& trees,
               const std::vector<PropInstance>& props)
{
    std::string gltfPath(path);
    std::string binPath = ReplaceExtension(gltfPath, ".bin");

    size_t vCount = m.v.size();
    size_t iCount = m.i.size();

    std::vector<float> positions;
    std::vector<float> colors;
    std::vector<float> uvs;
    positions.reserve(vCount * 3);
    colors.reserve(vCount * 4);
    uvs.reserve(vCount * 2);

    Vec3 posMin{std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()};
    Vec3 posMax{-std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max()};

    for(const auto& v : m.v){
        positions.push_back(v.pos.x);
        positions.push_back(v.pos.y);
        positions.push_back(v.pos.z);

        colors.push_back(v.color.r);
        colors.push_back(v.color.g);
        colors.push_back(v.color.b);
        colors.push_back(v.color.a);

        uvs.push_back(v.uv.x);
        uvs.push_back(v.uv.y);

        posMin.x = std::min(posMin.x, v.pos.x);
        posMin.y = std::min(posMin.y, v.pos.y);
        posMin.z = std::min(posMin.z, v.pos.z);
        posMax.x = std::max(posMax.x, v.pos.x);
        posMax.y = std::max(posMax.y, v.pos.y);
        posMax.z = std::max(posMax.z, v.pos.z);
    }

    std::vector<std::uint8_t> buffer;
    size_t posOffset = AppendData(buffer, positions.data(), positions.size());
    size_t posLength = positions.size() * sizeof(float);

    size_t colorOffset = AppendData(buffer, colors.data(), colors.size());
    size_t colorLength = colors.size() * sizeof(float);

    size_t uvOffset = AppendData(buffer, uvs.data(), uvs.size());
    size_t uvLength = uvs.size() * sizeof(float);

    size_t idxOffset = AppendData(buffer, m.i.data(), m.i.size());
    size_t idxLength = m.i.size() * sizeof(unsigned);

    std::ofstream bout(binPath, std::ios::binary);
    bout.write(reinterpret_cast<const char*>(buffer.data()), (std::streamsize)buffer.size());

    std::ofstream jout(gltfPath);
    jout <<
        "{\n"
        "  \"asset\": {\"version\": \"2.0\"},\n"
        "  \"buffers\": [\n"
        "    {\"uri\": \"" << binPath.substr(binPath.find_last_of("/\\") + 1)
        << "\", \"byteLength\": " << buffer.size() << "}\n"
        "  ],\n"
        "  \"bufferViews\": [\n"
        "    {\"buffer\": 0, \"byteOffset\": " << posOffset << ", \"byteLength\": " << posLength << ", \"target\": 34962},\n"
        "    {\"buffer\": 0, \"byteOffset\": " << colorOffset << ", \"byteLength\": " << colorLength << ", \"target\": 34962},\n"
        "    {\"buffer\": 0, \"byteOffset\": " << uvOffset << ", \"byteLength\": " << uvLength << ", \"target\": 34962},\n"
        "    {\"buffer\": 0, \"byteOffset\": " << idxOffset << ", \"byteLength\": " << idxLength << ", \"target\": 34963}\n"
        "  ],\n"
        "  \"accessors\": [\n"
        "    {\"bufferView\": 0, \"componentType\": 5126, \"count\": " << vCount
        << ", \"type\": \"VEC3\", \"min\": [" << posMin.x << ", " << posMin.y << ", " << posMin.z
        << "], \"max\": [" << posMax.x << ", " << posMax.y << ", " << posMax.z << "]},\n"
        "    {\"bufferView\": 1, \"componentType\": 5126, \"count\": " << vCount
        << ", \"type\": \"VEC4\"},\n"
        "    {\"bufferView\": 2, \"componentType\": 5126, \"count\": " << vCount
        << ", \"type\": \"VEC2\"},\n"
        "    {\"bufferView\": 3, \"componentType\": 5125, \"count\": " << iCount
        << ", \"type\": \"SCALAR\", \"min\": [0], \"max\": [" << (iCount ? (unsigned)(*std::max_element(m.i.begin(), m.i.end())) : 0u) << "]}\n"
        "  ],\n"
        "  \"meshes\": [\n"
        "    {\"primitives\": [\n"
        "      {\"attributes\": {\"POSITION\": 0, \"COLOR_0\": 1, \"TEXCOORD_0\": 2}, \"indices\": 3}\n"
        "    ]}\n"
        "  ],\n"
        "  \"nodes\": [\n"
        "    {\"mesh\": 0}";
    for(const auto& tree : trees){
        std::string variant = EscapeJsonString(tree.variant);
        jout << ",\n"
             << "    {\"translation\": [" << tree.position.x << ", " << tree.position.y << ", "
             << tree.position.z << "], \"extras\": {\"spawnType\": \"tree\", \"variant\": \""
             << variant << "\"}}";
    }
    for(const auto& prop : props){
        std::string type = EscapeJsonString(prop.type);
        jout << ",\n"
             << "    {\"translation\": [" << prop.position.x << ", " << prop.position.y << ", "
             << prop.position.z << "], \"extras\": {\"spawnType\": \"prop\", \"propType\": \""
             << type << "\"}}";
    }
    jout <<
        "\n"
        "  ],\n"
        "  \"scenes\": [\n"
        "    {\"nodes\": [";
    size_t nodeCount = trees.size() + props.size() + 1;
    for(size_t i=0;i<nodeCount;i++){
        if(i) jout << ", ";
        jout << i;
    }
    jout <<
        "]}\n"
        "  ],\n"
        "  \"scene\": 0\n"
        "}\n";
}

void WriteTreeInstancesJson(const char* path, const std::vector<TreeInstance>& trees)
{
    std::ofstream out(path);
    out << "{\n"
        << "  \"space\": \"world\",\n"
        << "  \"trees\": [\n";
    for(size_t i=0;i<trees.size();i++){
        const auto& tree = trees[i];
        std::string variant = EscapeJsonString(tree.variant);
        out << "    {\"variant\": \"" << variant << "\", \"position\": ["
            << tree.position.x << ", " << tree.position.y << ", " << tree.position.z << "]}";
        if(i + 1 < trees.size()){
            out << ",";
        }
        out << "\n";
    }
    out << "  ]\n"
        << "}\n";
}

void WritePropInstancesJson(const char* path, const std::vector<PropInstance>& props)
{
    std::ofstream out(path);
    out << "{\n"
        << "  \"space\": \"world\",\n"
        << "  \"props\": [\n";
    for(size_t i=0;i<props.size();i++){
        const auto& prop = props[i];
        std::string type = EscapeJsonString(prop.type);
        out << "    {\"type\": \"" << type << "\", \"position\": ["
            << prop.position.x << ", " << prop.position.y << ", " << prop.position.z << "]}";
        if(i + 1 < props.size()){
            out << ",";
        }
        out << "\n";
    }
    out << "  ]\n"
        << "}\n";
}
