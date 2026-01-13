#include <iostream>

#include "config.h"
#include "io.h"
#include "semantics.h"
#include "style_midcentury.h"
#include "style_brutalist.h"

int main(int argc, char** argv)
{
    Config cfg;
    std::string err;
    const char* configPath = (argc > 1) ? argv[1] : nullptr;
    if(!LoadConfig(configPath, &cfg, &err)){
        std::cerr << "Failed to load config: " << err << "\n";
        return 1;
    }

    sbl::BuildingPlan plan = sbl::BuildPlanFromConfig(cfg);
    Mesh building;
    if(plan.style == "brutalist"){
        building = BuildBrutalistBuilding(plan);
    }else{
        building = BuildMidcenturyBuilding(plan);
    }

    WriteOBJ("simcity_midcentury_office.obj",building);
    WriteGLTF("simcity_midcentury_office.gltf",building);
    std::cout << "Wrote simcity_midcentury_office.obj and simcity_midcentury_office.gltf\n";
    return 0;
}
