#ifndef PANEL_H_INCLUDED
#define PANEL_H_INCLUDED
#include "../util/vectormath.h"
#include "../graphics/render.h"

typedef struct
{
int vertices[4];
}panel_t;


typedef struct
{
int num_vertices;
int num_panels;
vector3_t* vertices;
panel_t* panels;
vector3_t* panel_vertices;
vector3_t* panel_normals;
}mesh_t;

typedef struct
{
int num_points;
vector3_t center;
vector3_t local_x;
vector3_t local_z;
vector3_t local_y;
vector2_t p[4];
double d[4];
double m[4];
}panel_local_basis_t;



int mesh_load(mesh_t* mesh,const char* filename);
int mesh_init_render_object(mesh_t* mesh,object_t* object);
int mesh_update_render_object(mesh_t* mesh,object_t* object,double* panel_values_x,double* panel_values_y);
vector3_t mesh_get_panel_vertex(mesh_t* mesh,int panel,int vertex);
vector3_t mesh_get_panel_normal(mesh_t* mesh,int panel);
vector3_t mesh_get_panel_collocation_point(mesh_t* mesh,int panel);
void mesh_get_panel_local_basis(mesh_t* mesh,int panel,panel_local_basis_t* basis);
void mesh_get_panel_influence(panel_local_basis_t* basis,vector3_t point,double* source_influence,double* doublet_influence);
void mesh_get_panel_velocities(mesh_t* mesh,double* source_strengths,double* doublet_strengths,vector3_t* velocities);

#endif

