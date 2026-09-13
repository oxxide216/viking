#ifndef VIKING_H
#define VIKING_H

#define WINX_VULKAN
#include "winx/winx.h"
#include "shl/shl-str.h"

#define vik_make_mesh(instance, data, len, indices, indices_len) \
  vik_make_mesh_sized(instance, data, len, sizeof(*data), indices, indices_len)

typedef struct VikInstance VikInstance;
typedef struct VikShader VikShader;
typedef struct VikUBO VikUBO;
typedef struct VikPipeline VikPipeline;
typedef struct VikExecutor VikExecutor;
typedef struct VikMesh VikMesh;

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

VikInstance *vik_make_instance(WinxWindow *window);
// "vf" stands for vertex/fragment and "bc" stands for bytecode
VikShader   *vik_make_shader_vf(VikInstance *instance, Str vertex_bc, Str fragment_bc);
VikUBO      *vik_make_ubo(VikInstance *instance, u32 size);
VikPipeline *vik_make_pipeline(VikInstance *instance, VikShader *shader,
                               VikAttr *attrs, u32 attrs_len,
                               VikUBO **ubos, u32 ubos_len);
VikExecutor *vik_make_executor(VikInstance *instance);
VikMesh     *vik_make_mesh_sized(VikInstance *instance, void *data,
                                 u32 len, u32 vertex_size,
                                 u32 *indices, u32 indices_len);

bool vik_begin_frame(VikExecutor *executor, VikPipeline *pipeline,
                     f32 r, f32 g, f32 b, f32 a);
bool vik_end_frame(VikExecutor *executor, VikPipeline *pipeline);

void vik_cmd_use_pipeline(VikExecutor *executor, VikPipeline *pipeline);
void vik_cmd_wait(VikExecutor *executor);
void vik_cmd_draw(VikExecutor *executor, VikMesh *mesh, u32 instances_len);

void vik_update_ubo(VikUBO *ubo, void *data);

void vik_delete_instance(VikInstance *instance);
void vik_delete_shader(VikShader *shader);
void vik_delete_ubo(VikUBO *ubo);
void vik_delete_pipeline(VikPipeline *pipeline);
void vik_delete_executor(VikExecutor *executor);
void vik_delete_mesh(VikMesh *mesh);

const char *vik_get_error_str(void);

#endif // VIKING_H
