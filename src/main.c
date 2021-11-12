#include<stdlib.h>
#include<stdio.h>
#include<GL/gl.h>
#include<gtk/gtk.h>
#include<math.h>
#include "graphics/mesh.h"
#include "graphics/section.h"
#include "solver/panel.h"
#include "geometry/wing.h"

vector3_t vertices[8]={{-1,-1,-1},{1,-1,-1},{-1,1,-1},{1,1,-1},{-1,-1,1},{1,-1,1},{-1,1,1},{1,1,1}};
//vector3_t vertices[8]={{-0.5,-0.5,-0.5},{0.5,-0.5,-0.5},{-0.5,0.5,-0.5},{0.5,0.5,-0.5},{-0.5,-0.5,0.5},{0.5,-0.5,0.5},{-0.5,0.5,0.5},{0.5,0.5,0.5}};
panel_t panels[6]={{{0,1,3,2}},{{4,6,7,5}},{{1,5,7,3}},{{4,0,2,6}},{{0,4,5,1}},{{2,3,7,6}}};
mesh_t mesh={8,6,NULL,NULL,NULL,NULL,NULL,NULL};


shader_t object_shader;
shader_t section_shader;

object_t box;
section_t section;
section_t grid;

wing_t wing;

double aoa=0.0;
double* source_strengths;
double* doublet_strengths;
double* panel_pressure;


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

void vector_plot_func(int num_points,vector3_t* source_points,void* closure,vector3_t* output)
{
plot_data_t* data=(plot_data_t*)closure;
//Transform points so that the freestream is horizontal
double c=cos(aoa);
double s=sin(aoa);
vector3_t* points=calloc(num_points,sizeof(vector3_t));
	for(int j=0;j<num_points;j++)
	{
	points[j]=vector3(c*source_points[j].x+s*source_points[j].y,-s*source_points[j].x+c*source_points[j].y,source_points[j].z);
	}

panel_local_basis_t basis;
	for(int j=0;j<num_points;j++)
	{
	output[j]=vector3(-cos(aoa),sin(aoa),0);
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
free(points);

//Transform results back
	for(int j=0;j<num_points;j++)
	{
	output[j]=vector3(c*output[j].x-s*output[j].y,s*output[j].x+c*output[j].y,output[j].z);
	}
}


void on_realize (GtkGLArea *area)
{
gtk_gl_area_make_current (area);
  if(gtk_gl_area_get_error(area)!= NULL)return;

glEnable(GL_DEPTH_TEST); 
glEnable(GL_CULL_FACE);

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
double dist=5.0;

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
	dist/=1.2;
	}
	else if(event->scroll.direction==GDK_SCROLL_DOWN)
	{
	dist*=1.2;
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

matrix_t modelview=matrix_mult(matrix_translate(vector3(0.0,0.0,dist)),matrix_mult(matrix_rotate_x(-pitch*3.1415926),matrix_rotate_y(-yaw*3.1415926)));

object_render(&box,projection,camera,matrix_mult(modelview,matrix_rotate_z(aoa)),&object_shader);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);  
object_render(&(section.gl_object),projection,camera,modelview,&section_shader);
object_render(&(grid.gl_object),projection,camera,modelview,&section_shader);
glDisable(GL_BLEND);
return TRUE;
}

void update_panel_solution()
{
mesh_solve(&mesh,source_strengths,doublet_strengths,aoa);

vector3_t* panel_velocities=calloc(mesh.num_panels,sizeof(vector3_t));
mesh_get_panel_velocities(&mesh,source_strengths,doublet_strengths,aoa,panel_velocities);
	for(int i=0;i<mesh.num_panels;i++)panel_pressure[i]=scalar_plot_func(panel_velocities[i],NULL);
free(panel_velocities);
mesh_update_render_object(&mesh,&box,panel_pressure,NULL);
section_update(&section);
}

gboolean on_aoa_changed(GtkRange* self,GtkScrollType* scroll,gdouble value,gpointer user_data)
{
	if(value>30.0)value=30.0;
	if(value<-30.0)value=-30.0;
aoa=M_PI*value/180.0;
update_panel_solution();
gtk_widget_queue_draw(GTK_WIDGET(user_data));
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

//Initialize airfoil
wing.airfoil.num_splines=2;
wing.airfoil.x_points[0]=0;
wing.airfoil.x_points[1]=0.75;
wing.airfoil.x_points[2]=1;
wing.airfoil.centerline[0]=0;
wing.airfoil.centerline[1]=0;
wing.airfoil.centerline[2]=0;
wing.airfoil.thickness[0]=0;
wing.airfoil.thickness[1]=0.08;
wing.airfoil.thickness[2]=0.08;
airfoil_update_gradients(&(wing.airfoil));

wing.num_segments=1;
wing.segment_offset[0]=vector3(0,0,0);
wing.segment_offset[1]=vector3(0,0,1);
wing.segment_offset[2]=vector3(0,0,2);
wing.segment_chord[0]=1;
wing.segment_chord[1]=0.5;
wing.segment_chord[2]=0.25;

//mesh_init(&mesh,8,6,vertices,panels);
wing_init_mesh(&wing,&mesh,10,10);

source_strengths=calloc(mesh.num_panels,sizeof(double));
doublet_strengths=calloc(mesh.num_panels,sizeof(double));
panel_pressure=calloc(mesh.num_panels,sizeof(double));

aoa=0;

update_panel_solution();




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
