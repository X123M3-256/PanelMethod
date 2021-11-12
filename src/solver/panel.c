#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<assert.h>
#include<lapacke.h>
#include "panel.h"


double quadrilateral_area(vector2_t* p)
{
return 0.5*(p[0].x*p[1].y+p[1].x*p[2].y+p[2].x*p[3].y+p[3].x*p[0].y-p[1].x*p[0].y-p[2].x*p[1].y-p[3].x*p[2].y-p[0].x*p[3].y);
}

vector2_t quadrilateral_centroid(vector2_t* p)
{
double factor=1.0/(6.0*quadrilateral_area(p));
double x=(p[0].x+p[1].x)*(p[0].x*p[1].y-p[1].x*p[0].y)+(p[1].x+p[2].x)*(p[1].x*p[2].y-p[2].x*p[1].y)+(p[2].x+p[3].x)*(p[2].x*p[3].y-p[3].x*p[2].y)+(p[3].x+p[0].x)*(p[3].x*p[0].y-p[0].x*p[3].y);
double y=(p[0].y+p[1].y)*(p[0].x*p[1].y-p[1].x*p[0].y)+(p[1].y+p[2].y)*(p[1].x*p[2].y-p[2].x*p[1].y)+(p[2].y+p[3].y)*(p[2].x*p[3].y-p[3].x*p[2].y)+(p[3].y+p[0].y)*(p[3].x*p[0].y-p[0].x*p[3].y);
return vector2(factor*x,factor*y);
}

double quadrilateral_diameter(vector2_t* p)
{
return fmax(fmax(vector2_norm(vector2_sub(p[1],p[0])),vector2_norm(vector2_sub(p[3],p[2]))),fmax(vector2_norm(vector2_sub(p[2],p[0])),vector2_norm(vector2_sub(p[3],p[1]))));
}


void mesh_update_panel_data(mesh_t* mesh,int start,int end)
{
	for(int i=0;i<mesh->num_panels+mesh->num_wake_panels;i++)
	{
	panel_t panel=mesh->panels[i];
	vector3_t local_x=vector3_normalize(vector3_sub(mesh->vertices[panel.vertices[1]],mesh->vertices[panel.vertices[0]]));
	vector3_t local_z=vector3_normalize(vector3_cross(vector3_sub(mesh->vertices[panel.vertices[2]],mesh->vertices[panel.vertices[0]]),local_x));
	vector3_t local_y=vector3_cross(local_x,local_z);
	vector2_t p[4];
		for(int j=0;j<4;j++)
		{
		vector3_t offset=vector3_sub(mesh->vertices[panel.vertices[j]],mesh->vertices[panel.vertices[0]]);
		p[j]=vector2(vector3_dot(local_x,offset),vector3_dot(local_y,offset));
		}
	vector2_t centroid=quadrilateral_centroid(p);
	mesh->areas[i]=quadrilateral_area(p);
	mesh->diameters[i]=quadrilateral_diameter(p);
	mesh->normals[i]=local_z;
	mesh->collocation_points[i]=vector3_add(vector3_add(vector3_scale(local_x,centroid.x),vector3_scale(local_y,centroid.y)),mesh->vertices[panel.vertices[0]]);
	}
}

int mesh_init(mesh_t* mesh,int num_vertices,int num_panels,vector3_t* vertices,panel_t* panels,int num_wake_strip_panels,int num_wake_strip_vertices,int num_wake_strips,int* wake_vertices,int* upper_panels,int* lower_panels)
{
mesh->num_vertices=num_vertices;
mesh->num_panels=num_panels;
mesh->vertices=vertices;
mesh->panels=panels;
mesh->num_wake_strip_panels=num_wake_strip_panels;
mesh->num_wake_strip_vertices=num_wake_strip_vertices;
mesh->num_wake_strips=num_wake_strips;
mesh->wake_vertices=wake_vertices;
mesh->wake_upper_panels=upper_panels;
mesh->wake_lower_panels=lower_panels;


//Compute derived quantities
mesh->num_wake_panels=num_wake_strips*num_wake_strip_panels;
mesh->areas=calloc(num_panels+mesh->num_wake_panels,sizeof(double));
mesh->diameters=calloc(num_panels+mesh->num_wake_panels,sizeof(double));
mesh->collocation_points=calloc(num_panels+mesh->num_wake_panels,sizeof(vector3_t));
mesh->normals=calloc(num_panels+mesh->num_wake_panels,sizeof(vector3_t));
mesh_update_panel_data(mesh,0,mesh->num_panels+mesh->num_wake_panels);
}


/*Influence computation routines*/

void mesh_get_panel_local_basis(mesh_t* mesh,int panel,panel_local_basis_t* basis)
{
basis->num_points=(mesh->panels[panel].vertices[2]==mesh->panels[panel].vertices[3])?3:4;
//Get local coordinate system in which the panel lies in the XY plane
basis->center=mesh->collocation_points[panel];
basis->local_x=vector3_normalize(vector3_sub(mesh->vertices[mesh->panels[panel].vertices[1]],mesh->vertices[mesh->panels[panel].vertices[0]]));
basis->local_z=mesh->normals[panel];
basis->local_y=vector3_cross(basis->local_z,basis->local_x);
basis->diameter=mesh->diameters[panel];
basis->area=mesh->areas[panel];
//Transform panel vertices into local coordinate system

	for(int i=0;i<4;i++)
	{
	vector3_t diff=vector3_sub(mesh->vertices[mesh->panels[panel].vertices[i]],basis->center);
	basis->p[i]=vector2(vector3_dot(basis->local_x,diff),vector3_dot(basis->local_y,diff));
	}

	for(int i=0;i<basis->num_points;i++)
	{
	basis->d[i]=vector2_norm(vector2_sub(basis->p[i],basis->p[(i+3)&3]));
	basis->m[i]=(basis->p[i].y-basis->p[(i+3)&3].y)/(basis->p[i].x-basis->p[(i+3)&3].x);
	}
/*
printf("Num points %d\n",basis->num_points);
printf("Local X %f %f %f\n",basis->local_x.x,basis->local_x.y,basis->local_x.z);
printf("Local Y %f %f %f\n",basis->local_y.x,basis->local_y.y,basis->local_y.z);
printf("Local Z %f %f %f\n",basis->local_z.x,basis->local_z.y,basis->local_z.z);

printf("d %f %f %f %f\n",basis->d[0],basis->d[1],basis->d[2],basis->d[3]);
printf("m %f %f %f %f\n",basis->m[0],basis->m[1],basis->m[2],basis->m[3]);
*/
}

typedef struct
{
double r[4];
double e[4];
double h[4];
double logs[4];
}influence_intermediates_t;

void mesh_get_panel_influence_intermediates(panel_local_basis_t* basis,vector3_t point,influence_intermediates_t* intermediates)
{
	for(int i=0;i<4;i++)
	{
	intermediates->r[i]=vector3_norm(vector3(point.x-basis->p[i].x,point.y-basis->p[i].y,point.z));
	intermediates->e[i]=(point.x-basis->p[i].x)*(point.x-basis->p[i].x)+point.z*point.z;
	intermediates->h[i]=(point.x-basis->p[i].x)*(point.y-basis->p[i].y);
	}
	for(int i=0;i<basis->num_points;i++)
	{
	int j=(3+i)&3;
	intermediates->logs[i]=log((intermediates->r[j]+intermediates->r[i]+basis->d[i])/(intermediates->r[j]+intermediates->r[i]-basis->d[i]));
	}
}

void mesh_get_panel_influence(panel_local_basis_t* basis,vector3_t source_point,double* source_influence,double* doublet_influence)
{
vector3_t diff=vector3_sub(source_point,basis->center);
double r=vector3_norm(diff);

	if(0&&r>3.0*basis->diameter)
	{
	//Far field approximation
	*source_influence=-basis->area/(2*r*M_PI);
	*doublet_influence=vector3_dot(basis->local_z,diff)*(*source_influence)/r;
	return;
	}

vector3_t point=vector3(vector3_dot(basis->local_x,diff),vector3_dot(basis->local_y,diff),vector3_dot(basis->local_z,diff));

influence_intermediates_t intermediates;
mesh_get_panel_influence_intermediates(basis,point,&intermediates);

//Page 245 Katz&Plotkin
double A=0.0;
double B=0.0;
	for(int i=0;i<basis->num_points;i++)
	{
	int j=(3+i)&3;
	A+=intermediates.logs[i]*((point.x-basis->p[j].x)*(basis->p[i].y-basis->p[j].y)-(point.y-basis->p[j].y)*(basis->p[i].x-basis->p[j].x))/basis->d[i];
	B+=(atan((basis->m[i]*intermediates.e[(3+i)&3]-intermediates.h[(3+i)&3])/(point.z*intermediates.r[(3+i)&3]))-atan((basis->m[i]*intermediates.e[i]-intermediates.h[i])/(point.z*intermediates.r[i])));
	}
*source_influence=-(A-point.z*B)/(4*M_PI);
*doublet_influence=-B/(4*M_PI);
}

void mesh_get_panel_velocity_influence(panel_local_basis_t* b,vector3_t source_point,vector3_t* source_influence,vector3_t* doublet_influence)
{
vector3_t diff=vector3_sub(source_point,b->center);
double r=vector3_norm(diff);
vector3_t point=vector3(vector3_dot(b->local_x,diff),vector3_dot(b->local_y,diff),vector3_dot(b->local_z,diff));

vector3_t source=vector3(0,0,0);
vector3_t doublet=vector3(0,0,0);

	if(0&&r>3.0*b->diameter)
	{
	//Far field approximation
	double factor=b->area/(4*M_PI*r*r*sqrt(r));
	source=vector3_scale(point,factor);
	doublet=vector3_scale(vector3(3*point.x*point.z,3*point.y*point.z,point.x*point.x+point.y*point.y-2*point.z*point.z),factor);	
	}
	else
	{
	influence_intermediates_t im;
	mesh_get_panel_influence_intermediates(b,point,&im);

	double doublet_factors[4];
		for(int i=0;i<b->num_points;i++)
		{
		int j=(3+i)&3;
		doublet_factors[i]=(im.r[i]+im.r[j])/(im.r[j]*im.r[i]*(im.r[j]*im.r[i]+((point.x-b->p[j].x)*(point.x-b->p[i].x)+(point.y-b->p[j].y)*(point.y-b->p[i].y)+point.z*point.z)));
		}

		for(int i=0;i<b->num_points;i++)
		{
		int j=(3+i)&3;
		source.x+=im.logs[i]*(b->p[j].y-b->p[i].y)/b->d[i];
		source.y+=im.logs[i]*(b->p[i].x-b->p[j].x)/b->d[i];
		source.z+=(atan((b->m[i]*im.e[j]-im.h[j])/(point.z*im.r[j]))
			  -atan((b->m[i]*im.e[i]-im.h[i])/(point.z*im.r[i])));
		doublet.x+=point.z*(b->p[j].y-b->p[i].y)*doublet_factors[i];
		doublet.y+=point.z*(b->p[i].x-b->p[j].x)*doublet_factors[i];
		doublet.z+=((point.x-b->p[i].x)*(point.y-b->p[j].y)-(point.x-b->p[j].x)*(point.y-b->p[i].y))*doublet_factors[i];
		}
	source=vector3_scale(source,1.0/(4.0*M_PI));
	doublet=vector3_scale(doublet,1.0/(4.0*M_PI));
	}
*source_influence=vector3_add(vector3_add(vector3_scale(b->local_x,source.x),vector3_scale(b->local_y,source.y)),vector3_scale(b->local_z,source.z));
*doublet_influence=vector3_add(vector3_add(vector3_scale(b->local_x,doublet.x),vector3_scale(b->local_y,doublet.y)),vector3_scale(b->local_z,doublet.z));
}


/*Solver code*/

void mesh_initialize_wake(mesh_t* mesh,vector3_t freestream,double wake_length)
{
	for(int i=0;i<mesh->num_wake_strips;i++)
	for(int j=0;j<mesh->num_wake_strip_vertices;j++)
	{
	double dist=-wake_length*(i+1)/(double)(mesh->num_wake_strips);
	mesh->vertices[mesh->num_vertices+j+i*mesh->num_wake_strip_vertices]=vector3_add(mesh->vertices[mesh->wake_vertices[j]],vector3_scale(freestream,dist));
	}
mesh_update_panel_data(mesh,mesh->num_panels,mesh->num_wake_panels);
}

/*
void mesh_update_wake(mesh_t* mesh,double* source_strengths,double* doublet_strengths,double aoa)
{
vector3_t freestream=vector3(cos(aoa),-sin(aoa),0);

vector3_t* previous_row=calloc(mesh->num_wake_strip_vertices,sizeof(vector3_t));
vector3_t* current_row=calloc(mesh->num_wake_strip_vertices,sizeof(vector3_t));
vector3_t row_velocities=calloc(mesh->num_wake_strip_vertices,sizeof(vector3_t));

	for(int i=0;i<mesh->num_wake_strip_vertices;i++)previous_row[i]=mesh->vertices[mesh->wake_vertices[j]];

	for(int i=0;i<mesh->num_wake_strips;i++)
	{
	//Because the solution is singular at the panel vertices, we compute the velocity at the panel center and then average two
	

	//Load current vertex positions
		for(int i=0;i<mesh->num_wake_strip_vertices;i++)current_row[i]=mesh->vertices[mesh->wake_vertices[j]];


	for(int j=0;j<mesh->num_wake_strip_vertices;j++)
	{


	double dist=-wake_length*(i+1)/(double)(mesh->num_wake_strips);
	mesh->vertices[mesh->num_vertices+j+i*mesh->num_wake_strip_vertices]=vector3_add(,vector3_scale(freestream,dist));
	}
	}


free(cur_row);
free(previous_row);
}
*/


void mesh_solve(mesh_t* mesh,double* source_strengths,double* doublet_strengths,double aoa)
{
int n=mesh->num_panels+mesh->num_wake_strip_panels;
double* matrix=calloc(n*n,sizeof(double));
double* rhs=doublet_strengths;
int* ipiv=calloc(n,sizeof(int));


//Initialize wake shape and source strength
vector3_t freestream=vector3(cos(aoa),-sin(aoa),0);
mesh_initialize_wake(mesh,freestream,3.0);
	for(int i=0;i<mesh->num_panels;i++)
	{
	source_strengths[i]=vector3_dot(mesh->normals[i],freestream);
	rhs[i]=0;
	}

//Compute surface panel influence matrix TODO it appears this is a symmetric matrix so could be optimized
panel_local_basis_t basis;
	for(int i=0;i<mesh->num_panels;i++)
	{
	mesh_get_panel_local_basis(mesh,i,&basis);
		for(int j=0;j<mesh->num_panels;j++)
		{
		double source=0.0;
		double doublet=0.0;
		vector3_t collocation_point=mesh->collocation_points[j];
		mesh_get_panel_influence(&basis,collocation_point,&source,&doublet);
		matrix[j+i*n]=doublet;

		rhs[j]-=source_strengths[i]*source;
		}
	matrix[i+i*n]=0.5;
	}

//Compute influence of wake panels on surface panels
	for(int i=0;i<1;i++)//mesh->num_wake_strips;i++)
	{
		for(int j=0;j<mesh->num_wake_strip_panels;j++)
		{
		mesh_get_panel_local_basis(mesh,mesh->num_panels+j+i*mesh->num_wake_strip_panels,&basis);
			for(int k=0;k<mesh->num_panels;k++)
			{
			double source=0.0;
			double doublet=0.0;
			vector3_t collocation_point=mesh->collocation_points[k];
			mesh_get_panel_influence(&basis,collocation_point,&source,&doublet);
			matrix[k+(mesh->num_panels+j)*n]+=doublet;
			}
		}
	}

//Impose Kutta condition on wake
	for(int i=0;i<mesh->num_wake_strip_panels;i++)
	{
	matrix[(mesh->num_panels+i)+(mesh->num_panels+i)*n]=1.0;
	matrix[mesh->wake_upper_panels[i]*n+(mesh->num_panels+i)]=-1.0;
	matrix[mesh->wake_lower_panels[i]*n+(mesh->num_panels+i)]=1.0;
	rhs[mesh->num_panels+i]=0.0;
	}

	if(LAPACKE_dgesv(LAPACK_COL_MAJOR,n,1,matrix,n,ipiv,rhs,n)!=0)
	{
	printf("Solution failed\n");
	}

	for(int i=0;i<mesh->num_wake_strip_panels;i++)
	{
	printf("Wake strength %d %f\n",i,doublet_strengths[mesh->num_panels+i]);
	//printf("Upper panel strength %d %f\n",i,doublet_strengths[mesh->wake_upper_panels[i]]);
	}

free(matrix);
}

void mesh_compute_velocities_general(mesh_t* mesh,double* source_strengths,double* doublet_strengths,double aoa,int num_points,vector3_t* points,vector3_t* velocities,int surface)
{
panel_local_basis_t basis;
vector3_t freestream=vector3(-cos(aoa),sin(aoa),0);
	for(int j=0;j<mesh->num_panels;j++)
	{
	velocities[j]=freestream;
	}
	for(int i=0;i<mesh->num_panels;i++)
	{
	mesh_get_panel_local_basis(mesh,i,&basis);
		for(int j=0;j<num_points;j++)
		{
		vector3_t source,doublet;
		mesh_get_panel_velocity_influence(&basis,points[j],&source,&doublet);
			//Source velocities are discontinuous on the panel surface, so use the outer limit
			if(surface&&i==j)source=vector3_scale(mesh->normals[j],0.5);
		velocities[j]=vector3_add(velocities[j],vector3_add(vector3_scale(source,source_strengths[i]),vector3_scale(doublet,doublet_strengths[i])));
		}
	}

	for(int i=0;i<mesh->num_wake_strips;i++)
	{
		for(int j=0;j<mesh->num_wake_strip_panels;j++)
		{
		mesh_get_panel_local_basis(mesh,mesh->num_panels+j+i*mesh->num_wake_strip_panels,&basis);
			for(int k=0;k<num_points;k++)
			{
			vector3_t source,doublet;
			mesh_get_panel_velocity_influence(&basis,points[k],&source,&doublet);
			velocities[k]=vector3_add(velocities[k],vector3_scale(doublet,doublet_strengths[mesh->num_panels+j]));
			}
		}
	}

}

void mesh_compute_velocity(mesh_t* mesh,double* source_strengths,double* doublet_strengths,double aoa,int num_points,vector3_t* points,vector3_t* velocities)
{
mesh_compute_velocities_general(mesh,source_strengths,doublet_strengths,aoa,num_points,points,velocities,0);
}

void mesh_compute_surface_velocity(mesh_t* mesh,double* source_strengths,double* doublet_strengths,double aoa,vector3_t* velocities)
{
mesh_compute_velocities_general(mesh,source_strengths,doublet_strengths,aoa,mesh->num_panels,mesh->collocation_points,velocities,1);
}
