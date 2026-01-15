#include <iostream>

#include "city_builder.h"
#include "city_config.h"
#include "io.h"

int main(int argc, char** argv)
{
    CityConfig cfg;
    std::string err;
    const char* configPath = (argc > 1) ? argv[1] : nullptr;
    if(!LoadCityConfig(configPath, &cfg, &err)){
        std::cerr << "Failed to load city config: " << err << "\n";
        return 1;
    }

    CityBuild city = BuildCity(cfg);
    std::string baseName = cfg.outputName;
    std::string gltfPath = baseName + ".gltf";
    std::string treesPath = baseName + "_trees.json";
    std::string propsPath = baseName + "_props.json";

    if(cfg.emitPropsInGltf && cfg.emitTreesInGltf){
        WriteGLTF(gltfPath.c_str(), city.mesh, city.trees, city.props);
    }else if(cfg.emitTreesInGltf){
        WriteGLTF(gltfPath.c_str(), city.mesh, city.trees);
    }else if(cfg.emitPropsInGltf){
        std::vector<TreeInstance> noTrees;
        WriteGLTF(gltfPath.c_str(), city.mesh, noTrees, city.props);
    }else{
        WriteGLTF(gltfPath.c_str(), city.mesh);
    }
    WriteTreeInstancesJson(treesPath.c_str(), city.trees);
    WritePropInstancesJson(propsPath.c_str(), city.props);
    std::cout << "Wrote " << gltfPath << ", " << treesPath << ", and " << propsPath << "\n";
    return 0;
}
