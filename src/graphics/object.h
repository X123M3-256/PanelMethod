#ifndef OBJECT_H_INCLUDED
#define OBJECT_H_INCLUDED
#include<GL/glew.h>
#include<GL/gl.h>

#include "../util/vectormath.h"

enum object_flags
{
OBJECT_TEXTURE_REPEAT=1,
};

typedef struct
{
GLuint program;
GLuint mvp_loc;
GLuint transform_loc;
GLuint tex_loc;
}shader_t;

typedef struct
{
int num_vertices;
int num_indices;
GLuint vao;
GLuint vbo; 
GLuint ibo;
GLuint tex;
}object_t;

int shader_load_from_files(shader_t* shader,const char* vertex_filename,const char* fragment_filename);

void object_init(object_t* object,int num_vertices,int num_indices,int tex_width,int tex_height,float* vertex_data,int* index_data,char* texdata,int flags);
void object_render(object_t* object,matrix_t projection,matrix_t camera,matrix_t transform,shader_t* shader);
void object_destroy(object_t* object);
int render_init();



#endif
