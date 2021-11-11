#include<stdlib.h>
#include<stdio.h>
#include<assert.h>
#include<math.h>

#include "wing.h"


double wing_compute_segment_span(wing_t* wing,int segments)
{
vector3_t span_vector=vector3_sub(wing->segment_offset[segments+1],wing->segment_offset[segments]);
return sqrt(span_vector.y*span_vector.y+span_vector.z*span_vector.z);
}

double wing_compute_span(wing_t* wing)
{
double span=0.0;
	for(int i=0;i<wing->num_segments;i++)span+=wing_compute_segment_span(wing,i);
return span;
}






float x_panel_distribution(float u)
{
return 0.5*(1-cos(M_PI*u));
}

void wing_compute_section_points(wing_t* wing,vector3_t* points,int segment,int chordwise_panels,int mirror)
{
printf("Compute section points segment %d chordpan %d mirror %d\n",segment,chordwise_panels,mirror);
	for(int i=0;i<=chordwise_panels;i++)
	{
	float u=x_panel_distribution(i/(double)chordwise_panels);
	vector2_t point=vector2_scale(airfoil_point(&(wing->airfoil),u,AIRFOIL_LAYER_UPPER),wing->segment_chord[segment]);
	points[i]=vector3_add(vector3(point.x,point.y,0),wing->segment_offset[segment]);
		if(mirror)points[i].z*=-1;
	}

	for(int i=1;i<chordwise_panels;i++)
	{
	float u=x_panel_distribution((chordwise_panels-i)/(double)chordwise_panels);
	vector2_t point=vector2_scale(airfoil_point(&(wing->airfoil),u,AIRFOIL_LAYER_LOWER),wing->segment_chord[segment]);
	points[chordwise_panels+i]=vector3_add(vector3(point.x,point.y,0),wing->segment_offset[segment]);
		if(mirror)points[chordwise_panels+i].z*=-1;
	}
}

void wing_compute_tip_points(wing_t* wing,vector3_t* points,int chordwise_panels,int mirror)
{
	for(int i=1;i<chordwise_panels;i++)
	{
	float u=x_panel_distribution(i/(double)chordwise_panels);
	vector2_t point=vector2_scale(airfoil_point(&(wing->airfoil),u,AIRFOIL_LAYER_CENTRE),wing->segment_chord[wing->num_segments]);
	points[i-1]=vector3_add(vector3(point.x,point.y,0),wing->segment_offset[wing->num_segments]);
		if(mirror)points[i-1].z*=-1;
	}

}

void wing_compute_segment_points(wing_t* wing,vector3_t* points,int segment_panels,int chordwise_panels)
{
int end_section_index=2*segment_panels*chordwise_panels;
	for(int i=1;i<segment_panels;i++)
	{
	double u=i/(double)segment_panels;
		for(int j=0;j<2*chordwise_panels;j++)
		{
		points[2*i*chordwise_panels+j]=vector3_add(vector3_scale(points[j],(1-u)),vector3_scale(points[end_section_index+j],u));
		}
	}
}

void wing_compute_segment_panels(wing_t* wing,panel_t* panels,int segment,int spanwise_panels,int chordwise_panels)
{
int section_vertices=2*chordwise_panels;

	for(int i=0;i<spanwise_panels;i++)
	{
	int spanwise_index=i*section_vertices;
	//Upper surface
		for(int j=0;j<chordwise_panels;j++)
		{
		int panel_index=j+2*i*chordwise_panels;
		panels[panel_index].vertices[0]=spanwise_index+j;
		panels[panel_index].vertices[1]=spanwise_index+j+1;
		panels[panel_index].vertices[2]=spanwise_index+section_vertices+1+j;
		panels[panel_index].vertices[3]=spanwise_index+section_vertices+j;
		}
	//Lower surface
		for(int j=0;j<chordwise_panels;j++)
		{
		int panel_index=j+(2*i+1)*chordwise_panels;
		panels[panel_index].vertices[0]=spanwise_index+chordwise_panels+j;
			if(j<chordwise_panels-1)
			{
			panels[panel_index].vertices[1]=spanwise_index+chordwise_panels+(j+1)%chordwise_panels;
			panels[panel_index].vertices[2]=spanwise_index+chordwise_panels+section_vertices+(j+1)%chordwise_panels;
			}
			else
			{
			panels[panel_index].vertices[1]=spanwise_index;
			panels[panel_index].vertices[2]=spanwise_index+section_vertices;
			}	
		panels[panel_index].vertices[3]=spanwise_index+chordwise_panels+section_vertices+j;
		}
	}


}


void set_panel(panel_t* panel,int i,int j,int k,int w,int flip)
{
panel->vertices[0]=flip?j:i;
panel->vertices[1]=flip?i:j;
panel->vertices[2]=flip?w:k;
panel->vertices[3]=flip?k:w;
}

void wing_compute_wingtip_panels(wing_t* wing,panel_t* panels,int wingtip_start,int tip_section_start,int chordwise_panels,int flip)
{

set_panel(panels,tip_section_start+1,wingtip_start,tip_section_start,tip_section_start,flip);
set_panel(panels+(chordwise_panels-1),wingtip_start+chordwise_panels-2,tip_section_start+chordwise_panels-1,tip_section_start+chordwise_panels,tip_section_start+chordwise_panels,flip);
set_panel(panels+chordwise_panels,wingtip_start,tip_section_start+2*chordwise_panels-1,tip_section_start,tip_section_start,flip);
set_panel(panels+(2*chordwise_panels-1),tip_section_start+chordwise_panels+1,wingtip_start+chordwise_panels-2,tip_section_start+chordwise_panels,tip_section_start+chordwise_panels,flip);
	for(int i=1;i<chordwise_panels-1;i++)
	{
	set_panel(panels+i,tip_section_start+i+1,wingtip_start+i,wingtip_start+i-1,tip_section_start+i,flip);
	set_panel(panels+(chordwise_panels+i),wingtip_start+i-1,wingtip_start+i,tip_section_start+2*chordwise_panels-i-1,tip_section_start+2*chordwise_panels-i,flip);
	}
}

void wing_init_mesh(wing_t* wing,mesh_t* mesh,int chordwise_panels,int spanwise_panels)
{
assert(wing->num_segments<=spanwise_panels);

int segment_panels[WING_MAX_SEGMENTS];
double total_span=wing_compute_span(wing);
double cumulative_span=0.0;
int segment_start_panel=0;
	for(int i=0;i<wing->num_segments;i++)
	{
	cumulative_span+=wing_compute_segment_span(wing,i);
	int index=(int)round(spanwise_panels*cumulative_span/total_span);
	segment_panels[i]=index-segment_start_panel;	
	segment_start_panel=index;
	}
assert(segment_start_panel==spanwise_panels);



int section_vertices=2*chordwise_panels;

int num_vertices=(2*spanwise_panels+1)*section_vertices+2*(chordwise_panels-1);
int num_panels=4*spanwise_panels*chordwise_panels+4*chordwise_panels;

vector3_t* vertices=calloc(num_vertices,sizeof(vector3_t));


//Main wing
int segment_start_vertex=0;
wing_compute_section_points(wing,vertices,wing->num_segments,chordwise_panels,1);
 
	for(int i=0;i<2*wing->num_segments;i++)
	{
	int index=i<wing->num_segments?wing->num_segments-(i+1):i-wing->num_segments;
	wing_compute_section_points(wing,vertices+segment_start_vertex+segment_panels[index]*section_vertices,i<wing->num_segments?(wing->num_segments-i-1):(i-wing->num_segments+1),chordwise_panels,i<wing->num_segments);

	wing_compute_segment_points(wing,vertices+segment_start_vertex,segment_panels[index],chordwise_panels);
	segment_start_vertex+=segment_panels[index]*section_vertices;
	}


//Wingtip points
wing_compute_tip_points(wing,vertices+(2*spanwise_panels+1)*section_vertices,chordwise_panels,1);
wing_compute_tip_points(wing,vertices+(2*spanwise_panels+1)*section_vertices+(chordwise_panels-1),chordwise_panels,0);


panel_t* panels=calloc(num_panels,sizeof(panel_t));

wing_compute_segment_panels(wing,panels,0,2*spanwise_panels,chordwise_panels);


int wingtip_start=2*(2*spanwise_panels+1)*chordwise_panels;
int tip_section_start=0;
wing_compute_wingtip_panels(wing,panels+4*spanwise_panels*chordwise_panels,wingtip_start,tip_section_start,chordwise_panels,1);

wingtip_start=2*(2*spanwise_panels+1)*chordwise_panels+(chordwise_panels-1);
tip_section_start=4*spanwise_panels*chordwise_panels;
wing_compute_wingtip_panels(wing,panels+4*spanwise_panels*chordwise_panels+2*chordwise_panels,wingtip_start,tip_section_start,chordwise_panels,0);

mesh_init(mesh,num_vertices,num_panels,vertices,panels);
}

