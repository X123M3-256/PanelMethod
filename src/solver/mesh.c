#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<assert.h>
#include<lapacke.h>
#include "mesh.h"


const double epsilon=0.0001;

double quadrilateral_area(vector2_t* p)
{
return 0.5*fabs(p[0].x*p[1].y+p[1].x*p[2].y+p[2].x*p[3].y+p[3].x*p[0].y-p[1].x*p[0].y-p[2].x*p[1].y-p[3].x*p[2].y-p[0].x*p[3].y);
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
	for(int i=0;i<mesh->num_panels;i++)
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

int mesh_init(mesh_t* mesh,int num_vertices,int num_panels,vector3_t* vertices,panel_t* panels,int num_wake_segments,int num_wake_vertices,int* wake_vertices,segment_t* wake_segments,int* wake_upper_panels,int* wake_lower_panels,int wake_max_length)
{
mesh->num_vertices=num_vertices;
mesh->num_panels=num_panels;
mesh->vertices=realloc(vertices,(num_vertices+wake_max_length*num_wake_vertices)*sizeof(vector3_t));
mesh->panels=panels;

mesh->wake.max_length=wake_max_length;
mesh->wake.length=0;
mesh->wake.num_segments=num_wake_segments;
mesh->wake.num_vertices=num_wake_vertices;
mesh->wake.vertices=wake_vertices;
mesh->wake.segments=wake_segments;
mesh->wake.upper_panels=wake_upper_panels;
mesh->wake.lower_panels=wake_lower_panels;
mesh->wake.doublet_strengths=calloc(wake_max_length*num_wake_segments,sizeof(double));

//Compute derived quantities
mesh->areas=calloc(num_panels,sizeof(double));
mesh->diameters=calloc(num_panels,sizeof(double));
mesh->collocation_points=calloc(num_panels,sizeof(vector3_t));
mesh->normals=calloc(num_panels,sizeof(vector3_t));
mesh_update_panel_data(mesh,0,mesh->num_panels);
}


/*Influence computation routines*/


//Compute the local coordinate system for a polygon defined by three or points. Assumes that num_points and center have been set by the caller.
void compute_panel_local_basis(vector3_t* vertices,panel_local_basis_t* basis)
{
//Get local coordinate system in which the panel lies in the XY plane
basis->local_x=vector3_normalize(vector3_sub(vertices[1],vertices[0]));
basis->local_z=vector3_normalize(vector3_cross(vector3_sub(vertices[2],vertices[0]),basis->local_x));//TODO consider using stored normal here
basis->local_y=vector3_cross(basis->local_z,basis->local_x);

//Transform panel vertices into local coordinate system
	for(int i=0;i<4;i++)
	{
	vector3_t diff=vector3_sub(vertices[i],basis->center);
	basis->p[i]=vector2(vector3_dot(basis->local_x,diff),vector3_dot(basis->local_y,diff));
	}

//Compute intermediate values that depend only on the panel
	for(int i=0;i<basis->num_points;i++)
	{
	int j=(i+3)&3;
	basis->d[i]=vector2_norm(vector2_sub(basis->p[i],basis->p[j]));
	basis->m[i]=(basis->p[i].y-basis->p[j].y)/(basis->p[i].x-basis->p[j].x);
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


void mesh_get_panel_local_basis(mesh_t* mesh,int panel,panel_local_basis_t* basis)
{
basis->num_points=(mesh->panels[panel].vertices[2]==mesh->panels[panel].vertices[3])?3:4;
basis->center=mesh->collocation_points[panel];
basis->diameter=mesh->diameters[panel];
basis->area=mesh->areas[panel];
vector3_t points[4];
	for(int i=0;i<4;i++)points[i]=mesh->vertices[mesh->panels[panel].vertices[i]];
compute_panel_local_basis(points,basis);
}


panel_t mesh_get_wake_panel(mesh_t* mesh,int row,int i)
{
panel_t panel;
	if(row==0)
	{
	panel.vertices[0]=mesh->wake.vertices[mesh->wake.segments[i].vertices[0]];
	panel.vertices[1]=mesh->wake.vertices[mesh->wake.segments[i].vertices[1]];
	}
	else
	{
	panel.vertices[0]=mesh->num_vertices+mesh->wake.segments[i].vertices[0]+(row-1)*mesh->wake.num_vertices;
	panel.vertices[1]=mesh->num_vertices+mesh->wake.segments[i].vertices[1]+(row-1)*mesh->wake.num_vertices;
	}

panel.vertices[2]=mesh->num_vertices+mesh->wake.segments[i].vertices[1]+row*mesh->wake.num_vertices;
panel.vertices[3]=mesh->num_vertices+mesh->wake.segments[i].vertices[0]+row*mesh->wake.num_vertices;
return panel;
}

void mesh_get_wake_panel_local_bases(mesh_t* mesh,int row,int i,panel_local_basis_t* bases)
{
panel_t panel=mesh_get_wake_panel(mesh,row,i);
const int tri_vertices[2][4]={{0,1,2,2},{2,3,0,0}};
	for(int tri=0;tri<2;tri++)
	{
	panel_local_basis_t* basis=bases+tri;
	basis->num_points=3;
	vector3_t points[4];
		for(int i=0;i<4;i++)points[i]=mesh->vertices[panel.vertices[tri_vertices[tri][i]]];
	basis->center=vector3_scale(vector3_add(vector3_add(points[0],points[1]),points[2]),1.0/3.0);
	compute_panel_local_basis(points,basis);
	//TODO consider using triangle formula
	basis->area=quadrilateral_area(basis->p);
	basis->diameter=quadrilateral_diameter(basis->p);
	}
}

typedef struct
{
double r[4];
double e[4];
double h[4];
double logs[4];
}influence_intermediates_t;

void mesh_get_panel_influence_intermediates(panel_local_basis_t* basis,vector3_t point,influence_intermediates_t* intermediates,int doublet_only)
{
	for(int i=0;i<4;i++)
	{
	intermediates->r[i]=vector3_norm(vector3(point.x-basis->p[i].x,point.y-basis->p[i].y,point.z));
	intermediates->e[i]=(point.x-basis->p[i].x)*(point.x-basis->p[i].x)+point.z*point.z;
	intermediates->h[i]=(point.x-basis->p[i].x)*(point.y-basis->p[i].y);
	}

//logs are only required for computation of source influence
	if(doublet_only)return;

	for(int i=0;i<basis->num_points;i++)
	{
	int j=(3+i)&3;
	intermediates->logs[i]=log((intermediates->r[j]+intermediates->r[i]+basis->d[i])/(intermediates->r[j]+intermediates->r[i]-basis->d[i]));
		if(isnan(intermediates->logs[i]))printf("Warning\n");
	}
}

void mesh_get_panel_influence(panel_local_basis_t* basis,vector3_t source_point,double* source_influence,double* doublet_influence)
{
vector3_t diff=vector3_sub(source_point,basis->center);
double r=vector3_norm(diff);


	if(r>3.0*basis->diameter)
	{
	//Far field approximation
	*source_influence=-basis->area/(4*M_PI*r);
	*doublet_influence=vector3_dot(basis->local_z,diff)*(*source_influence)/(r*r);
	return;
	}

vector3_t point=vector3(vector3_dot(basis->local_x,diff),vector3_dot(basis->local_y,diff),vector3_dot(basis->local_z,diff));

influence_intermediates_t intermediates;
mesh_get_panel_influence_intermediates(basis,point,&intermediates,0);

//Page 245 Katz&Plotkin
double A=0.0;
double B=0.0;
	for(int i=0;i<basis->num_points;i++)
	{
	int j=(3+i)&3;
	A+=intermediates.logs[i]*((point.x-basis->p[j].x)*(basis->p[i].y-basis->p[j].y)-(point.y-basis->p[j].y)*(basis->p[i].x-basis->p[j].x))/basis->d[i];
		if(fabs(point.z)>epsilon)B+=atan((basis->m[i]*intermediates.e[j]-intermediates.h[j])/(point.z*intermediates.r[j]))-atan((basis->m[i]*intermediates.e[i]-intermediates.h[i])/(point.z*intermediates.r[i]));
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

	if(r>3.0*b->diameter)
	{
	//Far field approximation
	double factor=b->area/(4*M_PI*r*r*r);
	source=vector3_scale(point,factor);
	factor/=r*r;
	doublet=vector3_scale(vector3(3*point.x*point.z,3*point.y*point.z,-point.x*point.x-point.y*point.y+2*point.z*point.z),factor);
	}
	else
	{
	influence_intermediates_t im;
	mesh_get_panel_influence_intermediates(b,point,&im,0);

		for(int i=0;i<b->num_points;i++)
		{
		int j=(3+i)&3;
		source.x+=im.logs[i]*(b->p[j].y-b->p[i].y)/b->d[i];
		source.y+=im.logs[i]*(b->p[i].x-b->p[j].x)/b->d[i];
		source.z+=(atan((b->m[i]*im.e[j]-im.h[j])/(point.z*im.r[j]))
			  -atan((b->m[i]*im.e[i]-im.h[i])/(point.z*im.r[i])));
		double doublet_factor=(im.r[i]+im.r[j])/(im.r[j]*im.r[i]*(im.r[j]*im.r[i]+((point.x-b->p[j].x)*(point.x-b->p[i].x)+(point.y-b->p[j].y)*(point.y-b->p[i].y)+point.z*point.z)));
		doublet.x+=point.z*(b->p[j].y-b->p[i].y)*doublet_factor;
		doublet.y+=point.z*(b->p[i].x-b->p[j].x)*doublet_factor;
		doublet.z+=((point.x-b->p[i].x)*(point.y-b->p[j].y)-(point.x-b->p[j].x)*(point.y-b->p[i].y))*doublet_factor;
		}
	source=vector3_scale(source,1.0/(4.0*M_PI));
	doublet=vector3_scale(doublet,1.0/(4.0*M_PI));
	}

*source_influence=vector3_add(vector3_add(vector3_scale(b->local_x,source.x),vector3_scale(b->local_y,source.y)),vector3_scale(b->local_z,source.z));
*doublet_influence=vector3_add(vector3_add(vector3_scale(b->local_x,doublet.x),vector3_scale(b->local_y,doublet.y)),vector3_scale(b->local_z,doublet.z));
}


double mesh_get_wake_panel_influence(panel_local_basis_t* bases,vector3_t source_point)
{
//Sum contribution from each triangle
double total_potential=0.0;
const int tri_vertices[2][3]={{0,1,2},{2,3,0}};
const int corner_vertex[2]={1,3};
	for(int tri=0;tri<2;tri++)
	{
	panel_local_basis_t* basis=bases+tri;
	
	vector3_t diff=vector3_sub(source_point,basis->center);
	double r=vector3_norm(diff);
		//Far field approximation
		if(r>3.0*basis->diameter)
		{
		total_potential-=basis->area*vector3_dot(basis->local_z,diff)/(4*M_PI*r*r*r);
		continue;	
		}
	
	vector3_t point=vector3(vector3_dot(basis->local_x,diff),vector3_dot(basis->local_y,diff),vector3_dot(basis->local_z,diff));
		if(fabs(point.z)<epsilon)continue;
	
	influence_intermediates_t intermediates;
	mesh_get_panel_influence_intermediates(basis,point,&intermediates,1);

	double potential=0.0;
		for(int i=0;i<3;i++)
		{
		int j=(2+i)%3;
		double test=atan((basis->m[i]*intermediates.e[j]-intermediates.h[j])/(point.z*intermediates.r[j]))-atan((basis->m[i]*intermediates.e[i]-intermediates.h[i])/(point.z*intermediates.r[i]));
		potential+=test;
		}

	total_potential-=potential/(4*M_PI);
	}
return total_potential;
}

vector3_t vortex_segment_velocity(vector3_t x0,vector3_t x1,vector3_t point)
{
vector3_t r0=vector3_sub(x1,x0);
vector3_t r1=vector3_sub(point,x0);
vector3_t r2=vector3_sub(point,x1);
vector3_t cross=vector3_cross(r1,r2);
double r1_norm=vector3_norm(r1);
double r2_norm=vector3_norm(r2);
double denom=4.0*M_PI*vector3_dot(cross,cross);

//Check if point lies on vortex line as solution is singular there
	if(r1_norm<epsilon||r2_norm<epsilon||denom<epsilon)return vector3(0,0,0);

double k=(vector3_dot(r0,r1)/r1_norm-vector3_dot(r0,r2)/r2_norm)/denom;
return vector3_scale(cross,k);
}

vector3_t mesh_get_wake_panel_velocity_influence(mesh_t* mesh,int row,int i,vector3_t point)
{
panel_t panel=mesh_get_wake_panel(mesh,row,i);
vector3_t total=vector3(0,0,0);
	for(int i=0;i<4;i++)
	{
	total=vector3_add(total,vortex_segment_velocity(mesh->vertices[panel.vertices[(i+1)%4]],mesh->vertices[panel.vertices[i]],point));
	}
return total;
}



