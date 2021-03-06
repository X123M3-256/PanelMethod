#ifndef SECTION_H_INCLUDED
#define SECTION_H_INCLUDED
#include "object.h"

enum
{
SECTION_SHOW_GRID=1,
SECTION_SHOW_SCALAR=2,
SECTION_SHOW_DERIVED_SCALAR=4,
SECTION_SHOW_VECTOR=8
};

typedef struct
{
int flags;
vector3_t center;
vector3_t normal;
float x_range;
float y_range;
int subdivisions;
float alpha;
void (*vector)(int,vector3_t*,void*,vector3_t*);
	union
	{
	void (*scalar)(int,vector3_t*,void*,double*);
	double (*derived_scalar)(vector3_t,void*);
	};
void* closure;
//Derived quantities
object_t gl_object;
float step_x;
float step_y;
vector3_t vertices[4];
}section_t;



int section_init(section_t* section,vector3_t center,vector3_t normal,float x_range,float y_range,float alpha,int flags,void (*vector)(int,vector3_t*,void*,vector3_t*),void* scalar,void* closure);
void section_update(section_t* section);
void section_destroy(section_t* section);
#endif
