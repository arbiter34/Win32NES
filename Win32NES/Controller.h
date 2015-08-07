#pragma once
#include <stdint.h>
#include <map>

class Controller
{

	typedef enum button {
		Button_A,
		Button_B,
		Button_Select,
		Button_Start,
		Button_Up,
		Button_Down,
		Button_Left,
		Button_Right
	};

public:
	Controller();
	~Controller();

	void key_down(WPARAM wParam);
	void key_up(WPARAM wParam);

	uint8_t read_controller_state(int controller_num);
	void write_value(uint8_t word);

private:

	static bool buttons[2][8];
	static const std::map<unsigned char, int> controller_map_1;
	static const std::map<unsigned char, int> controller_map_2;
	uint8_t index;
	uint8_t strobe;
};

