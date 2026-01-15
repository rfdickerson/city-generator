#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "io.h"

namespace {

Mesh MakeTriangleMesh()
{
    Mesh mesh;
    mesh.v.push_back({{0.0f, 0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}, {0.0f, 0.0f}});
    mesh.v.push_back({{1.0f, 0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}, {1.0f, 0.0f}});
    mesh.v.push_back({{0.0f, 0.0f, 1.0f}, {0.7f, 0.7f, 0.7f, 1.0f}, {0.0f, 1.0f}});
    mesh.i = {0, 1, 2};
    return mesh;
}

std::string ReadFile(const std::filesystem::path& path)
{
    std::ifstream in(path);
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return contents;
}

std::filesystem::path TempPath(const char* name)
{
    auto stamp = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path() / (std::string(name) + "_" + stamp);
}

} // namespace

TEST(IO, WriteOBJEmitsVerticesAndFaces)
{
    Mesh mesh = MakeTriangleMesh();
    std::filesystem::path path = TempPath("io_test.obj");

    WriteOBJ(path.string().c_str(), mesh);

    EXPECT_TRUE(std::filesystem::exists(path));
    std::string contents = ReadFile(path);
    EXPECT_NE(contents.find("v "), std::string::npos);
    EXPECT_NE(contents.find("vt "), std::string::npos);
    EXPECT_NE(contents.find("f "), std::string::npos);
}

TEST(IO, WriteGLTFEmitsBinAndExtras)
{
    Mesh mesh = MakeTriangleMesh();
    std::filesystem::path path = TempPath("io_test.gltf");

    std::vector<TreeInstance> trees = {{{1.0f, 2.0f, 3.0f}, "maple"}};
    std::vector<PropInstance> props = {{{-1.0f, 0.0f, 2.0f}, "bench"}};
    WriteGLTF(path.string().c_str(), mesh, trees, props);

    std::filesystem::path binPath = path;
    binPath.replace_extension(".bin");
    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_TRUE(std::filesystem::exists(binPath));

    std::string contents = ReadFile(path);
    EXPECT_NE(contents.find("\"buffers\""), std::string::npos);
    EXPECT_NE(contents.find("\"spawnType\": \"tree\""), std::string::npos);
    EXPECT_NE(contents.find("\"spawnType\": \"prop\""), std::string::npos);
}

TEST(IO, WriteInstanceJsonIncludesPayload)
{
    std::filesystem::path treePath = TempPath("io_trees.json");
    std::filesystem::path propPath = TempPath("io_props.json");

    std::vector<TreeInstance> trees = {{{0.0f, 0.5f, 1.0f}, "oak"}};
    std::vector<PropInstance> props = {{{2.0f, 0.0f, -1.0f}, "car"}};

    WriteTreeInstancesJson(treePath.string().c_str(), trees);
    WritePropInstancesJson(propPath.string().c_str(), props);

    std::string treeContents = ReadFile(treePath);
    std::string propContents = ReadFile(propPath);
    EXPECT_NE(treeContents.find("\"trees\""), std::string::npos);
    EXPECT_NE(treeContents.find("\"oak\""), std::string::npos);
    EXPECT_NE(propContents.find("\"props\""), std::string::npos);
    EXPECT_NE(propContents.find("\"car\""), std::string::npos);
}
