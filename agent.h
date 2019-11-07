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
#include "weight.h"
#include <fstream>
#include <cmath>

#define TUPLE4_SIZE 8
#define TUPLE6_SIZE 4

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
 * base agent for agents with weight tables
 */
 /**
 * array-based board for 2048
 *
 * index (1-d form):
 *  (0)  (1)  (2)  (3)
 *  (4)  (5)  (6)  (7)
 *  (8)  (9) (10) (11)
 * (12) (13) (14) (15)
 *
 */ 

class weight_agent : public agent {
public:
weight_agent(const std::string& args = "") : agent(args),
    tuple4({{ 
        {{0,1,2,3}},
        {{3,7,11,15}},
        {{15,14,13,12}},
        {{12,8,4,0}},
        {{1,5,9,13}},
        {{7,6,5,4}},
        {{14,10,6,2}},
        {{8,9,10,11}},
    }}),
    tuple6({{
        {{8,5,2,4,1,0}},
        {{1,6,11,2,7,3}},
        {{7,10,13,11,14,15}},
        {{14,9,4,13,8,12}},
    }})
    {
		if (meta.find("init") != meta.end()) // pass init=... to initialize the weight
			init_weights(meta["init"]);
		if (meta.find("load") != meta.end()) // pass load=... to load from a specific file
			load_weights(meta["load"]);
	}
	virtual ~weight_agent() {
		if (meta.find("save") != meta.end()) // pass save=... to save to a specific file
			save_weights(meta["save"]);
	}

protected:
	virtual void init_weights(const std::string& info) {
        uint32_t n=TUPLE4_SIZE>>2;
        for(uint32_t i=0;i<n;i++){
            net.emplace_back(65536);// create an empty weight table with size 65536
        }
        n=TUPLE6_SIZE>>2;
        for(uint32_t i=0;i<n;i++){
            net.emplace_back(16777216);
        }
        
        // now net.size() == 2; net[0].size() == 65536; net[1].size() == 65536 
    }
	virtual void load_weights(const std::string& path) {
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;
		in.close();
	}
	virtual void save_weights(const std::string& path) {
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (weight& w : net) out << w;
		out.close();
	}
    
public:
    float V_function(const board& board,bool storefeatures){
        float value=0;
        if(net.size()==0)return float(rand());
        for(uint32_t i=0;i<TUPLE4_SIZE;i++){
            uint32_t feature=0;
            for(int pos : tuple4[i]){
                feature=(feature<<4);
                feature+=board(pos);
            }
            uint32_t j=i>>2; 
            value+=net[j][feature];

            if(storefeatures)
                features4[i]=feature;
        }
        for(uint32_t i=0;i<TUPLE6_SIZE;i++){
            uint32_t feature=0;
            for(int pos : tuple6[i]){
                feature=(feature<<4);
                feature+=board(pos);
            }
            uint32_t j=(i+TUPLE4_SIZE)>>2;
            value+=net[j][feature];

            if(storefeatures)
                features6[i]=feature;
        }
        return value;
    }
    
    void weight_update(float loss,float learning_rate){
        if(net.size()==0)return;
        float dW=learning_rate*loss;
        for(uint32_t i=0;i<TUPLE4_SIZE;i++){
            uint32_t j=i>>2;
            net[j][features4[i]]+=dW;
            uint32_t prediction_features=features4[i]+0b0001000100010001;
            net[j][prediction_features]+=dW;
        }
        for(uint32_t i=0;i<TUPLE6_SIZE;i++){
            uint32_t j=(i+TUPLE4_SIZE)>>2;
            net[j][features6[i]]+=1.5*dW;
            uint32_t prediction_features=features6[i]+0b000100010001000100010001;
            net[j][prediction_features]+=1.5*dW;
        }
    }

	friend std::ostream& operator <<(std::ostream& out, const weight_agent& w) {
		out << "Weights:" << std::endl;
		for (int i=0;i<100;i++){
            std::cout<<w.net[0][i*100]<<std::endl;
        }
		out << std::endl;
		return out;
	}
protected:
    std::array<std::array<int, 4>, TUPLE4_SIZE> tuple4;
	std::array<std::array<int, 6>, TUPLE6_SIZE> tuple6;
    std::array<uint32_t, TUPLE4_SIZE> features4;
    std::array<uint32_t, TUPLE6_SIZE> features6;
    std::vector<weight> net;
};

/*

class weight_agent : public agent {
public:
    weight_agent(const std::string& args = "") : agent(args),
    tuple4({{ 
        { {0,1,2,3} },
        { {3,7,11,15} },
        { {15,14,13,12} },
        { {12,8,4,0} },
        { {1,5,9,13} },
        { {7,6,5,4} },
        { {14,10,6,2} },
        { {8,9,10,11} },
        { {1,2,3,7} },
        { {7,11,15,14} },
        { {14,13,12,8} },
        { {8,4,0,1} },
        { {0,1,4,5} },
        { {3,7,6,2} },
        { {15,14,11,10} },
        { {12,8,9,13} },
    }})
    {
		if (meta.find("init") != meta.end()) // pass init=... to initialize the weight
			init_weights(meta["init"]);
		if (meta.find("load") != meta.end()) // pass load=... to load from a specific file
			load_weights(meta["load"]);
	}
	virtual ~weight_agent() {
		if (meta.find("save") != meta.end()) // pass save=... to save to a specific file
			save_weights(meta["save"]);
	}

protected:
	virtual void init_weights(const std::string& info) {
		net.emplace_back(65536); // create an empty weight table with size 65536
		net.emplace_back(65536); // create an empty weight table with size 65536
		net.emplace_back(65536); // create an empty weight table with size 65536
		net.emplace_back(65536);
        
        // now net.size() == 2; net[0].size() == 65536; net[1].size() == 65536 
    }
	virtual void load_weights(const std::string& path) {
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;
		in.close();
	}
	virtual void save_weights(const std::string& path) {
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (weight& w : net) out << w;
		out.close();
	}
    
public:
    float V_function(const board& board,bool storefeatures){

        float value=0;
        if(net.size()==0)return float(rand());
        for(uint32_t i=0;i<tuple4.size();i++){
            uint32_t feature=0;
            for(int pos : tuple4[i]){
                feature=(feature<<4);
                feature+=board(pos);
            }
            uint32_t j=i>>2; 
            value+=net[j][feature];

            if(storefeatures)
                features[i]=feature;
        }

        return value;
    }
    
    void weight_update(float loss,float learning_rate){
        if(net.size()==0)return;
        float dW=learning_rate*loss;
        for(uint32_t i=0;i<tuple4.size();i++){
            uint32_t j=i>>2;
            net[j][features[i]]+=dW;
        }
    }

	friend std::ostream& operator <<(std::ostream& out, const weight_agent& w) {
		out << "Weights:" << std::endl;
		for (int i=0;i<100;i++){
            std::cout<<w.net[0][i*100]<<std::endl;
        }
		out << std::endl;
		return out;
	}
protected:
    std::array<std::array<int, 4>, 16> tuple4;
	std::vector<weight> net;
    std::array<uint32_t, 16> features;
};

*/


/**
 * base agent for agents with a learning rate
 */
class learning_agent : public agent {
public:
	learning_agent(const std::string& args = "") : agent(args), alpha(0.1f) {
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~learning_agent() {}
    
    inline float get_alpha(){return alpha;}

protected:
	float alpha;
};



/**
 * dummy player
 * select a legal action randomly
 */
class player : public random_agent{
public:
	player(const std::string& args = "") : 
        random_agent("name=dummy role=player " + args),
		WTF_weight_agent(args),
        WTF_learning_agent(args),
        last_opcode(666),
        opcode({ 0, 1, 2, 3 }),
        movecnt(0),last_V(0){}

	virtual action take_action(const board& before) {
        //std::cout<<"player act"<<std::endl;
		//std::shuffle(opcode.begin(), opcode.end(), engine);
		
        board best_board(before);
        unsigned best_op=6;
        float best_VR=0;
        for (unsigned op : opcode) {
            board b(before);
			board::reward R = b.slide(op);
            if(R!=-1){
                float V = WTF_weight_agent.V_function(b,false);
                float VR=V+(float)R;
                if(best_op==6){
                    best_op=op;
                    best_VR=VR;
                    best_board=b;
                }
                else if(best_VR<VR){
                    best_op=op;
                    best_VR=VR;
                    best_board=b;
                }
            }
        }
        
        if(best_op==6){
            //std::cout<<"Game over"<<std::endl;
            WTF_weight_agent.weight_update(-last_V,WTF_learning_agent.get_alpha());
            return action();
            //illegel -> game over
        }
        if(movecnt>0)
            WTF_weight_agent.weight_update(best_VR-last_V,WTF_learning_agent.get_alpha());
        
        movecnt+=1;
        last_opcode=best_op;
        last_V=WTF_weight_agent.V_function(best_board,true);
        return action::slide(best_op);
	}
    virtual void open_episode(const std::string& flag = "") {last_opcode=666;movecnt=0;last_V=0;}
public:
    weight_agent WTF_weight_agent;
    learning_agent WTF_learning_agent;
    unsigned last_opcode;
private:
	std::array<unsigned, 4> opcode;
    float movecnt;
    float last_V;
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
        space({{ 
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

