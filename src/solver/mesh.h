#ifndef SOLVER_MESH_H_INCLUDED
#define SOLVER_MESH_H_INCLUDED
#include "../util/vectormath.h"

typedef struct
{
int vertices[4];
}panel_t;

typedef struct
{
int vertices[2];
}segment_t;

typedef struct
{
//Number of shedding line segments
int num_segments;
//Number of shedding vertices
int num_vertices;
//Number of panels shed from each segment
int length;
//Maximum lengthwise panels
int max_length;
//Indices of the shedding vertices
int* vertices;
//Indices of the upper and lower shedding panels
int* upper_panels;
int* lower_panels;
//Segments
segment_t* segments;
//Doublet strengths of panels
double* doublet_strengths;
}wake_t;

typedef struct
{
int num_vertices;
int num_panels;
vector3_t* vertices;
panel_t* panels;
wake_t wake;

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


int wake_init(wake_t* wake);
int mesh_init(mesh_t* mesh,int num_vertices,int num_panels,vector3_t* vertices,panel_t* panels,int num_wake_segments,int num_wake_vertices,int* wake_vertices,segment_t* wake_segments,int* wake_upper_panels,int* wake_lower_panels,int wake_max_length);
void mesh_update_wake(mesh_t* mesh,double* source_strengths,double* doublet_strengths,double aoa);

panel_t mesh_get_wake_panel(mesh_t* mesh,int row,int panel);
void mesh_solve(mesh_t* mesh,double* source_strengths,double* doublet_strengths,double aoa);
void mesh_get_panel_local_basis(mesh_t* mesh,int panel,panel_local_basis_t* basis);
void mesh_get_wake_panel_local_bases(mesh_t* mesh,int row,int i,panel_local_basis_t* bases);


void mesh_get_panel_influence(panel_local_basis_t* basis,vector3_t source_point,double* source_influence,double* doublet_influence);
void mesh_get_panel_velocity_influence(panel_local_basis_t* b,vector3_t source_point,vector3_t* source_influence,vector3_t* doublet_influence);
double mesh_get_wake_panel_influence(panel_local_basis_t* bases,vector3_t source_point);
vector3_t mesh_get_wake_panel_velocity_influence(mesh_t* mesh,int row,int i,vector3_t point);






#endif
