#ifndef PORTABLE_H
#define PORTABLE_H

#include "portmacro.h"
#include "task.h"

StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters );
BaseType_t 	xPortStartScheduler( void );

#endif /* PORTABLE_H */
