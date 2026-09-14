#ifndef VIKING_H
#define VIKING_H

#define WINX_VULKAN
#include "winx/winx.h"
#include "shl/shl-str.h"

// Before calling this, ensure that one element of your data is the whole vertex.
// Otherwise, restructure your data ot call `vik_make_mesh_sized` directly
#define vik_make_mesh(instance, data, len, indices, indices_len) \
  vik_make_mesh_sized(instance, data, len, sizeof(*data), indices, indices_len)

#define vik_make_image(instance, data, width, height) \
  vik_make_image_ex(instance, data, width, height,    \
                    VikImageFormatRGBA, VikImageFilterLinear)

typedef struct VikInstance VikInstance;
typedef struct VikShader VikShader;
typedef struct VikBuffer VikBuffer;
typedef struct VikPipeline VikPipeline;
typedef struct VikExecutor VikExecutor;
typedef struct VikMesh VikMesh;
typedef struct VikImage VikImage;

typedef enum {
  VikAttrFloat = 0,
  VikAttrVec2,
  VikAttrVec3,
  VikAttrVec4,
  VikAttrInt,
  VikAttrIVec2,
  VikAttrIVec3,
  VikAttrIVec4,
  VikAttrUInt,
  VikAttrUVec2,
  VikAttrUVec3,
  VikAttrUVec4,
} VikAttr;

typedef enum {
  VikBufferKindUBO = 0,
  VikBufferKindSSBO,
} VikBufferKind;

typedef enum {
  VikImageFormatR = 0,
  VikImageFormatRG,
  VikImageFormatRGB,
  VikImageFormatRGBA,
} VikImageFormat;

typedef enum {
  VikImageFilterLinear = 0,
  VikImageFilterNearest,
} VikImageFilter;

VikInstance *vik_make_instance(WinxWindow *window);
// "vf" stands for vertex/fragment and "bc" stands for bytecode
VikShader   *vik_make_shader_vf(VikInstance *instance, Str vertex_bc, Str fragment_bc);
VikBuffer   *vik_make_buffer(VikInstance *instance, u32 size, VikBufferKind kind);
VikPipeline *vik_make_pipeline(VikInstance *instance, VikShader *shader,
                               VikAttr *attrs, u32 attrs_len,
                               VikBuffer **buffers, u32 buffers_len,
                               VikImage **images, u32 images_len);
VikExecutor *vik_make_executor(VikInstance *instance);
VikMesh     *vik_make_mesh_sized(VikInstance *instance, void *data,
                                 u32 len, u32 vertex_size,
                                 u32 *indices, u32 indices_len);
VikImage    *vik_make_image_ex(VikInstance *instance, void *data,
                               u32 width, u32 height,
                               VikImageFormat format,
                               VikImageFilter filter);

bool vik_begin_frame(VikExecutor *executor, VikPipeline *pipeline,
                     f32 r, f32 g, f32 b, f32 a);
bool vik_end_frame(VikExecutor *executor, VikPipeline *pipeline);

void vik_cmd_use_pipeline(VikExecutor *executor, VikPipeline *pipeline);
void vik_cmd_wait_on_buffer(VikExecutor *executor, VikBuffer *buffer);
void vik_cmd_wait_on_image(VikExecutor *executor, VikImage *image);
void vik_cmd_draw(VikExecutor *executor, VikMesh *mesh, u32 instances_len);

void *vik_get_buffer_data(VikBuffer *buffer);
void  vik_update_buffer(VikBuffer *buffer, void *data);

void vik_delete_instance(VikInstance *instance);
void vik_delete_shader(VikShader *shader);
void vik_delete_buffer(VikBuffer *buffer);
void vik_delete_pipeline(VikPipeline *pipeline);
void vik_delete_executor(VikExecutor *executor);
void vik_delete_mesh(VikMesh *mesh);
void vik_delete_image(VikImage *image);

const char *vik_get_error_str(void);

#endif // VIKING_H
