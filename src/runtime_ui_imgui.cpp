// runtime_ui_imgui.cpp — Dear ImGui presentation glue for the in-game
// runtime settings overlay. C ABI surface declared in runtime_ui_imgui.h.
//
// The overlay model (RecompRuntimeUi) is C; this file owns everything ImGui:
// the context, the SDL3 platform backend (shared with the launcher helper),
// the locally ported SDL3 renderer backend (imgui_impl_sdlrenderer3.cpp),
// font loading, and the per-frame NewFrame/Render/RenderDrawData cycle.
//
// The game renders at 256x224 with an SDL logical presentation; ImGui draws
// in window coordinates at the true output size. Per recomp-ui's
// docs/RUNTIME_UI.md, a host using a logical presentation size must render
// the ImGui frame with the logical size temporarily disabled, then restore it
// before the next game frame. The host does exactly that around
// st_imgui_render_overlay() (see the main loop in src/main.c).

#include "runtime_ui_imgui.h"

#include <stdlib.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "recomp_runtime_ui.h"   // recomp_runtime_ui_render_imgui()

struct SuperTennisImGui {
  ImGuiContext *ctx;
};

extern "C" SuperTennisImGui *st_imgui_create(SDL_Window *window,
                                             SDL_Renderer *renderer) {
  IMGUI_CHECKVERSION();
  ImGuiContext *ctx = ImGui::CreateContext();
  if (!ctx) return nullptr;
  ImGui::SetCurrentContext(ctx);

  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr; /* never write imgui.ini next to the exe */

  /* Same staged Lato asset and glyph ranges the pre-boot launcher uses, so
   * the in-game menu matches its look. Falls back to the built-in font when
   * the asset is missing (e.g. running from a source checkout). */
  ImFontConfig cfg;
  cfg.OversampleH = 2;
  cfg.OversampleV = 2;
  cfg.PixelSnapH = true;
  static const ImWchar kRanges[] = {
      0x0020, 0x00FF, /* Basic Latin + Latin-1 Supplement */
      0x2010, 0x2027, /* dashes, curly quotes, ellipsis */
      0,
  };
  if (!io.Fonts->AddFontFromFileTTF("assets/fonts/LatoLatin-Regular.ttf",
                                    18.0f, &cfg, kRanges)) {
    cfg.SizePixels = 18.0f;
    io.Fonts->AddFontDefault(&cfg);
  }

  bool ok = ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  if (ok) ok = ImGui_ImplSDLRenderer3_Init(renderer);
  if (!ok) {
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(ctx);
    return nullptr;
  }

  SuperTennisImGui *ig = (SuperTennisImGui *)malloc(sizeof(*ig));
  if (!ig) {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(ctx);
    return nullptr;
  }
  ig->ctx = ctx;
  return ig;
}

extern "C" void st_imgui_destroy(SuperTennisImGui *ig) {
  if (!ig) return;
  ImGui::SetCurrentContext(ig->ctx);
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext(ig->ctx);
  free(ig);
}

extern "C" void st_imgui_process_event(SuperTennisImGui *ig,
                                       const SDL_Event *event) {
  ImGui::SetCurrentContext(ig->ctx);
  ImGui_ImplSDL3_ProcessEvent(event);
}

extern "C" void st_imgui_render_overlay(SuperTennisImGui *ig,
                                        SuperTennisRuntimeUi *rt,
                                        SDL_Renderer *renderer) {
  ImGui::SetCurrentContext(ig->ctx);

  /* The game draws through a 256x224 logical presentation; ImGui draws in
   * window coordinates at the true output size. Temporarily disable the
   * logical presentation (the SDL_Renderer backend's documented requirement),
   * then hand the renderer back to the game BEFORE the host presents:
   * draw commands keep the scale they were queued with, so the game texture
   * (queued under the logical presentation) and the overlay geometry (queued
   * raw) both render correctly, and no stale viewport/logical state survives
   * into the next game frame. */
  SDL_SetRenderLogicalPresentation(renderer, 0, 0,
                                   SDL_LOGICAL_PRESENTATION_DISABLED);
  SDL_SetRenderViewport(renderer, nullptr);

  ImGui_ImplSDL3_NewFrame();
  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui::NewFrame();
  recomp_runtime_ui_render_imgui(SuperTennisRuntimeUiCore(rt));
  ImGui::Render();
  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

  /* Full-target viewport + re-enabled logical presentation (see
   * runtime_ui.c). The host presents afterwards. */
  SuperTennisRuntimeUiRestoreRendererState(rt, renderer);
}