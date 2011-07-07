#include <stdio.h>
#include <stdint.h>

typedef unsigned char byte;

enum wkb_geometry {   
    wkb_point = 1,
    wkb_linestring = 2,
    wkb_polygon = 3,
    wkb_multipoint = 4,
    wkb_multilinestring = 5,
    wkb_multipolygon = 6,
    wkb_geometry_collection = 7
};

typedef struct {
    double x;
    double y;
} pt;

typedef struct {
    uint32_t     num_pts;
    pt          *points;
} wkb_linear_ring_t;

enum wkb_endianness {
   wkbXDR = 0,          // big endian
   wkbNDR = 1           // little endian
};

typedef struct {
    byte        byte_order;
    uint32_t    wkb_type;                       // 1
    pt          point;
} wkb_point_t;

typedef struct {
    byte        byte_order;
    uint32_t    wkb_type;                       // 2
    uint32_t    num_pts;
    pt          *points;
} wkb_linestring_t;

typedef struct {
    byte                byte_order;
    uint32_t            wkb_type;               // 3
    uint32_t            num_rings;
    wkb_linear_ring_t   *rings;
} wkb_polygon_t;

typedef struct {
    byte            byte_order;
    uint32_t        wkb_type;                       // 4
    uint32_t        num_wkb_points;
    wkb_point_t     *wkb_points;
} wkb_multipoint_t;

typedef struct {
    byte                byte_order;
    uint32_t            wkb_type;                       // 5
    uint32_t            num_wkb_linestrings;
    wkb_linestring_t    *wkb_linestrings;
} wkb_multilinestring_t;

typedef struct {
    byte            byte_order;                                            
    uint32_t        wkb_type;                       // 6
    uint32_t        num_wkb_polygons;
    wkb_polygon_t   *wkb_polygons;
} wkb_multipolygon_t;

typedef union {
    pt                              point;
    wkb_linestring_t                linestring;
    wkb_polygon_t                   polygon;    
    /* wkb_geometry_collection_t    *geometry_collection; */
    wkb_multipoint_t                multipoint;
    wkb_multilinestring_t           multilinestring;
    wkb_multipolygon_t              multipolygon;
} wkb_geometry_t;

typedef struct {
    byte                byte_order;
    uint32_t            wkb_type;                       // 7
    uint32_t            num_wkb_geometries;
    wkb_geometry_t      *wkb_geometries;
} wkb_geometry_collection_t;

int test_defs () {
    printf("size of byte: %ld\n", sizeof(byte));
    printf("size of int: %ld\n", sizeof(int));
    printf("size of uint32_t: %ld\n", sizeof(int));
    printf("size of double: %ld\n", sizeof(double));
    printf("size of wkb_point: %ld\n", sizeof(wkb_point_t));
    printf("size of linestring:%ld\n", sizeof(wkb_linestring_t));
    printf("size of polygon: %ld\n", sizeof(wkb_polygon_t));
    printf("size of multipoint: %ld\n", sizeof(wkb_multipoint_t));
    printf("size of multilinestring: %ld\n", sizeof(wkb_multilinestring_t));
    printf("size of multipolygon: %ld\n", sizeof(wkb_multipolygon_t));
    printf("size of geometry type: %ld\n", sizeof(wkb_geometry_t));
    printf("size of geometry collection: %ld\n", sizeof(wkb_geometry_collection_t));
    printf("size of linear ring: %ld\n", sizeof(wkb_linear_ring_t));
    printf("size of point: %ld\n", sizeof(pt));
    pt test_point1;
    test_point1.x = 123.4;
    test_point1.y = 123.4;
    wkb_point_t tpt1;
    tpt1.byte_order = 0;
    tpt1.wkb_type = 1;
    tpt1.point = test_point1;
    printf("sample lon point: %f \n", tpt1.point.x);
    return 0;
}

int main () {
    test_defs();
    return 0;
}
