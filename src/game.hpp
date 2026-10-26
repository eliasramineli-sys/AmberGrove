#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace amber {
constexpr int W=640, H=360;
constexpr float Step=1.f/120.f, WorldWidth=4200.f;
using Color=uint32_t;
struct Vec { float x=0,y=0; };
struct Rect { float x,y,w,h; };
inline bool overlap(Rect a,Rect b) {
    return a.x<b.x+b.w && a.x+a.w>b.x && a.y<b.y+b.h && a.y+a.h>b.y;
}
struct Input { bool left=false,right=false,jump=false,attack=false,start=false,pause=false,restart=false,mute=false; };
enum class Mode { Title, Playing, Paused, Won, GameOver };
enum class Sound { Jump, Coin, Hit, Hurt, Checkpoint, Win };
struct Player {
    Vec pos{72,270}, vel{};
    bool grounded=false;
    int facing=1,lives=3;
    float coyote=0,buffer=0,invincible=0,attack=0,cooldown=0;
    Rect box() const { return {pos.x,pos.y,18,30}; }
};
struct Coin { Vec pos; bool taken=false; };
struct Enemy { Vec pos; float left,right,speed; bool alive=true; Rect box() const { return {pos.x,pos.y,25,19}; } };
struct Particle { Vec p,v; float life,total; Color color; int size; };
struct Checkpoint { float x; bool active=false; };
struct Game {
    Player player;
    Mode mode=Mode::Title;
    std::vector<Rect> solids,spikes;
    std::vector<Coin> coins;
    std::vector<Enemy> enemies;
    std::vector<Particle> particles;
    std::vector<Checkpoint> checkpoints;
    std::vector<Sound> sounds;
    int score=0,collected=0,best=0;
    float camera=0,time=0,elapsed=0,shake=0,respawn=72,banner=0;
    bool muted=false;
    uint32_t random=127123;
    Game();
    void reset();
    void update(const Input&,float dt=Step);
    void damage();
    void burst(Vec p,Color color,int count);
    void emit(Sound s) { if (!muted) sounds.push_back(s); }
};

struct Canvas {
    std::vector<Color> pixels;
    Canvas():pixels(W*H){}
    void clear(Color c);
    void pixel(int x,int y,Color c);
    void blend(int x,int y,Color c,int alpha);
    void rect(int x,int y,int w,int h,Color c);
    void line(int x0,int y0,int x1,int y1,Color c);
    void circle(int x,int y,int radius,Color c);
    void ellipse(int x,int y,int rx,int ry,Color c);
    void poly(const std::vector<Vec>& points,Color c);
    void text(int x,int y,const std::string&,Color c,int scale=1);
    int textWidth(const std::string& s,int scale=1) const { return int(s.size())*6*scale; }
    void centered(int y,const std::string& s,Color c,int scale=1) { text((W-textWidth(s,scale))/2,y,s,c,scale); }
    void savePPM(const std::string&) const;
};
void render(const Game&,Canvas&);
} // namespace amber
