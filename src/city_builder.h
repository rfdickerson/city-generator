#pragma once

#include "city_config.h"
#include "instances.h"
#include "mesh.h"

struct CityBuild {
    Mesh mesh;
    std::vector<TreeInstance> trees;
    std::vector<PropInstance> props;
};

Mesh BuildCityMesh(const CityConfig& cfg);
CityBuild BuildCity(const CityConfig& cfg);
