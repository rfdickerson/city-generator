#pragma once

#include <cmath>

struct Vec2 { float x, y; };
struct Vec3 { float x, y, z; };
struct Vec4 { float r, g, b, a; };

inline Vec2 operator+(Vec2 a, Vec2 b){ return {a.x+b.x, a.y+b.y}; }
inline Vec2 operator-(Vec2 a, Vec2 b){ return {a.x-b.x, a.y-b.y}; }
inline Vec2 operator*(Vec2 a, float s){ return {a.x*s, a.y*s}; }

inline float Dot(Vec2 a, Vec2 b){ return a.x*b.x + a.y*b.y; }
inline float Length(Vec2 v){ return std::sqrt(v.x*v.x + v.y*v.y); }
inline Vec2 Normalize(Vec2 v){ float len = Length(v); return {v.x/len, v.y/len}; }
inline Vec2 Perp(Vec2 v){ return {-v.y, v.x}; }
inline float Cross(Vec2 a, Vec2 b){ return a.x*b.y - a.y*b.x; }
