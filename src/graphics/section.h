#ifndef SECTION_H_INCLUDED
#define SECTION_H_INCLUDED
#include "render.h"

typedef struct
{
vector3_t center;
vector3_t normal;
int subdivisions;
float x_range;
float y_range;
float step_x;
float step_y;
vector3_t vertices[4];
cairo_t* context;
cairo_surface_t* surface;
object_t gl_object;
}section_t;



int section_init(section_t* section,vector3_t center,vector3_t normal,float x_range,float y_range);
void section_update_coords(section_t* section);
void section_draw_scalar_field(section_t* section,void (*scalar)(int,vector3_t*,void*,double*),void* closure);
void section_draw_vector_field(section_t* section,void (*vector)(int,vector3_t*,void*,vector3_t*),void* closure);
void section_draw_vector_field_with_scalar(section_t* section,void (*vector)(int,vector3_t*,void*,vector3_t*),double (*scalar)(vector3_t,void*),void* closure);
void section_draw_grid(section_t* section);
void section_update_texture(section_t* section);


#endif
