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
	{ 0x31, Button_A },
	{ 0x32, Button_B },
	{ 0x33, Button_Select },
	{ 0x34, Button_Start },
	{ 0x51, Button_Up },
	{ 0x57, Button_Down },
	{ 0x45, Button_Left },
	{ 0x52, Button_Right }
};

const std::map<unsigned char, int> Controller::controller_map_2 = {
	{ 0x31, Button_A },
	{ 0x32, Button_B },
	{ 0x33, Button_Select },
	{ 0x34, Button_Start },
	{ 0x51, Button_Up },
	{ 0x57, Button_Down },
	{ 0x45, Button_Left },
	{ 0x52, Button_Right }
};

bool Controller::buttons[2][8];
