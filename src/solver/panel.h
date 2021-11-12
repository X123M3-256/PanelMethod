#ifndef PANEL_H_INCLUDED
#define PANEL_H_INCLUDED
#include "../util/vectormath.h"

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
//Derived quantities
double* diameters;
double* areas;
vector3_t* collocation_points;
vector3_t* normals;
}mesh_t;

typedef struct
{
int num_points;
vector3_t center;
vector3_t local_x;
vector3_t local_z;
vector3_t local_y;
vector2_t p[4];
double diameter;
double area;
double d[4];
double m[4];
}panel_local_basis_t;



int mesh_init(mesh_t* mesh,int num_vertices,int num_panels,vector3_t* vertices,panel_t* panels);
void mesh_solve(mesh_t* mesh,double* source_strengths,double* doublet_strengths,double aoa);
void mesh_get_panel_local_basis(mesh_t* mesh,int panel,panel_local_basis_t* basis);
void mesh_get_panel_influence(panel_local_basis_t* basis,vector3_t point,double* source_influence,double* doublet_influence);
void mesh_get_panel_velocities(mesh_t* mesh,double* source_strengths,double* doublet_strengths,double aoa,vector3_t* velocities);

#endif

