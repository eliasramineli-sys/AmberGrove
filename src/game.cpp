#include "game.hpp"

namespace amber {
Game::Game() { reset(); mode=Mode::Title; }
void Game::reset() {
    player=Player{}; score=collected=0; elapsed=camera=shake=0; respawn=72; banner=0;
    particles.clear(); sounds.clear(); coins.clear(); enemies.clear(); checkpoints={{1510,false},{2640,false}};
    solids={{0,304,900,100},{1010,304,800,100},{1920,304,910,100},{2950,304,1250,100},
            {300,250,120,24},{490,206,120,24},{690,251,100,24},
            {1100,250,120,24},{1280,206,120,24},{1490,248,100,24},{1620,240,100,24},
            {2010,250,120,24},{2190,204,120,24},{2390,248,120,24},{2580,204,120,24},
            {3010,248,120,24},{3190,202,120,24},{3400,248,120,24},{3620,250,96,24}};
    spikes={{640,292,48,12},{1420,292,48,12},{2330,292,48,12},{3330,292,72,12}};
    for (int x=160;x<4100;x+=72) {
        bool safe=false; for (auto s:solids) if(s.y==304 && x>s.x+24 && x<s.x+s.w-24) safe=true;
        for(auto s:spikes) if(x>s.x-24 && x<s.x+s.w+24) safe=false;
        if(safe) coins.push_back({{float(x),272},false});
    }
    for(auto s:solids) if(s.y<300) for(float x=s.x+22;x<s.x+s.w-10;x+=28) coins.push_back({{x,s.y-24},false});
    for(auto p:std::vector<Vec>{{916,250},{950,235},{984,250},{1828,250},{1864,235},{1900,250},{2846,246},{2890,232},{2930,246}}) coins.push_back({p,false});
    enemies={{{420,285},360,540,35},{{760,285},700,850,-29},{{1180,285},1080,1270,34},
             {{1570,285},1500,1740,-37},{{2040,285},1970,2140,35},{{2460,285},2380,2560,-40},
             {{3110,285},3010,3240,40},{{3540,285},3450,3700,-44},{{3880,285},3780,4020,44}};
    mode=Mode::Playing;
}
void Game::burst(Vec p,Color c,int count) {
    for(int i=0;i<count;++i) {
        random^=random<<13; random^=random>>17; random^=random<<5;
        float vx=float(int(random%181)-90),vy=-float(30+random%140);
        float life=.35f+float(random%50)/100;
        particles.push_back({p,{vx,vy},life,life,c,1+int(random%3)});
    }
}
void Game::damage() {
    if(player.invincible>0 || mode!=Mode::Playing) return;
    burst({player.pos.x+9,player.pos.y+15},0xffb67a,18); emit(Sound::Hurt); shake=.25f;
    --player.lives;
    if(player.lives<=0) { mode=Mode::GameOver; best=std::max(best,score); return; }
    player.pos={respawn,274}; player.vel={0,0}; player.invincible=1.7f;
    player.attack=player.cooldown=player.coyote=player.buffer=0; player.grounded=false;
    camera=std::clamp(respawn-150.f,0.f,WorldWidth-W);
}
void Game::update(const Input& in,float dt) {
    dt=std::clamp(dt,0.f,.033f); sounds.clear();
    if(in.mute) muted=!muted;
    if(mode==Mode::Title) { time+=dt; if(in.start||in.jump) reset(); return; }
    if(in.restart) { reset(); return; }
    if(in.pause) { if(mode==Mode::Playing) mode=Mode::Paused; else if(mode==Mode::Paused) mode=Mode::Playing; }
    if(mode!=Mode::Playing) { if((mode==Mode::Won||mode==Mode::GameOver)&&in.start) reset(); return; }
    time+=dt; elapsed+=dt; shake=std::max(0.f,shake-dt); banner=std::max(0.f,banner-dt);
    for(auto& p:particles) { p.life-=dt; p.v.y+=210*dt; p.p.x+=p.v.x*dt; p.p.y+=p.v.y*dt; }
    particles.erase(std::remove_if(particles.begin(),particles.end(),[](auto& p){return p.life<=0;}),particles.end());
    auto& p=player;
    p.invincible=std::max(0.f,p.invincible-dt); p.attack=std::max(0.f,p.attack-dt); p.cooldown=std::max(0.f,p.cooldown-dt);
    p.buffer=in.jump?.12f:std::max(0.f,p.buffer-dt);
    p.coyote=p.grounded?.10f:std::max(0.f,p.coyote-dt);
    int dir=int(in.right)-int(in.left); if(dir) p.facing=dir;
    float target=dir*190.f,accel=(p.grounded?1800.f:1100.f)*dt;
    p.vel.x+=std::clamp(target-p.vel.x,-accel,accel);
    if(p.buffer>0 && p.coyote>0) { p.vel.y=-340; p.buffer=p.coyote=0; p.grounded=false; burst({p.pos.x+9,p.pos.y+29},0xb4c9aa,7); emit(Sound::Jump); }
    if(in.attack && p.cooldown<=0) { p.attack=.19f; p.cooldown=.35f; emit(Sound::Hit); }
    p.vel.y=std::min(p.vel.y+860*dt,620.f);
    p.pos.x+=p.vel.x*dt;
    for(auto s:solids) if(overlap(p.box(),s)) { p.pos.x=p.vel.x>0?s.x-18:s.x+s.w; p.vel.x=0; }
    p.pos.x=std::clamp(p.pos.x,0.f,WorldWidth-18);
    float oldBottom=p.pos.y+30; p.pos.y+=p.vel.y*dt; p.grounded=false;
    for(auto s:solids) if(overlap(p.box(),s)) {
        if(p.vel.y>=0) { p.pos.y=s.y-30; p.grounded=true; } else p.pos.y=s.y+s.h;
        p.vel.y=0;
    }
    for(auto& coin:coins) if(!coin.taken && overlap(p.box(),{coin.pos.x-6,coin.pos.y-8,12,16})) {
        coin.taken=true; collected++; score+=100; emit(Sound::Coin); burst(coin.pos,0xffd57c,7);
    }
    for(auto& e:enemies) if(e.alive) {
        e.pos.x+=e.speed*dt;
        if(e.pos.x<e.left) { e.pos.x=e.left; e.speed=std::abs(e.speed); }
        if(e.pos.x>e.right) { e.pos.x=e.right; e.speed=-std::abs(e.speed); }
        Rect slash{p.pos.x+(p.facing>0?12.f:-40.f),p.pos.y-4,46,40};
        if(p.attack>0 && overlap(slash,e.box())) { e.alive=false; score+=250; burst({e.pos.x+12,e.pos.y+10},0x8cdf87,16); shake=.08f; }
        else if(overlap(p.box(),e.box())) {
            if(p.vel.y>0 && oldBottom<=e.pos.y+8) { e.alive=false; p.vel.y=-240; score+=250; burst(e.pos,0x8cdf87,15); emit(Sound::Hit); }
            else { damage(); if(mode!=Mode::Playing) return; }
        }
    }
    for(auto s:spikes) if(overlap(p.box(),s)) { damage(); break; }
    if(p.pos.y>H+70) { p.invincible=0; damage(); }
    for(auto& cp:checkpoints) if(!cp.active && p.pos.x>cp.x) {
        cp.active=true; respawn=cp.x+16; score+=500; banner=2.5f; emit(Sound::Checkpoint); burst({cp.x+8,257},0xffd57c,25);
    }
    if(p.pos.x>4080) { mode=Mode::Won; score+=player.lives*500; best=std::max(best,score); emit(Sound::Win); }
    float targetCam=std::clamp(p.pos.x-210.f,0.f,WorldWidth-W);
    camera+=(targetCam-camera)*std::min(1.f,dt*7);
}
} // namespace amber
