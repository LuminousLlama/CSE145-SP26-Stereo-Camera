// RayFile.c

#include "RayFile.h"

#include <linux/cdev.h>

///////////////////////////////////////////////////////////
// <1> linux file

RAY_BUILD_BUG_ON( sizeof(RAY_cdev_t) < sizeof(struct cdev) );

