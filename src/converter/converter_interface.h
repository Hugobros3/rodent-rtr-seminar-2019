/* converter_interface.h : Impala interface file generated by impala */
#ifndef CONVERTER_INTERFACE_H
#define CONVERTER_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

struct OutputNode8Tri4 {
    int nodes_count;
    struct Node8 const* nodes;
    int tris_count;
    struct Tri4 const* tris;
};

struct OutputNode2Tri1 {
    int nodes_count;
    struct Node2 const* nodes;
    int tris_count;
    struct Tri1 const* tris;
};

struct ConverterInputTriMesh {
    int const* indices;
    int tri_count;
    float const* vertices;
};

//struct OutputNode8Tri4 make_bvh8_4(struct ConverterInputTriMesh input_data);
struct OutputNode8Tri4 make_bvh8_4(int vertices_count, int const* indices, float const* vertices);

struct OutputNode2Tri1 make_bvh2_1(int vertices_count, int const* indices, float const* vertices);

#ifdef __cplusplus
}
#endif

#endif /* CONVERTER_INTERFACE_H */

