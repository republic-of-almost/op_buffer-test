#ifdef TEST_IMPL


#include <math/math.hpp>


namespace
{
  opID shader_id   = 0;
  opID shd_tex_id  = 0;
  opID shd_wvp_id  = 0;
  opID target_id   = 0;
  opID vertex_id   = 0;
  opID geometry_id = 0;
  opID index_id    = 0;
  opID inedex_id   = 0;
  opID raster_id   = 0;
  opID texture_id  = 0;
  opID texture2_id = 0;
  opID filter_id   = 0;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
extern "C" const char*
test_name()
{
  return "Basic opBuffer Test";
}


EMSCRIPTEN_KEEPALIVE
extern "C" const char*
test_desc()
{
  return "Simple test to check that the most basic setup works in opBuffer.";
}
#endif


void
test_init(opContext *context,
          opBuffer *buffer)
{
  /*
    Initalize the device.
  */
  opBufferDeviceCreate(context, buffer);

  /*
    Back Buffer
  */
  opTargetDesc back_buffer_desc;
  {
    memset(&back_buffer_desc, 0, sizeof(back_buffer_desc));
    back_buffer_desc.clear_red_color   = 0.06f;
    back_buffer_desc.clear_green_color = 0.09f;
    back_buffer_desc.clear_blue_color  = 0.06f;
  }

  target_id = opBufferTargetCreate(context, buffer, &back_buffer_desc);


  /*
    Create shader Resource
  */
  #ifdef __EMSCRIPTEN__
  constexpr char glsl_header[] = R"GLSL(
    #version 100
    #define VERT_IN attribute
    #define FRAG_IN varying mediump
    #define UNIFORM uniform mediump
    #define SAMPLE_2D(tex, uv) texture2D(tex, uv)
    #define OUTPUT varying
    #define FRAG_OUT
    #define FRAG_COL_SET(v) gl_FragColor = v
  )GLSL";
  #else
  constexpr char glsl_header[] = R"GLSL(
    #version 330 core
    #define VERT_IN in
    #define FRAG_IN in
    #define UNIFORM uniform
    #define SAMPLE_2D(tex, uv) texture(tex, uv)
    #define OUTPUT out
    #define FRAG_OUT out vec4 out_color;
    #define FRAG_COL_SET(v) out_color = v;
  )GLSL";
  #endif

  constexpr char vs[] = R"GLSL(
    VERT_IN vec3 position;
    VERT_IN vec2 tex_coord;

    UNIFORM mat4 wvp;

    OUTPUT vec2 frag_tex_coord;

    void
    main()
    {
      gl_Position = wvp * vec4(position, 1.0);
      frag_tex_coord = tex_coord;
    }
  )GLSL";

  constexpr char ps[] = R"GLSL(
    precision mediump float;

    FRAG_IN vec2 frag_tex_coord;

    uniform sampler2D diffuse;

    FRAG_OUT

    void
    main()
    {
      vec4 color = SAMPLE_2D(diffuse, frag_tex_coord);
      FRAG_COL_SET(vec4(color.rgb, 1));
    }
  )GLSL";

  char vs_src[512];
  memset(vs_src, 0, sizeof(vs_src));
  strcat(vs_src, glsl_header);
  strcat(vs_src, vs);

  char ps_src[512];
  memset(ps_src, 0, sizeof(ps_src));
  strcat(ps_src, glsl_header);
  strcat(ps_src, ps);

  opShaderDesc shd_desc;
  memset(&shd_desc, 0, sizeof(shd_desc));

  const char shd_name[] = "fullbright";

  shader_id = opBufferShaderCreate(context, buffer, shd_name, vs_src, nullptr, ps_src, &shd_desc);

  /*
    Shader Uniform
  */
  opShaderDataDesc shd_texture_desc;

  shd_tex_id = opBufferShaderDataCreate(context, buffer, shader_id, "diffuse", &shd_texture_desc);
  shd_wvp_id = opBufferShaderDataCreate(context, buffer, shader_id, "wvp", &shd_texture_desc);

  /*
    Vertex Format
  */
  constexpr size_t vertex_fmt_count = 2;
  opVertexDesc vert_format[vertex_fmt_count]
  {
    opVertexDesc{"position", opType_FLOAT3},
    opVertexDesc{"tex_coord", opType_FLOAT2},
  };

  vertex_id = opBufferVertexFormatCreate(context, buffer, vert_format, vertex_fmt_count);

  /*
    Geometry
  */
  opGeometryDesc geo_desc;
  memset(&geo_desc, 0, sizeof(geo_desc));

  constexpr size_t vertex_count = 20;

  float vert_data[vertex_count] = {
    +5.f, 0.f, +5.f, 1.f, 0.f,
    +5.f, 0.f, -5.f, 1.f, 1.f,
    -5.f, 0.f, +5.f, 0.f, 0.f,
    -5.f, 0.f, -5.f, 0.f, 1.f,
  };

  geometry_id = opBufferGeometryCreate(
    context,
    buffer,
    (void*)vert_data,
    sizeof(vert_data),
    vertex_count,
    &geo_desc
  );
  assert(geometry_id);

  /*
    Index
  */
  opIndexDesc index_desc;
  memset(&index_desc, 0, sizeof(index_desc));

  GLuint elements[] = {
    0, 1, 2,
    1, 3, 2,
  };

  index_id = opBufferIndexCreate(
    context,
    buffer,
    &elements,
    sizeof(elements),
    6,
    &index_desc
  );
  assert(index_id);

  /*
    Rasterizer
  */
  opRasterizerDesc rasterizer_desc;
  memset(&rasterizer_desc, 0, sizeof(rasterizer_desc));
  rasterizer_desc.winding_order = opWindingOrder_CCW;
  rasterizer_desc.primitive = opPrimitive_TRIANGLE;
  rasterizer_desc.cull_face = opCullFace_BACK;

  raster_id = opBufferRasterizerCreate(context, buffer, &rasterizer_desc);

  /*
    Texture
  */
  int width, height, comp;
  stbi_uc *img = stbi_load
  (
    "/Users/PhilCK/Developer/op_buffer_test/test/test1.png",
    &width,
    &height,
    &comp,
    0
  );

  if(!img)
  {
    printf("NO test1.png\n");
  }

  opTextureDesc texture_desc;
  memset(&texture_desc, 0, sizeof(texture_desc));
  texture_desc.width = width;
  texture_desc.height = height;
  texture_desc.dimention = opDimention_TWO;

  if(comp == 1) {
    texture_desc.format = opPixelFormat_R8;
  }
  else if(comp == 2) {
    texture_desc.format = opPixelFormat_RG8;
  }
  else if(comp == 3) {
    texture_desc.format = opPixelFormat_RGB8;
  }
  else if(comp == 4) {
    texture_desc.format = opPixelFormat_RGBA8;
  }

  texture_id = opBufferTextureCreate(context, buffer, img, &texture_desc);

  stbi_uc *img2 = stbi_load
  (
    "/Users/PhilCK/Developer/op_buffer_test/test/test2.png",
    &width,
    &height,
    &comp,
    0
  );

  if(!img2)
  {
    printf("NO test2.png\n");
  }

  opTextureDesc texture_desc2;
  memset(&texture_desc2, 0, sizeof(texture_desc2));
  texture_desc2.width = width;
  texture_desc2.height = height;
  texture_desc2.dimention = opDimention_TWO;

  if(comp == 1) {
    texture_desc2.format = opPixelFormat_R8;
  }
  else if(comp == 2) {
    texture_desc2.format = opPixelFormat_RG8;
  }
  else if(comp == 3) {
    texture_desc2.format = opPixelFormat_RGB8;
  }
  else if(comp == 4) {
    texture_desc2.format = opPixelFormat_RGBA8;
  }

  texture2_id = opBufferTextureCreate(context, buffer, img2, &texture_desc2);

  /*
    Texture Filter
  */
  opTextureFilterDesc filter_desc;
  memset(&filter_desc, 0, sizeof(filter_desc));
  filter_desc.filter_mode = opFilterMode_BILINEAR;

  filter_id = opBufferTextureFilterCreate(context, buffer, &filter_desc);

  /*
    Apply
  */
  opBufferExec(context, buffer);

  /*
    UpdateTexture
  */
// opBufferTextureUpdate(opContext *ctx, opBuffer *buf, const opID id, const size_t offset_x, const size_t offset_y, const size_t offset_z, const size_t width, const size_t height, const size_t depth, void *data);
  opBufferTextureUpdate(context, buffer, texture_id, 0, 0, 512, 512, img2);
  opBufferExec(context, buffer);

  /*
    Clean up
  */
  stbi_image_free(img);
  stbi_image_free(img2);
}


void
test_tick(opContext *context,
          opBuffer *buffer,
          const int width,
          const int height,
          const float dt)
{
  /*
    Calculat Orbit
  */
  static float time = 0;
  time += dt;

  constexpr float radius = 10.f;
  const float x = math::sin(time) * radius;
  const float z = math::cos(time) * radius;

  /*
    WVP Mat
  */
  math::mat4 proj_mat = math::mat4_projection(width, height, 0.1, 100, math::quart_tau() * 0.5f);
  math::mat4 view_mat = math::mat4_lookat(math::vec3_init(x,3,z), math::vec3_init(0,1,0), math::vec3_init(0, 1, 0));
  math::mat4 wvp      = math::mat4_multiply(view_mat, proj_mat);

  /*
    Build Render Buffer and Exec it.
  */
  opBufferDeviceReset(buffer);
  opBufferTargetClear(buffer, target_id, true, true);
  opBufferRasterizerBind(buffer, raster_id);
  opBufferVertexFormatBind(buffer, vertex_id);
  opBufferShaderBind(buffer, shader_id);
  opBufferShaderDataBind(buffer, shd_tex_id, texture_id);
  opBufferShaderDataBind(buffer, shd_wvp_id, &wvp);
  opBufferGeometryBind(buffer, geometry_id);
  opBufferIndexBind(buffer, index_id);
  opBufferRender(buffer);
  opBufferExec(context, buffer);
}

#endif // test guard
