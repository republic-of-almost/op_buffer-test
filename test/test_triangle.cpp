#ifdef TEST_IMPL


namespace
{
  op::resource_id shader_id   = op::resource_id{0,0};
  op::resource_id shd_col_id  = op::resource_id{0,0};
  op::resource_id target_id   = op::resource_id{0,0};
  op::resource_id vertex_id   = op::resource_id{0,0};
  op::resource_id geometry_id = op::resource_id{0,0};
  op::resource_id raster_id   = op::resource_id{0,0};
}


void
test_init(op::context *context,
          op::buffer *buffer)
{
  /*
    Initalize the device.
  */
  buffer->device_initialize();

  /*
    Back Buffer
  */
  op::target_create_desc back_buffer_desc {};
  {
    memset(&back_buffer_desc, 0, sizeof(back_buffer_desc));
    back_buffer_desc.clear_red_color   = 1.f;
    back_buffer_desc.clear_green_color = 1.f;
    back_buffer_desc.clear_blue_color  = 0.f;
  }

  target_id = buffer->target_create(context, &back_buffer_desc);
  assert(target_id.type);

  /*
    Create shader Resource
  */
  #ifdef __EMSCRIPTEN__
  constexpr char glsl_header[] = R"GLSL(
    #version 100
    #define INPUT attribute
    #define UNIFORM uniform mediump
    #define OUTPUT varying
    #define FRAG_OUT
    #define FRAG_COL_SET(v) gl_FragColor = v;
  )GLSL";
  #else
  constexpr char glsl_header[] = R"GLSL(
    #version 330 core
    #define INPUT in
    #define UNIFORM uniform
    #define OUTPUT out
    #define FRAG_OUT out vec4 out_color;
    #define FRAG_COL_SET(v) out_color = v;
  )GLSL";
  #endif

  constexpr char vs[] = R"GLSL(
    INPUT vec3 position;

    void
    main()
    {
      gl_Position = vec4(position, 1.0);
    }
  )GLSL";

  constexpr char ps[] = R"GLSL(
    UNIFORM vec3 color;

    FRAG_OUT

    void
    main()
    {
      FRAG_COL_SET(vec4(color, 1));
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

  op::shader_desc shd_desc;
  memset(&shd_desc, 0, sizeof(shd_desc));

  const char shd_name[] = "fullbright";

  shader_id = buffer->shader_create(context, vs_src, nullptr, ps_src, shd_name, &shd_desc);
  assert(shader_id.type);

  /*
    Shader Uniform
  */
  op::shader_data_desc shd_color_desc;

  shd_col_id = buffer->shader_data_create(context, shader_id, "color", &shd_color_desc);
  assert(shd_col_id.type);


  /*
    Vertex Format
  */
  constexpr size_t vertex_count = 1;
  op::vertex_attr vert_format[vertex_count]
  {
    op::vertex_attr{"position", op::type::FLOAT3},
  };

  vertex_id = buffer->vertex_format_create(context, vert_format, vertex_count);
  assert(vertex_id.type);

  /*
    Geometry
  */
  op::geometry_desc geo_desc;
  memset(&geo_desc, 0, sizeof(geo_desc));

  float vert_data[9] = {
    +0.0f, +0.5f, 0.f,
    -0.5f, -0.5f, 0.f,
    +0.5f, -0.5f, 0.f,
  };

  geometry_id = buffer->geometry_create(context, (void*)vert_data, sizeof(vert_data), 3, &geo_desc);
  assert(geometry_id.type);

  /*
    Rasterizer
  */
  opRasterizerDesc rasterizer_desc;
  memset(&rasterizer_desc, 0, sizeof(rasterizer_desc));
  rasterizer_desc.winding_order = op::winding_order::CCW;

  raster_id = buffer->rasterizer_create(context, &rasterizer_desc);
  assert(raster_id.type);

  /*
    Apply
  */
  buffer->exec(context);
}


void
test_tick(op::context *context,
          op::buffer *buffer)
{
  /*
    Clear buffer Render the triangle.
  */

  constexpr float red[3] = {1.f, 0.f, 0.f};

  buffer->device_reset();
  buffer->target_clear(target_id, true, true);
  buffer->rasterizer_bind(raster_id);
  buffer->vertex_format_bind(vertex_id);
  buffer->shader_bind(shader_id);
  buffer->shader_data_bind(shd_col_id, (void*)&red);
  buffer->geometry_bind(geometry_id);
  buffer->render();

  /*
    Apply
  */
  buffer->exec(context);
}

#endif // test guard
