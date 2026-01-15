#include <iostream>

#include "config.h"
#include "io.h"
#include "semantics.h"
#include "style_bungalow.h"
#include "style_brutalist.h"
#include "style_midcentury.h"

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
    }else if(plan.style == "bungalow"){
        building = BuildBungalowBuilding(plan);
    }else{
        building = BuildMidcenturyBuilding(plan);
    }

    std::string outPath = cfg.outputName + ".gltf";
    WriteGLTF(outPath.c_str(), building);
    std::cout << "Wrote " << outPath << "\n";
    return 0;
}
