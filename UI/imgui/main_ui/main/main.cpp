//
// Created by ethan on 12/2/24.
//

#include "../../imgui.h"
#include "../../backends/imgui_impl_sdl2.h"
#include "../../backends/imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include "../../../../NES.h"
#include "main.h"
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL2/SDL_opengl.h>
#endif
// System includes
#include <stdint.h>     // intptr_t
#include <stdio.h>
#include <bits/fs_fwd.h>
#include <bits/fs_path.h>

#include "portable-file-dialogs.h"

// Keybinding function
SDL_Scancode rebindKey() {
    SDL_Event event;
    while (true) {
        if (SDL_WaitEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                return event.key.keysym.scancode;
            }
        }
    }
}

int main(int, char**)
{
    NES nes;

    float R = 1;
    float G = 1;
    float B = 1;

    bool showDebug = false;
    bool showRebind = false;

    // Initialize input
    Input input;
    int skipInputFrames = 0;

    // Keybinding map
    struct KeyBind {
      const char* name;
      SDL_Scancode* key;
    };

    KeyBind keyBindTable[8];
    keyBindTable[0] = {"Up", &input.up};
    keyBindTable[1] = {"Down", &input.down};
    keyBindTable[2] = {"Left", &input.left};
    keyBindTable[3] = {"Right", &input.right};
    keyBindTable[4] = {"A", &input.a};
    keyBindTable[5] = {"B", &input.b};
    keyBindTable[6] = {"Select", &input.select};
    keyBindTable[7] = {"Start", &input.start};

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("NES EMULATOR", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 720, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        {
            // Generate random color
            if (nes.A_changed == true && nes.paused == false) {
                R = rand() / (float)RAND_MAX;
                G = rand() / (float)RAND_MAX;
                B = rand() / (float)RAND_MAX;
            }

            // Render the NES screen
            ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
            float menuBarHeight = ImGui::GetFrameHeight(); // Offset by menu bar height
            ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(viewportSize.x, viewportSize.y - menuBarHeight), ImGuiCond_Always);

            ImGui::Begin("NES Screen", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoBringToFrontOnFocus
            );
            //ImGui::Begin("NES Emulator", nullptr, ImGuiWindowFlags_NoResize;		// don't allow resizing?
            ImVec2 widgetSize = ImGui::GetContentRegionAvail();

            uint32_t* framebuffer = nes.getFramebuffer();


            // Set the width and height of the NES screen
            int screenWidth = 256;  // NES screen width
            int screenHeight = 240; // NES screen height

            // Calculate the scaling factor based on the widget size
            float aspectRatio = static_cast<float>(screenWidth) / screenHeight;
            float widgetAspectRatio = widgetSize.x / widgetSize.y;

            float renderWidth = widgetSize.x;
            float renderHeight = widgetSize.y;

            // If the widget aspect ratio is greater than the NES aspect ratio, scale by height
            if (widgetAspectRatio > aspectRatio) {
                renderWidth = widgetSize.y * aspectRatio;
            } else { // Otherwise, scale by width
                renderHeight = widgetSize.x / aspectRatio;
            }

            //ImGui::SetWindowSize(ImVec2(renderWidth, renderHeight), 0);

            // Handle skipping input
            if (skipInputFrames > 0) {
              skipInputFrames--;
            }

            if (skipInputFrames == 0) {
                const Uint8 *keyboard;

                SDL_PumpEvents();
                keyboard = SDL_GetKeyboardState(NULL);
                // Handle the Start key
                if (keyboard[input.start]) {
                    nes.bus.controller1.start = 1;
                } else {
                    nes.bus.controller1.start = 0;
                }

                // Handle the Up arrow key
                if (keyboard[input.up]) {
                    nes.bus.controller1.up = 1;
                } else {
                    nes.bus.controller1.up = 0;
                }

                // Handle the Down arrow key
                if (keyboard[input.down]) {
                    nes.bus.controller1.down = 1;
                } else {
                    nes.bus.controller1.down = 0;
                }

                // Handle the Left arrow key
                if (keyboard[input.left]) {
                    nes.bus.controller1.left = 1;
                } else {
                    nes.bus.controller1.left = 0;
                }

                // Handle the Right arrow key
                if (keyboard[input.right]) {
                    nes.bus.controller1.right = 1;
                } else {
                    nes.bus.controller1.right = 0;
                }

                // Handle the Select key
                if (keyboard[input.select]) {
                    nes.bus.controller1.select = 1;
                } else {
                    nes.bus.controller1.select = 0;
                }

                // Handle the A key
                if (keyboard[input.a]) {
                    nes.bus.controller1.a = 1;
                } else {
                    nes.bus.controller1.a = 0;
                }

                // Handle the B key
                if (keyboard[input.b]) {
                    nes.bus.controller1.b = 1;
                } else {
                    nes.bus.controller1.b = 0;
                }
            }

            GLuint textureID;

            // Create/OpenGL texture if not already created
            if (textureID == 0) {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            // Upload the framebuffer data to the texture
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);

            // Render the texture with Image()
            ImGui::Image(reinterpret_cast<ImTextureID>(reinterpret_cast<void *>(static_cast<intptr_t>(textureID))), ImVec2(renderWidth, renderHeight)); // Render the texture with the NES screen size
            ImGui::End();

            // Display settings buttons
            ImGui::BeginMainMenuBar();
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Load ROM")) {
                    nes.on = false;
                    auto selection = pfd::open_file("NES files", std::filesystem::current_path(), {"NES Files", "*.nes"}).result();
                    if (!selection.empty()) {
                        nes.load_rom(selection[0].c_str());
                    }
                    nes.initNES();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug")) {
                ImGui::MenuItem("Show Debug Window", nullptr, &showDebug);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Controls")) {
                ImGui::MenuItem("Rebind Controls", nullptr, &showRebind);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();

            if (showRebind) {
                ImGui::Begin("Rebind");
                for (auto& keyBind : keyBindTable) {
                    if (ImGui::Button((std::string(keyBind.name) + ": " + SDL_GetScancodeName(*keyBind.key)).c_str())) {
                        nes.end();
                        nes.paused = true;

                        *(keyBind.key) = rebindKey();

                        nes.paused = false;
                        nes.on = true;
                        skipInputFrames = 5;
                    }
                }

                ImGui::End();
            }


            // Display the current registers, controller input, and additional controls
            if (showDebug) {
                ImGui::Begin("Debug");
                // Pause button
                if (ImGui::Button("PAUSE")) {
                    nes.end();
                    nes.paused = true;
                }

                // Continue button
                ImGui::SameLine();
                if (ImGui::Button("CONTINUE")) {
                    nes.paused = false;
                    nes.on = true;
                }

                // Cycle button
                ImGui::SameLine();
                if (ImGui::Button("CYCLE")) {
                    if (nes.on == false) {
                        nes.on = true;
                        nes.cycle();
                        //nes.RandomizeFramebuffer();
                        // if (nes.A_changed == true) {
                        //     R = rand() / (float)RAND_MAX;
                        //     G = rand() / (float)RAND_MAX;
                        //     B = rand() / (float)RAND_MAX;
                        // }
                        nes.on = false;
                    }
                }

                // Display registers and buttons
                ImGui::Text("Registers      Buttons");
                //ImGui::TextColored(ImVec4(R, G, B, 1.0f), "A: [%02x]", nes.cpu.A);
                ImGui::Text("A:    [%02x]     A:      [%01x]", nes.cpu.A, nes.bus.controller1.a);
                ImGui::Text("X:    [%02x]     B:      [%01x]", nes.cpu.X, nes.bus.controller1.b);
                ImGui::Text("Y:    [%02x]     Select: [%01x]", nes.cpu.Y, nes.bus.controller1.select);
                ImGui::Text("PC: [%04x]     Start:  [%01x]", nes.cpu.PC, nes.bus.controller1.start);
                ImGui::Text("S:  [%04x]     Up:     [%01x]", nes.cpu.S, nes.bus.controller1.up);
                ImGui::Text("P:  [%04x]     Down:   [%01x]", nes.cpu.P, nes.bus.controller1.down);
                ImGui::Text("               Left:   [%01x]", nes.bus.controller1.left);
                ImGui::Text("               Right:  [%01x]", nes.bus.controller1.right);

                ImGui::End();
            }
        }

        // Cycle the NES
                if (nes.on == true && nes.rom_loaded == true && nes.paused == false) {
                    //nes.RandomizeFramebuffer();
                    nes.cycle();

                    // Get the NES framebuffer (assuming it returns 32-bit RGBA data)
                    uint32_t* pixels = nes.getFramebuffer();
                }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}


