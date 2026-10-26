#include "game.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace amber;
int checked=0;
void check(bool ok,const char* name) { if(!ok) throw std::runtime_error(name); ++checked; std::cout<<"PASS "<<name<<'\n'; }
void tick(Game& g,int n,Input in={}) { for(int i=0;i<n;++i) g.update(in); }
void land(Game& g) { tick(g,35); }
int main() {
    try {
        Game g; check(g.mode==Mode::Title,"initial title");
        Input in; in.start=true; g.update(in); land(g); check(g.player.grounded,"start and ground collision");
        float x=g.player.pos.x; in={}; in.right=true; tick(g,60,in);
        check(g.player.pos.x>x+65&&g.player.pos.x<x+100,"accelerated movement");
        in={}; in.jump=true; float y=g.player.pos.y; g.update(in); tick(g,30);
        check(g.player.pos.y<y-45,"jump height"); tick(g,120); check(g.player.grounded,"jump landing");
        g.reset(); land(g); g.player.pos={298,250}; g.player.vel={190,0}; in={}; in.right=true; tick(g,12,in);
        check(g.player.pos.x<=282.1f,"horizontal collision blocks wall");
        g.reset(); land(g); g.player.pos={890,274}; g.player.vel={190,0}; in={}; in.right=true;
        tick(g,14,in); check(!g.player.grounded&&g.player.coyote>0,"coyote grace after ledge");
        in.jump=true; g.update(in); check(g.player.vel.y< -300,"coyote jump");
        g.reset(); land(g); g.player.pos={300,200}; g.player.vel={0,170}; in={}; in.jump=true; g.update(in); tick(g,12);
        check(g.player.vel.y<0,"buffered jump on landing");
        g.reset(); g.player.pos={151,258}; g.update({}); check(g.collected==1&&g.score==100,"coin collected exactly once");
        g.update({}); check(g.collected==1,"coin does not duplicate");
        g.reset(); g.player.pos={390,274}; g.player.facing=1; in={}; in.attack=true; g.update(in);
        check(!g.enemies[0].alive&&g.score==250,"sword defeats enemy");
        g.reset(); g.player.pos={420,253}; g.player.vel={0,200}; tick(g,8);
        check(!g.enemies[0].alive&&g.player.vel.y<0,"stomp defeats enemy and bounces");
        g.reset(); g.player.pos={1514,274}; g.update({}); check(g.checkpoints[0].active&&g.respawn==1526,"checkpoint activated");
        g.player.pos={641,274}; g.update({}); check(g.player.lives==2&&g.player.pos.x==1526,"spike damage respawns at checkpoint");
        g.damage(); check(g.player.lives==2,"invincibility prevents repeat damage");
        g.player.invincible=0; g.damage(); g.player.invincible=0; g.damage(); check(g.mode==Mode::GameOver,"three lives and game over");
        in={}; in.restart=true; g.update(in); check(g.mode==Mode::Playing&&g.player.lives==3&&g.score==0,"restart resets run");
        in={}; in.pause=true; g.update(in); x=g.player.pos.x; in={}; in.right=true; tick(g,60,in);
        check(g.mode==Mode::Paused&&g.player.pos.x==x,"pause freezes world");
        in={}; in.pause=true; g.update(in); check(g.mode==Mode::Playing,"pause resumes");
        in={}; in.mute=true; g.update(in); check(g.muted,"mute state");
        g.player.pos={4090,274}; g.update({}); check(g.mode==Mode::Won&&g.best==g.score,"portal wins and records score");
        int best=g.best; g.reset(); check(g.best==best&&g.muted,"restart preserves record and settings");
        Canvas c; render(g,c); check(c.pixels.size()==size_t(W*H),"render complete framebuffer");
        c.rect(-50,-50,100,100,0x123456); c.rect(W+100,0,10,10,0); c.poly({{-100,-100},{-50,900},{900,100}},0xabc123);
        check(c.pixels[0]==0xabc123,"renderer clips offscreen geometry");

        // Automated traversal follows the ground route, crossing every gap and hazard.
        g=Game{}; g.reset();
        for(int frame=0;frame<120*60&&g.mode==Mode::Playing;++frame) {
            in={}; in.right=true; in.attack=true;
            if(g.player.grounded) {
                for(auto s:g.spikes) if(s.x-(g.player.pos.x+18)>0&&s.x-(g.player.pos.x+18)<18) in.jump=true;
                for(auto s:g.solids) if(s.y==304&&g.player.pos.x+9>=s.x&&g.player.pos.x+9<s.x+s.w&&s.x+s.w-(g.player.pos.x+18)<14) in.jump=true;
            }
            g.update(in);
        }
        std::cout<<"Traversal x="<<g.player.pos.x<<" lives="<<g.player.lives<<" time="<<g.elapsed<<'\n';
        check(g.mode==Mode::Won,"full level traversal reaches portal");
        std::cout<<checked<<" checks passed\n"; return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL "<<e.what()<<'\n'; return 1; }
}
