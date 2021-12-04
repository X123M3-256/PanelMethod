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

void section_draw_vector_array(section_t* section,cairo_t* cr,vector3_t* points,vector3_t* data)
{
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

void section_draw_scalar_array(section_t* section,cairo_t* cr,double* data)
{
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
	cairo_mesh_pattern_set_corner_color_rgba(pattern, 0, a_col.x, a_col.y, a_col.z,section->alpha);
	cairo_mesh_pattern_set_corner_color_rgba(pattern, 1, b_col.x, b_col.y, b_col.z,section->alpha);
	cairo_mesh_pattern_set_corner_color_rgba(pattern, 2, c_col.x, c_col.y, c_col.z,section->alpha);
	cairo_mesh_pattern_set_corner_color_rgba(pattern, 3, d_col.x, d_col.y, d_col.z,section->alpha);
	cairo_mesh_pattern_end_patch(pattern);
	}
cairo_set_source(cr, pattern);
cairo_rectangle(cr,-0.5*section->x_range,-0.5*section->y_range,section->x_range,section->y_range);
cairo_fill(cr);
}

void section_draw_grid(section_t* section,cairo_t* cr)
{
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

void section_update(section_t* section)
{
//TODO this would be better at the end, but section_get_vertex_data updates needed values
vector3_t xaxis,yaxis;
section->normal=vector3_normalize(section->normal);
section_get_axes(section,&xaxis,&yaxis);
section->vertices[0]=vector3_add(section->center,vector3_add(vector3_scale(xaxis,-0.5*section->x_range),vector3_scale(yaxis,-0.5*section->y_range)));
section->vertices[1]=vector3_add(section->center,vector3_add(vector3_scale(xaxis,0.5*section->x_range),vector3_scale(yaxis,-0.5*section->y_range)));
section->vertices[2]=vector3_add(section->center,vector3_add(vector3_scale(xaxis,0.5*section->x_range),vector3_scale(yaxis,0.5*section->y_range)));
section->vertices[3]=vector3_add(section->center,vector3_add(vector3_scale(xaxis,-0.5*section->x_range),vector3_scale(yaxis,0.5*section->y_range)));
section->step_x=section->x_range/section->subdivisions;
section->step_y=section->y_range/section->subdivisions;

int resolution=256;

//Set up Cairo context
int width=(int)ceil(resolution*section->x_range);
int height=(int)ceil(resolution*section->y_range);
cairo_surface_t* surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,width,height);
cairo_t* context=cairo_create(surface);

//Set transform
cairo_set_line_width(context,1.0/resolution);
cairo_scale(context,width/section->x_range,height/section->y_range);
cairo_translate(context,0.5*section->x_range,0.5*section->y_range);

//Paint surface
int num_points=0;
vector3_t* points=NULL;
vector3_t* vector_data=NULL;
double* scalar_data=NULL;
	if(section->flags&(SECTION_SHOW_SCALAR|SECTION_SHOW_VECTOR))
	{
	num_points=(section->subdivisions+1)*(section->subdivisions+1);
	points=section_get_grid_points(section);
	}
	if(section->flags&(SECTION_SHOW_VECTOR))
	{
	vector_data=calloc(num_points,sizeof(vector3_t));
	section->vector(num_points,points,section->closure,vector_data);
	}
	if(section->flags&(SECTION_SHOW_SCALAR))
	{
	scalar_data=calloc(num_points,sizeof(double));
	section->scalar(num_points,points,section->closure,scalar_data);
	}
	else if(section->flags&(SECTION_SHOW_DERIVED_SCALAR))
	{
	scalar_data=calloc(num_points,sizeof(double));
		for(int i=0;i<num_points;i++)scalar_data[i]=section->derived_scalar(vector_data[i],section->closure);
	}

	if(section->flags&(SECTION_SHOW_SCALAR|SECTION_SHOW_DERIVED_SCALAR))
	{
	section_draw_scalar_array(section,context,scalar_data);
	}
	else
	{
	cairo_set_source_rgba(context,0.5,0.5,0.5,section->alpha);
	cairo_rectangle(context,-0.5*section->x_range,-0.5*section->y_range,section->x_range,section->y_range);
	cairo_fill(context);
	}
	if(section->flags&SECTION_SHOW_GRID)section_draw_grid(section,context);
	if(section->flags&SECTION_SHOW_VECTOR)section_draw_vector_array(section,context,points,vector_data);

//Free data if it was allocated
free(points);
free(vector_data);
free(scalar_data);


//Update texture data
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D,section->gl_object.tex);
glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_BGRA,GL_UNSIGNED_BYTE,cairo_image_surface_get_data(surface)); 

//Update vertex data
float vertex_data[32]={
	section->vertices[0].x,section->vertices[0].y,section->vertices[0].z,section->normal.x,section->normal.y,section->normal.z,0.0,0.0,
	section->vertices[1].x,section->vertices[1].y,section->vertices[1].z,section->normal.x,section->normal.y,section->normal.z,1.0,0.0,
	section->vertices[2].x,section->vertices[2].y,section->vertices[2].z,section->normal.x,section->normal.y,section->normal.z,1.0,1.0,
	section->vertices[3].x,section->vertices[3].y,section->vertices[3].z,section->normal.x,section->normal.y,section->normal.z,0.0,1.0
	};
glBindBuffer(GL_ARRAY_BUFFER,section->gl_object.vbo);
glBufferSubData(GL_ARRAY_BUFFER,0,32*sizeof(float),vertex_data);
}

int section_init(section_t* section,vector3_t center,vector3_t normal,float x_range,float y_range,float alpha,int flags,void (*vector)(int,vector3_t*,void*,vector3_t*),void* scalar,void* closure)
{
//Set parameters
section->center=center;
section->normal=normal;
section->x_range=x_range;
section->y_range=y_range;
section->alpha=alpha;
section->flags=flags;
	if(flags&SECTION_SHOW_VECTOR)section->vector=vector;
	if(flags&SECTION_SHOW_SCALAR)section->scalar=scalar;
	else if(flags&SECTION_SHOW_DERIVED_SCALAR)section->derived_scalar=scalar;
section->closure=closure;
section->subdivisions=20;

//Create OpenGL object
float vertex_data[32]={0};
int index_data[12]={0,1,2,0,2,3,1,0,3,1,3,2};
unsigned char tex_data[4]={0,0,0,255};
object_init(&(section->gl_object),4,12,1,1,vertex_data,index_data,tex_data,0);

//Set everything else
section_update(section);
}

