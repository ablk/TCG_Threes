/**
 * Basic Environment for Game 2048
 * use 'g++ -std=c++11 -O3 -g -o 2048 2048.cpp' to compile the source
 *
 * Computer Games and Intelligence (CGI) Lab, NCTU, Taiwan
 * http://www.aigames.nctu.edu.tw
 */

#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include "board.h"
#include "action.h"
#include "agent.h"
#include "episode.h"
#include "statistic.h"

int main(int argc, const char* argv[]) {
	std::cout << "threes-Demo: ";
	std::copy(argv, argv + argc, std::ostream_iterator<const char*>(std::cout, " "));
	std::cout << std::endl << std::endl;

	size_t total = 1000, block = 0, limit = 0;
	std::string play_args, evil_args;
	std::string load, save;
	bool summary = false;
    bool vb=false;
	for (int i = 1; i < argc; i++) {
		std::string para(argv[i]);
		if (para.find("--total=") == 0) {
			total = std::stoull(para.substr(para.find("=") + 1));
		} else if (para.find("--block=") == 0) {
			block = std::stoull(para.substr(para.find("=") + 1));
		} else if (para.find("--limit=") == 0) {
			limit = std::stoull(para.substr(para.find("=") + 1));
		} else if (para.find("--play=") == 0) {
			play_args = para.substr(para.find("=") + 1);
		} else if (para.find("--evil=") == 0) {
			evil_args = para.substr(para.find("=") + 1);
		} else if (para.find("--load=") == 0) {
			load = para.substr(para.find("=") + 1);
		} else if (para.find("--save=") == 0) {
			save = para.substr(para.find("=") + 1);
		} else if (para.find("--summary") == 0) {
			summary = true;
		} else if (para.find("-v") == 0) {
			vb = true;
		}
	}

	statistic stat(total, block, limit);

	if (load.size()) {
		std::ifstream in(load, std::ios::in);
		in >> stat;
		in.close();
		summary |= stat.is_finished();
	}

    //agent
	player play(play_args);
	rndenv evil(evil_args,&play);

	while (!stat.is_finished()) {
		play.open_episode("~:" + evil.name());
		evil.open_episode(play.name() + ":~");

		stat.open_episode(play.name() + ":" + evil.name());
		episode& game = stat.back();
        int i=0;
		while (true) {

			agent& who = game.take_turns(play, evil);
			//return play or evil
            action move = who.take_action(game.state());
            //strategy define in agent.take_action
            //return action::slide or action::place
            
            if(vb){
                std::cout<<"================="<<i++<<"================="<<std::endl;
                std::cout<<game.state()<<std::endl;
            }
            //std::cout<<move<<std::endl;
            
			if (game.apply_action(move) != true) break;
            //reward and system define in board.slide/place
            //call action::slide/place.apply(ep_state)
            //than call board.slide/place, abtain reward
            //reward==-1 than game over
			if (who.check_for_win(game.state())) break;
            //no win for this game, always false 
		}
        if(vb)std::cout<<game.state()<<std::endl;
		agent& win = game.last_turns(play, evil);
		stat.close_episode(win.name());

		play.close_episode(win.name());
		evil.close_episode(win.name());
	}
    if(vb)std::cout<<play.WTF_weight_agent<<std::endl;

	if (summary) {
		stat.summary();
	}

	if (save.size()) {
		std::ofstream out(save, std::ios::out | std::ios::trunc);
		out << stat;
		out.close();
	}

	return 0;
}
