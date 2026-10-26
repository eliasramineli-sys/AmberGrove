#include "game.hpp"
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <stdexcept>

namespace amber {
namespace {
constexpr Color ink=0x142c35,cream=0xffefd0,gold=0xffc86b;
int hash(int x,int y=0) { uint32_t z=uint32_t(x)*374761393u+uint32_t(y)*668265263u; z=(z^(z>>13))*1274126177u; return int((z^(z>>16))&0x7fffffff); }
Color mix(Color a,Color b,float t) {
    auto ch=[&](int n){return int(((a>>n)&255)*(1-t)+((b>>n)&255)*t);};
    return Color((ch(16)<<16)|(ch(8)<<8)|ch(0));
}
std::string number(int n,int digits=0) { std::ostringstream s; s<<std::setw(digits)<<std::setfill('0')<<n; return s.str(); }
void panel(Canvas& c,int x,int y,int w,int h) {
    c.rect(x+3,y+4,w,h,0x102b31); c.rect(x,y,w,h,0x547f78); c.rect(x+1,y+1,w-2,h-2,0x152f36);
    c.line(x+2,y+2,x+w-3,y+2,0x325750);
}
void star(Canvas& c,int x,int y,Color color,int r=3) { c.line(x-r,y,x+r,y,color); c.line(x,y-r,x,y+r,color); c.pixel(x+1,y+1,0xffebc0); }
void coinSprite(Canvas& c,int x,int y,float t) {
    int rw=2+int(3*std::abs(std::cos(t)));
    c.ellipse(x,y,rw+1,7,0x6a431f); c.ellipse(x,y-1,rw,6,0xffbd4b); c.ellipse(x-1,y-2,std::max(1,rw-2),4,0xffe49b);
    c.line(x,y-4,x,y+2,0xfff2c3); c.pixel(x+rw,y-3,0xfff5d2);
}
void tree(Canvas& c,int x,int y,int scale,int seed,bool distant=false) {
    Color dark=distant?0x28595a:0x254b39,mid=distant?0x326660:0x3e7647,light=distant?0x3d7469:0x65a056;
    c.poly({{float(x-5*scale),float(y)},{float(x-2*scale),float(y-79*scale)},{float(x+5*scale),float(y-74*scale)},{float(x+8*scale),float(y)}},distant?0x2e5551:0x604e3e);
    c.rect(x,y-73*scale,2*scale,73*scale,distant?0x35605a:0x92704a);
    for(int pass=0;pass<3;++pass) for(int i=0;i<18;++i) {
        int dx=(hash(seed,i)%61-30)*scale,dy=(hash(seed+20,i)%47-23)*scale;
        int r=(13+hash(seed+100,i)%13)*scale;
        if(pass==0) c.circle(x+dx,y-80*scale+dy,r,dark);
        if(pass==1) c.circle(x+dx-2*scale,y-83*scale+dy,r-2*scale,mid);
        if(pass==2&&i%3==0) c.ellipse(x+dx-4*scale,y-88*scale+dy,(r-5*scale),std::max(1,(r-5*scale)/2),light);
    }
    if(!distant) for(int i=0;i<32;++i) { int dx=(hash(seed+90,i)%63-32)*scale,dy=(hash(seed+80,i)%53-27)*scale; c.rect(x+dx,y-85*scale+dy,2*scale,scale,0xacc578); }
}
void background(const Game& g,Canvas& c) {
    for(int y=0;y<H;++y) c.rect(0,y,W,1,y<230?mix(0x194f6b,0xb6cba5,float(y)/230):mix(0xb6cba5,0x315e5a,float(y-230)/130));
    int sunX=474-int(g.camera*.025f);
    for(int r=65;r>28;r-=5) { int alpha=3; for(int yy=-r;yy<=r;++yy) for(int xx=-r;xx<=r;++xx) if(xx*xx+yy*yy<r*r) c.blend(sunX+xx,90+yy,0xffd793,alpha); }
    c.circle(sunX,90,28,0xe3d598); c.circle(sunX-4,85,22,0xf0dda2);
    for(int i=-1;i<7;++i) {
        int x=i*150-int(g.camera*.06f)%150,y=48+hash(i+40)%62;
        c.rect(x,y,90,3,0xa0bbb0); c.rect(x+19,y-3,76,3,0xb0c4b2); c.rect(x+41,y-6,40,3,0xb0c4b2);
    }
    for(int layer=0;layer<3;++layer) {
        float speed=.10f+layer*.10f; int spacing=190-layer*25,base=222+layer*31;
        Color col=layer==0?0x558b8e:layer==1?0x396e76:0x28575e;
        for(int i=-2;i<8;++i) {
            int idx=int(g.camera*speed)/spacing+i,x=idx*spacing-int(g.camera*speed);
            int height=75+hash(idx+layer*50)%55;
            c.poly({{float(x-80),float(base)},{float(x+30),float(base-height)},{float(x+55),float(base-height+18)},{float(x+140),float(base)},{float(x+160),float(H)},{float(x-80),float(H)}},col);
            if(layer==0) c.poly({{float(x+4),float(base-height+24)},{float(x+30),float(base-height)},{float(x+55),float(base-height+18)},{float(x+38),float(base-height+15)},{float(x+28),float(base-height+11)}},0x9cbbaf);
        }
    }
    for(int i=-2;i<12;++i) {
        int idx=int(g.camera*.35f)/85+i,x=idx*85-int(g.camera*.35f);
        tree(c,x,333+(hash(idx+8)%20),1,idx+30,true);
    }
    for(int i=-1;i<6;++i) {
        int idx=int(g.camera*.58f)/190+i,x=idx*190-int(g.camera*.58f);
        tree(c,x,333,1,idx+180);
    }
    // A distant waterfall, animated in world space.
    int wx=1395-int(g.camera*.38f);
    if(wx>-30&&wx<W+30) { c.rect(wx,164,18,160,0x5d9e9d); c.rect(wx+3,164,4,160,0x9ec7b5); for(int i=0;i<12;++i) c.rect(wx+8,169+(i*17+int(g.time*24))%155,6,9,0x81b5ac); }
    for(int y=258;y<304;y+=3) for(int x=0;x<W;x+=3) c.blend(x,y,0xb6ccb1,5);
}
void terrain(Canvas& c,Rect s,int camera) {
    int x=int(s.x)-camera,y=int(s.y),w=int(s.w),h=int(s.h);
    if(x>W+24||x+w< -24) return;
    c.rect(x+3,y+7,w,h,0x18332f);
    c.rect(x,y+6,w,h-6,0x463f36);
    for(int row=0;row<(h+18)/18;++row) for(int col=0;col<(w+23)/24;++col) {
        int gx=int(s.x)/24+col,xx=x+col*24+(row%2?-10:0),yy=y+9+row*18;
        int variation=hash(gx,row)%4;
        const Color shades[]={0x78533b,0x8b6241,0x704d37,0x967046};
        int left=std::max(x,xx+1),right=std::min(x+w-1,xx+23);
        int bottom=std::min(y+h,yy+17);
        if(right<=left||bottom<=yy) continue;
        c.rect(left+1,yy+1,right-left-2,bottom-yy-2,shades[variation]);
        c.rect(left+3,yy,right-left-6,1,shades[variation]);
        c.rect(left,yy+3,1,std::max(0,bottom-yy-6),shades[variation]);
        c.line(left+2,yy,right-3,yy,0x9a7950);
        c.line(left,yy+1,left,bottom-2,0x8d6e48);
        c.line(left+2,bottom-2,right-2,bottom-2,0x4e4638);
        if(right-left>10&&bottom-yy>10) { c.rect(left+5,yy+5,4,2,0x9a7950); c.pixel(left+12,yy+8,0x534734); }
    }
    c.rect(x,y,w,5,0x254e38); c.rect(x+1,y,w-2,3,0x92bc5e); c.rect(x+2,y-2,w-4,2,0xc1d97b);
    for(int i=0;i<w;i+=6) {
        int depth=4+hash(int(s.x)+i)%9;
        c.rect(x+i,y+3,5,depth,0x3d7138); c.rect(x+i,y+3,2,depth-2,0x7ba449);
        if(hash(i+int(s.x),12)%3==0) { c.line(x+i,y-2,x+i-2,y-5,0x85b561); c.line(x+i+1,y-1,x+i+3,y-6,0xa2c476); }
    }
    if(s.y<300) {
        for(int i=10;i<w-7;i+=21) {
            int len=8+hash(i+int(s.x))%19;
            c.line(x+i,y+h,x+i-2,y+h+len,0x3a6747);
            c.rect(x+i-4,y+h+len/2,4,2,0x799958); c.rect(x+i-1,y+h+len-4,4,2,0x517e4b);
        }
    }
}
void hero(Canvas& c,int x,int y,const Game& g) {
    const auto& p=g.player;
    if(p.invincible>0&&int(g.time*18)%2) return;
    bool running=std::abs(p.vel.x)>20&&p.grounded;
    int stride=running?int(std::sin(g.time*17)*4):0;
    int bob=running?int(std::sin(g.time*34)>0):int(std::sin(g.time*3)>0);
    y-=bob;
    auto r=[&](int a,int b,int w,int h,Color col) { c.rect(x+(p.facing>0?a:18-a-w),y+b,w,h,col); };
    // Scarf, boots, tunic, hair and satchel use a limited hand-selected palette.
    r(-5,9,11,5,0x592d32); r(-8-int(std::sin(g.time*12)*2),11,8,3,0xcb654b); r(-7,10,6,1,0xf29968);
    r(4+stride,24,6,6,ink); r(11-stride,23,6,7,ink);
    r(4+stride,25,5,3,0x634c43); r(10-stride,24,5,4,0x96734d); r(5+stride,29,7,2,0x1a3035); r(10-stride,29,8,2,0x1a3035);
    r(3,13,14,12,ink); r(4,13,12,10,0x287e83); r(5,14,4,8,0x5db4ac); r(12,15,4,8,0x23595e);
    r(3,22,14,3,0x7e4f35); r(10,22,3,3,gold); r(4,21,2,2,0xb79862);
    r(1,15,5,7,0x18444e); r(0,20,5,4,0xe1ac73); r(1,20,3,2,0xffd596);
    r(13,14,6,7,0x215e67); r(15,20,5,4,0xe8b47c); r(15,20,4,2,0xffd596);
    r(3,3,14,12,ink); r(4,4,12,10,0xf4c38a); r(5,5,9,3,0xffdb9e); r(14,8,5,4,0xf4c38a);
    r(13,7,2,3,0x203a3b); r(14,7,1,1,cream); r(10,12,5,1,0xa86547);
    r(2,2,14,5,0x44352e); r(4,0,9,4,0x44352e); r(1,4,5,7,0x44352e); r(2,3,5,3,0x916245); r(5,1,7,2,0xb2814c);
    r(3,6,14,3,0xd27649); r(3,6,14,1,0xffbf6c); r(3,9,3,3,0x7d5038);
    r(-1,14,5,8,0x5b4b35); r(-1,14,4,2,0xab8a50);
    if(p.attack>0) {
        int cx=x+(p.facing>0?19:-4);
        c.line(cx,y+20,cx+p.facing*26,y-1,ink); c.line(cx+1,y+19,cx+p.facing*26,y-3,0xb9e5d5);
        c.line(cx+2,y+20,cx+p.facing*28,y-1,0xfff2be);
        for(int i=0;i<26;++i) { float a=-1.3f+i*.08f; c.rect(cx+int(std::cos(a)*29)*p.facing,y+11+int(std::sin(a)*27),2,2,i%3?0xf3d494:0xffffff); }
    }
}
void slime(Canvas& c,int x,int y,float time,float phase) {
    int b=int(std::sin(time*6+phase)*2);
    c.ellipse(x+13,y+18,16,3,0x1b3735);
    c.ellipse(x+12,y+11+b,13,10-b,ink); c.ellipse(x+12,y+9+b,12,9-b,0x438254); c.ellipse(x+10,y+7+b,9,7-b,0x77b56a);
    c.rect(x+4,y+4+b,4,2,0xb6d57e); c.rect(x+16,y+4+b,2,1,0x9fc979);
    c.rect(x+6,y+9+b,4,6,cream); c.rect(x+16,y+9+b,4,6,cream); c.rect(x+8,y+11+b,2,3,ink); c.rect(x+18,y+11+b,2,3,ink);
    c.rect(x+11,y+16,4,1,0x254637); c.rect(x+2,y+17,5,2,0x658f54); c.rect(x+20,y+17,5,2,0x658f54);
}
void flag(Canvas& c,int x,bool active,float t) {
    c.rect(x-3,298,13,6,0x32443a); c.rect(x+1,248,3,53,0xb5ab7c); c.rect(x+4,254,23,15,active?0xeab76c:0x476b65);
    c.rect(x+4,254,23,2,active?0xffdc8d:0x74978a); c.rect(x+25,258+int(std::sin(t*5)),4,11,active?0xce9150:0x375850);
    star(c,x+15,261,active?cream:0x91aa91,3); c.circle(x+2,247,3,gold);
}
void portal(Canvas& c,int x,float time) {
    c.rect(x-4,238,8,65,0x8d8b69); c.rect(x+40,238,8,65,0x72785d);
    c.rect(x-7,232,14,10,0xbac09a); c.rect(x+37,232,14,10,0xa4af86);
    c.poly({{float(x),236},{float(x+8),218},{float(x+20),212},{float(x+34),219},{float(x+44),236}},0x9da885);
    c.poly({{float(x+7),241},{float(x+12),227},{float(x+21),223},{float(x+32),229},{float(x+38),242},{float(x+38),303},{float(x+7),303}},0x234b46);
    for(int yy=238;yy<301;yy+=3) for(int xx=8;xx<38;xx+=2) { float k=(std::sin(time*3+yy*.12f+xx*.1f)+1)/2; c.pixel(x+xx,yy,mix(0x3e877b,0xffd98a,k)); }
    for(int i=0;i<8;++i) star(c,x+int(22+std::sin(time+i*4)*24),int(238+std::fmod(time*12+i*8,62.f)),gold,1);
    c.rect(x-10,302,65,3,0xb5bf94);
}
void hud(const Game& g,Canvas& c) {
    panel(c,12,12,616,35);
    star(c,29,29,gold,7); c.text(43,22,"AMBER",cream,1); c.text(43,33,"GROVE",0x82b5a3,1);
    c.rect(86,20,1,19,0x3b5c55);
    for(int i=0;i<3;++i) {
        int x=102+i*17; Color col=i<g.player.lives?0xea9379:0x3b5a57;
        c.circle(x,26,3,col); c.circle(x+5,26,3,col); c.poly({{float(x-3),27},{float(x+8),27},{float(x+2),34}},col);
    }
    coinSprite(c,169,29,0); c.text(182,26,number(g.collected,2),gold);
    c.text(227,21,"SCORE",0x82b5a3); c.text(227,32,number(g.score,6),cream);
    c.text(303,21,"BOSQUE DE AMBAR",cream); c.rect(303,34,158,3,0x36564e);
    c.rect(303,34,int(158*std::clamp(g.player.pos.x/4080,0.f,1.f)),3,gold);
    c.text(487,21,"01 / 01",cream); c.text(487,32,number(int(g.elapsed)/60,2)+":"+number(int(g.elapsed)%60,2),0x82b5a3);
    c.text(560,22,"P PAUSE",0xc6cba7); c.text(560,33,g.muted?"M  OFF":"M   ON",0x82b5a3);
    if(g.player.pos.x<260&&g.elapsed<12) {
        panel(c,16,319,328,27); c.text(26,329,"A D  MOVE   SPACE  JUMP   J  ATTACK",cream);
    } else { c.text(18,338,"AMBER GROVE  /  THE FIRST LIGHT",0xb6c5a5); }
    if(g.banner>0) { panel(c,211,60,218,27); c.centered(70,"CHECKPOINT ATIVADO",gold); }
}
}

void Canvas::clear(Color c) { std::fill(pixels.begin(),pixels.end(),c); }
void Canvas::pixel(int x,int y,Color c) { if(x>=0&&x<W&&y>=0&&y<H) pixels[y*W+x]=c; }
void Canvas::blend(int x,int y,Color c,int a) { if(x>=0&&x<W&&y>=0&&y<H) pixels[y*W+x]=mix(pixels[y*W+x],c,float(a)/255); }
void Canvas::rect(int x,int y,int w,int h,Color c) {
    int x0=std::max(0,x),x1=std::min(W,x+w),y0=std::max(0,y),y1=std::min(H,y+h);
    if(x1<=x0) return;
    for(int yy=y0;yy<y1;++yy) std::fill(pixels.begin()+yy*W+x0,pixels.begin()+yy*W+x1,c);
}
void Canvas::line(int x0,int y0,int x1,int y1,Color c) {
    int dx=std::abs(x1-x0),sx=x0<x1?1:-1,dy=-std::abs(y1-y0),sy=y0<y1?1:-1,e=dx+dy;
    for(;;) { pixel(x0,y0,c); if(x0==x1&&y0==y1) break; int e2=2*e; if(e2>=dy) { e+=dy; x0+=sx; } if(e2<=dx) { e+=dx; y0+=sy; } }
}
void Canvas::circle(int x,int y,int r,Color c) { ellipse(x,y,r,r,c); }
void Canvas::ellipse(int x,int y,int rx,int ry,Color c) {
    if(rx<=0||ry<=0) return;
    for(int yy=std::max(-ry,-y);yy<=ry&&y+yy<H;++yy) { float f=1-float(yy*yy)/float(ry*ry); int w=int(rx*std::sqrt(std::max(0.f,f))); rect(x-w,y+yy,w*2+1,1,c); }
}
void Canvas::poly(const std::vector<Vec>& p,Color col) {
    if(p.size()<3) return;
    float low=p[0].y,high=low; for(auto v:p) { low=std::min(low,v.y); high=std::max(high,v.y); }
    for(int y=std::max(0,int(low));y<=std::min(H-1,int(high));++y) {
        std::vector<float> nodes;
        for(size_t i=0,j=p.size()-1;i<p.size();j=i++) if((p[i].y<y&&p[j].y>=y)||(p[j].y<y&&p[i].y>=y)) nodes.push_back(p[i].x+(y-p[i].y)/(p[j].y-p[i].y)*(p[j].x-p[i].x));
        std::sort(nodes.begin(),nodes.end()); for(size_t i=0;i+1<nodes.size();i+=2) rect(int(nodes[i]),y,int(nodes[i+1]-nodes[i])+1,1,col);
    }
}
void Canvas::text(int x,int y,const std::string& s,Color c,int scale) {
    static const std::string letters="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:./-+!?%";
    static const std::array<std::array<int,7>,43> glyphs={{
        {{14,17,17,31,17,17,17}},{{30,17,17,30,17,17,30}},{{14,17,16,16,16,17,14}},{{30,17,17,17,17,17,30}},
        {{31,16,16,30,16,16,31}},{{31,16,16,30,16,16,16}},{{14,17,16,23,17,17,15}},{{17,17,17,31,17,17,17}},
        {{31,4,4,4,4,4,31}},{{7,2,2,2,18,18,12}},{{17,18,20,24,20,18,17}},{{16,16,16,16,16,16,31}},
        {{17,27,21,21,17,17,17}},{{17,25,21,19,17,17,17}},{{14,17,17,17,17,17,14}},{{30,17,17,30,16,16,16}},
        {{14,17,17,17,21,18,13}},{{30,17,17,30,20,18,17}},{{15,16,16,14,1,1,30}},{{31,4,4,4,4,4,4}},
        {{17,17,17,17,17,17,14}},{{17,17,17,17,17,10,4}},{{17,17,17,21,21,27,17}},{{17,17,10,4,10,17,17}},
        {{17,17,10,4,4,4,4}},{{31,1,2,4,8,16,31}},{{14,17,19,21,25,17,14}},{{4,12,4,4,4,4,14}},
        {{14,17,1,2,4,8,31}},{{30,1,1,14,1,1,30}},{{2,6,10,18,31,2,2}},{{31,16,16,30,1,1,30}},
        {{14,16,16,30,17,17,14}},{{31,1,2,4,8,8,8}},{{14,17,17,14,17,17,14}},{{14,17,17,15,1,1,14}},
        {{0,4,4,0,4,4,0}},{{0,0,0,0,0,6,6}},{{1,2,2,4,8,8,16}},{{0,0,0,31,0,0,0}},
        {{0,4,4,31,4,4,0}},{{4,4,4,4,4,0,4}},{{14,17,1,2,4,0,4}}
    }};
    for(char ch:s) {
        size_t k=letters.find(char(std::toupper(static_cast<unsigned char>(ch))));
        if(k<glyphs.size()) for(int row=0;row<7;++row) for(int col=0;col<5;++col) if(glyphs[k][row]&(1<<(4-col))) rect(x+col*scale,y+row*scale,scale,scale,c);
        x+=6*scale;
    }
}
void Canvas::savePPM(const std::string& path) const {
    std::ofstream out(path,std::ios::binary); if(!out) throw std::runtime_error("Cannot create screenshot: "+path);
    out<<"P6\n"<<W<<" "<<H<<"\n255\n";
    for(Color p:pixels) { const char bytes[]={char(p>>16),char(p>>8),char(p)}; out.write(bytes,3); }
    if(!out) throw std::runtime_error("Failed writing screenshot");
}
void render(const Game& g,Canvas& c) {
    background(g,c);
    int cam=int(g.camera)+(g.shake>0?int(std::sin(g.time*130)*3):0);
    for(auto s:g.solids) terrain(c,s,cam);
    for(auto s:g.spikes) for(int i=0;i<int(s.w);i+=12) {
        int x=int(s.x)+i-cam,y=int(s.y);
        c.poly({{float(x),float(y+12)},{float(x+5),float(y)},{float(x+11),float(y+12)}},ink);
        c.poly({{float(x+2),float(y+11)},{float(x+5),float(y+2)},{float(x+8),float(y+11)}},0xbec4a8);
        c.line(x+5,y+3,x+6,y+9,0xe0d9b8);
    }
    for(auto cp:g.checkpoints) flag(c,int(cp.x)-cam,cp.active,g.time);
    portal(c,4082-cam,g.time);
    for(size_t i=0;i<g.coins.size();++i) if(!g.coins[i].taken) { const auto& p=g.coins[i].pos; coinSprite(c,int(p.x)-cam,int(p.y+std::sin(g.time*3+i)*2),g.time*3+i*.4f); }
    for(size_t i=0;i<g.enemies.size();++i) if(g.enemies[i].alive) slime(c,int(g.enemies[i].pos.x)-cam,int(g.enemies[i].pos.y),g.time,float(i));
    if(g.player.grounded) c.ellipse(int(g.player.pos.x+9)-cam,int(g.player.pos.y+31),12,2,0x254337);
    hero(c,int(g.player.pos.x)-cam-1,int(g.player.pos.y)-2,g);
    // Foreground grasses and glowing motes break up the rigid tile edges.
    for(auto s:g.solids) if(s.y==304) for(int i=15;i<int(s.w);i+=53) {
        int x=int(s.x)+i-cam; c.line(x,304,x-2,298,0x5d9456); c.line(x,304,x+4,299,0x8ab46b);
        if(hash(i+int(s.x))%4==0) { c.line(x+5,304,x+5,297,0x658556); c.rect(x+3,294,5,3,0xe7c282); c.pixel(x+5,295,0xffe1a2); }
    }
    for(const auto& p:g.particles) c.rect(int(p.p.x)-cam,int(p.p.y),p.size,p.size,p.color);
    for(int i=0;i<27;++i) {
        int x=(hash(i+590)%760-int(g.camera*.7f)+int(g.time*(4+i%4)))%760; if(x<0) x+=760;
        int y=90+hash(i+100)%215+int(std::sin(g.time*.7+i)*9);
        if(i%4==0) star(c,x,y,0xb8bd77,1); else c.pixel(x,y,0xb5c391);
    }
    hud(g,c);
    if(g.mode==Mode::Title) {
        for(int y=55;y<313;++y) for(int x=12;x<389;++x) c.blend(x,y,0x0d2b32,180);
        c.text(35,77,"UMA AVENTURA EM PIXEL ART",0x99bba6);
        c.text(34,105,"AMBER",0x0d2731,6); c.text(32,102,"AMBER",cream,6);
        c.text(35,153,"GROVE",0x0d2731,6); c.text(33,150,"GROVE",gold,6);
        c.text(36,210,"BOSQUE DE AMBAR",0xa4c3ad,2);
        c.text(36,237,"ATRAVESSE O BOSQUE. ENCONTRE A LUZ.",cream);
        panel(c,35,263,268,30); c.text(48,274,"ENTER / SPACE  INICIAR A AVENTURA",gold);
        c.text(36,304,"C++17  /  ARTE E MOTOR PROPRIOS",0x9eaf98);
        c.text(424,327,"F11  TELA CHEIA",cream);
    } else if(g.mode!=Mode::Playing) {
        for(int y=0;y<H;++y) for(int x=0;x<W;++x) c.blend(x,y,0x0a202a,150);
        panel(c,133,105,374,166);
        std::string title=g.mode==Mode::Paused?"PAUSADO":g.mode==Mode::Won?"O BOSQUE DESPERTOU":"FIM DA AVENTURA";
        c.centered(125,title,gold,2);
        c.centered(158,g.mode==Mode::Won?"VOCE ENCONTROU O PORTAL DE AMBAR.":g.mode==Mode::Paused?"RESPIRE. A AVENTURA ESPERA.":"CADA TENTATIVA ABRE UM CAMINHO.",cream);
        c.centered(182,"PONTOS  "+number(g.score,6)+"   RECORDE  "+number(g.best,6),0xa8c4ac);
        c.centered(206,"MOEDAS  "+number(g.collected)+" / "+number(int(g.coins.size())),gold);
        c.centered(238,g.mode==Mode::Paused?"P / ESC  VOLTAR     R  REINICIAR":"ENTER  JOGAR NOVAMENTE",cream);
    }
}
} // namespace amber
