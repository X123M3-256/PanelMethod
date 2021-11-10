#include<stdlib.h>
#include<stdio.h>
#include "render.h"

struct
{
	struct
	{
	}object_shader;
}
resources;

char* read_file(const char* filename)
{
FILE* file;
file=fopen(filename,"rb");
    if(file==NULL)return NULL;
fseek(file,0,SEEK_END);
long length=ftell(file);
fseek(file,0,SEEK_SET);
char* buf=malloc(length+1);
fread(buf,length,1,file);
fclose(file);
buf[length]=0;
return buf;
}


GLuint shader_compile(GLenum type,const char* filename)
{
GLuint shader=glCreateShader(type);
char* source=read_file(filename);
	if(source==NULL)
	{
	printf("Could not read shader file\n");
	exit(0);
	}
glShaderSource(shader,1,&source,NULL);
glCompileShader(shader);
free(source);
char error[1024];
glGetShaderInfoLog(shader,1023,NULL,error);
printf("version %s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));
printf("error %s\n",error);
return shader;
}

//TODO check for error
int shader_load_from_files(shader_t* shader,const char* vertex_filename,const char* fragment_filename)
{
shader->program=glCreateProgram();
GLuint vertex_shader=shader_compile(GL_VERTEX_SHADER,vertex_filename);
GLuint fragment_shader=shader_compile(GL_FRAGMENT_SHADER,fragment_filename);
glAttachShader(shader->program,vertex_shader);
glAttachShader(shader->program,fragment_shader);
glLinkProgram(shader->program);

shader->mvp_loc=glGetUniformLocation(shader->program,"mvp");
shader->transform_loc=glGetUniformLocation(shader->program,"transform");
shader->tex_loc=glGetUniformLocation(shader->program,"tex");
return 0;
}



int render_init()
{
glEnable(GL_DEPTH_TEST); 
glEnable(GL_CULL_FACE);
return 0;
}

void object_init(object_t* object,int num_vertices,int num_indices,int tex_width,int tex_height,float* vertex_data,int* index_data,char* texdata)
{
glGenBuffers(1,&(object->vbo));
glGenBuffers(1,&(object->ibo));
glGenTextures(1,&(object->tex));

object->num_vertices=num_vertices;
object->num_indices=num_indices;

//Send buffer data to graphics card

glBindBuffer(GL_ARRAY_BUFFER,object->vbo);
glBufferData(GL_ARRAY_BUFFER,8*num_vertices*sizeof(float),vertex_data,GL_STATIC_DRAW);

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,object->ibo);
glBufferData(GL_ELEMENT_ARRAY_BUFFER,num_indices*sizeof(GLuint),index_data,GL_STATIC_DRAW);

//Send texture data to graphics card
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D,object->tex);
glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,tex_width,tex_height,0,GL_BGRA,GL_UNSIGNED_BYTE,texdata); 

//Set up VAOs
glGenVertexArrays(1, &(object->vao));  
glBindVertexArray(object->vao);
glBindBuffer(GL_ARRAY_BUFFER,object->vbo);
glEnableVertexAttribArray(0);
glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
glEnableVertexAttribArray(1);
glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)12);
glEnableVertexAttribArray(2);
glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)24);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,object->ibo);
glBindVertexArray(0);
}

void object_render(object_t* object,matrix_t projection,matrix_t camera,matrix_t transform,shader_t* shader)
{
glUseProgram(shader->program);
glBindVertexArray(object->vao);

matrix_t mvp=matrix_mult(projection,camera);
float transform_entries[16];
float mvp_entries[16];
	for(int i=0;i<16;i++)
	{
	transform_entries[i]=transform.entries[i];
	mvp_entries[i]=mvp.entries[i];
	}

//Bind textures
glBindTexture(GL_TEXTURE_2D,object->tex);
glUniform1i(shader->tex_loc,0);

//Assign uniforms
glUniformMatrix4fv(shader->mvp_loc,1,GL_TRUE,mvp_entries);
glUniformMatrix4fv(shader->transform_loc,1,GL_TRUE,transform_entries);

//Render patch
glDrawElements(GL_TRIANGLES,object->num_indices,GL_UNSIGNED_INT,0);
glBindVertexArray(0);
}

void object_destroy(object_t* object)
{
//TODO implement this
}
