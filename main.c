#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "json.h"

//come negli anni 80....
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define SOLAR_SYSTEM_RADIUS 60 //astronomical units
#define MAX_ASTEROIDS       100
#define ASTEROID_NAME_SIZE  32

struct asteroid
{
    char name[ASTEROID_NAME_SIZE + 1];

    //meters
    double min_diameter;
    double max_diameter;

    int is_hazardous;
};

//rgba
static unsigned char* img = 0;
static size_t img_size = 0;

static struct asteroid asteroids[MAX_ASTEROIDS];
static size_t num_asteroids = 0;

struct json_object_element_s* get_element_by_name(struct json_object_element_s* start, const char* name)
{
    struct json_object_element_s* e = start;

    while(e)
    {
        if(e->name)
            if(e->name->string_size > 0)
                if(!strcmp(e->name->string, name))
                    return e;

        e = e->next;
    }

    return e;
}

int get_string(char* dst, size_t n, struct json_value_s* value)
{
    if(!value || !dst)
        return 0;

    if(value->type != json_type_string)
        return 0;

    if(!value->payload)
        return 0;

    size_t len = MIN(n, strlen(value->payload));

    memcpy(dst, value->payload, len);
    dst[len] = 0; //cazz di terminatori

    return 1;
}

int draw_circle(uint32_t* img, uint32_t w, uint32_t h, uint32_t xc, uint32_t yc, uint32_t r, uint32_t color)
{
    if(!img)
        return 0;

    float ca = 2.0f * M_PI / 3600.0f;
    float cai = ca;

    //coordinate polari
    for(uint16_t a = 0; a < 3600; a++)
    {
        uint32_t x = xc + r * cos(cai);
        uint32_t y = yc + r * sin(cai);

        if(x < w && y < h)
            *(img + y * w + x) = color;

        cai += ca;
    }

    return 1;
}

int draw_plus(uint32_t* img, uint32_t w, uint32_t h, uint32_t x, uint32_t y, uint32_t color)
{
    if(!img)
        return 0;

    //uint32_t* p = img + y * w + x;

    return 1;
}

__attribute__((export_name("alloc_memory")))
unsigned char* alloc_memory(size_t size)
{
    return malloc(size);
}

__attribute__((export_name("free_memory")))
void free_memory(unsigned char* memory)
{
    if(memory)
        free(memory);
}

__attribute__((export_name("draw_asteroids")))
unsigned char* draw_asteroids(unsigned int w, unsigned int h)
{
    unsigned int size = w * h * 4;

    if(size == 0)
        return 0;

    if(img_size < size)
    {
        if(img)
            free(img);

        img = malloc(size);
        img_size = size;
    }

    if(!img)
        return 0;

    unsigned int* p2 = (unsigned int*)img;
    unsigned int background = 0xff3b190f; //#0f193b

    for(int y = 0; y < h; y++)
        for(int x = 0; x < w; x++)
            *p2++ = background;

    //sistema solare!!!
    draw_circle((uint32_t*)img, w, h, w / 2, h / 2, w / 2, 0xffffffff);

    //unsigned char r = 255;
    //unsigned char g = 0;
    //unsigned char b = 255;



    //for(int y = 0; y < h; y++)
    //    for(int x = 0; x < w; x++)
    //        *p2++ = 255 << 24 | 255 << 16 | 0 << 8 | 255;

    return img;
}

__attribute__((export_name("load_asteroids")))
int load_asteroids(const char* json, size_t n)
{
    struct json_value_s* root = json_parse(json, n);

    if(!root)
        return 0;

    if(root->type != json_type_object)
        return 0;

    struct json_object_s* obj = (struct json_object_s*)root->payload;

    if(!obj || !obj->length)
        return 0;

    if(!obj->start->name)
        return 0;

    struct json_object_element_s* neo = get_element_by_name(obj->start, "near_earth_objects");

    if(!neo || !neo->value)
        return 0;

    if(neo->value->type != json_type_object)
        return 0;

    struct json_object_s* neo_obj = (struct json_object_s*)neo->value->payload;

    if(!neo_obj)
        return 0;

    //data di osservazione 
    struct json_object_element_s* date_arr = neo_obj->start;

    while(date_arr)
    {
        if(!date_arr->value)
        {
            date_arr = date_arr->next;
            continue;
        }

        if(date_arr->value->type != json_type_array)
            return -1;

        struct json_array_s* a_array = (struct json_array_s*)date_arr->value->payload;
        struct json_array_element_s* a = a_array->start;

        while(a)
        {
            if(!a->value)
                return -1;

            struct json_object_s* a_obj = a->value->payload;
            struct json_object_element_s* e = 0;

            e = get_element_by_name(a_obj->start, "name");

            if(!e)
            {
                a = a->next;
                continue;
            }
                
            if(!get_string(asteroids[num_asteroids].name, ASTEROID_NAME_SIZE, e->value))
            {
                a = a->next;
                continue;
            }

            e = get_element_by_name(a_obj->start, "near_earth_objects");

            num_asteroids++;

            if(num_asteroids == MAX_ASTEROIDS)
                return 1;

            a = a->next;
        }

        date_arr = date_arr->next;
    }

    return 1;
}

__attribute__((export_name("get_num_asteroids")))
size_t get_num_asteroids()
{
    return num_asteroids;
}