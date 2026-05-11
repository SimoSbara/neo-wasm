#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "json.h"

//come negli anni 80....
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define MAX_ASTEROIDS           100
#define ASTEROID_NAME_SIZE      32

//un pochino di piu di marte
#define OBSERVATION_RADIUS      1.7f

enum planets_e
{
    MERCURY = 0,
    VENUS,
    EARTH,
    MARS,
    NUM_PLANETS
};

enum planets_info_e
{
    THETA_J2000 = 0, //angolo di riferimento nel primo gennaio 2000
    YEAR_PERIOD, //quanti giorni per un anno solare?
    ECCENTRICITY, //visto che le orbite sono ellissi
    PERIHELION, //perielio... fammi stare zitto
    SIZE_SCALING, //per la grafica
    NUM_PLANET_INFOS
};

struct position
{
    uint32_t x;
    uint32_t y;
};

struct asteroid
{
    char name[ASTEROID_NAME_SIZE + 1];

    //kilometers
    float min_diameter;
    float max_diameter;

    //kilometers
    float miss_distance;

    //km/h
    float velocity;

    int is_hazardous;

    int64_t timestamp_since_epoch;

    //posizione img dopo draw
    struct position pos;
};

//rgba
static unsigned char* img = 0;
static size_t img_size = 0;

//per ogni osservazione, c'è un pianeta di riferimento
static struct asteroid asteroids[NUM_PLANETS][MAX_ASTEROIDS] = {0};
static size_t num_asteroids[NUM_PLANETS] = {0};

//per stimare l'orbita, si capit??
//in millisecondi
static int64_t timestamp_since_epoch = 0;
static int64_t timestamp_since_j2000 = 0;
static int64_t days_since_j2000 = 0;

//pianeti
struct position planet_positions[NUM_PLANETS] = {0};

//astronomical units
static float planet_radius[NUM_PLANETS] =
{
    0.39f,
    0.72f,
    1.0f,
    1.52f
};

static float planet_infos[NUM_PLANETS][NUM_PLANET_INFOS] =
{
    {252.25f, 87.97f, 0.2056f, 77.45f, 1},
    {181.98f, 224.7f, 0.0067f, 131.53f, 2},
    {100.46f, 365.25f, 0.0167f, 102.94f, 2},
    {355.45f, 686.98f, 0.0934f, 336.04f, 2}
};

//palette di colori per i pianeti
#define PLANET_W    8
#define PLANET_H    8

//mercurio
#define C 0xffb0b0b0
#define M 0xff888888
#define S 0xff555555

//venere
#define B1 0xffb0e2f8
#define G 0xff76bbe3
#define A1 0xff368fc1

//terra
#define B2 0xffb0e2f8
#define O 0xffcb822b
#define V 0xff2c8b4a

//marte
#define B3 0xfffafaff
#define A2 0xff117de7
#define R 0xff0e44c1

//sole
#define G2 0xfffafaff
#define A3 0xff117de7
#define R2 0xff0e44c1

//lut per il carissimo sole.... che ci tiene caldi
uint32_t sun_ascii[8*8] =
{
    R2, 0, 0, R2, R2, 0, 0, R2,
    0, A3, A3, A3, A3, A3, A3, 0,
    0, A3, G2, G2, G2, G2, A3, 0,
    R2, A3, G2, G2, G2, G2, A3, R2,
    R2, A3, G2, G2, G2, G2, A3, R2,
    0, A3, G2, G2, G2, G2, A3, 0,
    0, A3, A3, A3, A3, A3, A3, 0,
    R2, 0, 0, R2, R2, 0, 0, R2
};

//lut per pianeti, così li disegno con i colori
uint32_t planet_ascii[NUM_PLANETS][8*8] =
{
    {
        0, 0, C, C, C, C, 0, 0,
        0, C, M, M, M, M, C, 0,
        C, M, M, S, M, M, M, C,
        M, M, S, M, M, M, M, M,
        M, M, M, M, S, M, M, M,
        C, M, M, S, M, M, M, C,
        0, C, M, M, M, M, C, 0,
        0, 0, C, C, C, C, 0, 0
    },
    {
        0, 0, B1, B1, B1, B1, 0, 0,
        0, G, G, G, G, G, G, 0,
        A1, A1, A1, A1, A1, A1, A1, A1,
        G, G, G, G, G, G, G, G,
        A1, A1, A1, A1, A1, A1, A1, A1,
        G, G, G, G, G, G, G, G,
        0, G, G, G, G, G, G, 0,
        0, 0, B1, B1, B1, B1, 0, 0
    },
    {
        0, 0, B2, B2, B2, B2, 0, 0,
        0, O, O, V, V, O, O, 0,
        O, V, V, V, V, O, O, O,
        O, V, V, V, O, O, O, O,
        O, O, V, O, O, O, V, V,
        O, O, O, O, O, V, V, V,
        0, O, O, O, O, O, O, 0,
        0, 0, B2, B2, B2, B2, 0, 0
    }, 
    {
        0, 0, B3, B3, B3, B3, 0, 0,
        0, A2, A2, R, R, A2, A2, 0,
        A2, R, R, R, R, R, R, A2,
        A2, A2, R, R, R, R, A2, A2,
        A2, R, R, R, R, R, R, A2,
        A2, A2, R, R, R, R, A2, A2,
        0, A2, A2, R, R, A2, A2, 0,
        0, 0, B3, B3, B3, B3, 0, 0
    }
};

int check_element_value_type(struct json_object_element_s* e, json_type_t type)
{
    if(!e)
        return 0;

    if(!e->value)
        return 0;

    if(!e->value->payload)
        return 0;

    return e->value->type == type;
}

struct json_object_element_s* get_element_by_name_type(struct json_object_element_s* start, const char* name, json_type_t type)
{
    struct json_object_element_s* e = start;

    while(e)
    {
        if(e->name)
            if(e->name->string_size > 0)
                //trovato bastardo maledetto    
                if(!strcmp(e->name->string, name))
                {
                    if(check_element_value_type(e, type))
                        return e;
                    else
                        return 0; //che delusione...
                }

        e = e->next;
    }

    return e;
}

struct json_object_s* get_object_from_array_element(struct json_array_element_s* e)
{
    if(!e)
        return 0;

    if(!e->value)
        return 0;

    if(e->value->type != json_type_object)
        return 0;

    return e->value->payload;
}

int64_t get_epoch_approach(struct json_object_s* ap)
{
    struct json_object_element_s* e = get_element_by_name_type(ap->start, "epoch_date_close_approach", json_type_number);

    if(!check_element_value_type(e, json_type_number))
        return 0;
    
    struct json_number_s* num = e->value->payload;

    //champagne
    return atoll(num->number);
}

int get_string(char* dst, size_t n, struct json_value_s* value)
{
    if(!value || !dst)
        return 0;

    if(value->type != json_type_string)
        return 0;

    if(!value->payload)
        return 0;

    struct json_string_s* str = value->payload;
    size_t len = MIN(n, str->string_size);

    memcpy(dst, str->string, len);
    dst[len] = 0; //cazz di terminatori

    return 1;
}

int get_planet_from_name(char* name)
{
    //fa schifo da morire!
    if(!strcmp(name, "Earth"))
        return EARTH;
    else if(!strcmp(name, "Mars"))
        return MARS;
    else if(!strcmp(name, "Merc")) //è tagliato
        return MERCURY;
    else if(!strcmp(name, "Venus"))
        return VENUS;

    return -1;
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

int draw_planet(uint32_t* img, uint32_t w, uint32_t h, uint32_t xc, uint32_t yc, uint32_t* planet, uint8_t multiplier)
{
    uint32_t wp = PLANET_W * multiplier;
    uint32_t hp = PLANET_H * multiplier;
    uint32_t wp2 = wp >> 1;
    uint32_t hp2 = hp >> 1;

    if(xc < wp2 || w - xc < wp2 || yc < hp2 || h - yc < hp2)
        return 0;

    uint32_t* ascii = planet;
    uint32_t* dst = img + (yc - hp2) * w + (xc - wp2);

    float cx = (float)1.0f / (float)multiplier;
    float cy = (float)1.0f / (float)multiplier;
    float yp = 0;
    float xp = 0;

    for(uint32_t y = 0; y < hp; y++, yp += cy)
    {
        xp = 0;
        for(uint32_t x = 0; x < wp; x++, xp += cx)
        {
            //branchless e criptico! dai si capisce
            uint32_t pix = ascii[(uint32_t)yp * PLANET_W + (uint32_t)xp];
            *(dst + x) = ((pix >> 24) == 0xff) * pix + ((pix >> 24) != 0xff) * *(dst + x);
        }

        dst += w;
    }

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


__attribute__((export_name("set_timestamp_since_j2000")))
void set_timestamp_since_j2000(int64_t t)
{
    timestamp_since_j2000 = t;
    days_since_j2000 = t / (1000 * 60 * 60 * 24);
}

__attribute__((export_name("set_timestamp_since_epoch")))
void set_timestamp_since_epoch(int64_t t)
{
    timestamp_since_epoch = t;
}

__attribute__((export_name("get_planet_by_position")))
int get_planet_by_position(int x, int y)
{
    struct position* pos = planet_positions;
    float* info = (float*)planet_infos + SIZE_SCALING;

    for(int i = 0; i < NUM_PLANETS; i++, pos++, info += NUM_PLANET_INFOS)
    {
        int dx = (int)pos->x - x;
        int dy = (int)pos->y - y;

        float dist = 8 * 8;

        //senza radice quadrata!!!!!!!
        //vivremo solo di questo
        if(dx * dx + dy * dy < dist)
            return i;
    }

    return -1;
}

__attribute__((export_name("draw_asteroids")))
unsigned char* draw_asteroids(uint32_t w, uint32_t h)
{
    uint32_t size = w * h * 4;

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

    uint32_t* p2 = (uint32_t*)img;
    uint32_t background = 0xff3b190f; //#0f193b

    for(int y = 0; y < h; y++)
        for(int x = 0; x < w; x++)
            *p2++ = background;

    uint32_t w2 = w >> 1;
    uint32_t h2 = h >> 1;

    //sole mio
    draw_planet((uint32_t*)img, w, h, w2, h2, sun_ascii, 6);

    //vicino alla terra... il sistema solare è un po troppo
    for(int p = 0; p < NUM_PLANETS; p++)
    {
        float rf = planet_radius[p] / OBSERVATION_RADIUS;
        uint32_t r = rf * w / 2;

        float m = planet_infos[p][THETA_J2000];
        float period = planet_infos[p][YEAR_PERIOD];
        float ec = planet_infos[p][ECCENTRICITY];
        float pe = planet_infos[p][PERIHELION];
        float scale = planet_infos[p][SIZE_SCALING];

        //anomalia media
        float mean = m + (float)days_since_j2000 * 360.0f / period;
        
        //correzione orbita
        //int c = 360.0f / M_PI * ec * sin((mean - pe) * 180.0f / M_PI);

        uint32_t tetha = (mean) * 180.0f / M_PI;

        uint32_t xp = r * cos(tetha) + w2;
        uint32_t yp = r * sin(tetha) + h2;

        planet_positions[p].x = xp;
        planet_positions[p].y = yp;

        draw_circle((uint32_t*)img, w, h, w2, h2, r, 0xffffffff);
        draw_planet((uint32_t*)img, w, h, xp, yp, planet_ascii[p], scale);
    }

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

    struct json_object_element_s* neo = get_element_by_name_type(obj->start, "near_earth_objects", json_type_object);

    if(!neo)
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
            continue;

        struct json_array_s* a_array = (struct json_array_s*)date_arr->value->payload;
        struct json_array_element_s* a = a_array->start;

        while(a)
        {
            if(!a->value)
                return -1;

            struct json_object_s* a_obj = a->value->payload;
            struct json_object_element_s* e = 0;

            int hazardous = 0;

            //sarà mica pericoloso
            if(get_element_by_name_type(a_obj->start, "is_potentially_hazardous_asteroid", json_type_true))
            {
                //o cazzo
                hazardous = 1;
            }

            //prima devo trovare il pianeta di riferimento
            //ci potrebbero essere più approach
            if(!(e = get_element_by_name_type(a_obj->start, "close_approach_data", json_type_array)))
            {
                a = a->next;
                continue;
            }

            struct json_array_s* approach_array = (struct json_array_s*)e->value->payload;
            struct json_array_element_s* ap = approach_array->start;
            struct json_array_element_s* near_ap = 0;
            int64_t near_epoch_diff = 0;
            int64_t near_epoch = 0;

            //trovo quella più vicina ad oggi
            while(ap)
            {
                struct json_object_s* content = get_object_from_array_element(ap);

                if(!content)
                {
                    ap = ap->next;
                    continue;
                }

                int64_t ep = get_epoch_approach(content);

                if(near_ap != 0)
                {
                    int64_t diff = llabs(timestamp_since_epoch - ep);

                    if(diff < near_epoch_diff)
                    {
                        near_epoch = ep;
                        near_epoch_diff = diff;
                        near_ap = ap;   
                    }
                }
                else //prendo la prima data come riferimento
                {
                    near_epoch = ep;
                    near_epoch_diff = llabs(timestamp_since_epoch - ep);
                    near_ap = ap;
                }
                
                ap = ap->next;
            }

            //non ho trovato un cazzo oppure è successo qualcosa
            if(!near_ap)
            {
                a = a->next;
                continue;
            }

            struct json_object_s* content = get_object_from_array_element(near_ap);

            //problemi problemi
            if(!content)
            {
                a = a->next;
                continue;
            }

            char planet_name[16] = {0};
            if(!(e = get_element_by_name_type(content->start, "orbiting_body", json_type_string)))
            {
                a = a->next;
                continue;
            }

            if(!get_string(planet_name, 15, e->value))
            {
                a = a->next;
                continue;
            }

            int planet = get_planet_from_name(planet_name);

            //chissà che cazzo di pianeta è
            if(planet == -1)
            {
                a = a->next;
                continue;
            }

            size_t* num = &num_asteroids[planet];
            
            //un po troppi
            if(*num == MAX_ASTEROIDS)
            {
                a = a->next;
                continue;
            }

            struct asteroid* asteroid = &asteroids[planet][*num];
            
            if(!(e = get_element_by_name_type(a_obj->start, "name", json_type_string)))
            {
                a = a->next;
                continue;
            }

            if(!get_string(asteroid->name, ASTEROID_NAME_SIZE, e->value))
            {
                a = a->next;
                continue;
            }

            //diameter
            if(!(e = get_element_by_name_type(a_obj->start, "estimated_diameter", json_type_object)))
            {
                a = a->next;
                continue;
            }

            struct json_object_s* estimated_diameter = e->value->payload;

            if(!(e = get_element_by_name_type(estimated_diameter->start, "kilometers", json_type_object)))
            {
                a = a->next;
                continue;
            }

            struct json_object_s* diameter = e->value->payload;

            if(!(e = get_element_by_name_type(diameter->start, "estimated_diameter_min", json_type_number)))
            {
                a = a->next;
                continue;
            }

            asteroid->min_diameter = atof(((struct json_number_s*)e->value->payload)->number);

            if(!(e = get_element_by_name_type(diameter->start, "estimated_diameter_max", json_type_number)))
            {
                a = a->next;
                continue;
            }

            asteroid->max_diameter = atof(((struct json_number_s*)e->value->payload)->number);
            //

            //miss distance
            if(!(e = get_element_by_name_type(content->start, "miss_distance", json_type_object)))
            {
                a = a->next;
                continue;
            }

            struct json_object_s* distance = e->value->payload;

            if(!(e = get_element_by_name_type(distance->start, "kilometers", json_type_string)))
            {
                a = a->next;
                continue;
            }

            asteroid->miss_distance = atof(((struct json_string_s*)e->value->payload)->string);
            //

            //relative velocity
            if(!(e = get_element_by_name_type(content->start, "relative_velocity", json_type_object)))
            {
                a = a->next;
                continue;
            }

            struct json_object_s* vel = e->value->payload;

            if(!(e = get_element_by_name_type(vel->start, "kilometers_per_hour", json_type_string)))
            {
                a = a->next;
                continue;
            }

            asteroid->velocity = atof(((struct json_string_s*)e->value->payload)->string);
            //

            asteroid->timestamp_since_epoch = near_epoch;
            asteroid->is_hazardous = hazardous;

            (*num)++;

            a = a->next;
        }

        date_arr = date_arr->next;
    }

    return 1;
}

__attribute__((export_name("get_num_asteroids")))
size_t get_num_asteroids(uint8_t planet)
{
    if(planet >= NUM_PLANETS)
        return 0;

    return num_asteroids[planet];
}

__attribute__((export_name("get_asteroid_name")))
char* get_asteroid_name(uint8_t planet, uint8_t i)
{
    if(planet >= NUM_PLANETS)
        return 0;

    if(i < 0 || i >= num_asteroids[planet])
        return 0;

    return asteroids[planet][i].name;
}

__attribute__((export_name("get_asteroid_min_diameter")))
float get_asteroid_min_diameter(uint8_t planet, uint8_t i)
{
    if(planet >= NUM_PLANETS)
        return 0;

    if(i < 0 || i >= num_asteroids[planet])
        return 0;

    return asteroids[planet][i].min_diameter;
}

__attribute__((export_name("get_asteroid_max_diameter")))
float get_asteroid_max_diameter(uint8_t planet, uint8_t i)
{
    if(planet >= NUM_PLANETS)
        return 0;

    if(i < 0 || i >= num_asteroids[planet])
        return 0;

    return asteroids[planet][i].max_diameter;
}

__attribute__((export_name("get_asteroid_timestamp")))
int64_t get_asteroid_timestamp(uint8_t planet, uint8_t i)
{
    if(planet >= NUM_PLANETS)
        return 0;

    if(i < 0 || i >= num_asteroids[planet])
        return 0;

    return asteroids[planet][i].timestamp_since_epoch;
}

__attribute__((export_name("get_asteroid_miss_distance")))
float get_asteroid_miss_distance(uint8_t planet, uint8_t i)
{
    if(planet >= NUM_PLANETS)
        return 0;

    if(i < 0 || i >= num_asteroids[planet])
        return 0;

    return asteroids[planet][i].miss_distance;
}

__attribute__((export_name("get_asteroid_velocity")))
float get_asteroid_velocity(uint8_t planet, uint8_t i)
{
    if(planet >= NUM_PLANETS)
        return 0;

    if(i < 0 || i >= num_asteroids[planet])
        return 0;

    return asteroids[planet][i].velocity;
}

__attribute__((export_name("is_asteroid_hazardous")))
int is_asteroid_hazardous(uint8_t planet, uint8_t i)
{
    if(planet >= NUM_PLANETS)
        return 0;

    if(i < 0 || i >= num_asteroids[planet])
        return 0;

    return asteroids[planet][i].is_hazardous;
}