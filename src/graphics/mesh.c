#include<stdlib.h>
#include<math.h>
#include "mesh.h"


float texcoord(float x)
{
return 0.5+x/6.0;
}


int mesh_index_to_object_index(mesh_t* mesh,int i)
{
	if(i<mesh->num_vertices)return i;
return i+mesh->num_panels;
}

float* mesh_get_vertices(mesh_t* mesh,double* panel_values_x,double* panel_values_y)
{
float* vertices=calloc(8*(mesh->num_panels+mesh->num_vertices),sizeof(float));
int* vertex_panel_counts=calloc(8*(mesh->num_panels+mesh->num_vertices),sizeof(int));

//Write vertices
	for(int i=0;i<mesh->num_vertices;i++)
	{
	vertices[8*i]=mesh->vertices[i].x;
	vertices[8*i+1]=mesh->vertices[i].y;
	vertices[8*i+2]=mesh->vertices[i].z;
	}
//Write vertex normals
	for(int i=0;i<mesh->num_panels;i++)
	{
	vector3_t normal=mesh->normals[i];	
	vector3_t collocation_point=mesh->collocation_points[i];
	vertices[8*(mesh->num_vertices+i)]=  collocation_point.x;
	vertices[8*(mesh->num_vertices+i)+1]=collocation_point.y;
	vertices[8*(mesh->num_vertices+i)+2]=collocation_point.z;
	vertices[8*(mesh->num_vertices+i)+3]=normal.x;
	vertices[8*(mesh->num_vertices+i)+4]=normal.y;
	vertices[8*(mesh->num_vertices+i)+5]=normal.z;
	vertices[8*(mesh->num_vertices+i)+6]=texcoord(panel_values_x!=NULL?panel_values_x[i]:0.5);
	vertices[8*(mesh->num_vertices+i)+7]=texcoord(panel_values_y!=NULL?panel_values_y[i]:0.5);

		for(int j=0;j<4;j++)
		{
		int vertex_index=mesh->panels[i].vertices[j];
		vertices[8*vertex_index+3]+=normal.x;
		vertices[8*vertex_index+4]+=normal.y;
		vertices[8*vertex_index+5]+=normal.z;
		vertices[8*vertex_index+6]+=texcoord(panel_values_x!=NULL?panel_values_x[i]:0.0);
		vertices[8*vertex_index+7]+=texcoord(panel_values_y!=NULL?panel_values_y[i]:0.0);
		vertex_panel_counts[vertex_index]++;
		}
	}
//Normalize vertex normals
	for(int i=0;i<mesh->num_vertices;i++)
	{
	float norm=sqrt(vertices[8*i+3]*vertices[8*i+3]+vertices[8*i+4]*vertices[8*i+4]+vertices[8*i+5]*vertices[8*i+5]);
	vertices[8*i+3]/=norm;
	vertices[8*i+4]/=norm;
	vertices[8*i+5]/=norm;
	vertices[8*i+6]/=vertex_panel_counts[i];
	vertices[8*i+7]/=vertex_panel_counts[i];
	}
free(vertex_panel_counts);



return vertices;
}

int* mesh_get_indices(mesh_t* mesh)
{
int* indices=calloc(12*(mesh->num_panels+mesh->num_wake_panels),sizeof(int));

//Write indices	
	for(int i=0;i<mesh->num_panels;i++)
	{
	indices[12*i+0]=mesh->panels[i].vertices[0];
	indices[12*i+1]=mesh->panels[i].vertices[1];
	indices[12*i+2]=mesh->num_vertices+i;
	indices[12*i+3]=mesh->panels[i].vertices[1];
	indices[12*i+4]=mesh->panels[i].vertices[2];
	indices[12*i+5]=mesh->num_vertices+i;
	indices[12*i+6]=mesh->panels[i].vertices[2];
	indices[12*i+7]=mesh->panels[i].vertices[3];
	indices[12*i+8]=mesh->num_vertices+i;
	indices[12*i+9]=mesh->panels[i].vertices[3];
	indices[12*i+10]=mesh->panels[i].vertices[0];
	indices[12*i+11]=mesh->num_vertices+i;
	}
return indices;
}

int mesh_init_render_object(mesh_t* mesh,object_t* object)
{
float* vertices=mesh_get_vertices(mesh,NULL,NULL);
int* indices=mesh_get_indices(mesh);

unsigned char pixels[28]={0,0,0,255, 255,0,128,255,  255,0,0,255 ,128,128,128,225, 0,0,255,255,  0,255,255,255, 255,255,255,255};

//TODO check if successful
object_init(object,mesh->num_panels+mesh->num_vertices,12*(mesh->num_panels+mesh->num_wake_panels),7,1,vertices,indices,pixels);

free(vertices);
free(indices);
}

int mesh_update_render_object(mesh_t* mesh,object_t* object,double* panel_values_x,double* panel_values_y)
{
float* vertex_data=mesh_get_vertices(mesh,panel_values_x,panel_values_y);
glBindBuffer(GL_ARRAY_BUFFER,object->vbo);
glBufferSubData(GL_ARRAY_BUFFER,0,8*(mesh->num_vertices+mesh->num_panels)*sizeof(float),vertex_data);
free(vertex_data);
return 0;
}



float* wake_get_vertices(mesh_t* mesh)
{
float* vertices=calloc(64*mesh->num_wake_panels,sizeof(float));

//Write wake vertices
	for(int i=0;i<mesh->num_wake_panels;i++)
	{
	float u[4]={0.0,1.0,1.0,0.0};
	float v[4]={0.0,0.0,1.0,1.0};
		for(int j=0;j<4;j++)
		{
		int vert_index=8*(8*i+2*j);
		vector3_t vertex=mesh->vertices[mesh->panels[mesh->num_panels+i].vertices[j]];
		vertices[vert_index]=vertex.x;
		vertices[vert_index+1]=vertex.y;
		vertices[vert_index+2]=vertex.z;
		vertices[vert_index+3]=mesh->normals[mesh->num_panels+i].x;
		vertices[vert_index+4]=mesh->normals[mesh->num_panels+i].y;
		vertices[vert_index+5]=mesh->normals[mesh->num_panels+i].z;
		vertices[vert_index+6]=0.5;//u[j];
		vertices[vert_index+7]=0.5;//v[j];
		vertices[vert_index+8]=vertex.x;
		vertices[vert_index+9]=vertex.y;
		vertices[vert_index+10]=vertex.z;
		vertices[vert_index+11]=-mesh->normals[mesh->num_panels+i].x;
		vertices[vert_index+12]=-mesh->normals[mesh->num_panels+i].y;
		vertices[vert_index+13]=-mesh->normals[mesh->num_panels+i].z;
		vertices[vert_index+14]=0.5;//u[j];
		vertices[vert_index+15]=0.5;//v[j];
		}
	}
return vertices;
}

int* wake_get_indices(mesh_t* mesh)
{
int* indices=calloc(12*mesh->num_wake_panels,sizeof(int));
	for(int i=0;i<mesh->num_wake_panels;i++)
	{
	int panel_index=12*i;
	int vertex_index=8*i;
	indices[panel_index]=vertex_index+0;
	indices[panel_index+1]=vertex_index+2;
	indices[panel_index+2]=vertex_index+4;
	indices[panel_index+3]=vertex_index+0;
	indices[panel_index+4]=vertex_index+4;
	indices[panel_index+5]=vertex_index+6;
	indices[panel_index+6]=vertex_index+1;
	indices[panel_index+7]=vertex_index+5;
	indices[panel_index+8]=vertex_index+3;
	indices[panel_index+9]=vertex_index+1;
	indices[panel_index+10]=vertex_index+7;
	indices[panel_index+11]=vertex_index+5;
	}
return indices;
}


//TODO reduce vertex duplication/smooth shading
int wake_init_render_object(mesh_t* mesh,object_t* object)
{
float* vertices=wake_get_vertices(mesh);
int* indices=wake_get_indices(mesh);

//TODO make actual texture
unsigned char pixels[4]={255,106,77,255};

object_init(object,8*mesh->num_wake_panels,12*mesh->num_wake_panels,1,1,vertices,indices,pixels);

free(vertices);
free(indices);
}



int wake_update_render_object(mesh_t* mesh,object_t* object)
{
float* vertex_data=wake_get_vertices(mesh);
glBindBuffer(GL_ARRAY_BUFFER,object->vbo);
glBufferSubData(GL_ARRAY_BUFFER,0,64*(mesh->num_wake_panels)*sizeof(float),vertex_data);
free(vertex_data);
return 0;
}
