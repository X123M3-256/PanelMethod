#ifndef WING_H_INCLUDED
#define WING_H_INCLUDED

#include "airfoil.h"
#include "../util/vectormath.h"
#include "../solver/panel.h"

#define WING_MAX_SEGMENTS 16

typedef struct
{
airfoil_t airfoil;
int num_segments;
vector3_t segment_offset[WING_MAX_SEGMENTS+1];
float segment_chord[WING_MAX_SEGMENTS+1];
}wing_t;


void wing_init_mesh(wing_t* wing,mesh_t* mesh,int chordwise_panels,int spanwise_panels,int wake_panels);


#endif
