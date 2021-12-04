#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED
#include "object.h"
#include "../solver/panel.h"

int mesh_init_render_object(mesh_t* mesh,object_t* object);
int mesh_update_render_object(mesh_t* mesh,object_t* object,double* panel_values_x,double* panel_values_y);

int wake_init_render_object(mesh_t* mesh,object_t* object);
int wake_update_render_object(mesh_t* mesh,object_t* object);

#endif
