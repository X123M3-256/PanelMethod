#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<time.h>
#include<math.h>
#include<assert.h>
#include<lapacke.h>
#include "panel.h"

void solver_init(solver_t* solver,mesh_t* mesh,double aoa)
{
solver->t=0.0;
solver->aoa=aoa;
solver->freestream=vector3(-cos(aoa),sin(aoa),0);
solver->mesh=mesh;
solver->source_strengths=calloc(mesh->num_panels,sizeof(double));
solver->doublet_strengths=calloc(mesh->num_panels,sizeof(double));
solver->velocities=calloc(mesh->num_panels,sizeof(vector3_t));
solver->pressures=calloc(mesh->num_panels,sizeof(double));
}

/*Velocity computation routines*/

void solver_compute_potential(solver_t* solver,int num_points,vector3_t* points,double* potentials)
{
panel_local_basis_t basis;
panel_local_basis_t bases[2];
	for(int j=0;j<num_points;j++)
	{
	potentials[j]=0;
	}

	for(int i=0;i<solver->mesh->num_panels;i++)
	{
	mesh_get_panel_local_basis(solver->mesh,i,&basis);
		for(int j=0;j<num_points;j++)
		{
		double source,doublet;
		mesh_get_panel_influence(&basis,points[j],&source,&doublet);
		potentials[j]+=source*solver->source_strengths[i]+doublet*solver->doublet_strengths[i];
		}
	}

//Wake
	for(int i=0;i<solver->mesh->wake.length;i++)
	for(int j=0;j<solver->mesh->wake.num_segments;j++)
	{
	panel_local_basis_t bases[2];
	mesh_get_wake_panel_local_bases(solver->mesh,i,j,bases);
		for(int k=0;k<num_points;k++)
		{
		potentials[k]+=solver->mesh->wake.doublet_strengths[j+i*solver->mesh->wake.num_segments]*mesh_get_wake_panel_influence(bases,points[k]);
		}
	}
}

void solver_compute_velocities_general(solver_t* solver,int num_points,vector3_t* points,vector3_t* velocities,int surface)
{
panel_local_basis_t basis;
	for(int j=0;j<num_points;j++)
	{
	velocities[j]=solver->freestream;
	}

	for(int i=0;i<solver->mesh->num_panels;i++)
	{
	mesh_get_panel_local_basis(solver->mesh,i,&basis);
		for(int j=0;j<num_points;j++)
		{
		vector3_t source,doublet;
		mesh_get_panel_velocity_influence(&basis,points[j],&source,&doublet);
			//Source velocities are discontinuous on the panel surface, so use the outer limit
			if(surface&&i==j)source=vector3_scale(solver->mesh->normals[j],0.5);
		velocities[j]=vector3_add(velocities[j],vector3_add(vector3_scale(source,solver->source_strengths[i]),vector3_scale(doublet,solver->doublet_strengths[i])));
		}
	}

//Wake
	for(int i=0;i<solver->mesh->wake.length;i++)
	for(int j=0;j<solver->mesh->wake.num_segments;j++)
	{
		for(int k=0;k<num_points;k++)
		{
		vector3_t doublet=mesh_get_wake_panel_velocity_influence(solver->mesh,i,j,points[k]);
		velocities[k]=vector3_add(velocities[k],vector3_scale(doublet,solver->mesh->wake.doublet_strengths[j+i*solver->mesh->wake.num_segments]));
		}
	}
}

void solver_compute_velocity(solver_t* solver,int num_points,vector3_t* points,vector3_t* velocities)
{
solver_compute_velocities_general(solver,num_points,points,velocities,0);
}



/*Solution routines*/

void solver_update_wake(solver_t* solver,float dt)
{
//Advect previous vertices
vector3_t* velocities=calloc(solver->mesh->wake.length*solver->mesh->wake.num_vertices,sizeof(vector3_t));
solver_compute_velocity(solver,solver->mesh->wake.length*solver->mesh->wake.num_vertices,solver->mesh->vertices+solver->mesh->num_vertices,velocities);
	for(int j=0;j<solver->mesh->wake.length;j++)
	for(int i=0;i<solver->mesh->wake.num_vertices;i++)
	{
	int vertex_index=i+j*solver->mesh->wake.num_vertices;
	double step=(1+j)*dt;
	solver->mesh->vertices[solver->mesh->num_vertices+vertex_index]=vector3_add(vector3_scale(velocities[vertex_index],step),solver->mesh->vertices[solver->mesh->num_vertices+vertex_index]);	
	}

//Move data back
int rows_to_copy=solver->mesh->wake.length==solver->mesh->wake.max_length?solver->mesh->wake.max_length-1:solver->mesh->wake.length;
memmove(solver->mesh->vertices+solver->mesh->num_vertices+solver->mesh->wake.num_vertices,solver->mesh->vertices+solver->mesh->num_vertices,rows_to_copy*solver->mesh->wake.num_vertices*sizeof(vector3_t));
memmove(solver->mesh->wake.doublet_strengths+solver->mesh->wake.num_segments,solver->mesh->wake.doublet_strengths,rows_to_copy*solver->mesh->wake.num_segments*sizeof(double));
	if(solver->mesh->wake.length<solver->mesh->wake.max_length)solver->mesh->wake.length++;

//Add new vertex row
	for(int i=0;i<solver->mesh->wake.num_vertices;i++)
	{
	vector3_t velocity=solver->freestream;
		if(solver->mesh->wake.length>1)velocity=velocities[i];
	solver->mesh->vertices[solver->mesh->num_vertices+i]=vector3_add(vector3_scale(velocity,dt),solver->mesh->vertices[solver->mesh->wake.vertices[i]]);	
	}

free(velocities);
}

void solver_update_solution(solver_t* solver)
{
int n=solver->mesh->num_panels+solver->mesh->wake.num_segments;
double* matrix=calloc(n*n,sizeof(double));
double* rhs=calloc(n,sizeof(double));
int* ipiv=calloc(n,sizeof(int));

clock_t t1=clock();

//Set source strength and rhs
	for(int i=0;i<solver->mesh->num_panels;i++)
	{
	solver->source_strengths[i]=-vector3_dot(solver->mesh->normals[i],solver->freestream);
	rhs[i]=0;
	}

//Compute influence of previously shed wake panels with known strength
	for(int i=1;i<solver->mesh->wake.length;i++)
	for(int j=0;j<solver->mesh->wake.num_segments;j++)
	{
	panel_local_basis_t bases[2];
	mesh_get_wake_panel_local_bases(solver->mesh,i,j,bases);
		for(int k=0;k<solver->mesh->num_panels;k++)
		{
		rhs[k]-=solver->mesh->wake.doublet_strengths[j+i*solver->mesh->wake.num_segments]*mesh_get_wake_panel_influence(bases,solver->mesh->collocation_points[k]);
		}
	}


//Compute surface panel influence matrix TODO it appears this is a symmetric matrix so could be optimized
panel_local_basis_t basis;
	for(int i=0;i<solver->mesh->num_panels;i++)
	{
	mesh_get_panel_local_basis(solver->mesh,i,&basis);
		for(int j=0;j<solver->mesh->num_panels;j++)
		{
		double source=0.0;
		double doublet=0.0;
		vector3_t collocation_point=solver->mesh->collocation_points[j];
		mesh_get_panel_influence(&basis,collocation_point,&source,&doublet);
		matrix[j+i*n]=doublet;

		rhs[j]-=solver->source_strengths[i]*source;
		}
	matrix[i+i*n]=0.5;
	}

//Compute influence of wake panels on surface panels
panel_local_basis_t bases[2];
	for(int j=0;j<solver->mesh->wake.num_segments;j++)
	{
	mesh_get_wake_panel_local_bases(solver->mesh,0,j,bases);
		for(int k=0;k<solver->mesh->num_panels;k++)
		{
		matrix[k+(solver->mesh->num_panels+j)*n]+=mesh_get_wake_panel_influence(bases,solver->mesh->collocation_points[k]);
		}
	}

//Impose Kutta condition on wake

	for(int i=0;i<solver->mesh->wake.num_segments;i++)
	{
	matrix[(solver->mesh->num_panels+i)+(solver->mesh->num_panels+i)*n]=1.0;
	matrix[solver->mesh->wake.upper_panels[i]*n+(solver->mesh->num_panels+i)]=-1.0;
	matrix[solver->mesh->wake.lower_panels[i]*n+(solver->mesh->num_panels+i)]=1.0;
	rhs[solver->mesh->num_panels+i]=0;
	}

clock_t t2=clock();

	if(LAPACKE_dgesv(LAPACK_COL_MAJOR,n,1,matrix,n,ipiv,rhs,n)!=0)
	{
	printf("Solution failed\n");
	}

memcpy(solver->doublet_strengths,rhs,solver->mesh->num_panels*sizeof(double));
memcpy(solver->mesh->wake.doublet_strengths,rhs+solver->mesh->num_panels,solver->mesh->wake.num_segments*sizeof(double));

clock_t t3=clock();

printf("Matrix assembly %f\n",(t2-t1)/(float)CLOCKS_PER_SEC);
printf("Matrix solution %f\n",(t3-t2)/(float)CLOCKS_PER_SEC);


//	for(int i=0;i<solver->mesh->wake.num_segments;i++)
//	{
//	printf("Wake strength %d %f\n",i,solver->mesh->wake.solver->doublet_strengths[i]);
	//printf("Upper panel strength %d %f\n",i,solver->doublet_strengths[solver->mesh->wake_upper_panels[i]]);
///	}

free(matrix);
free(rhs);
}

void solver_update_velocities(solver_t* solver)
{
solver_compute_velocities_general(solver,solver->mesh->num_panels,solver->mesh->collocation_points,solver->velocities,1);
	for(int i=0;i<solver->mesh->num_panels;i++)solver->pressures[i]=1.0-vector3_dot(solver->velocities[i],solver->velocities[i]);
}

void solver_compute_step(solver_t* solver,double dt)
{
clock_t t1=clock();
solver_update_wake(solver,dt);
clock_t t2=clock();
printf("Wake update %f\n",(t2-t1)/(float)CLOCKS_PER_SEC);
solver_update_solution(solver);
clock_t t3=clock();
//solver_update_velocities(solver);
clock_t t4=clock();
printf("Velocity update %f\n",(t4-t3)/(float)CLOCKS_PER_SEC);
printf("Total step time %f\n",(t4-t1)/(float)CLOCKS_PER_SEC);
}


