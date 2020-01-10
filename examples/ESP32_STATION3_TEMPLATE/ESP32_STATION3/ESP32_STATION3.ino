/*
 Name:		ESP32_STATION3.ino
 Created:	09.01.2020 23:17:56
 Author:	piotr
*/

// the setup function runs once when you press reset or power the board
#include <Preferences.h>
#include <xb_ScreenText_VT100.h>
#include <xb_ScreenText.h>
#include <xb_GUI_Gadget.h>
#include <xb_GUI.h>
#include <xb_SERIAL.h>
#include <xb_board.h>
void setup() {
    XB_BOARD_SETUP();
    board.AddTask(&XB_SERIAL_DefTask);

}

// the loop function runs over and over again until power down or reset
void loop() {
    XB_BOARD_LOOP();
}
