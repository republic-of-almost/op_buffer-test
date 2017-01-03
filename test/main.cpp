#include <op/op.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GLES2/gl2.h>
#else
#include <OpenGL/gl3.h>
#endif

#ifdef OP_BUFFER_TEST_DEBUG_UI
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"


#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 480

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#define UNUSED(a) (void)a
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define LEN(a) (sizeof(a)/sizeof(a)[0])
#endif


// -- Tests -- //
/*
  Uncomment One test only
*/
#define TEST_IMPL
// #include "test_triangle.cpp"
#include "test_texture.cpp"


// -- Test App Vars -- //
SDL_Window *sdl_window = nullptr;
SDL_GLContext gl_context = nullptr;

int width = 800;
int height = 480;
bool is_running = true;

opContext *context = nullptr;
opBuffer *buffer = nullptr;

#ifdef OP_BUFFER_TEST_DEBUG_UI
struct nk_context *ctx;
#endif

float clear_color[3] = {1,0,0};


/*
  Exposed to Javascript
*/
#ifdef __EMSCRIPTEN__

EMSCRIPTEN_KEEPALIVE
extern "C" int
hello(int c)
{
  clear_color[2] = 0.f;

  return clear_color[0];
}


EMSCRIPTEN_KEEPALIVE
extern "C" int
resize(int new_width, int new_height)
{
  width = new_width;
  height = new_height;

  if(sdl_window)
  {
    SDL_SetWindowSize(sdl_window, width, height);
    opBufferViewport(buffer, 0, 0, width, height);
  }

  return width;
}

EMSCRIPTEN_KEEPALIVE
extern "C" const char*
test_name();

EMSCRIPTEN_KEEPALIVE
extern "C" const char*
test_desc();
#endif


void
app_init()
{
  // -- Setup SDL -- //
  SDL_Init(SDL_INIT_EVERYTHING);

  sdl_window = SDL_CreateWindow("Op Renderer",
                SDL_WINDOWPOS_CENTERED,
                SDL_WINDOWPOS_CENTERED,
                width,
                height,
                /*SDL_WINDOW_ALLOW_HIGHDPI | */SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

  // GL //
  #ifndef __EMSCRIPTEN__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  #else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  #endif
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 2);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  gl_context = SDL_GL_CreateContext(sdl_window);

  SDL_GL_MakeCurrent(sdl_window, gl_context);

  // -- Setup UI -- //
  #ifdef OP_BUFFER_TEST_DEBUG_UI
  struct nk_context *ctx;
  ctx = nk_sdl_init(sdl_window);

  struct nk_font_atlas *atlas;
  nk_sdl_font_stash_begin(&atlas);
  nk_sdl_font_stash_end();
  #endif

  // -- Setup Buffer -- //
  uint32_t alloc_count = 0;

  opCallbackAlloc([](size_t requested_size, uintptr_t user_data)
  {
    *reinterpret_cast<uint32_t*>(user_data) += 1;
    return malloc(requested_size);
  });

  opCallbackResize([](size_t requested_size, void *old_data, uintptr_t user_data)
  {
    *reinterpret_cast<uint32_t*>(user_data) += 1;
    return realloc(old_data, requested_size);
  });

  opCallbackDestroy([](void *data, uintptr_t user_data)
  {
    *reinterpret_cast<uint32_t*>(user_data) += 1;
    free(data);
  });

  opCallbackUserData((uintptr_t)&alloc_count);

  
  context = opContextCreate();
  buffer = opBufferCreate();


  // -- Init Test -- //
  test_init(context, buffer);
}


void
app_tick()
{
  static int frame_count = 0;
  {
    // Process Events
    SDL_Event sdl_event;

    #ifdef OP_BUFFER_TEST_DEBUG_UI
    // nk_input_begin(ctx);
    #endif

    while (SDL_PollEvent(&sdl_event))
    {
      if (sdl_event.type == SDL_QUIT)
      {
        is_running = false;
      }

      if (sdl_event.type == SDL_WINDOWEVENT)
      {
        switch (sdl_event.window.event)
        {
          case SDL_WINDOWEVENT_RESIZED:
            printf("Resize!\n");
        }
      }

      #ifdef OP_BUFFER_TEST_DEBUG_UI
      // nk_sdl_handle_event(&sdl_event);
      #endif
    }

    #ifdef OP_BUFFER_TEST_DEBUG_UI
    // nk_input_end(ctx);
    #endif

    // is_running = false; // bail once

    test_tick(context, buffer, width, height, 0.001);

    // -- Menu -- //

    #ifdef OP_BUFFER_TEST_DEBUG_UI
    // if (nk_begin(ctx, "Demo", nk_rect(50, 50, 200, 200),
    //     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
    //     NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
    // {
    //     nk_menubar_begin(ctx);
    //     nk_layout_row_begin(ctx, NK_STATIC, 25, 2);
    //     nk_layout_row_push(ctx, 45);
    //     if (nk_menu_begin_label(ctx, "FILE", NK_TEXT_LEFT, nk_vec2(120, 200))) {
    //         nk_layout_row_dynamic(ctx, 30, 1);
    //         nk_menu_item_label(ctx, "OPEN", NK_TEXT_LEFT);
    //         nk_menu_item_label(ctx, "CLOSE", NK_TEXT_LEFT);
    //         nk_menu_end(ctx);
    //     }
    //     nk_layout_row_push(ctx, 45);
    //     if (nk_menu_begin_label(ctx, "EDIT", NK_TEXT_LEFT, nk_vec2(120, 200))) {
    //         nk_layout_row_dynamic(ctx, 30, 1);
    //         nk_menu_item_label(ctx, "COPY", NK_TEXT_LEFT);
    //         nk_menu_item_label(ctx, "CUT", NK_TEXT_LEFT);
    //         nk_menu_item_label(ctx, "PASTE", NK_TEXT_LEFT);
    //         nk_menu_end(ctx);
    //     }
    //     nk_layout_row_end(ctx);
    //     nk_menubar_end(ctx);
    //
    //     enum {EASY, HARD};
    //     static int op = EASY;
    //     static int property = 20;
    //     nk_layout_row_static(ctx, 30, 80, 1);
    //     if (nk_button_label(ctx, "button"))
    //         fprintf(stdout, "button pressed\n");
    //     nk_layout_row_dynamic(ctx, 30, 2);
    //     if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
    //     if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
    //     nk_layout_row_dynamic(ctx, 25, 1);
    //     nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
    // }
    // nk_end(ctx);
    //
    // nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
    #endif

    SDL_GL_SwapWindow(sdl_window);
  }
}


int
main()
{
  app_init();

  #ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(app_tick, -1, 1);
  #else
  while(is_running)
  {
    app_tick();
  }
  #endif

  // assert(alloc_count == 0);

  return 0;
}
