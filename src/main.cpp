#include "game.hpp"
#include <exception>
#include <iostream>

#ifdef _WIN32
int runWindows();
#endif

int main(int argc,char** argv) {
    try {
        if(argc>=3 && std::string(argv[1])=="--screenshot") {
            amber::Game game; game.reset(); game.time=3.2f;
            std::string scene=argc>=4?argv[3]:"play";
            if(scene=="title") game.mode=amber::Mode::Title;
            else if(scene=="end") { game.camera=3560; game.player.pos={3930,274}; game.score=7300; game.collected=53; game.elapsed=162; }
            else { game.camera=320; game.player.pos={710,209}; game.player.vel={170,-70}; game.score=1200; game.collected=12; game.elapsed=24; }
            amber::Canvas canvas; amber::render(game,canvas); canvas.savePPM(argv[2]);
            std::cout<<"Frame rendered: "<<argv[2]<<'\n'; return 0;
        }
#ifdef _WIN32
        return runWindows();
#else
        std::cout<<"Amber Grove: graphical play is available on Windows.\n"
                    "Headless rendering: ./AmberGrove --screenshot frame.ppm [title|play|end]\n";
        return 0;
#endif
    } catch(const std::exception& e) { std::cerr<<"Amber Grove: "<<e.what()<<'\n'; return 1; }
}
