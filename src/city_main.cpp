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
    if(cfg.emitPropsInGltf && cfg.emitTreesInGltf){
        WriteGLTF("commercial_blocks.gltf", city.mesh, city.trees, city.props);
    }else if(cfg.emitTreesInGltf){
        WriteGLTF("commercial_blocks.gltf", city.mesh, city.trees);
    }else if(cfg.emitPropsInGltf){
        std::vector<TreeInstance> noTrees;
        WriteGLTF("commercial_blocks.gltf", city.mesh, noTrees, city.props);
    }else{
        WriteGLTF("commercial_blocks.gltf", city.mesh);
    }
    WriteOBJ("commercial_blocks.obj", city.mesh);
    WriteTreeInstancesJson("commercial_blocks_trees.json", city.trees);
    WritePropInstancesJson("commercial_blocks_props.json", city.props);
    std::cout << "Wrote commercial_blocks.gltf, commercial_blocks.obj, commercial_blocks_trees.json, "
              << "and commercial_blocks_props.json\n";
    return 0;
}
