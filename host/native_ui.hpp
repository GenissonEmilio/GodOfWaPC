#pragma once
#include <atomic>
#include <array>
#include <string>
#include <vector>
struct SDL_Window;
union SDL_Event;

namespace gow2 {
struct Settings {
    std::string iso;
    int scale = 1;
    bool fullscreen = false;
    bool widescreen = false;
    bool stats = true;
    float volume = 0.8f;
    float deadzone = 0.15f;
    std::array<int,16> keys{42,0,0,40,82,79,81,80,30,31,20,26,22,27,29,4};
};
extern Settings settings;
extern std::atomic<bool> pause_requested;
extern std::atomic<bool> pause_acknowledged;
extern std::atomic<bool> quit_requested;
std::string user_directory();
void load_settings();
void save_settings();
// Opens the native setup screen and returns the arguments for the game.
bool launcher(std::vector<std::string>& args, const std::string& capture = {});
bool install_iso(const std::string& iso);
void init_overlay(SDL_Window* window);
void process_event(const SDL_Event& event);
void draw_software_overlay(void* renderer);
void audio_volume(const short* input, short* output, std::size_t count);
void set_metrics(double fps, double cpu, double gpu, double memory);
bool capture_input();
std::uint64_t overlay_frames();
}
