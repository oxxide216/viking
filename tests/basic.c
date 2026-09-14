#include "shl/shl-defs.h"
#include "winx/event.h"
#include "viking/viking.h"
#define SHL_STR_IMPLEMENTATION
#include "shl/shl-str.h"

typedef struct {
  f32 x, y;
  f32 u, v;
} Vertex;

typedef struct {
  f32 offset_x, offset_y;
} Buffer;

static VikAttr attrs[] = {
  VikAttrVec2,
  VikAttrVec2,
};

static Vertex mesh_data[] = {
  { -0.5f,  0.5f, 0.0f, 1.0f },
  { -0.5f, -0.5f, 0.0f, 0.0f },
  {  0.5f, -0.5f, 1.0f, 0.0f },
  {  0.5f,  0.5f, 1.0f, 1.0f },
};

static u32 index_data[] = { 0, 1, 2, 2, 3, 0 };

static Buffer buffer = { 0.0, 0.0 };

static u32 image_data[] = {
  0xffff00ff, 0xff000000,
  0xff000000, 0xffff00ff,
};

Str read_file(char *path) {
  Str content;

  FILE *file = fopen(path, "r");
  if (!file)
    return (Str) { NULL, (unsigned int) -1 };

  fseek(file, 0, SEEK_END);
  content.len = ftell(file);
  content.ptr = malloc(content.len);
  fseek(file, 0, SEEK_SET);
  fread(content.ptr, 1, content.len, file);
  fclose(file);

  return content;
}

int main(void) {
  Winx *winx = winx_init();
  WinxWindow *window = winx_init_window(winx, STR_LIT("Viking"),
                                        1600, 900,
                                        WinxGraphicsModeVulkan,
                                        NULL);
  window->target_fps = winx_get_refresh_rate(window);

  Str vertex_bc = read_file("vert.spv");
  Str fragment_bc = read_file("frag.spv");

  VikInstance *instance = vik_make_instance(window, VikRequestFlagsNone);
  VikShader *shader = vik_make_shader_vf(instance, vertex_bc, fragment_bc);
  VikBuffer *ubo = vik_make_buffer(instance, sizeof(Buffer), VikBufferKindUBO);
  vik_update_buffer(ubo, &buffer);
  VikImage *image = vik_make_image_ex(instance, &image_data, 2, 2,
                                      VikImageFormatRGBA8, VikImageFilterNearest);
  VikPipeline *pipeline = vik_make_pipeline(instance, shader,
                                            attrs, ARRAY_LEN(attrs),
                                            &ubo, 1, &image, 1);
  vik_delete_shader(shader);
  VikExecutor *executor = vik_make_executor(instance);
  VikMesh *mesh = vik_make_mesh(instance,
                                mesh_data, ARRAY_LEN(mesh_data),
                                index_data, ARRAY_LEN(index_data));

  free(vertex_bc.ptr);
  free(fragment_bc.ptr);

  bool is_running = true;

  while (is_running) {
    WinxEvent event;
    while ((event = winx_get_event(window, false)).kind != WinxEventKindNone) {
      is_running = event.kind != WinxEventKindQuit;
      if (!is_running)
        break;
    }

    if (!vik_begin_frame(executor, 0.0, 0.0, 0.0, 1.0)) {
      winx_draw(window);
      continue;
    }
    vik_cmd_use_pipeline(executor, pipeline);
    vik_cmd_draw(executor, mesh, 1);
    vik_end_frame(executor);

    winx_draw(window);
  }

  vik_delete_mesh(mesh);
  vik_delete_executor(executor);
  vik_delete_pipeline(pipeline);
  vik_delete_image(image);
  vik_delete_buffer(ubo);
  vik_delete_instance(instance);

  winx_destroy_window(window);
  winx_cleanup(winx);

  return 0;
}
