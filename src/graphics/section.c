#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<cairo/cairo.h>
#include "section.h"

#define COLOR_SCALE_MAX_COLORS 16

typedef struct
{
int num_colors;
float min_value;
float max_value;
vector3_t colors[COLOR_SCALE_MAX_COLORS];
}color_scale_t;

color_scale_t colors={7,-3,3,{{0,0,0},{0.5,0,1.0},{0,0,1.0},{0.5,0.5,0.5},{1.0,0,0},{1.0,1.0,0},{1.0,1.0,1.0}}};

vector3_t color_scale_get_color(color_scale_t* color_scale,double level)
{
double u=(color_scale->num_colors-1)*(level-color_scale->min_value)/(color_scale->max_value-color_scale->min_value);
int index=(int)floor(u);
u=u-index;
	if(index<0)return color_scale->colors[0];
	else if(index>=color_scale->num_colors-1)return color_scale->colors[color_scale->num_colors-1];
	else return vector3_add(color_scale->colors[index],vector3_scale(vector3_sub(color_scale->colors[index+1],color_scale->colors[index]),u));
}

void section_get_axes(section_t* section,vector3_t* xaxis,vector3_t* yaxis)
{
	if(section->normal.y<0.5)
	{
	*xaxis=vector3_normalize(vector3_cross(section->normal,vector3(0,1,0)));
	*yaxis=vector3_normalize(vector3_cross(*xaxis,section->normal));
	}
	else
	{
	*yaxis=vector3_normalize(vector3_cross(section->normal,vector3(1,0,0)));
	*xaxis=vector3_normalize(vector3_cross(*yaxis,section->normal));	
	}

}

void draw_arrow(cairo_t* cr,vector2_t start,vector2_t dir,float size)
{
	if(dir.x==0.0&&dir.y==0.0)return;
vector2_t n=vector2_normalize(dir);

cairo_move_to(cr,start.x-0.5*dir.x,start.y-0.5*dir.y);
cairo_line_to(cr,start.x+0.5*dir.x,start.y+0.5*dir.y);
cairo_line_to(cr,start.x+0.5*dir.x-size*(n.y+n.x),start.y+0.5*dir.y+size*(n.x-n.y));
cairo_move_to(cr,start.x+0.5*dir.x,start.y+0.5*dir.y);
cairo_line_to(cr,start.x+0.5*dir.x+size*(n.y-n.x),start.y+0.5*dir.y-size*(n.x+n.y));
}

vector3_t section_get_grid_point(section_t* section,int i,int j)
{
float u=i/(float)section->subdivisions;
float v=j/(float)section->subdivisions;
vector3_t left=vector3_add(vector3_scale(section->vertices[0],1-v),vector3_scale(section->vertices[3],v));
vector3_t right=vector3_add(vector3_scale(section->vertices[1],1-v),vector3_scale(section->vertices[2],v));
return vector3_add(vector3_scale(left,1-u),vector3_scale(right,u));
}

vector3_t* section_get_grid_points(section_t* section)
{
vector3_t* points=calloc((section->subdivisions+1)*(section->subdivisions+1),sizeof(vector3_t));
	for(int i=0;i<=section->subdivisions;i++)
	for(int j=0;j<=section->subdivisions;j++)
	{
	points[i+j*(section->subdivisions+1)]=section_get_grid_point(section,i,j);
	}
return points;
}

void section_draw_vector_array(section_t* section,vector3_t* points,vector3_t* data)
{
cairo_t* cr=section->context;

vector3_t xaxis,yaxis;
section_get_axes(section,&xaxis,&yaxis);

cairo_set_source_rgba(cr,0,0,0,1.0);
	for(int i=0;i<=section->subdivisions;i++)
	for(int j=0;j<=section->subdivisions;j++)
	{
	vector3_t point_offset=vector3_sub(points[i+j*(section->subdivisions+1)],section->center);
	vector2_t projected_point=vector2(vector3_dot(point_offset,xaxis),vector3_dot(point_offset,yaxis));
	vector2_t projected_vector=vector2(vector3_dot(data[i+j*(section->subdivisions+1)],xaxis),vector3_dot(data[i+j*(section->subdivisions+1)],yaxis));
	draw_arrow(cr,projected_point,vector2_scale(projected_vector,0.75*section->step_x),0.1*section->step_x);
	}
cairo_stroke(cr);
}

//TODO make this take points as an argument for consistency
void section_draw_scalar_array(section_t* section,double* data)
{
cairo_t* cr=section->context;
cairo_pattern_t * pattern = cairo_pattern_create_mesh();
	for(int i=0;i<section->subdivisions;i++)
	for(int j=0;j<section->subdivisions;j++)
	{
	vector3_t a_col=color_scale_get_color(&colors,data[i+j*(section->subdivisions+1)]);
	vector3_t b_col=color_scale_get_color(&colors,data[(i+1)+j*(section->subdivisions+1)]);
	vector3_t c_col=color_scale_get_color(&colors,data[(i+1)+(j+1)*(section->subdivisions+1)]);
	vector3_t d_col=color_scale_get_color(&colors,data[i+(j+1)*(section->subdivisions+1)]);

	cairo_mesh_pattern_begin_patch(pattern);
	cairo_mesh_pattern_move_to(pattern,-0.5*section->x_range+i*section->step_x,-0.5*section->y_range+j*section->step_y);
	cairo_mesh_pattern_line_to(pattern,-0.5*section->x_range+(i+1)*section->step_x,-0.5*section->y_range+j*section->step_y);
	cairo_mesh_pattern_line_to(pattern,-0.5*section->x_range+(i+1)*section->step_x,-0.5*section->y_range+(j+1)*section->step_y);
	cairo_mesh_pattern_line_to(pattern,-0.5*section->x_range+i*section->step_x,-0.5*section->y_range+(j+1)*section->step_y);
	cairo_mesh_pattern_line_to(pattern,-0.5*section->x_range+i*section->step_x,-0.5*section->y_range+j*section->step_y);
	cairo_mesh_pattern_set_corner_color_rgba(pattern, 0, a_col.x, a_col.y, a_col.z,1.0);
	cairo_mesh_pattern_set_corner_color_rgba(pattern, 1, b_col.x, b_col.y, b_col.z,1.0);
	cairo_mesh_pattern_set_corner_color_rgba(pattern, 2, c_col.x, c_col.y, c_col.z,1.0);
	cairo_mesh_pattern_set_corner_color_rgba(pattern, 3, d_col.x, d_col.y, d_col.z,1.0);
	cairo_mesh_pattern_end_patch(pattern);
	}
cairo_set_source(cr, pattern);
cairo_rectangle(cr,-0.5*section->x_range,-0.5*section->y_range,section->x_range,section->y_range);
cairo_fill(cr);
}

void section_draw_scalar_field(section_t* section,void (*scalar)(int,vector3_t*,void*,double*),void* closure)
{
vector3_t* points=section_get_grid_points(section);
double* data=calloc((section->subdivisions+1)*(section->subdivisions+1),sizeof(double));
scalar((section->subdivisions+1)*(section->subdivisions+1),points,closure,data);
section_draw_scalar_array(section,data);
free(points);
free(data);
}

void section_draw_vector_field(section_t* section,void (*vector)(int,vector3_t*,void*,vector3_t*),void* closure)
{
vector3_t* points=section_get_grid_points(section);
vector3_t* data=calloc((section->subdivisions+1)*(section->subdivisions+1),sizeof(vector3_t));
vector((section->subdivisions+1)*(section->subdivisions+1),points,closure,data);
section_draw_vector_array(section,points,data);
free(points);
free(data);
}

void section_draw_vector_field_with_scalar(section_t* section,void (*vector)(int,vector3_t*,void*,vector3_t*),double (*scalar)(vector3_t,void*),void* closure)
{
int num_points=(section->subdivisions+1)*(section->subdivisions+1);
vector3_t* points=section_get_grid_points(section);
vector3_t* vector_data=calloc(num_points,sizeof(vector3_t));
double* scalar_data=calloc(num_points,sizeof(double));
vector(num_points,points,closure,vector_data);
	for(int i=0;i<num_points;i++)scalar_data[i]=scalar(vector_data[i],closure);
section_draw_scalar_array(section,scalar_data);
section_draw_vector_array(section,points,vector_data);
free(points);
free(vector_data);
free(scalar_data);
}




void section_clear(section_t* section)
{
cairo_save(section->context);
cairo_set_source_rgba(section->context,0,0,0,0);
cairo_set_operator(section->context, CAIRO_OPERATOR_SOURCE);
cairo_paint (section->context);
cairo_restore(section->context);
}

void section_draw_grid(section_t* section)
{
cairo_t* cr=section->context;

cairo_set_source_rgba(cr,0.5,0.5,0.5,0.25);
cairo_rectangle(cr,-0.5*section->x_range,-0.5*section->y_range,section->x_range,section->y_range);
cairo_fill(cr);


cairo_set_source_rgba(cr,0.5,0.5,0.5,0.5);

float step=0.1;
int steps_x=(int)(0.5*section->x_range/step);
int steps_y=(int)(0.5*section->y_range/step);
	
	for(int i=-steps_y;i<=steps_y;i++)
	{
	cairo_move_to(cr,-0.5*section->x_range,i*step);
	cairo_line_to(cr,0.5*section->x_range,i*step);
	}
	for(int i=-steps_x;i<=steps_x;i++)
	{
	cairo_move_to(cr,i*step,-0.5*section->y_range);
	cairo_line_to(cr,i*step,0.5*section->y_range);
	}
cairo_stroke(cr);

cairo_set_source_rgba(cr,0.0,0.0,0.0,1.0);
for(int i=-steps_y/10;i<=steps_y/10;i++)
	{
	cairo_move_to(cr,-0.5*section->x_range,10*i*step);
	cairo_line_to(cr,0.5*section->x_range,10*i*step);
	}
	for(int i=-steps_x/10;i<=steps_x/10;i++)
	{
	cairo_move_to(cr,10*i*step,-0.5*section->y_range);
	cairo_line_to(cr,10*i*step,0.5*section->y_range);
	}

cairo_stroke(cr);
}


void section_update_texture(section_t* section)
{
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D,section->gl_object.tex);
int width=cairo_image_surface_get_width(section->surface);
int height=cairo_image_surface_get_height(section->surface);
unsigned char* data=cairo_image_surface_get_data(section->surface);
glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_BGRA,GL_UNSIGNED_BYTE,data); 
}

void section_get_vertex_data(section_t* section,float* vertex_data)
{
section->normal=vector3_normalize(section->normal);

vector3_t xaxis,yaxis;
section_get_axes(section,&xaxis,&yaxis);


section->vertices[0]=vector3_add(section->center,vector3_add(vector3_scale(xaxis,-0.5*section->x_range),vector3_scale(yaxis,-0.5*section->y_range)));
section->vertices[1]=vector3_add(section->center,vector3_add(vector3_scale(xaxis,0.5*section->x_range),vector3_scale(yaxis,-0.5*section->y_range)));
section->vertices[2]=vector3_add(section->center,vector3_add(vector3_scale(xaxis,0.5*section->x_range),vector3_scale(yaxis,0.5*section->y_range)));
section->vertices[3]=vector3_add(section->center,vector3_add(vector3_scale(xaxis,-0.5*section->x_range),vector3_scale(yaxis,0.5*section->y_range)));


vertex_data[0]=section->vertices[0].x;
vertex_data[1]=section->vertices[0].y;
vertex_data[2]=section->vertices[0].z;
vertex_data[3]=section->normal.x;
vertex_data[4]=section->normal.y;
vertex_data[5]=section->normal.z;
vertex_data[6]=0.0;
vertex_data[7]=0.0;
vertex_data[8]=section->vertices[1].x;
vertex_data[9]=section->vertices[1].y;
vertex_data[10]=section->vertices[1].z;
vertex_data[11]=section->normal.x;
vertex_data[12]=section->normal.y;
vertex_data[13]=section->normal.z;
vertex_data[14]=1.0;
vertex_data[15]=0.0;
vertex_data[16]=section->vertices[2].x;
vertex_data[17]=section->vertices[2].y;
vertex_data[18]=section->vertices[2].z;
vertex_data[19]=section->normal.x;
vertex_data[20]=section->normal.y;
vertex_data[21]=section->normal.z;
vertex_data[22]=1.0;
vertex_data[23]=1.0;
vertex_data[24]=section->vertices[3].x;
vertex_data[25]=section->vertices[3].y;
vertex_data[26]=section->vertices[3].z;
vertex_data[27]=section->normal.x;
vertex_data[28]=section->normal.y;
vertex_data[29]=section->normal.z;
vertex_data[30]=0.0;
vertex_data[31]=1.0;
}

void section_update_coords(section_t* section)
{
float vertex_data[32];
section_get_vertex_data(section,vertex_data);
glBindBuffer(GL_ARRAY_BUFFER,section->gl_object.vbo);
glBufferSubData(GL_ARRAY_BUFFER,0,32*sizeof(float),vertex_data);
}

int section_init(section_t* section,vector3_t center,vector3_t normal,float x_range,float y_range)
{
section->center=center;
section->normal=normal;
section->x_range=x_range;
section->y_range=y_range;
section->subdivisions=30;
section->step_x=section->x_range/section->subdivisions;
section->step_y=section->y_range/section->subdivisions;

float vertex_data[32];
int index_data[12]={0,1,2,0,2,3,1,0,3,1,3,2};
section_get_vertex_data(section,vertex_data);


int resolution=256;

int width=(int)ceil(resolution*x_range);
int height=(int)ceil(resolution*y_range);

section->surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,width,height);
section->context=cairo_create(section->surface);

cairo_set_line_width(section->context,1.0/resolution);
cairo_scale(section->context,width/section->x_range,height/section->y_range);
cairo_translate(section->context,0.5*section->x_range,0.5*section->y_range);

object_init(&(section->gl_object),4,12,width,height,vertex_data,index_data,cairo_image_surface_get_data(section->surface));
}

