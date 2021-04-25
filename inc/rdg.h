#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <rkv/rkv_id.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char byte;

typedef void (* rdg_change_callback )( void * user_context );
typedef bool (* rdg_point_iterator   )( byte red, byte green, byte blue, double x, double y, void * user_context );
typedef bool (* rdg_circle_iterator  )( byte red, byte green, byte blue, double x, double y, double r, void * user_context );
typedef bool (* rdg_polygon_iterator )( byte red, byte green, byte blue, unsigned count, const double * pts, void * user_context );

DLL_PUBLIC bool rdg_new( const char * group, unsigned short port, rdg_change_callback listener, void * user_context );
DLL_PUBLIC bool rdg_add_point( byte red, byte green, byte blue, double x, double y, rkv_id * id );
DLL_PUBLIC bool rdg_add_circle( byte red, byte green, byte blue, double center_x, double center_y, double radius, rkv_id * id );
DLL_PUBLIC bool rdg_add_polygon( byte red, byte green, byte blue, uint32_t vertex_count, const double * vertices, rkv_id * id );
DLL_PUBLIC bool rdg_get_shapes( rdg_point_iterator pts, rdg_circle_iterator circles, rdg_polygon_iterator polys, void * user_context );
DLL_PUBLIC void rdg_delete( void );

typedef enum {
   rdg_POINT = 1,
   rdg_CIRCLE,
   rdg_POLYGON
} rdg_shape_type;

#ifdef __cplusplus
}
#endif
