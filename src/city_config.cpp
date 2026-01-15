#include "city_config.h"

#include <fstream>
#include <sstream>

#include "json.h"

CityConfig DefaultCityConfig()
{
    CityConfig c{};
    c.seed = 1337;
    c.blocksX = 2;
    c.blocksY = 2;
    c.blockSizeX = 300.0f;
    c.blockSizeY = 300.0f;
    c.roadWidth = 14.0f;
    c.roadThickness = 0.2f;

    c.lotDepth = 100.0f;
    c.lotDepthMax = 140.0f;
    c.lotWidth = 40.0f;
    c.lotWidthMax = 60.0f;
    c.resLotDepth = 50.0f;
    c.resLotDepthMax = 70.0f;
    c.resLotWidth = 22.0f;
    c.resLotWidthMax = 34.0f;
    c.lotGap = 4.0f;
    c.sidewalk = 4.0f;
    c.lotSetback = 2.0f;
    c.parkChance = 0.08f;
    c.parkingChance = 0.10f;
    c.lotRotationMaxDeg = 8.0f;
    c.resLotChance = 0.55f;

    c.minFloors = 3;
    c.maxFloors = 8;
    c.lowMaxFloors = 4;
    c.tallChance = 0.20f;
    c.pilotisChance = 0.35f;
    c.disablePilotis = false;
    c.lShapeChance = 0.30f;

    c.styles = {"midcentury", "brutalist", "bungalow"};
    c.treeVariants = {"maple", "oak", "pine"};
    c.treeSpacing = 6.0f;
    c.treeInset = 2.0f;
    c.treeJitter = 0.75f;
    c.treeChance = 0.75f;
    c.emitTreesInGltf = false;

    c.rooftopAcSpacing = 6.0f;
    c.rooftopAcInset = 2.0f;
    c.rooftopAcJitter = 0.5f;
    c.rooftopAcChance = 0.6f;

    c.streetBinSpacing = 12.0f;
    c.streetBinOffset = 1.2f;
    c.streetBinJitter = 0.4f;
    c.streetBinChance = 0.5f;

    c.streetLightSpacing = 24.0f;
    c.streetLightOffset = 0.8f;
    c.streetLightChance = 0.7f;

    c.parkBenchSpacing = 10.0f;
    c.parkBenchInset = 2.5f;
    c.parkBenchJitter = 0.6f;
    c.parkBenchChance = 0.6f;

    c.parkingCarSpacing = 5.5f;
    c.parkingCarInset = 1.5f;
    c.parkingCarJitter = 0.35f;
    c.parkingCarChance = 0.75f;

    c.emitPropsInGltf = false;

    c.outputName = "commercial_blocks";

    c.roadColor = {0.12f, 0.12f, 0.12f};
    c.lotColor = {0.18f, 0.28f, 0.18f};
    c.parkColor = {0.18f, 0.40f, 0.22f};
    c.parkingColor = {0.20f, 0.20f, 0.20f};
    c.showLots = true;

    return c;
}

static bool ReadFile(const char* path, std::string* out, std::string* err)
{
    std::ifstream in(path);
    if(!in){
        if(err) *err = "failed to open city config file";
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    *out = ss.str();
    return true;
}

static const JsonValue* FindKey(const JsonValue& obj, const char* key)
{
    auto it = obj.obj.find(key);
    if(it == obj.obj.end()){
        return nullptr;
    }
    return &it->second;
}

static bool GetNumber(const JsonValue& obj, const char* key, float* out)
{
    const JsonValue* v = FindKey(obj, key);
    if(!v || v->type != JsonValue::Type::Number) return false;
    *out = (float)v->num;
    return true;
}

static bool GetInt(const JsonValue& obj, const char* key, int* out)
{
    const JsonValue* v = FindKey(obj, key);
    if(!v || v->type != JsonValue::Type::Number) return false;
    *out = (int)v->num;
    return true;
}

static bool GetBool(const JsonValue& obj, const char* key, bool* out)
{
    const JsonValue* v = FindKey(obj, key);
    if(!v || v->type != JsonValue::Type::Bool) return false;
    *out = v->b;
    return true;
}

static bool GetString(const JsonValue& obj, const char* key, std::string* out)
{
    const JsonValue* v = FindKey(obj, key);
    if(!v || v->type != JsonValue::Type::String) return false;
    *out = v->str;
    return true;
}

static bool GetVec3(const JsonValue& obj, const char* key, Vec3* out)
{
    const JsonValue* v = FindKey(obj, key);
    if(!v || v->type != JsonValue::Type::Array || v->arr.size() < 3) return false;
    if(v->arr[0].type != JsonValue::Type::Number ||
       v->arr[1].type != JsonValue::Type::Number ||
       v->arr[2].type != JsonValue::Type::Number){
        return false;
    }
    out->x = (float)v->arr[0].num;
    out->y = (float)v->arr[1].num;
    out->z = (float)v->arr[2].num;
    return true;
}

static bool GetStringArray(const JsonValue& obj, const char* key, std::vector<std::string>* out)
{
    const JsonValue* v = FindKey(obj, key);
    if(!v || v->type != JsonValue::Type::Array) return false;
    std::vector<std::string> vals;
    for(const auto& item : v->arr){
        if(item.type != JsonValue::Type::String) return false;
        vals.push_back(item.str);
    }
    if(!vals.empty()){
        *out = vals;
        return true;
    }
    return false;
}

bool LoadCityConfig(const char* path, CityConfig* out, std::string* err)
{
    if(!out) return false;
    *out = DefaultCityConfig();
    if(!path || path[0] == '\0'){
        return true;
    }

    std::string text;
    if(!ReadFile(path, &text, err)){
        return false;
    }

    JsonValue root;
    if(!ParseJson(text, &root, err)){
        return false;
    }
    if(root.type != JsonValue::Type::Object){
        if(err) *err = "root must be object";
        return false;
    }

    GetInt(root, "seed", &out->seed);
    GetInt(root, "blocksX", &out->blocksX);
    GetInt(root, "blocksY", &out->blocksY);
    GetNumber(root, "blockSizeX", &out->blockSizeX);
    GetNumber(root, "blockSizeY", &out->blockSizeY);
    GetNumber(root, "roadWidth", &out->roadWidth);
    GetNumber(root, "roadThickness", &out->roadThickness);

    GetNumber(root, "lotDepth", &out->lotDepth);
    GetNumber(root, "lotDepthMax", &out->lotDepthMax);
    GetNumber(root, "lotWidth", &out->lotWidth);
    GetNumber(root, "lotWidthMax", &out->lotWidthMax);
    GetNumber(root, "resLotDepth", &out->resLotDepth);
    GetNumber(root, "resLotDepthMax", &out->resLotDepthMax);
    GetNumber(root, "resLotWidth", &out->resLotWidth);
    GetNumber(root, "resLotWidthMax", &out->resLotWidthMax);
    GetNumber(root, "lotGap", &out->lotGap);
    GetNumber(root, "sidewalk", &out->sidewalk);
    GetNumber(root, "lotSetback", &out->lotSetback);
    GetNumber(root, "parkChance", &out->parkChance);
    GetNumber(root, "parkingChance", &out->parkingChance);
    GetNumber(root, "lotRotationMaxDeg", &out->lotRotationMaxDeg);
    GetNumber(root, "resLotChance", &out->resLotChance);

    GetInt(root, "minFloors", &out->minFloors);
    GetInt(root, "maxFloors", &out->maxFloors);
    GetInt(root, "lowMaxFloors", &out->lowMaxFloors);
    GetNumber(root, "tallChance", &out->tallChance);
    GetNumber(root, "pilotisChance", &out->pilotisChance);
    GetBool(root, "disablePilotis", &out->disablePilotis);
    GetNumber(root, "lShapeChance", &out->lShapeChance);

    GetStringArray(root, "styles", &out->styles);
    GetStringArray(root, "treeVariants", &out->treeVariants);

    GetNumber(root, "treeSpacing", &out->treeSpacing);
    GetNumber(root, "treeInset", &out->treeInset);
    GetNumber(root, "treeJitter", &out->treeJitter);
    GetNumber(root, "treeChance", &out->treeChance);
    GetBool(root, "emitTreesInGltf", &out->emitTreesInGltf);

    GetNumber(root, "rooftopAcSpacing", &out->rooftopAcSpacing);
    GetNumber(root, "rooftopAcInset", &out->rooftopAcInset);
    GetNumber(root, "rooftopAcJitter", &out->rooftopAcJitter);
    GetNumber(root, "rooftopAcChance", &out->rooftopAcChance);

    GetNumber(root, "streetBinSpacing", &out->streetBinSpacing);
    GetNumber(root, "streetBinOffset", &out->streetBinOffset);
    GetNumber(root, "streetBinJitter", &out->streetBinJitter);
    GetNumber(root, "streetBinChance", &out->streetBinChance);

    GetNumber(root, "streetLightSpacing", &out->streetLightSpacing);
    GetNumber(root, "streetLightOffset", &out->streetLightOffset);
    GetNumber(root, "streetLightChance", &out->streetLightChance);

    GetNumber(root, "parkBenchSpacing", &out->parkBenchSpacing);
    GetNumber(root, "parkBenchInset", &out->parkBenchInset);
    GetNumber(root, "parkBenchJitter", &out->parkBenchJitter);
    GetNumber(root, "parkBenchChance", &out->parkBenchChance);

    GetNumber(root, "parkingCarSpacing", &out->parkingCarSpacing);
    GetNumber(root, "parkingCarInset", &out->parkingCarInset);
    GetNumber(root, "parkingCarJitter", &out->parkingCarJitter);
    GetNumber(root, "parkingCarChance", &out->parkingCarChance);

    GetBool(root, "emitPropsInGltf", &out->emitPropsInGltf);
    GetString(root, "outputName", &out->outputName);

    GetVec3(root, "roadColor", &out->roadColor);
    GetVec3(root, "lotColor", &out->lotColor);
    GetVec3(root, "parkColor", &out->parkColor);
    GetVec3(root, "parkingColor", &out->parkingColor);
    GetBool(root, "showLots", &out->showLots);

    if(!FindKey(root, "lotDepthMax")){
        out->lotDepthMax = out->lotDepth;
    }
    if(!FindKey(root, "lotWidthMax")){
        out->lotWidthMax = out->lotWidth;
    }
    if(!FindKey(root, "resLotDepthMax")){
        out->resLotDepthMax = out->resLotDepth;
    }
    if(!FindKey(root, "resLotWidthMax")){
        out->resLotWidthMax = out->resLotWidth;
    }
    if(out->lotDepthMax < out->lotDepth){
        std::swap(out->lotDepthMax, out->lotDepth);
    }
    if(out->lotWidthMax < out->lotWidth){
        std::swap(out->lotWidthMax, out->lotWidth);
    }
    if(out->resLotDepthMax < out->resLotDepth){
        std::swap(out->resLotDepthMax, out->resLotDepth);
    }
    if(out->resLotWidthMax < out->resLotWidth){
        std::swap(out->resLotWidthMax, out->resLotWidth);
    }

    return true;
}
