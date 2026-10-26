#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include "game.hpp"
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>

namespace {
amber::Game game;
amber::Canvas canvas;
std::array<bool,256> keys{};
amber::Input edge;
bool fullscreen=false,toggleFullscreen=false;
WINDOWPLACEMENT placement{};
constexpr wchar_t ClassName[]=L"AmberGroveWindow";

// Draw the bars and scaled image offscreen; expose only the finished frame.
// Clearing the visible window before StretchDIBits can produce black flashes.
class BackBuffer {
    HDC dc=nullptr;
    HBITMAP bitmap=nullptr;
    HGDIOBJ original=nullptr;
    int width=0,height=0;
public:
    BackBuffer()=default;
    BackBuffer(const BackBuffer&)=delete;
    BackBuffer& operator=(const BackBuffer&)=delete;
    ~BackBuffer() { clear(); }
    void clear() {
        if(dc&&original) SelectObject(dc,original);
        if(bitmap) DeleteObject(bitmap);
        if(dc) DeleteDC(dc);
        dc=nullptr; bitmap=nullptr; original=nullptr; width=height=0;
    }
    HDC get(HDC screen,int w,int h) {
        if(dc&&w==width&&h==height) return dc;
        clear();
        dc=CreateCompatibleDC(screen);
        if(!dc) return nullptr;
        bitmap=CreateCompatibleBitmap(screen,w,h);
        if(!bitmap) { clear(); return nullptr; }
        original=SelectObject(dc,bitmap);
        if(!original||original==HGDI_ERROR) { original=nullptr; clear(); return nullptr; }
        width=w; height=h; return dc;
    }
};

void present(HWND window) {
    RECT client; GetClientRect(window,&client);
    int cw=client.right,ch=client.bottom;
    if(cw<=0||ch<=0) return;
    int scale=std::max(1,std::min(cw/amber::W,ch/amber::H));
    int width=amber::W*scale,height=amber::H*scale;
    HDC dc=GetDC(window);
    if(!dc) return;
    static BackBuffer buffer;
    HDC frame=buffer.get(dc,cw,ch);
    if(!frame) { ReleaseDC(window,dc); return; }
    FillRect(frame,&client,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=amber::W; info.bmiHeader.biHeight=-amber::H;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    SetStretchBltMode(frame,COLORONCOLOR);
    StretchDIBits(frame,(cw-width)/2,(ch-height)/2,width,height,0,0,amber::W,amber::H,
                  canvas.pixels.data(),&info,DIB_RGB_COLORS,SRCCOPY);
    BitBlt(dc,0,0,cw,ch,frame,0,0,SRCCOPY);
    ReleaseDC(window,dc);
}
LRESULT CALLBACK windowProc(HWND hwnd,UINT message,WPARAM w,LPARAM l) {
    switch(message) {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(hwnd,&ps); EndPaint(hwnd,&ps); present(hwnd); return 0; }
    case WM_GETMINMAXINFO: {
        RECT r{0,0,amber::W,amber::H}; AdjustWindowRect(&r,WS_OVERLAPPEDWINDOW,FALSE);
        auto* m=reinterpret_cast<MINMAXINFO*>(l); m->ptMinTrackSize={r.right-r.left,r.bottom-r.top}; return 0;
    }
    case WM_KILLFOCUS:
        keys.fill(false); edge={}; if(game.mode==amber::Mode::Playing) game.mode=amber::Mode::Paused; return 0;
    case WM_KEYDOWN: case WM_SYSKEYDOWN:
        if(w<256) keys[w]=true;
        if(!(l&(1L<<30))) {
            if(w==VK_SPACE||w==VK_UP||w=='W') edge.jump=true;
            if(w=='J'||w=='Z') edge.attack=true;
            if(w==VK_RETURN) edge.start=true;
            if(w=='P'||w==VK_ESCAPE) { if(game.mode==amber::Mode::Title&&w==VK_ESCAPE) DestroyWindow(hwnd); else edge.pause=true; }
            if(w=='R') edge.restart=true;
            if(w=='M') edge.mute=true;
            if(w==VK_F11) toggleFullscreen=true;
            if(w==VK_F4&&(l&(1L<<29))) DestroyWindow(hwnd);
        }
        return 0;
    case WM_KEYUP: case WM_SYSKEYUP: if(w<256) keys[w]=false; return 0;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    default: return DefWindowProcW(hwnd,message,w,l);
    }
}
void toggle(HWND hwnd) {
    if(!fullscreen) {
        MONITORINFO mi{}; mi.cbSize=sizeof(MONITORINFO);
        placement.length=sizeof(WINDOWPLACEMENT);
        if(GetWindowPlacement(hwnd,&placement)&&GetMonitorInfoW(MonitorFromWindow(hwnd,MONITOR_DEFAULTTOPRIMARY),&mi)) {
            SetWindowLongPtrW(hwnd,GWL_STYLE,GetWindowLongPtrW(hwnd,GWL_STYLE)&~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hwnd,HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right-mi.rcMonitor.left,
                         mi.rcMonitor.bottom-mi.rcMonitor.top,SWP_NOOWNERZORDER|SWP_FRAMECHANGED); fullscreen=true;
        }
    } else {
        SetWindowLongPtrW(hwnd,GWL_STYLE,GetWindowLongPtrW(hwnd,GWL_STYLE)|WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd,&placement); SetWindowPos(hwnd,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED); fullscreen=false;
    }
}
std::filesystem::path savePath() {
    const char* appdata=std::getenv("LOCALAPPDATA");
    std::filesystem::path folder=appdata?std::filesystem::path(appdata)/"AmberGrove":std::filesystem::temp_directory_path()/"AmberGrove";
    std::error_code ec; std::filesystem::create_directories(folder,ec); return folder/"record.txt";
}
void loadRecord() {
    std::ifstream in(savePath()); int best=0,muted=0;
    if(in>>best>>muted) { game.best=std::clamp(best,0,10000000); game.muted=muted!=0; }
}
void saveRecord() {
    std::ofstream out(savePath()); if(out) out<<std::max(game.best,game.score)<<' '<<game.muted<<'\n';
}
class Audio {
    std::array<std::vector<char>,6> waves;
public:
    Audio() {
        const float frequencies[]={470,920,180,120,660,780};
        for(size_t k=0;k<waves.size();++k) {
            constexpr int rate=22050; int n=k>=4?rate/3:rate/9; auto& wav=waves[k]; wav.resize(44+n*2);
            auto put=[&](int pos,uint32_t value,int bytes) { for(int i=0;i<bytes;++i) wav[pos+i]=char(value>>(8*i)); };
            std::memcpy(wav.data(),"RIFF",4); put(4,36+n*2,4); std::memcpy(wav.data()+8,"WAVEfmt ",8);
            put(16,16,4); put(20,1,2); put(22,1,2); put(24,rate,4); put(28,rate*2,4); put(32,2,2); put(34,16,2);
            std::memcpy(wav.data()+36,"data",4); put(40,n*2,4);
            for(int i=0;i<n;++i) {
                float t=float(i)/rate,f=frequencies[k]+(k==0?t*2000:k==3?-t*250:0);
                float a=std::sin(t*f*6.2831853f)*.7f+std::sin(t*f*12.56637f)*.2f;
                int16_t v=int16_t(a*4200*(1.f-float(i)/n)*std::min(1.f,i/80.f)); put(44+i*2,uint16_t(v),2);
            }
        }
    }
    void play(amber::Sound s) { PlaySoundA(waves[size_t(s)].data(),nullptr,SND_MEMORY|SND_ASYNC|SND_NODEFAULT); }
    void stop() { PlaySoundA(nullptr,nullptr,0); }
    ~Audio() { stop(); }
};
}

int runWindows() {
    SetProcessDPIAware();
    HINSTANCE instance=GetModuleHandleW(nullptr);
    WNDCLASSW wc{}; wc.lpfnWndProc=windowProc; wc.hInstance=instance; wc.lpszClassName=ClassName;
    wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    if(!RegisterClassW(&wc)) { MessageBoxW(nullptr,L"Falha ao registrar a janela.",L"Amber Grove",MB_ICONERROR); return 1; }
    int scale=GetSystemMetrics(SM_CYSCREEN)>=800&&GetSystemMetrics(SM_CXSCREEN)>=1300?2:1;
    RECT r{0,0,amber::W*scale,amber::H*scale}; AdjustWindowRect(&r,WS_OVERLAPPEDWINDOW,FALSE);
    HWND hwnd=CreateWindowExW(0,ClassName,L"Amber Grove | Bosque de Ambar",WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,nullptr,nullptr,instance,nullptr);
    if(!hwnd) { MessageBoxW(nullptr,L"Falha ao criar a janela.",L"Amber Grove",MB_ICONERROR); return 1; }
    loadRecord(); Audio audio; ShowWindow(hwnd,SW_SHOW); UpdateWindow(hwnd);
    using Clock=std::chrono::steady_clock; auto previous=Clock::now(); float accumulator=0;
    bool running=true; int savedBest=game.best; bool savedMute=game.muted;
    while(running) {
        auto frameStart=Clock::now(); MSG msg;
        while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) { if(msg.message==WM_QUIT) running=false; TranslateMessage(&msg); DispatchMessageW(&msg); }
        if(!running) break;
        if(toggleFullscreen) { toggle(hwnd); toggleFullscreen=false; }
        float delta=std::chrono::duration<float>(frameStart-previous).count(); previous=frameStart;
        accumulator+=std::min(delta,.10f);
        while(accumulator>=amber::Step) {
            amber::Input input=edge; input.left=keys['A']||keys[VK_LEFT]; input.right=keys['D']||keys[VK_RIGHT];
            game.update(input); edge={}; accumulator-=amber::Step;
            if(game.muted) audio.stop(); else for(auto s:game.sounds) audio.play(s);
        }
        if(savedBest!=game.best||savedMute!=game.muted) { saveRecord(); savedBest=game.best; savedMute=game.muted; }
        amber::render(game,canvas); present(hwnd);
        std::this_thread::sleep_until(frameStart+std::chrono::microseconds(16667));
    }
    saveRecord(); return 0;
}
#endif
