#pragma once

#include <vector>

#include "instances.h"
#include "mesh.h"

void WriteOBJ(const char* path, const Mesh& m);
void WriteGLTF(const char* path, const Mesh& m);
void WriteGLTF(const char* path, const Mesh& m, const std::vector<TreeInstance>& trees);
void WriteGLTF(const char* path, const Mesh& m, const std::vector<TreeInstance>& trees,
               const std::vector<PropInstance>& props);
void WriteTreeInstancesJson(const char* path, const std::vector<TreeInstance>& trees);
void WritePropInstancesJson(const char* path, const std::vector<PropInstance>& props);
