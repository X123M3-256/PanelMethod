#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<lapacke.h>
#include "airfoil.h"


double hermite_spline(double x0,double y0,double x1,double y1,double m0,double m1,double x)
	{
	double t=(x-x0)/(x1-x0);
	double b0=t*t*(2*t-3)+1;
	double b1=t*t*(-2*t+3);
	double b2=t*(t*(t-2)+1);
	double b3=t*t*(t-1);
	return y0*b0+y1*b1+(m0*b2+m1*b3)*(x1-x0);
	}

double hermite_spline_derivative(double x0,double y0,double x1,double y1,double m0,double m1,double x)
	{
	double t=(x-x0)/(x1-x0);
	double b0=6*t*(t-1);
	double b1=6*t*(1-t);
	double b2=t*(3*t-4)+1;
	double b3=t*(3*t-2);
	return (y0*b0+y1*b1)/(x1-x0)+(m0*b2+m1*b3);
	}


void compute_spline_gradients(int num_splines,double* x_points,double* y_points,double* gradients)
{
double matrix[(AIRFOIL_MAX_SPLINES+1)*(AIRFOIL_MAX_SPLINES+1)];
double* rhs=gradients;
int ipiv[AIRFOIL_MAX_SPLINES+1];

int n=num_splines+1;

memset(matrix,0,n*n*sizeof(double));
memset(rhs,0,n*sizeof(double));

matrix[0+n*0]=2;
matrix[0+n*1]=1;
rhs[0]=3*(y_points[1]-y_points[0])/(x_points[1]-x_points[0]);


//matrix[(n-1)+n*(n-1)]=1;
//rhs[n-1]=0;

//Zero curvature
matrix[(n-1)+n*(n-1)]=2;
matrix[(n-1)+n*(n-2)]=1;
rhs[n-1]=3*(y_points[n-1]-y_points[n-2])/(x_points[n-1]-x_points[n-2]);

#define SQR(x) ((x)*(x))
	for(int i=1;i<n-1;i++)
	{
	double left=x_points[i]-x_points[i-1];	
	double right=x_points[i+1]-x_points[i];	
	double a=left*SQR(right);
	double b=SQR(left)*right;
	double c=3*(y_points[i-1]*SQR(right)+y_points[i]*(SQR(left)-SQR(right))-y_points[i+1]*SQR(left));
	double d=2*left*right*(left+right);
	matrix[i+n*(i-1)]=a;
	matrix[i+n*i]=d;
	matrix[i+n*(i+1)]=b;
	rhs[i]=-c;
	}

/*
	for(int i=0;i<n;i++)
	{
	for(int j=0;j<n;j++)
	{
	printf("%.2f\t",matrix[i+j*n]);
	}
	putchar('\n');
	}
	putchar('\n');
	putchar('\n');

	for(int j=0;j<n;j++)
	{
	printf("%.2f\n",rhs[j]);
	}
*/	

	if(LAPACKE_dgesv(LAPACK_COL_MAJOR,n,1,matrix,n,ipiv,rhs,n)!=0)
	{
	printf("Solution failed\n");
	}
}

void airfoil_update_gradients(airfoil_t* airfoil)
{
compute_spline_gradients(airfoil->num_splines,airfoil->x_points,airfoil->centerline,airfoil->centerline_gradient);
compute_spline_gradients(airfoil->num_splines,airfoil->x_points,airfoil->thickness,airfoil->thickness_gradient);
}

vector2_t airfoil_get_control_point(airfoil_t* airfoil,int point,int layer)
{
vector2_t centre_point=vector2(airfoil->x_points[point],airfoil->centerline[point]);
	if(layer==AIRFOIL_LAYER_CENTRE)return centre_point;
vector2_t normal=vector2_normalize(vector2(-airfoil->centerline_gradient[point],1));
			if(layer==AIRFOIL_LAYER_UPPER)return vector2_add(centre_point,vector2_scale(normal,airfoil->thickness[point]));
			else return vector2_sub(centre_point,vector2_scale(normal,airfoil->thickness[point]));


}

double airfoil_get_centerline(airfoil_t* airfoil,double x)
{
	for(int i=0;i<airfoil->num_splines;i++)
	{
		if(airfoil->x_points[i+1]>=x)
		{
		return hermite_spline(airfoil->x_points[i],airfoil->centerline[i],airfoil->x_points[i+1],airfoil->centerline[i+1],airfoil->centerline_gradient[i],airfoil->centerline_gradient[i+1],x);
		}
	};
return 0.0;
}

double airfoil_get_centerline_gradient(airfoil_t* airfoil,double x)
{
	for(int i=0;i<airfoil->num_splines;i++)
	{
		if(airfoil->x_points[i+1]>=x)
		{
		return hermite_spline_derivative(airfoil->x_points[i],airfoil->centerline[i],airfoil->x_points[i+1],airfoil->centerline[i+1],airfoil->centerline_gradient[i],airfoil->centerline_gradient[i+1],x);
		}
	};
return 0.0;
}

double airfoil_get_thickness(airfoil_t* airfoil,double x)
{
	for(int i=0;i<airfoil->num_splines;i++)
	{
		if(airfoil->x_points[i+1]>=x)
		{
		return hermite_spline(airfoil->x_points[i],airfoil->thickness[i],airfoil->x_points[i+1],airfoil->thickness[i+1],airfoil->thickness_gradient[i],airfoil->thickness_gradient[i+1],x);
		}
	};
return 0.0;
}

vector2_t airfoil_point(airfoil_t* airfoil,double x,int layer)
{
	for(int i=0;i<airfoil->num_splines;i++)
	{
		if(airfoil->x_points[i+1]>=x)
		{
		double centerline=hermite_spline(airfoil->x_points[i],airfoil->centerline[i],airfoil->x_points[i+1],airfoil->centerline[i+1],airfoil->centerline_gradient[i],airfoil->centerline_gradient[i+1],x);
			if(layer==AIRFOIL_LAYER_CENTRE)return vector2(x,centerline);

		double thickness=hermite_spline(airfoil->x_points[i],airfoil->thickness[i],airfoil->x_points[i+1],airfoil->thickness[i+1],airfoil->thickness_gradient[i],airfoil->thickness_gradient[i+1],x);
		
			if(i==airfoil->num_splines-1)
			{
			//double midpoint=0.5*(airfoil->x_points[i]+airfoil->x_points[i+1]);
			//double u=(x-midpoint)/(airfoil->x_points[i+1]-midpoint);
			double u=(x-airfoil->x_points[i])/(airfoil->x_points[i+1]-airfoil->x_points[i]);
				if(u>=0)thickness*=sqrt(1-u*u);
		
			}
		vector2_t normal=vector2_normalize(vector2(-hermite_spline_derivative(airfoil->x_points[i],airfoil->centerline[i],airfoil->x_points[i+1],airfoil->centerline[i+1],airfoil->centerline_gradient[i],airfoil->centerline_gradient[i+1],x),1));	
			if(layer==AIRFOIL_LAYER_LOWER)return vector2_sub(vector2(x,centerline),vector2_scale(normal,thickness));
			else return vector2_add(vector2(x,centerline),vector2_scale(normal,thickness));
		}
	}
return vector2(0,0);
}

