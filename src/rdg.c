#include <rdg.h>
#include <rkv/rkv.h>
#include <rkv/rkv_id.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE_ON 0
#define TRACE_ENTRY trace( __func__, "entry" )
#define TRACE_EXIT  trace( __func__, "exit"  )

typedef struct {
   byte red;
   byte green;
   byte blue;
} rdg_color;

typedef struct {
   rdg_color   color;
   double      x;
   double      y;
} rdg_point;

typedef struct {
   rdg_color   color;
   double      center_x;
   double      center_y;
   double      radius;
} rdg_circle;

typedef struct {
   rdg_color color;
   uint32_t  vertex_count;
   double *  vertices;
} rdg_polygon;

static const char * THE_TRANSACTION = "main";

static bool color_encode( const rdg_color * color, net_buff buffer ) {
   return net_buff_encode_byte( buffer, color->red )
      &&  net_buff_encode_byte( buffer, color->green )
      &&  net_buff_encode_byte( buffer, color->blue );
}

static bool trace( const char * func, const char * where ) {
   if( TRACE_ON ) {
      fprintf( stderr, "%s: %s\n", func, where );
   }
   return true;
}

static bool color_decode( net_buff buffer, rdg_color * color ) {
   TRACE_ENTRY;
   return net_buff_decode_byte( buffer, &color->red )
      &&  net_buff_decode_byte( buffer, &color->green )
      &&  net_buff_decode_byte( buffer, &color->blue )
      &&  TRACE_EXIT;
}

static bool point_encode( net_buff buffer, const void * src, utils_map codecs ) {
   TRACE_ENTRY;
   const rdg_point * point = src;
   return color_encode( &point->color, buffer )
      &&  net_buff_encode_double( buffer, point->x )
      &&  net_buff_encode_double( buffer, point->y )
      &&  TRACE_EXIT;
   (void)codecs;
}

static bool point_decode( void * dest, net_buff buffer, utils_map codecs ) {
   TRACE_ENTRY;
   rdg_point ** point_address = dest;
   rdg_point *  point         = *point_address;
   if( point == NULL ) {
      point = malloc( sizeof( rdg_point ));
      if( point == NULL ) {
         perror( "malloc" );
         TRACE_EXIT;
         return false;
      }
      memset( point, 0, sizeof( rdg_point ));
      *point_address = point;
   }
   return color_decode( buffer, &point->color )
      &&  net_buff_decode_double( buffer, &point->x )
      &&  net_buff_decode_double( buffer, &point->y )
      &&  TRACE_EXIT;
   (void)codecs;
}

static void point_releaser( void * data, utils_map codecs ) {
   TRACE_ENTRY;
   free( data);
   TRACE_EXIT;
   (void)codecs;
}

static bool circle_encode( net_buff buffer, const void * src, utils_map codecs ) {
   TRACE_ENTRY;
   const rdg_circle * circle = src;
   return color_encode( &circle->color, buffer )
      &&  net_buff_encode_double( buffer, circle->center_x )
      &&  net_buff_encode_double( buffer, circle->center_y )
      &&  net_buff_encode_double( buffer, circle->radius   )
      &&  TRACE_EXIT;
   (void)codecs;
}

static bool circle_decode( void * dest, net_buff buffer, utils_map codecs ) {
   TRACE_ENTRY;
   rdg_circle ** circle_address = dest;
   rdg_circle *  circle         = *circle_address;
   if( circle == NULL ) {
      circle = malloc( sizeof( rdg_circle ));
      if( circle == NULL ) {
         perror( "malloc" );
         TRACE_EXIT;
         return false;
      }
      memset( circle, 0, sizeof( rdg_circle ));
      *circle_address = circle;
   }
   return color_decode( buffer, &circle->color )
      &&  net_buff_decode_double( buffer, &circle->center_x )
      &&  net_buff_decode_double( buffer, &circle->center_y )
      &&  net_buff_decode_double( buffer, &circle->radius )
      && TRACE_EXIT;
   (void)codecs;
}

static void circle_releaser( void * data, utils_map codecs ) {
   TRACE_ENTRY;
   free( data);
   TRACE_EXIT;
   (void)codecs;
}

static bool polygon_encode( net_buff buffer, const void * src, utils_map codecs ) {
   TRACE_ENTRY;
   const rdg_polygon * polygon = src;
   if( ! color_encode( &polygon->color, buffer )) {
      TRACE_EXIT;
      return false;
   }
   if( ! net_buff_encode_uint32( buffer, (uint32_t)polygon->vertex_count )) {
      TRACE_EXIT;
      return false;
   }
   for( size_t i = 0; i < 2 * polygon->vertex_count; ++i ) {
      if( ! net_buff_encode_double( buffer, polygon->vertices[i] )) {
         TRACE_EXIT;
         return false;
      }
   }
   TRACE_EXIT;
   return true;
   (void)codecs;
}

static bool polygon_decode( void * dest, net_buff buffer, utils_map codecs ) {
   TRACE_ENTRY;
   rdg_polygon ** polygon_address = (rdg_polygon **)dest;
   rdg_polygon *  polygon         = *polygon_address;
   if( polygon == NULL ) {
      polygon = malloc( sizeof( rdg_polygon ));
      if( polygon == NULL ) {
         perror( "malloc" );
         TRACE_EXIT;
         return false;
      }
      memset( polygon, 0, sizeof( rdg_polygon ));
      *polygon_address = polygon;
   }
   if( ! color_decode( buffer, &polygon->color )) {
      TRACE_EXIT;
      return false;
   }
   if( ! net_buff_decode_uint32( buffer, &polygon->vertex_count )) {
      TRACE_EXIT;
      return false;
   }
   if( polygon->vertices == NULL ) {
      polygon->vertices = malloc( 2 * polygon->vertex_count * sizeof( double ));
      if( polygon->vertices == NULL ) {
         perror( "malloc" );
         TRACE_EXIT;
         return false;
      }
   }
   for( size_t i = 0; i < 2 * polygon->vertex_count; ++i ) {
      if( ! net_buff_decode_double( buffer, polygon->vertices + i )) {
         TRACE_EXIT;
         return false;
      }
   }
   TRACE_EXIT;
   return true;
   (void)codecs;
}

static void polygon_releaser( void * data, utils_map codecs ) {
   TRACE_ENTRY;
   rdg_polygon * polygon = (rdg_polygon *)data;
   free( polygon->vertices );
   free( polygon );
   TRACE_EXIT;
   (void)codecs;
}

static rdg_change_callback rdg_listener;

static void change_listener( rkv cache, void * user_context ) {
   if( rkv_refresh( cache )) {
      if( rdg_listener ) {
         (*rdg_listener)( user_context );
      }
   }
}

static rkv db;

DLL_PUBLIC bool rdg_new( const char * group, unsigned short port, rdg_change_callback listener, void * user_context ) {
   rkv_codec point_codec = {
      rdg_POINT,
      point_encode,
      point_decode,
      point_releaser
   };
   rkv_codec circle_codec = {
      rdg_CIRCLE,
      circle_encode,
      circle_decode,
      circle_releaser
   };
   rkv_codec polygon_codec = {
      rdg_POLYGON,
      polygon_encode,
      polygon_decode,
      polygon_releaser
   };
   const rkv_codec * const codecs[] = { &point_codec, &circle_codec, &polygon_codec };
   rdg_listener = listener;
   return rkv_new( &db, group, port, codecs, sizeof(codecs)/sizeof(codecs[0] ))
      &&  rkv_add_listener( db, change_listener, user_context );
}

DLL_PUBLIC bool rdg_add_point( byte red, byte green, byte blue, double x, double y, rkv_id * id ) {
   if( db == NULL ) {
      fprintf( stderr, "%s: call rdg_new first!", __func__ );
      return false;
   }
   rdg_point * point = malloc( sizeof( rdg_point ));
   memset( point, 0, sizeof( rdg_point ));
   point->color.red   = red;
   point->color.green = green;
   point->color.blue  = blue;
   point->x           = x;
   point->y           = y;
   if( rkv_id_new( id ) && rkv_put( db, THE_TRANSACTION, *id, rdg_POINT, point )) {
      return rkv_publish( db, THE_TRANSACTION );
   }
   return false;
}

DLL_PUBLIC bool rdg_add_circle( byte red, byte green, byte blue, double center_x, double center_y, double radius, rkv_id * id ) {
   if( db == NULL ) {
      fprintf( stderr, "%s: call rdg_new first!", __func__ );
      return false;
   }
   rdg_circle * circle = malloc( sizeof( rdg_circle ));
   memset( circle, 0, sizeof( rdg_circle ));
   circle->color.red   = red;
   circle->color.green = green;
   circle->color.blue  = blue;
   circle->center_x    = center_x;
   circle->center_y    = center_y;
   circle->radius      = radius;
   if( rkv_id_new( id ) && rkv_put( db, THE_TRANSACTION, *id, rdg_CIRCLE, circle )) {
      return rkv_publish( db, THE_TRANSACTION );
   }
   return false;
}

DLL_PUBLIC bool rdg_add_polygon( byte red, byte green, byte blue, uint32_t vertex_count, const double * vertices, rkv_id * id ) {
   if( db == NULL ) {
      fprintf( stderr, "%s: call rdg_new first!", __func__ );
      return false;
   }
   rdg_polygon * polygon = malloc( sizeof( rdg_polygon ));
   memset( polygon, 0, sizeof( rdg_polygon ));
   size_t length = 2 * vertex_count * sizeof( double );
   polygon->color.red    = red;
   polygon->color.green  = green;
   polygon->color.blue   = blue;
   polygon->vertex_count = vertex_count;
   polygon->vertices     = malloc( length );
   memcpy( polygon->vertices, vertices, length );
   if( rkv_id_new( id ) && rkv_put( db, THE_TRANSACTION, *id, rdg_POLYGON, polygon )) {
      return rkv_publish( db, THE_TRANSACTION );
   }
   return false;
}

typedef struct {
   rdg_point_iterator   point_iterator;
   rdg_circle_iterator  circle_iterator;
   rdg_polygon_iterator polygon_iterator;
   void *               user_context;
} rdg_iterator_cb_context;

static bool rdg_iterator_cb( size_t index, const rkv_id id, unsigned type, const void * data, void * user_context ) {
   rdg_iterator_cb_context * ctxt    = user_context;
   const rdg_point *         point   = data;
   const rdg_circle *        circle  = data;
   const rdg_polygon *       polygon = data;
   switch( type ) {
   case rdg_POINT:
      return ctxt->point_iterator(
         point->color.red, point->color.green, point->color.blue,
         point->x, point->y,
         ctxt->user_context );
   case rdg_CIRCLE:
      return ctxt->circle_iterator(
         circle->color.red, circle->color.green, circle->color.blue,
         circle->center_x, circle->center_y, circle->radius,
         ctxt->user_context );
   case rdg_POLYGON:
      return ctxt->polygon_iterator(
         polygon->color.red, polygon->color.green, polygon->color.blue,
         polygon->vertex_count, polygon->vertices,
         ctxt->user_context );
   default:
      return false;
   }
   (void)index;
   (void)id;
}

DLL_PUBLIC bool rdg_get_shapes( rdg_point_iterator pnt, rdg_circle_iterator crcl, rdg_polygon_iterator plgn, void * user_context ) {
   if( db == NULL ) {
      fprintf( stderr, "%s: call rdg_new first!", __func__ );
      return false;
   }
   if(( pnt == NULL )||( crcl == NULL )||( plgn == NULL )) {
      fprintf( stderr, "%s: iterator can't be nul", __func__ );
      return false;
   }
   rdg_iterator_cb_context iterator_context = {
      .point_iterator   = pnt,
      .circle_iterator  = crcl,
      .polygon_iterator = plgn,
      .user_context     = user_context
   };
   return rkv_foreach( db, rdg_iterator_cb, &iterator_context );
}

DLL_PUBLIC void rdg_delete( void ) {
   rkv_delete( &db );
}
