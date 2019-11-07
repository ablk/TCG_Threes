#pragma once
#include <array>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <map>

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
std::map<uint32_t,int> tile_decode_table=
{
    {0,0},
    {1,1},
    {2,2},
    {3,3},
    {4,6},
    {5,12},
    {6,24},
    {7,48},
    {8,96},
    {9,192},
    {10,384},
    {11,768},
    {12,1536},
    {13,3072},
    {14,6144},
    {15,12288}
};
unsigned opcode_decode_table[4][8]=
{//[op][board_flip]
    {0,3,2,1,0,3,2,1},
    {1,0,3,2,3,2,1,0},
    {2,1,0,3,2,1,0,3},
    {3,2,1,0,1,0,3,2},
};

class board {
public:
	typedef uint32_t cell;
	typedef std::array<cell, 4> row;
	typedef std::array<row, 4> grid;
	typedef uint64_t data;
	typedef int reward;

public:
	board() : tile(), attr(0){}
	board(const grid& b, data v = 0) : tile(b), attr(v){}
	board(const board& b) = default;
	board& operator =(const board& b) = default;

	operator grid&() { return tile; }
	operator const grid&() const { return tile; }
	row& operator [](unsigned i) { return tile[i]; }
	const row& operator [](unsigned i) const { return tile[i]; }
	cell& operator ()(unsigned i) { return tile[i / 4][i % 4]; }
	const cell& operator ()(unsigned i) const { return tile[i / 4][i % 4]; }

	data info() const { return attr; }
	data info(data dat) { data old = attr; attr = dat; return old; }

public:
	bool operator ==(const board& b) const { return tile == b.tile; }
	bool operator < (const board& b) const { return tile <  b.tile; }
	bool operator !=(const board& b) const { return !(*this == b); }
	bool operator > (const board& b) const { return b < *this; }
	bool operator <=(const board& b) const { return !(b < *this); }
	bool operator >=(const board& b) const { return !(*this < b); }

public:

	/**
	 * place a tile (index value) to the specific position (1-d form index)
	 * return 0 if the action is valid, or -1 if not
	 */
	reward place(unsigned pos, cell tile) {
		if (pos >= 16) return -1;
		if (tile != 1 && tile != 2&& tile!=3) return -1;
		operator()(pos) = tile;
		return 0;
	}

	/**
	 * apply an action to the board
	 * return the reward of the action, or -1 if the action is illegal
	 */
	reward slide(unsigned opcode) {
		switch (opcode & 0b11) {
		case 0: return slide_up();
		case 1: return slide_right();
		case 2: return slide_down();
		case 3: return slide_left();
		default: return -1;
		}
	}

	reward slide_left() {
		board prev = *this;
		reward score = 0;
		for (int r = 0; r < 4; r++) {
			auto& row = tile[r];
            uint32_t hold=row[0];
			for (int c = 1; c < 4; c++) {
                if(hold==0){
                    row[c-1]=row[c];
                    row[c]=0;
                    continue;
                }
                if (hold!=0) {
                    int sum=row[c]+hold;
                    if (sum==3){
                        row[c-1]=sum;
                        score += sum;
                        row[c]=0;
                        hold=0;
                    }
                    else if (row[c] == hold && row[c]>=3) {
                        row[c-1]+=1;
                        score += tile_decode_table[sum];
                        row[c]=0;
                        hold=0;
                    }
                    else {
                        hold=row[c];
                    }
                }
			}
		}
		return (*this != prev) ? score : -1;
	}
	reward slide_right() {
		reflect_horizontal();
		reward score = slide_left();
		reflect_horizontal();
		return score;
	}
	reward slide_up() {
		rotate_right();
		reward score = slide_right();
		rotate_left();
		return score;
	}
	reward slide_down() {
		rotate_right();
		reward score = slide_left();
		rotate_left();
		return score;
	}

	void transpose() {
		for (int r = 0; r < 4; r++) {
			for (int c = r + 1; c < 4; c++) {
				std::swap(tile[r][c], tile[c][r]);
			}
		}
	}

	void reflect_horizontal() {
		for (int r = 0; r < 4; r++) {
			std::swap(tile[r][0], tile[r][3]);
			std::swap(tile[r][1], tile[r][2]);
		}
	}

	void reflect_vertical() {
		for (int c = 0; c < 4; c++) {
			std::swap(tile[0][c], tile[3][c]);
			std::swap(tile[1][c], tile[2][c]);
		}
	}

	/**
	 * rotate the board clockwise by given times
	 */
	void rotate(int r = 1) {
		switch (((r % 4) + 4) % 4) {
		default:
		case 0: break;
		case 1: rotate_right(); break;
		case 2: reverse(); break;
		case 3: rotate_left(); break;
		}
	}
    
    void Flip_board(int r=0){
        switch(r){
            case 0: break;
            case 1: rotate_right(); break;
            case 2: reverse(); break;
            case 3: rotate_left(); break;
            case 4: reflect_horizontal(); break;
            case 5: rotate_right(); reflect_horizontal(); break;
            case 6: reflect_vertical(); break;
            case 7: rotate_left(); reflect_horizontal(); break;
            default : break;
        }
    }
    

	void rotate_right() { transpose(); reflect_horizontal(); } // clockwise
	void rotate_left() { transpose(); reflect_vertical(); } // counterclockwise
	void reverse() { reflect_horizontal(); reflect_vertical(); }
    
    void initboard(){
        std::array<int, 16> space={ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        std::array<cell, 9> initbag={1,1,1,2,2,2,3,3,3};
        std::random_shuffle(space.begin(),space.end());
        for(int i=0;i<9;i++){
            int pos=space[i];
            operator()(pos)=initbag[i];
        }
    
    }
    void clear(){
        for(unsigned i=0;i<16;i++)this->operator()(i)=0;
    }
    
    uint32_t& max_tile(){
        return *std::max_element(&(this->operator()(0)), &(this->operator()(16)));
    }
    

public:
	friend std::ostream& operator <<(std::ostream& out, const board& b) {
		out << "+------------------------+" << std::endl;
		for (auto& row : b.tile) {
			out << "|" << std::dec;
        for (auto t : row) out << std::setw(6) << tile_decode_table[t];
			out << "|" << std::endl;
		}
		out << "+------------------------+" << std::endl;
		return out;
	}

private:
	grid tile;
	data attr;
};
