#include "native_ui.hpp"
#include "ps2/disc.hpp"
#include "ps2/system.hpp"
#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#ifdef PS2_GPU
#include "gs/gpu/gpu.hpp"
#include "imgui_impl_wgpu.h"
#endif
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>

namespace gow2 {
Settings settings;
std::atomic<bool> pause_requested{false}, pause_acknowledged{false};
std::atomic<bool> quit_requested{false};
namespace {
SDL_Window* game_window = nullptr;
std::atomic<float> master_volume{0.8f};
bool overlay_ready = false, show_menu = false;
std::uint64_t drawn_frames = 0;
int tab = 0, binding = -1;
double game_fps = 0, cpu_percent = 0, gpu_percent = 0, memory_mb = 0;
const ImVec4 gold{0.87f,0.69f,0.39f,1}, muted{0.58f,0.59f,0.64f,1};
const char* action_names[] = {"Select", "L3", "R3", "Start / pausa", "D-pad: cima", "D-pad: direita", "D-pad: baixo", "D-pad: esquerda", "L2 / magia", "R2 / arma", "L1", "R1 / interagir", "Triangulo", "Circulo", "X / confirmar", "Quadrado",
    "Mover: cima", "Mover: direita", "Mover: baixo", "Mover: esquerda", "Esquiva: cima", "Esquiva: direita", "Esquiva: baixo", "Esquiva: esquerda"};
void bind_key(SDL_Scancode key) {
    if(key!=SDL_SCANCODE_ESCAPE) {
        int& value=binding<16?settings.keys[binding]:settings.stickkeys[binding-16];
        value=key==SDL_SCANCODE_DELETE?0:static_cast<int>(key);
    }
    binding=-1; save_settings();
}

std::filesystem::path path(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(),s.end())); }
std::string utf8(const std::filesystem::path& p) { auto s=p.u8string(); return {s.begin(),s.end()}; }

void theme() {
    ImGui::StyleColorsDark();
    auto& s = ImGui::GetStyle();
    s.WindowRounding=14; s.ChildRounding=10; s.FrameRounding=7; s.GrabRounding=7;
    s.WindowPadding={24,22}; s.FramePadding={12,9}; s.ItemSpacing={12,12};
    s.Colors[ImGuiCol_WindowBg]={0.055f,0.055f,0.07f,0.97f};
    s.Colors[ImGuiCol_ChildBg]={0.085f,0.085f,0.105f,0.95f};
    s.Colors[ImGuiCol_FrameBg]={0.12f,0.12f,0.15f,1};
    s.Colors[ImGuiCol_Button]={0.19f,0.14f,0.10f,1};
    s.Colors[ImGuiCol_ButtonHovered]={0.38f,0.26f,0.14f,1};
    s.Colors[ImGuiCol_ButtonActive]={0.49f,0.32f,0.15f,1};
    s.Colors[ImGuiCol_Header]={0.28f,0.18f,0.11f,1};
    s.Colors[ImGuiCol_HeaderHovered]={0.37f,0.25f,0.14f,1};
    s.Colors[ImGuiCol_CheckMark]=gold; s.Colors[ImGuiCol_SliderGrab]=gold;
    s.Colors[ImGuiCol_Border]={0.25f,0.22f,0.18f,1};
    auto& io=ImGui::GetIO();
    io.IniFilename=nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    char* base=SDL_GetBasePath();
    std::string font_path=std::string(base?base:"")+"fonts/Roboto-Medium.ttf"; SDL_free(base);
    if(std::filesystem::is_regular_file(path(font_path))) io.Fonts->AddFontFromFileTTF(font_path.c_str(),18.0f);
    else { io.Fonts->AddFontDefault(); io.FontGlobalScale=1.2f; }
}

void display_options() {
    ImGui::TextColored(gold,"TELA E DESEMPENHO");
    const char* scales[]={"1x - resolucao original", "2x - imagem mais definida", "3x - alta resolucao"};
    int scale=settings.scale-1;
    if(ImGui::Combo("Resolucao interna",&scale,scales,3)) { settings.scale=scale+1; save_settings(); }
    if(ImGui::Checkbox("Tela cheia ao iniciar",&settings.fullscreen)) save_settings();
    if(ImGui::Checkbox("Formato 16:9",&settings.widescreen)) save_settings();
    ImGui::TextWrapped("Para 16:9, ative tambem Widescreen nas opcoes do proprio jogo.");
    if(ImGui::Checkbox("Mostrar FPS e desempenho",&settings.stats)) save_settings();
    ImGui::TextColored(muted,"Resolucao e formato sao aplicados na proxima abertura.");
    if(game_window && ImGui::Button("Alternar tela cheia agora (F11)")) {
        bool full=SDL_GetWindowFlags(game_window)&SDL_WINDOW_FULLSCREEN;
        SDL_SetWindowFullscreen(game_window, full?0:SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowCursor(SDL_ENABLE);
    }
}

void controls_options() {
    ImGui::TextColored(gold,"CONTROLES");
    ImGui::TextWrapped("Controles USB/Bluetooth reconhecidos pelo SDL funcionam automaticamente, incluindo os dois analogicos e vibracao.");
    if(ImGui::SliderFloat("Zona morta",&settings.deadzone,0.0f,0.4f,"%.2f")) save_settings();
    ImGui::TextWrapped("Padrao: setas movem; I / J / K / L esquivam; T / F / G / H selecionam magias; C + V ativa a Furia dos Titas quando disponivel. F1: painel. F11: tela cheia.");
    ImGui::TextColored(muted,"Clique para remapear. Escape cancela; Delete limpa.");
    if(ImGui::BeginTable("keys",4,ImGuiTableFlags_SizingStretchSame)) {
        for(int i=0;i<24;++i) {
            ImGui::PushID(i); ImGui::TableNextColumn(); ImGui::TextUnformatted(action_names[i]); ImGui::TableNextColumn();
            const int code=i<16?settings.keys[i]:settings.stickkeys[i-16];
            const char* key = binding==i ? "Pressione..." : code ? SDL_GetScancodeName(static_cast<SDL_Scancode>(code)) : "Sem tecla";
            if(ImGui::Button(key,{-1,0})) binding=i;
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

void sound_options() {
    ImGui::TextColored(gold,"AUDIO");
    if(ImGui::SliderFloat("Volume do jogo",&settings.volume,0,1,"%.2f")) { master_volume.store(settings.volume); save_settings(); }
    ImGui::TextWrapped("Musica, vozes e efeitos sao lidos da sua ISO. A versao USA utiliza o audio original do disco.");
}

void about() {
    ImGui::TextColored(gold,"GOD OF WAR II / SCUS-97481");
    ImGui::TextWrapped("Adaptacao para Linux e Windows baseada no PS2 Web Recomp, de snuri00. Codigo do Emotion Engine recompilado para C++; demais componentes do PS2 atendidos pelo runtime, com partes emuladas.");
    ImGui::Spacing(); ImGui::TextWrapped("Interface: Dear ImGui. Janela, controle e audio: SDL2. Renderizador: wgpu-native. Nenhum dado do jogo e baixado por este programa.");
    ImGui::TextColored(muted,"Saves: cartao de memoria de 8 MB no seu perfil de usuario.");
    if(ImGui::Button("Abrir pasta de configuracoes e saves")) SDL_OpenURL(("file://"+user_directory()).c_str());
    ImGui::Spacing(); ImGui::TextWrapped("Base experimental. Testes locais de abertura e combate nao comprovam a campanha inteira. Consulte VALIDACAO.md para os resultados desta compilacao.");
}

void settings_body() {
    const char* tabs[]={"Tela", "Controles", "Audio", "Sobre"};
    for(int i=0;i<4;++i) { if(i) ImGui::SameLine(); if(ImGui::Selectable(tabs[i],tab==i,0,{110,32})) tab=i; }
    ImGui::Separator(); ImGui::Spacing();
    if(tab==0) display_options(); else if(tab==1) controls_options(); else if(tab==2) sound_options(); else about();
}

void overlay_frame() {
    ++drawn_frames;
    ImGui_ImplSDL2_NewFrame(); ImGui::NewFrame();
    if(settings.stats) {
        ImGui::SetNextWindowPos({16,16},ImGuiCond_Always); ImGui::SetNextWindowBgAlpha(0.65f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{14,10});
        ImGui::Begin("Desempenho",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoNav);
        ImGui::TextColored(gold,"%.0f FPS",game_fps); ImGui::SameLine(); ImGui::Text("/ %.0f%%",game_fps/59.94*100);
        ImGui::TextColored(muted,"CPU %.0f%%  GPU %.0f%%  %.0f MB",cpu_percent,gpu_percent,memory_mb);
        ImGui::End(); ImGui::PopStyleVar();
    }
    if(show_menu) {
        auto size=ImGui::GetIO().DisplaySize;
        ImGui::GetBackgroundDrawList()->AddRectFilled({0,0},size,IM_COL32(0,0,0,115));
        ImGui::SetNextWindowPos({size.x/2,size.y/2},ImGuiCond_Always,{0.5f,0.5f});
        ImGui::SetNextWindowSize({std::min(760.0f,size.x-24),std::min(610.0f,size.y-24)},ImGuiCond_Always);
        ImGui::Begin("God of War II - configuracoes",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove);
        ImGui::TextColored(gold,"GOD OF WAR II"); ImGui::SameLine(); ImGui::TextColored(muted,"/ PAUSADO");
        ImGui::BeginChild("settings",{0,-70},false); settings_body(); ImGui::EndChild();
        ImGui::SetCursorPosY(ImGui::GetWindowHeight()-65);
        if(ImGui::Button("Voltar ao jogo  /  F1",{260,40})) { show_menu=false; pause_requested.store(false); SDL_ShowCursor(SDL_DISABLE); }
        ImGui::SameLine(); if(ImGui::Button("Sair do jogo",{170,40})) { quit_requested.store(true); pause_requested.store(false); }
        ImGui::End();
    }
    ImGui::Render();
}

bool prepare_disc(const std::string& iso,std::string& elf,std::string& error) {
    if(!std::filesystem::is_regular_file(path(iso))) { error="Selecione uma ISO existente."; return false; }
    if(!ps2::disc::open(iso)) { error="Nao foi possivel abrir a ISO."; return false; }
    auto e=ps2::disc::find("SCUS_974.81");
    if(!e || e->size!=2125688 || !ps2::disc::find("PART1.PAK") || !ps2::disc::find("PART2.PAK") || !ps2::disc::layer1_start()) {
        error="Esta compilacao exige God of War II USA (SCUS-97481), com as duas camadas do DVD."; return false;
    }
    std::vector<std::uint8_t> data((e->size+2047)/2048*2048);
    if(!ps2::disc::read(e->sector,static_cast<std::uint32_t>(data.size()/2048),data.data())) { error="Erro lendo o executavel da ISO."; return false; }
    std::uint64_t hash=14695981039346656037ull;
    for(std::size_t i=0;i<e->size;++i) hash=(hash^data[i])*1099511628211ull;
    if(hash!=0x1ba04eb69e532d3full) { error="O executavel desta ISO difere da versao recompilada. E preciso recompilar para este disco."; return false; }
    elf=user_directory()+"SCUS_974.81";
    std::ofstream out(path(elf),std::ios::binary); out.write(reinterpret_cast<char*>(data.data()),e->size);
    if(!out) { error="Falha ao extrair o executavel para o seu perfil."; return false; }
    return true;
}

std::string browse_iso() {
#ifdef _WIN32
    wchar_t file[32768]={}; OPENFILENAMEW dialog{}; dialog.lStructSize=sizeof dialog;
    dialog.lpstrFile=file; dialog.nMaxFile=32768;
    dialog.lpstrFilter=L"Imagem de disco PS2 (*.iso)\0*.iso\0";
    dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    if(GetOpenFileNameW(&dialog)) return utf8(std::filesystem::path(file));
#else
    if(std::FILE* pipe=popen("zenity --file-selection --title='Selecionar ISO de God of War II' --file-filter='*.iso' 2>/dev/null","r")) {
        char buffer[4096]; std::string selected; while(std::fgets(buffer,sizeof buffer,pipe)) selected+=buffer;
        if(pclose(pipe)==0) { while(!selected.empty() && (selected.back()=='\n'||selected.back()=='\r')) selected.pop_back(); return selected; }
    }
#endif
    return {};
}
}

std::string user_directory() {
    static const std::string directory=[] {
        char* value=SDL_GetPrefPath("LocalPorts","GodOfWarII");
        std::string result=value?value:"./userdata/"; SDL_free(value);
        std::filesystem::create_directories(path(result)); return result;
    }();
    return directory;
}
void load_settings() {
    std::ifstream in(path(user_directory()+"settings.ini")); std::string key; unsigned keys_version=0;
    while(in>>key) {
        if(key=="iso") in>>std::quoted(settings.iso);
        else if(key=="scale") in>>settings.scale;
        else if(key=="fullscreen") in>>settings.fullscreen;
        else if(key=="wide") in>>settings.widescreen;
        else if(key=="stats") in>>settings.stats;
        else if(key=="volume") in>>settings.volume;
        else if(key=="deadzone") in>>settings.deadzone;
        else if(key=="keys") for(int& value:settings.keys) in>>value;
        else if(key=="stickkeys") for(int& value:settings.stickkeys) in>>value;
        else if(key=="keys_version") in>>keys_version;
        else { std::string ignored; std::getline(in,ignored); }
    }
    settings.scale=std::clamp(settings.scale,1,3); settings.volume=std::clamp(settings.volume,0.0f,1.0f);
    settings.deadzone=std::clamp(settings.deadzone,0.0f,0.4f);
    for(int& k:settings.keys) if(k<0||k>=SDL_NUM_SCANCODES) k=0;
    for(int& k:settings.stickkeys) if(k<0||k>=SDL_NUM_SCANCODES) k=0;
    keyboard::migrate(settings.keys,keys_version);
    master_volume.store(settings.volume);
}
void save_settings() {
    std::ofstream out(path(user_directory()+"settings.ini.tmp"));
    out<<"iso "<<std::quoted(settings.iso)<<"\nscale "<<settings.scale<<"\nfullscreen "<<settings.fullscreen<<"\nwide "<<settings.widescreen<<"\nstats "<<settings.stats<<"\nvolume "<<settings.volume<<"\ndeadzone "<<settings.deadzone<<"\nkeys";
    for(int k:settings.keys) out<<' '<<k;
    out<<"\nkeys_version 2\nstickkeys";
    for(int k:settings.stickkeys) out<<' '<<k;
    out<<'\n'; out.close();
    if(out) { std::error_code ec; std::filesystem::rename(path(user_directory()+"settings.ini.tmp"),path(user_directory()+"settings.ini"),ec);
        if(ec) { std::filesystem::copy_file(path(user_directory()+"settings.ini.tmp"),path(user_directory()+"settings.ini"),std::filesystem::copy_options::overwrite_existing,ec); }
    }
}

bool install_iso(const std::string& iso) {
    load_settings(); std::string elf,error;
    if (!prepare_disc(iso,elf,error)) { std::fprintf(stderr,"%s\n",error.c_str()); return false; }
    settings.iso=iso; save_settings();
    std::fprintf(stderr,"[install] ISO validada; cache em %s\n",elf.c_str()); return true;
}

bool launcher(std::vector<std::string>& args, const std::string& capture) {
    SDL_SetMainReady();
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)!=0) {
        std::fprintf(stderr,"[launcher] SDL initialization failed: %s\n",SDL_GetError()); return false;
    }
    load_settings();
    if(!capture.empty()) if(const char* selected=std::getenv("GOW2_CAPTURE_TAB")) tab=std::clamp(std::atoi(selected),0,3);
    auto* window=SDL_CreateWindow("God of War II - iniciar",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1000,680,SDL_WINDOW_ALLOW_HIGHDPI|SDL_WINDOW_RESIZABLE);
    if(!window) { std::fprintf(stderr,"[launcher] Cannot create window: %s\n",SDL_GetError()); SDL_Quit(); return false; }
    SDL_SetWindowMinimumSize(window,900,630);
    auto* renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!renderer) renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(!renderer) { std::fprintf(stderr,"[launcher] Cannot create renderer: %s\n",SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return false; }
    ImGui::CreateContext(); theme(); ImGui_ImplSDL2_InitForSDLRenderer(window,renderer); ImGui_ImplSDLRenderer2_Init(renderer);
    char iso[4096]={}; std::snprintf(iso,sizeof iso,"%s",settings.iso.c_str());
    bool done=false, play=false, captured=false; std::string error, elf;
    int capture_frames=0;
    while(!done) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if(e.type==SDL_QUIT) done=true;
            if(e.type==SDL_DROPFILE) { std::snprintf(iso,sizeof iso,"%s",e.drop.file); SDL_free(e.drop.file); }
            if(e.type==SDL_KEYDOWN && binding>=0) bind_key(e.key.keysym.scancode);
        }
        ImGui_ImplSDLRenderer2_NewFrame(); ImGui_ImplSDL2_NewFrame(); ImGui::NewFrame();
        auto size=ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos({0,0}); ImGui::SetNextWindowSize(size);
        ImGui::Begin("Inicio",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
        ImGui::BeginChild("hero",{285,0},false);
        auto* draw=ImGui::GetWindowDrawList(); auto start=ImGui::GetCursorScreenPos();
        draw->AddCircle({start.x+124,start.y+105},74,IM_COL32(167,98,48,90),64,4);
        draw->AddText(nullptr,100,{start.x+85,start.y+40},IM_COL32(215,167,90,255),"II");
        ImGui::SetCursorPosY(215); ImGui::TextColored(gold,"GOD OF WAR II");
        ImGui::Spacing(); ImGui::TextWrapped("O destino dos deuses esta em suas maos.");
        ImGui::Spacing(); ImGui::TextColored(muted,"SCUS-97481 / USA");
        ImGui::TextColored(muted,"Linux + Windows / x86-64");
        ImGui::SetCursorPosY(ImGui::GetWindowHeight()-90); ImGui::TextWrapped("Sua ISO. Seus saves. Execucao local.");
        ImGui::EndChild(); ImGui::SameLine();
        ImGui::BeginChild("content",{0,0},true);
        ImGui::TextColored(gold,"PREPARAR PARA JOGAR");
        ImGui::TextWrapped("Selecione a imagem original do jogo. Voce tambem pode arrastar a ISO para esta janela.");
        ImGui::SetNextItemWidth(-115); ImGui::InputText("##iso",iso,sizeof iso); ImGui::SameLine();
        if(ImGui::Button("Procurar")) { auto chosen=browse_iso(); if(!chosen.empty()) std::snprintf(iso,sizeof iso,"%s",chosen.c_str()); }
        if(!error.empty()) ImGui::TextWrapped("%s",error.c_str());
        ImGui::Spacing(); ImGui::BeginChild("settings",{0,-86},false); settings_body(); ImGui::EndChild();
        ImGui::SetCursorPosY(ImGui::GetWindowHeight()-70);
        if(ImGui::Button("INICIAR JOGO",{250,46})) {
            settings.iso=iso;
            if(prepare_disc(settings.iso,elf,error)) { save_settings(); play=done=true; }
        }
        ImGui::SameLine(); ImGui::TextColored(muted,"F1 abre o painel durante o jogo");
        ImGui::EndChild(); ImGui::End(); ImGui::Render();
        SDL_SetRenderDrawColor(renderer,12,12,16,255); SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(),renderer);
        if(!capture.empty() && ++capture_frames==3) {
            int w=0,h=0; SDL_GetRendererOutputSize(renderer,&w,&h);
            SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_ARGB8888);
            if(surface) {
                if(SDL_RenderReadPixels(renderer,nullptr,surface->format->format,surface->pixels,surface->pitch)==0)
                    captured=SDL_SaveBMP(surface,capture.c_str())==0;
                SDL_FreeSurface(surface);
            }
            if(!captured) std::fprintf(stderr,"[launcher] Capture failed: %s\n",SDL_GetError());
            done=true;
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(8);
    }
    ImGui_ImplSDLRenderer2_Shutdown(); ImGui_ImplSDL2_Shutdown(); ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
    if(!play) return captured;
    SDL_setenv("PS2_SCALE",std::to_string(settings.scale).c_str(),1);
    args={elf,"--disc",settings.iso,"--ntsc","--memcard",user_directory()+"Mcd001.ps2",settings.fullscreen?"--fullscreen":"--window"};
    if(settings.widescreen) args.push_back("--widescreen");
    return true;
}

void init_overlay(SDL_Window* window) {
    game_window=window; ImGui::CreateContext(); theme(); ImGui_ImplSDL2_InitForOther(window);
#ifdef PS2_GPU
    ps2::gs::gpu::set_overlay([](void* device,void* pass,unsigned format) {
        if(!overlay_ready) {
            ImGui_ImplWGPU_InitInfo info; info.Device=static_cast<WGPUDevice>(device); info.RenderTargetFormat=static_cast<WGPUTextureFormat>(format);
            ImGui_ImplWGPU_Init(&info); overlay_ready=true;
        }
        ImGui_ImplWGPU_NewFrame(); overlay_frame();
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(),static_cast<WGPURenderPassEncoder>(pass));
    });
#endif
}
void process_event(const SDL_Event& e) {
    ImGui_ImplSDL2_ProcessEvent(&e);
    if(e.type==SDL_KEYDOWN && binding>=0) {
        bind_key(e.key.keysym.scancode); return;
    }
    if(e.type==SDL_KEYDOWN && !e.key.repeat && e.key.keysym.scancode==SDL_SCANCODE_F1) {
        show_menu=!show_menu; pause_requested.store(show_menu); SDL_ShowCursor(show_menu?SDL_ENABLE:SDL_DISABLE);
    }
}
bool capture_input() { return show_menu || binding>=0; }
std::uint64_t overlay_frames() { return drawn_frames; }
void draw_software_overlay(void* renderer) {
    if(!overlay_ready) { ImGui_ImplSDLRenderer2_Init(static_cast<SDL_Renderer*>(renderer)); overlay_ready=true; }
    ImGui_ImplSDLRenderer2_NewFrame(); overlay_frame();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(),static_cast<SDL_Renderer*>(renderer));
}
void set_metrics(double fps,double cpu,double gpu,double memory) { game_fps=fps;cpu_percent=cpu;gpu_percent=gpu;memory_mb=memory; }
void audio_volume(const short* input,short* output,std::size_t count) {
    float volume=master_volume.load(); for(std::size_t i=0;i<count;++i) output[i]=static_cast<short>(input[i]*volume);
}
}
