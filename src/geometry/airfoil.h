#ifndef AIRFOIL_H_INCLUDED
#define AIRFOIL_H_INCLUDED

#define AIRFOIL_MAX_SPLINES 8
#define WING_MAX_SECTIONS 4

#include "../util/vectormath.h"

enum
{
AIRFOIL_LAYER_CENTRE=0,
AIRFOIL_LAYER_UPPER=1,
AIRFOIL_LAYER_LOWER=2,
};

typedef struct
{
int num_splines;
double x_points[AIRFOIL_MAX_SPLINES+1];
double centerline[AIRFOIL_MAX_SPLINES+1];
double thickness[AIRFOIL_MAX_SPLINES+1];
double centerline_gradient[AIRFOIL_MAX_SPLINES+1];
double thickness_gradient[AIRFOIL_MAX_SPLINES+1];
}airfoil_t;

vector2_t airfoil_point(airfoil_t* airfoil,double x,int lower);

#endif
