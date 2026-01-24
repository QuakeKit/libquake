#pragma once

struct vec4f_t
{
    float x;
    float y;
    float z;
    float w;
};

struct vec3f_t
{
    float x;
    float y;
    float z;
    float dot(const vec3f_t &b) { return x * b.x + y * b.y + z * b.z; };
};

struct vec3s_t
{
    short x;
    short y;
    short z;
};

struct vec2f_t
{
    float x;
    float y;
};

struct vec2i_t
{
    int x;
    int y;
};

struct vec3i_t
{
    short x;
    short y;
    short z;
};

struct bbox3s_t // Bounding Box, Short values
{
    vec3s_t min; // minimum values of X,Y,Z
    vec3s_t max; // maximum values of X,Y,Z
};

struct bbox3f_t // Bounding Box, Short values
{
    vec3f_t min; // minimum values of X,Y,Z
    vec3f_t max; // maximum values of X,Y,Z
};
