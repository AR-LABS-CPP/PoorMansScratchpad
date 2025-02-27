#include <stdio.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_impl_sdl3.h"
#include "../vendor/imgui/imgui_impl_sdlrenderer3.h"

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* colorBufferTexture = NULL;
SDL_Texture* drawingBufferTexture = NULL;
uint32_t* colorBuffer = NULL;
uint32_t* drawingBuffer = NULL;

int windowWidth = 0;
int windowHeight = 0;
int brushSize = 10;

bool isRunning;
bool leftMouseButtonDown = false;

float_t mouseX, mouseY;
float_t prevMouseX, prevMouseY;

// ABGR FORMAT (8 bits for each channel)
uint32_t gridColor = 0xFF444444;
uint32_t backColor = 0xEECDCDCD;
uint32_t lineColor = 0xFF444444;

ImVec4 convertedLineColor;
ImVec4 convertedBackColor;
ImVec4 convertedGridColor;

void initializeImGui();
bool initializeWindow();
void setupTexture();
void drawThickLine(int x0, int y0, int x1, int y1, uint32_t color);
void drawThickPixel(int x, int y, uint32_t color);
void drawGrid(uint32_t* buffer, uint32_t gridColor, uint32_t bgColor, uint32_t cellSize);
void processInput();
ImGuiWindowFlags getDockingWindowFlags();
void renderUI();
void renderStuff();
void destroy();

int main(void) {
    isRunning = initializeWindow();
    if (!isRunning) destroy();
    setupTexture();
    drawGrid(colorBuffer, gridColor, backColor, 30);

    while (isRunning) {
        processInput();
        renderStuff();
    }

    destroy();
    return 0;
}

void initializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
}

bool initializeWindow(void) {
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "gpu");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return false;
    }

    SDL_Rect displayBounds;
    if (!SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &displayBounds)) {
        printf("Failed to get display bounds, reverting to defaults!\n");

        windowWidth = 1280;
        windowHeight = 768;
    }
    else {
        windowWidth = displayBounds.w;
        windowHeight = displayBounds.h;
    }

    window = SDL_CreateWindow("ScratchPad", windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Failed to create SDL window, exiting! %s\n", SDL_GetError());
        return false;
    }

    SDL_SetWindowFullscreen(window, 0);
    SDL_MaximizeWindow(window);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        printf("Failed to create SDL renderer, exiting! %s\n", SDL_GetError());
        return false;
    }

    SDL_ShowWindow(window);
    initializeImGui();
    return true;
}

void setupTexture() {
    colorBuffer = (uint32_t*)malloc(sizeof(uint32_t) * windowWidth * windowHeight);
    drawingBuffer = (uint32_t*)malloc(sizeof(uint32_t) * windowWidth * windowHeight);
    if (!colorBuffer || !drawingBuffer) {
        printf("Failed to allocate memory for buffers.\n");
        isRunning = false;
        return;
    }

    colorBufferTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING, windowWidth, windowHeight);
    drawingBufferTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING, windowWidth, windowHeight);

    memset(drawingBuffer, 0, windowWidth * windowHeight * sizeof(uint32_t));
}

// Bresenham's Line Algorithm
void drawThickLine(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        drawThickPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void drawThickPixel(int x, int y, uint32_t color) {
    for (int i = -brushSize; i <= brushSize; i++) {
        for (int j = -brushSize; j <= brushSize; j++) {
            int dx = x + j;
            int dy = y + i;

            if (dx >= 0 && dx < windowWidth && dy >= 0 && dy < windowHeight) {
                drawingBuffer[(windowWidth * dy) + dx] = color;
            }
        }
    }
}

void drawGrid(uint32_t* buffer, uint32_t gridColor, uint32_t bgColor, uint32_t cellSize) {
    if (buffer) {
        for (uint32_t row = 0; row < windowHeight; row++) {
            for (uint32_t col = 0; col < windowWidth; col++) {
                buffer[(windowWidth * row) + col] = (row % cellSize == 0 || col % cellSize == 0)
                    ? gridColor : bgColor;
            }
        }
    }
}

void processInput() {
    SDL_Event event;
    SDL_PollEvent(&event);
    ImGui_ImplSDL3_ProcessEvent(&event);

    switch (event.type) {
    case SDL_EVENT_QUIT:
        isRunning = false;
        break;
    case SDL_EVENT_KEY_DOWN:
        if (event.key.key == SDLK_ESCAPE) {
            isRunning = false;
        }
        break;

    case SDL_EVENT_MOUSE_MOTION:
        SDL_GetMouseState(&mouseX, &mouseY);

        if (ImGui::GetIO().WantCaptureMouse) break;

        if (leftMouseButtonDown) {
            drawThickLine((int)prevMouseX, (int)prevMouseY, (int)mouseX, (int)mouseY, lineColor);
        }

        prevMouseX = mouseX;
        prevMouseY = mouseY;
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        brushSize += (event.wheel.y > 0) ? 1 : -1;
        brushSize = SDL_clamp(brushSize, 1, 10);
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
            leftMouseButtonDown = true;
            prevMouseX = mouseX;
            prevMouseY = mouseY;
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.button.button == SDL_BUTTON_LEFT) {
            leftMouseButtonDown = false;
        }
        break;
    }
}

ImGuiWindowFlags getDockingWindowFlags() {
    return ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDocking;
}

void renderUI() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(400, viewport->Size.y));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::Begin("Controls", NULL, getDockingWindowFlags());

    if (ImGui::Button("Clear Drawing")) {
        memset(drawingBuffer, 0, windowWidth * windowHeight * sizeof(uint32_t));
    }

    ImGui::Separator();
    ImGui::SliderInt("Brush Size", &brushSize, 1, 10);

    convertedLineColor = ImGui::ColorConvertU32ToFloat4(lineColor);
    if (ImGui::ColorEdit4("Line Color", (float*)&convertedLineColor)) {
        lineColor = ImGui::ColorConvertFloat4ToU32(convertedLineColor);
    }

    convertedBackColor = ImGui::ColorConvertU32ToFloat4(backColor);
    if (ImGui::ColorEdit4("Background Color", (float*)&convertedBackColor)) {
        backColor = ImGui::ColorConvertFloat4ToU32(convertedBackColor);
        drawGrid(colorBuffer, gridColor, backColor, 30);
    }

    convertedGridColor = ImGui::ColorConvertU32ToFloat4(gridColor);
    if (ImGui::ColorEdit4("Grid Color", (float*)&convertedGridColor)) {
        gridColor = ImGui::ColorConvertFloat4ToU32(convertedGridColor);
        drawGrid(colorBuffer, gridColor, backColor, 30);
    }

    ImGui::End();
    ImGui::Render();
}

void renderStuff() {
    renderUI();
    SDL_UpdateTexture(colorBufferTexture, NULL, colorBuffer, windowWidth * sizeof(uint32_t));
    SDL_UpdateTexture(drawingBufferTexture, NULL, drawingBuffer, windowWidth * sizeof(uint32_t));

    SDL_RenderTexture(renderer, colorBufferTexture, NULL, NULL);
    SDL_RenderTexture(renderer, drawingBufferTexture, NULL, NULL);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

void destroy() {
    if (colorBuffer) free(colorBuffer);
    if (drawingBuffer) free(drawingBuffer);

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}