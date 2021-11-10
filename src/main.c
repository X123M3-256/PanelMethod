#include<stdlib.h>
#include<stdio.h>
#include<GL/gl.h>
#include<gtk/gtk.h>
#include<math.h>
#include "graphics/render.h"
#include "graphics/section.h"
#include "solver/panel.h"

vector3_t panel_normals[6];
vector3_t panel_vertices[24];
//vector3_t vertices[8]={{-1,-1,-1},{1,-1,-1},{-1,1,-1},{1,1,-1},{-1,-1,1},{1,-1,1},{-1,1,1},{1,1,1}};
vector3_t vertices[8]={{-0.5,-0.5,-0.5},{0.5,-0.5,-0.5},{-0.5,0.5,-0.5},{0.5,0.5,-0.5},{-0.5,-0.5,0.5},{0.5,-0.5,0.5},{-0.5,0.5,0.5},{0.5,0.5,0.5}};
panel_t panels[6]={{{0,1,3,2}},{{4,6,7,5}},{{1,5,7,3}},{{4,0,2,6}},{{0,4,5,1}},{{2,3,7,6}}};
mesh_t mesh={8,6,vertices,panels,panel_vertices,panel_normals};


shader_t object_shader;
shader_t section_shader;

object_t box;
section_t section;
section_t grid;


double* source_strengths;
double* doublet_strengths;

typedef struct
{
mesh_t* mesh;
double* source_strengths;
double* doublet_strengths;
}plot_data_t;

plot_data_t plot_data;

double scalar_plot_func(vector3_t point,void* data)
{
return 1.0-vector3_dot(point,point);
}

void vector_plot_func(int num_points,vector3_t* points,void* closure,vector3_t* output)
{
plot_data_t* data=(plot_data_t*)closure;
panel_local_basis_t basis;
	for(int j=0;j<num_points;j++)
	{
	output[j]=vector3(1,0,0);
	}
	for(int i=0;i<data->mesh->num_panels;i++)
	{
	mesh_get_panel_local_basis(data->mesh,i,&basis);
		for(int j=0;j<num_points;j++)
		{
		vector3_t source,doublet;
		mesh_get_panel_velocity_influence(&basis,points[j],&source,&doublet);
		output[j]=vector3_add(output[j],vector3_add(vector3_scale(source,data->source_strengths[i]),vector3_scale(doublet,data->doublet_strengths[i])));
		}
	}
}

/*
void scalar_plot_func(int num_points,vector3_t* points,void* data,double* output)
{
panel_local_basis_t basis;
	for(int j=0;j<num_points;j++)
	{
	output[j]=points[j].x;
	}
	for(int i=0;i<mesh.num_panels;i++)
	{
	mesh_get_panel_local_basis(&mesh,i,&basis);
		for(int j=0;j<num_points;j++)
		{
		double source=0.0;
		double doublet=0.0;
		mesh_get_panel_influence(&basis,points[j],&source,&doublet);
		output[j]+=source_strengths[i]*source+doublet_strengths[i]*doublet;
		}
	}
return;
}

void vector_plot_func_numeric(int num_points,vector3_t* points,void* data,vector3_t* output)
{
panel_local_basis_t basis;
//Create sample point arrays
double h=0.001;
vector3_t* top=calloc(num_points,sizeof(vector3_t));
vector3_t* bottom=calloc(num_points,sizeof(vector3_t));
vector3_t* left=calloc(num_points,sizeof(vector3_t));
vector3_t* right=calloc(num_points,sizeof(vector3_t));
vector3_t* front=calloc(num_points,sizeof(vector3_t));
vector3_t* back=calloc(num_points,sizeof(vector3_t));

double* top_output=calloc(num_points,sizeof(double));
double* bottom_output=calloc(num_points,sizeof(double));
double* left_output=calloc(num_points,sizeof(double));
double* right_output=calloc(num_points,sizeof(double));
double* front_output=calloc(num_points,sizeof(double));
double* back_output=calloc(num_points,sizeof(double));

	for(int j=0;j<num_points;j++)
	{
	top[j]=vector3_add(points[j],vector3(0,h,0));
	bottom[j]=vector3_add(points[j],vector3(0,-h,0));
	left[j]=vector3_add(points[j],vector3(-h,0,0));
	right[j]=vector3_add(points[j],vector3(h,0,0));
	front[j]=vector3_add(points[j],vector3(0,0,h));
	back[j]=vector3_add(points[j],vector3(0,0,-h));
	}

scalar_plot_func(num_points,top,NULL,top_output);
scalar_plot_func(num_points,bottom,NULL,bottom_output);
scalar_plot_func(num_points,left,NULL,left_output);
scalar_plot_func(num_points,right,NULL,right_output);
scalar_plot_func(num_points,front,NULL,front_output);
scalar_plot_func(num_points,back,NULL,back_output);
vector_plot_func(num_points,points,NULL,output);

	for(int j=0;j<num_points;j++)
	{
	//output[j]=vector3((right_output[j]-left_output[j])/(2.0*h),(top_output[j]-bottom_output[j])/(2.0*h),(front_output[j]-back_output[j])/(2.0*h));
	output[j]=vector3_sub(output[j],vector3((right_output[j]-left_output[j])/(2.0*h),(top_output[j]-bottom_output[j])/(2.0*h),(front_output[j]-back_output[j])/(2.0*h)));
	//output[j].x-=1;
	}

free(top);
free(bottom);
free(left);
free(right);
free(front);
free(back);
free(top_output);
free(bottom_output);
free(left_output);
free(right_output);
free(front_output);
free(back_output);
return;
}
*/

void on_realize (GtkGLArea *area)
{
gtk_gl_area_make_current (area);
  if(gtk_gl_area_get_error(area)!= NULL)return;

render_init();

	if(shader_load_from_files(&object_shader,"shaders/object/vertex.glsl","shaders/object/fragment.glsl"))
	{
	printf("Failed loading object shader\n");
	exit(0);//TODO exit cleanly
	}
	if(shader_load_from_files(&section_shader,"shaders/section/vertex.glsl","shaders/section/fragment.glsl"))
	{
	printf("Failed loading object shader\n");
	exit(0);//TODO exit cleanly
	}

//  if (error != NULL)
//    {
//      gtk_gl_area_set_error (area, error);
//      g_error_free (error);
//      return;
//    }

mesh_init_render_object(&mesh,&box);


source_strengths=calloc(mesh.num_panels,sizeof(double));
doublet_strengths=calloc(mesh.num_panels,sizeof(double));

mesh_solve(&mesh,source_strengths,doublet_strengths);

double* panel_pressure=calloc(mesh.num_panels,sizeof(double));
vector3_t* panel_velocities=calloc(mesh.num_panels,sizeof(vector3_t));
mesh_get_panel_velocities(&mesh,source_strengths,doublet_strengths,panel_velocities);
	for(int i=0;i<mesh.num_panels;i++)panel_pressure[i]=scalar_plot_func(panel_velocities[i],NULL);

mesh_update_render_object(&mesh,&box,panel_pressure,NULL);

plot_data.mesh=&mesh;
plot_data.source_strengths=source_strengths;
plot_data.doublet_strengths=doublet_strengths;
section_init(&grid,vector3(0,0,0),vector3(0,1,0),4.0,4.0,0.25,SECTION_SHOW_GRID,NULL,NULL,NULL);
section_init(&section,vector3(0,0,0),vector3(0,0,-1),4.0,4.0,1.0,SECTION_SHOW_SCALAR|SECTION_SHOW_VECTOR,vector_plot_func,scalar_plot_func,&plot_data);
}

int drag_active=0;
int drag_start_x=0;
int drag_start_y=0;
int drag_button=0;
double drag_start_pitch=0.0;
double drag_start_yaw=0.0;
double drag_start_section_z=0.0;

double pitch=0.16666;
double yaw=-0.2;
double section_z=0.0;


gboolean on_button_press(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
	if(!drag_active)
	{
	drag_start_x=event->button.x;
	drag_start_y=event->button.y;
	drag_start_pitch=pitch;
	drag_start_yaw=yaw;
	drag_start_section_z=section_z;
	drag_button=event->button.button;
	drag_active=1;
	}
gtk_widget_queue_draw(GTK_WIDGET(widget));
}

gboolean on_button_release(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
drag_active=0;
gtk_widget_queue_draw(GTK_WIDGET(widget));
}

gboolean on_motion(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
	if(drag_active)
	{
		if(drag_button==3)
		{
		yaw=drag_start_yaw+0.002*(event->motion.x-drag_start_x);
		pitch=drag_start_pitch+0.002*(event->motion.y-drag_start_y);
		}
		else if(drag_button==1)
		{
			if(0.1*round(10*(drag_start_section_z+0.02*(event->motion.x-drag_start_x)))!=section_z)
			{
			section_z=0.1*round(10*(drag_start_section_z+0.02*(event->motion.x-drag_start_x)));
			section.center=vector3(0,0,section_z);
			section_update(&section);
			}
		}
	}
gtk_widget_queue_draw(GTK_WIDGET(widget));
}

gboolean on_scroll(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
	if(event->scroll.direction==GDK_SCROLL_UP)
	{
	
	}
	else if(event->scroll.direction==GDK_SCROLL_DOWN)
	{
	}
gtk_widget_queue_draw(GTK_WIDGET(widget));
return TRUE;  
}


gboolean render(GtkGLArea *area, GdkGLContext *context)
{
glClearColor(1,1,1,0);
glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

GtkAllocation alloc;
gtk_widget_get_allocation(GTK_WIDGET(area),&alloc); 

double fov=0.333333*3.1415926;
double aspect_ratio=alloc.width/(double)alloc.height;
double near=1.0;
double far=100.0;

matrix_t projection={{
	1/(aspect_ratio*tan(0.5*fov)),0.0f,0.0f,0.0f,
	0.0,1/tan(0.5*fov),0.0f,0.0f,
	0.0f,0.0,(near+far)/(far-near),-2*far*near/(far-near),
	0.0f,0.0f,1.0f,0.0f,
	}};
matrix_t camera=matrix_identity();

matrix_t modelview=matrix_mult(matrix_translate(vector3(0.0,0.0,5.0)),matrix_mult(matrix_rotate_x(-pitch*3.1415926),matrix_rotate_y(-yaw*3.1415926)));

object_render(&box,projection,camera,modelview,&object_shader);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);  
object_render(&(section.gl_object),projection,camera,modelview,&section_shader);
object_render(&(grid.gl_object),projection,camera,modelview,&section_shader);
glDisable(GL_BLEND);
return TRUE;
}


int main(int argc,char **argv)
{
gtk_init(&argc,&argv);

GtkBuilder* builder=gtk_builder_new();

	if(gtk_builder_add_from_file(builder,"main.glade",NULL)==0)
	{
	printf("gtk_builder_add_from_file FAILED\n");
	return 0;
	}

//mesh_update_panel_data(&mesh);

	if(mesh_load_from_file(&mesh,"wing.obj"))
	{
	printf("Failed loading mesh\n");
	return;
	}

GtkWidget* window=GTK_WIDGET(gtk_builder_get_object(builder,"window1"));
//gtk_window_fullscreen(GTK_WINDOW(window));
GtkWidget* draw_area=GTK_WIDGET(gtk_builder_get_object(builder,"draw_area"));
gtk_gl_area_set_has_depth_buffer(draw_area,TRUE);
gtk_widget_add_events(draw_area,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_SCROLL_MASK);
gtk_builder_connect_signals(builder,NULL);

gtk_widget_show_all(window);

gtk_main();
	
return 0;
}
