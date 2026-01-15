#include "config.h"

#include <fstream>
#include <sstream>

#include "json.h"

Config DefaultConfig()
{
    Config c{};
    c.style = "midcentury";
    c.outputName = "simcity_midcentury_office";
    c.lot = {{
        {-18,-12},{18,-12},{22,10},{-14,14}
    }};
    c.lotShrink = 5.0f;
    c.lotSnap = 0.5f;
    c.showLot = true;

    c.floors = 4;
    c.floorH = 3.6f;
    c.slabT = 0.75f;
    c.glassInset = 1.2f;

    c.enablePilotis = true;
    c.pilotisHeight = 3.0f;

    c.finEvery = 2;
    c.finThickness = 0.20f;
    c.finProjection = 0.60f;

    c.usePodiumTower = false;
    c.podiumFloors = 2;
    c.towerInset = 2.5f;

    c.useLShape = true;
    c.lCutX = 6.0f;
    c.lCutY = 4.0f;

    c.roofCapT = 0.60f;
    c.roofCapOverhang = 0.40f;
    c.roofDeckT = 0.15f;
    c.roofDeckInset = 1.0f;

    c.curtainInset = 0.80f;
    c.curtainEvery = 3;
    c.curtainBandFloors = 3;

    c.concrete = {0.55f,0.56f,0.57f};
    c.window   = {0.10f,0.65f,0.95f};
    c.roofDeck = {0.62f,0.62f,0.64f};
    c.lotFill  = {0.25f,0.50f,0.30f};

    return c;
}

static bool ReadFile(const char* path, std::string* out, std::string* err)
{
    std::ifstream in(path);
    if(!in){
        if(err) *err = "failed to open config file";
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

static bool GetLot(const JsonValue& obj, const char* key, Polygon2D* out)
{
    const JsonValue* v = FindKey(obj, key);
    if(!v || v->type != JsonValue::Type::Array || v->arr.size() < 3) return false;
    Polygon2D lot;
    for(const auto& pt : v->arr){
        if(pt.type != JsonValue::Type::Array || pt.arr.size() < 2) return false;
        if(pt.arr[0].type != JsonValue::Type::Number || pt.arr[1].type != JsonValue::Type::Number) return false;
        lot.v.push_back({(float)pt.arr[0].num, (float)pt.arr[1].num});
    }
    *out = lot;
    return true;
}

static void ApplyConfigValues(const JsonValue& obj, Config* out)
{
    GetLot(obj, "lot", &out->lot);
    GetString(obj, "style", &out->style);
    GetString(obj, "outputName", &out->outputName);
    GetNumber(obj, "lotShrink", &out->lotShrink);
    GetNumber(obj, "lotSnap", &out->lotSnap);
    GetBool(obj, "showLot", &out->showLot);

    GetInt(obj, "floors", &out->floors);
    GetNumber(obj, "floorH", &out->floorH);
    GetNumber(obj, "slabT", &out->slabT);
    GetNumber(obj, "glassInset", &out->glassInset);

    GetBool(obj, "enablePilotis", &out->enablePilotis);
    GetNumber(obj, "pilotisHeight", &out->pilotisHeight);

    GetInt(obj, "finEvery", &out->finEvery);
    GetNumber(obj, "finThickness", &out->finThickness);
    GetNumber(obj, "finProjection", &out->finProjection);

    GetBool(obj, "usePodiumTower", &out->usePodiumTower);
    GetInt(obj, "podiumFloors", &out->podiumFloors);
    GetNumber(obj, "towerInset", &out->towerInset);

    GetBool(obj, "useLShape", &out->useLShape);
    GetNumber(obj, "lCutX", &out->lCutX);
    GetNumber(obj, "lCutY", &out->lCutY);

    GetNumber(obj, "roofCapT", &out->roofCapT);
    GetNumber(obj, "roofCapOverhang", &out->roofCapOverhang);
    GetNumber(obj, "roofDeckT", &out->roofDeckT);
    GetNumber(obj, "roofDeckInset", &out->roofDeckInset);

    GetNumber(obj, "curtainInset", &out->curtainInset);
    GetInt(obj, "curtainEvery", &out->curtainEvery);
    GetInt(obj, "curtainBandFloors", &out->curtainBandFloors);

    GetVec3(obj, "concrete", &out->concrete);
    GetVec3(obj, "window", &out->window);
    GetVec3(obj, "roofDeck", &out->roofDeck);
    GetVec3(obj, "lotFill", &out->lotFill);
}

bool LoadConfig(const char* path, Config* out, std::string* err)
{
    if(!out) return false;
    *out = DefaultConfig();
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

    const JsonValue* presetName = FindKey(root, "preset");
    const JsonValue* presets = FindKey(root, "presets");
    if(presetName && presetName->type == JsonValue::Type::String &&
       presets && presets->type == JsonValue::Type::Object){
        auto it = presets->obj.find(presetName->str);
        if(it != presets->obj.end() && it->second.type == JsonValue::Type::Object){
            ApplyConfigValues(it->second, out);
        }
    }

    ApplyConfigValues(root, out);

    return true;
}
