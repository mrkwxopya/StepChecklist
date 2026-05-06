$ErrorActionPreference = "Stop"

$imgui = "vendor/imgui"

$files = @(
  "src/main.cpp",
  "$imgui/imgui.cpp",
  "$imgui/imgui_draw.cpp",
  "$imgui/imgui_tables.cpp",
  "$imgui/imgui_widgets.cpp",
  "$imgui/backends/imgui_impl_glfw.cpp",
  "$imgui/backends/imgui_impl_opengl3.cpp"
)

g++ $files `
  -I"$imgui" `
  -I"$imgui/backends" `
  -std=c++17 `
  -O2 `
  -mwindows `
  -lglfw3 `
  -lopengl32 `
  -lgdi32 `
  -limm32 `
  -o StepChecklist.exe

Write-Host "DONE: StepChecklist.exe" -ForegroundColor Green