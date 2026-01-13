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

    Mesh city = BuildCityMesh(cfg);
    WriteGLTF("commercial_blocks.gltf", city);
    WriteOBJ("commercial_blocks.obj", city);
    std::cout << "Wrote commercial_blocks.gltf and commercial_blocks.obj\n";
    return 0;
}
