# xb_board
<P>
<P>// Szablon projektu .INO z oparty na bibliotece xb_board
<P>/*
<P>#include <xb_board.h>
<P>  
<P>void setup()
<P>{
<P>	XB_BOARD_SETUP();
<P>}
<P>
<P>void loop()
<P>{
<P>	XB_BOARD_LOOP();
<P>}
<P>*/
<P>
<P>// Obowiązkowa inkluda z parametrami wstępnymi projektu
<P>/*
<P>#ifndef XB_BOARD_DEF_H
<P>#define XB_BOARD_DEF_H
<P>
<P>#define DEVICE_NAME "..."
<P>#define DEVICE_VERSION "?.? (2019.01.1)"
<P>
<P>// Standardowe ustawienia dla UART
<P>#define SerialBoard			Serial
<P>#define SerialBoard_BAUD	115200
<P>// Wskazanie GPIO na których ma zostać uruchomiony podstawowy UART
<P>//#define SerialBoard_RX_PIN  ?
<P>//#define SerialBoard_TX_PIN  ?
<P>
<P>//#define BOARD_LED_TX_PIN ?
<P>//#define BOARD_LED_RX_PIN ?
<P>//#define BOARD_LED_TX_STATUS_OFF LOW
<P>//#define BOARD_LED_RX_STATUS_OFF LOW
<P>//#define TICK_LED_BLINK 250
<P>
<P>// Definicje powodujące wstawienie standardowego GUI z GADGETAMI na terminalach VT100(?)
<P>//#define SCREENTEXT_TYPE_BOARDLOG
<P>//#define XB_GUI
<P>//#define XB_GUIGADGET
<P>
<P>// Definicja powodująca że task XB_Board systematycznie sprawdza dostępnąć połączenia internetowego i WIFI
<P>//#define XB_WIFI
<P>
<P>// GPIO do którego podłączony jest np LED informujący użytkownika czy urządzenie się nie zawiesiło
<P>#define BOARD_LED_LIFE_PIN 5
<P>
<P>#endif 
<P>*/
