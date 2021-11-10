#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<assert.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include<lapacke.h>
#include "panel.h"

vector3_t mesh_get_panel_vertex(mesh_t* mesh,int panel,int vertex)
{
return mesh->panel_vertices[vertex+4*panel];
}

vector3_t mesh_get_panel_normal(mesh_t* mesh,int panel)
{
return mesh->panel_normals[panel];
}

vector3_t mesh_get_panel_collocation_point(mesh_t* mesh,int panel)
{
return vector3_scale(vector3_add(vector3_add(mesh_get_panel_vertex(mesh,panel,0),mesh_get_panel_vertex(mesh,panel,1)),vector3_add(mesh_get_panel_vertex(mesh,panel,2),mesh_get_panel_vertex(mesh,panel,3))),0.25); 
}

void mesh_update_panel_data(mesh_t* mesh)
{
//Update panel vertices
	for(int i=0;i<mesh->num_panels;i++)
	for(int j=0;j<4;j++)
	{
	mesh->panel_vertices[j+4*i]=mesh->vertices[mesh->panels[i].vertices[j]];
	}
//Update panel normals
	for(int i=0;i<mesh->num_panels;i++)
	{
	mesh->panel_normals[i]=vector3_normalize(vector3_cross(vector3_sub(mesh_get_panel_vertex(mesh,i,2),mesh_get_panel_vertex(mesh,i,0)),vector3_sub(mesh_get_panel_vertex(mesh,i,1),mesh_get_panel_vertex(mesh,i,0))));
	}
}

int mesh_load_from_file(mesh_t* mesh,const char* filename)
{
const struct aiScene* scene = aiImportFile(filename,aiProcess_FlipWindingOrder|aiProcess_JoinIdenticalVertices|aiProcess_FindDegenerates);

	if(!scene)
	{
	printf("Importing file \"%s\" failed with error: %s\n",filename,aiGetErrorString());
	return 1;
	}

//Count vertices and faces in scene
mesh->num_vertices=0;
mesh->num_panels=0;

	for(uint32_t j=0;j<scene->mNumMeshes;j++)
	{
	mesh->num_vertices+=scene->mMeshes[j]->mNumVertices;
	mesh->num_panels+=scene->mMeshes[j]->mNumFaces;
	}

//TODO detect if UVs are not used and do not load them
mesh->vertices=malloc(mesh->num_vertices*sizeof(vector3_t));
mesh->panels=malloc(mesh->num_panels*sizeof(panel_t));

uint32_t mesh_start_vertex=0;	
uint32_t mesh_start_face=0;
	
	for(uint32_t j=0;j<scene->mNumMeshes;j++)
	{
	const struct aiMesh* aimesh=scene->mMeshes[j];
	printf("Mesh vertices %d faces %d\n",aimesh->mNumVertices,aimesh->mNumFaces);
	
		for(uint32_t i=0;i<aimesh->mNumVertices;i++)
		{
		mesh->vertices[mesh_start_vertex+i]=vector3(aimesh->mVertices[i].x,aimesh->mVertices[i].y,aimesh->mVertices[i].z);
		}

		for(uint32_t i=0;i<aimesh->mNumFaces;i++)
		{
		assert(aimesh->mFaces[i].mNumIndices==3||aimesh->mFaces[i].mNumIndices==4);
			for(uint32_t j=0;j<aimesh->mFaces[i].mNumIndices;j++)
			{
				if(i==2)printf("Load Index %d\n",mesh_start_vertex+aimesh->mFaces[i].mIndices[j]);
			mesh->panels[mesh_start_face+i].vertices[j]=mesh_start_vertex+aimesh->mFaces[i].mIndices[j];
			//	if(i==2)mesh->panels[mesh_start_face+i].vertices[j]=0;//esh_start_vertex+aimesh->mFaces[i].mIndices[j];
			}
		//Treat triangles as degenerate quads
			if(aimesh->mFaces[i].mNumIndices==3)mesh->panels[mesh_start_face+i].vertices[3]=mesh_start_vertex+aimesh->mFaces[i].mIndices[2];
		}
	mesh_start_vertex+=aimesh->mNumVertices;
	mesh_start_face+=aimesh->mNumFaces;
	}

aiReleaseImport(scene);
mesh->panel_vertices=calloc(4*mesh->num_panels,sizeof(vector3_t));
mesh->panel_normals=calloc(mesh->num_panels,sizeof(vector3_t));
mesh_update_panel_data(mesh);
}


/*Influence computation routines*/

void mesh_get_panel_local_basis(mesh_t* mesh,int panel,panel_local_basis_t* basis)
{
basis->num_points=(mesh->panels[panel].vertices[2]==mesh->panels[panel].vertices[3])?3:4;
//Get local coordinate system in which the panel lies in the XY plane
basis->center=mesh_get_panel_collocation_point(mesh,panel);
basis->local_x=vector3_normalize(vector3_sub(mesh_get_panel_vertex(mesh,panel,1),mesh_get_panel_vertex(mesh,panel,0)));
basis->local_z=mesh_get_panel_normal(mesh,panel);
basis->local_y=vector3_cross(basis->local_z,basis->local_x);

//Transform panel vertices into local coordinate system

	for(int i=0;i<4;i++)
	{
	vector3_t diff=vector3_sub(mesh_get_panel_vertex(mesh,panel,i),basis->center);
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
vector3_t point=vector3(vector3_dot(b->local_x,diff),vector3_dot(b->local_y,diff),vector3_dot(b->local_z,diff));

influence_intermediates_t im;
mesh_get_panel_influence_intermediates(b,point,&im);

double doublet_factors[4];
	for(int i=0;i<b->num_points;i++)
	{
	int j=(3+i)&3;
	doublet_factors[i]=(im.r[i]+im.r[j])/(im.r[j]*im.r[i]*(im.r[j]*im.r[i]+((point.x-b->p[j].x)*(point.x-b->p[i].x)+(point.y-b->p[j].y)*(point.y-b->p[i].y)+point.z*point.z)));
	}

vector3_t source=vector3(0,0,0);
vector3_t doublet=vector3(0,0,0);
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



*source_influence=vector3_add(vector3_add(vector3_scale(b->local_x,source.x),vector3_scale(b->local_y,source.y)),vector3_scale(b->local_z,source.z));
*doublet_influence=vector3_add(vector3_add(vector3_scale(b->local_x,doublet.x),vector3_scale(b->local_y,doublet.y)),vector3_scale(b->local_z,doublet.z));
}


/*Solver code*/

void mesh_solve(mesh_t* mesh,double* source_strengths,double* doublet_strengths)
{
double* matrix=calloc(mesh->num_panels*mesh->num_panels,sizeof(double));
double* rhs=doublet_strengths;
int* ipiv=calloc(mesh->num_panels,sizeof(int));

	for(int i=0;i<mesh->num_panels;i++)
	{
	vector3_t normal=mesh_get_panel_normal(mesh,i);
	source_strengths[i]=normal.x;
	}

panel_local_basis_t basis;
	for(int i=0;i<mesh->num_panels;i++)
	{
	mesh_get_panel_local_basis(mesh,i,&basis);
		for(int j=0;j<mesh->num_panels;j++)
		{
		double source=0.0;
		double doublet=0.0;
		vector3_t collocation_point=mesh_get_panel_collocation_point(mesh,j);
		mesh_get_panel_influence(&basis,collocation_point,&source,&doublet);
		matrix[j+i*mesh->num_panels]=doublet;

		rhs[j]-=source_strengths[i]*source;
		}

	
	matrix[i+i*mesh->num_panels]=0.5;
	}

	if(LAPACKE_dgesv(LAPACK_COL_MAJOR,mesh->num_panels,1,matrix,mesh->num_panels,ipiv,rhs,mesh->num_panels)!=0)
	{
	printf("Solution failed\n");
	}

free(matrix);
}

void mesh_get_panel_velocities(mesh_t* mesh,double* source_strengths,double* doublet_strengths,vector3_t* velocities)
{
panel_local_basis_t basis;
	for(int j=0;j<mesh->num_panels;j++)
	{
	velocities[j]=vector3(-1,0,0);
	}
	for(int i=0;i<mesh->num_panels;i++)
	{
	mesh_get_panel_local_basis(mesh,i,&basis);
		for(int j=0;j<mesh->num_panels;j++)
		{
		vector3_t source,doublet;
		mesh_get_panel_velocity_influence(&basis,mesh_get_panel_collocation_point(mesh,j),&source,&doublet);
			//Source velocities are discontinuous on the panel surface, so use the outer limit
			if(i==j)source=vector3_scale(mesh_get_panel_normal(mesh,j),0.5);
		velocities[j]=vector3_add(velocities[j],vector3_add(vector3_scale(source,source_strengths[i]),vector3_scale(doublet,doublet_strengths[i])));
		}
	}
}


