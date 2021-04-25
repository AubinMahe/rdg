#include <rdg.h>
#include <tst/tests_report.h>
#include <utils/utils_time.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const double vertices[] = {
   200, 200,
   250, 150,
   300, 150,
   350, 200,
   350, 250,
   300, 300,
   250, 300,
   200, 250
};
// Attention : c'est le nombre de sommet {x, y}, pas le nombre de double !
static const uint32_t vertex_count = sizeof( vertices ) / sizeof( vertices[0] ) / 2;

static unsigned shapes_count;

static void cache_listener( void * user_context ) {
   struct tests_report * report = user_context;
   ASSERT( report, ++shapes_count < 4 );
}

static bool point_iterator( byte red, byte green, byte blue, double x, double y, void * user_context ) {
   struct tests_report * report = user_context;
   ASSERT( report, ++shapes_count == 1 );
   ASSERT( report, red   == 0xFF );
   ASSERT( report, green == 0x00 );
   ASSERT( report, blue  == 0x00 );
   ASSERT( report, x - 200 < 0.01 );
   ASSERT( report, y - 200 < 0.01 );
   return true;
}

static bool circle_iterator( byte red, byte green, byte blue, double x, double y, double r, void * user_context ) {
   struct tests_report * report = user_context;
   ASSERT( report, ++shapes_count == 2 );
   ASSERT( report, red   == 0x00 );
   ASSERT( report, green == 0xFF );
   ASSERT( report, blue  == 0x00 );
   ASSERT( report, x - 200 < 0.01 );
   ASSERT( report, y - 200 < 0.01 );
   ASSERT( report, r - 150 < 0.01 );
   return true;
}

static bool polygon_iterator( byte red, byte green, byte blue, unsigned count, const double * coordinates, void * user_context ) {
   struct tests_report * report = user_context;
   ASSERT( report, ++shapes_count == 3 );
   ASSERT( report, red   == 0x00 );
   ASSERT( report, green == 0x00 );
   ASSERT( report, blue  == 0xFF );
   ASSERT( report, count == vertex_count );
   for( unsigned i = 0, max = ( count < vertex_count ) ? count : vertex_count; i < max; ++i ) {
      ASSERT( report, ( vertices[i] - coordinates[i] ) < 0.01 );
   }
   return true;
}

static void rdg_test( struct tests_report * report ) {
   tests_chapter( report, "rdg" );
   rkv_id id;
   ASSERT( report, rdg_new( "239.0.0.66", 2416, cache_listener, report ));
   shapes_count = 0;
   ASSERT( report, rdg_add_point( 0xFF, 0, 0, 123.45, 98.76, &id ));
   ASSERT( report, rdg_add_circle( 0, 0xFF, 0, 200, 200, 150, &id ));
   ASSERT( report, rdg_add_polygon( 0, 0, 0xFF, vertex_count, vertices, &id ));
   while( shapes_count < 3 ) {
      utils_sleep( 20 ); // ms
   }
   shapes_count = 0;
   ASSERT( report, rdg_get_shapes( point_iterator, circle_iterator, polygon_iterator, report ));
   ASSERT( report, shapes_count == 3 );
   rdg_delete();
}

int main( int argc, char * argv[] ) {
   return tests_run( argc, argv,
      "rdg_test", rdg_test,
      NULL );
}
