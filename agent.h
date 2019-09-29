#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include <iostream>
#include "board.h"
#include "action.h"

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}
protected:
	std::default_random_engine engine;
};

/**
 * dummy player
 * select a legal action randomly
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=dummy role=player " + args),
		last_opcode(666),
        opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {
        //std::cout<<"player act"<<std::endl;
		std::shuffle(opcode.begin(), opcode.end(), engine);
		for (unsigned op : opcode) {
			board::reward reward = board(before).slide(op);
            //check if action legel
			if (reward != -1){
                last_opcode=op;
                return action::slide(op);
            }
        }
        //std::cout<<"Game over"<<std::endl;
		return action();
        //illegel -> game over
	}
    
    virtual void open_episode(const std::string& flag = "") {last_opcode=666;}
public:
    unsigned last_opcode;

private:
	std::array<unsigned, 4> opcode;
};




/**
 * random environment
 * add a new random tile to an empty cell
 * 2-tile: 90%
 * 4-tile: 10%
 */
class rndenv : public random_agent {
public:
    rndenv(const std::string& args = "",player* pp=0) : random_agent("name=random role=environment " + args),
        space(
        {{ 
            { {12,13,14,15} },
            { {0,4,8,12} },
            { {0,1,2,3} },
            { {3,7,11,15} }
        }}),bag({1,2,3}),used_tiles(0),pplayer(pp),
        WTF_space_only_for_initial({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15})
        {
            std::shuffle(WTF_space_only_for_initial.begin(), WTF_space_only_for_initial.end(), engine);
        }
        
	virtual action take_action(const board& after) {
        //std::cout<<"env act"<<std::endl;
        board::cell tile;
        //board::cell next_tile;
        tile=bag[used_tiles];
        used_tiles++;
        if(used_tiles==3){
            std::shuffle(bag.begin(), bag.end(), engine);
            used_tiles=0;
        }
        //next_tile=bag[used_tiles];
        //std::cout<<"last_opcode:"<<pplayer->last_opcode<<std::endl;
        /*
        switch (pplayer->last_opcode) {
            case 0: space={12,13,14,15};//slide_up() 
            case 1: space={0,4,8,12};//slide_right() 
            case 2: space={0,1,2,3};//slide_down() 
            case 3: space={3,7,11,15};//slide_left()
		}*/
        unsigned lop=pplayer->last_opcode;
        if(lop==666){
            for (int pos : WTF_space_only_for_initial) {
                if (after(pos) != 0) continue;
                return action::place(pos, tile);
            }
        }
		std::shuffle(space[lop].begin(), space[lop].end(), engine);
		for (int pos : space[lop]) {
			if (after(pos) != 0) continue;
            //check if action legel
            //pos = location
            //tile = the number
			return action::place(pos, tile);
		}
        
		return action();
        //illegel -> game over
	}
    virtual void open_episode(const std::string& flag = "") {
        used_tiles=0;
        std::shuffle(WTF_space_only_for_initial.begin(), WTF_space_only_for_initial.end(), engine);
        }
private:

    std::array<std::array<int, 4>, 4> space;
    std::array<board::cell, 3> bag;
    int used_tiles;
    player* pplayer;
    std::array<int,16> WTF_space_only_for_initial;
	//std::uniform_int_distribution<int> popup;
};

