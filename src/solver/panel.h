#ifndef PANEL_H_INCLUDED
#define PANEL_H_INCLUDED
#include "mesh.h"
#include "../util/vectormath.h"

typedef struct
{
double t;
double aoa;
vector3_t freestream;
mesh_t* mesh;
double* doublet_strengths;
double* source_strengths;
vector3_t* velocities;
double* pressures;
}solver_t;

void solver_init(solver_t* solver,mesh_t* mesh,double aoa);
void solver_compute_step(solver_t* solver,double dt);
void solver_compute_potential(solver_t* solver,int num_points,vector3_t* points,double* potentials);
void solver_compute_velocity(solver_t* solver,int num_points,vector3_t* points,vector3_t* velocities);

#endif

