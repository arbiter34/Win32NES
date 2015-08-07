#include "stdafx.h"
#include "Controller.h"


Controller::Controller()
{
	for (int i = 0; i < 8; i++) {
		buttons[0][i] = 0;
		buttons[1][i] = 0;
	}
}


Controller::~Controller()
{
}

void  Controller::key_down(WPARAM wParam){
	if (controller_map_1.find(wParam) != controller_map_1.end()) {
		buttons[0][controller_map_1.at(wParam)] = 1;
	}

	if (controller_map_2.find(wParam) != controller_map_2.end()) {
		buttons[1][controller_map_2.at(wParam)] = 1;
	}
}

void  Controller::key_up(WPARAM wParam){
	if (controller_map_1.find(wParam) != controller_map_1.end()) {
		buttons[0][controller_map_1.at(wParam)] = 0;
	}

	if (controller_map_2.find(wParam) != controller_map_2.end()) {
		buttons[1][controller_map_2.at(wParam)] = 0;
	}
}


uint8_t  Controller::read_controller_state(int controller_num){
	uint8_t state = 0;
	if (index < 0x08 && buttons[controller_num][index]) {
		state = 1;
	}
	index++;
	if ((strobe & 1) == 1) {
		index = 0;
	}
	return state;
}


void  Controller::write_value(uint8_t word){
	strobe = word;

	if ((strobe & 0x01) == 1) {
		index = 0;
	}
}

const std::map<unsigned char, int> Controller::controller_map_1 = {
	{ 0x31, 0 },
	{ 0x32, 1 },
	{ 0x33, 2 },
	{ 0x34, 3 },
	{ 0x51, 4 },
	{ 0x57, 5 },
	{ 0x45, 6 },
	{ 0x52, 7 },
	{ 0x41, 8 },
	{ 0x53, 9 },
	{ 0x44, 10 },
	{ 0x46, 11 },
	{ 0x5A, 12 },
	{ 0x58, 13 }

};

const std::map<unsigned char, int> Controller::controller_map_2 = {
	{ 0x31, 0 },
	{ 0x32, 1 },
	{ 0x33, 2 },
	{ 0x34, 3 },
	{ 0x51, 4 },
	{ 0x57, 5 },
	{ 0x45, 6 },
	{ 0x52, 7 },
	{ 0x41, 8 },
	{ 0x53, 9 },
	{ 0x44, 10 },
	{ 0x46, 11 },
	{ 0x5A, 12 },
	{ 0x58, 13 }

};

bool Controller::buttons[2][8];
