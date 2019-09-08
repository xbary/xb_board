# xb_board

*** Uruchamianie projektu .INO opartego na bibliotece XB_BOARD ***

 -------------------------------------
 KROK 1. Dodajemy bibliotekę XB_BOARD.

Tak powinien wyglądać główny plik projektu, należy dopisa dwa makra których kod odwołuje się do funkcji biblioteki.

#include <xb_board.h>
 
 void setup()
{
	XB_BOARD_SETUP();
}

void loop()
{
	XB_BOARD_LOOP();
}
- eof -

-----------------------------------------------------------------------------
KROK 2. Dodanie flagi dla kompilatora aby szukał inkludy w katalogu projektu.

We właściwościach projektu dodajemy w pozycji Extra flags: taki wpis -I{build.path}


-----------------------------------------------------
KROK 3. Dodanie pliku inkludy o nazwie xb_board_def.h

#ifndef XB_BOARD_DEF_H
#define XB_BOARD_DEF_H

#endif


------------------------------------------------------------------------------------------------
KROK 4. Kompilacja i uruchomienie. Sprawdzono działanie bilioteki na płytkach: ESP32 Dev module, 
        ESP32 wRover module.






*** MAKRA KONFIGURACJI ORAZ URUCHAMIAJĄCE FUNKCJE BIBLIOTEKI ***


#define DEVICE_NAME "..."
// Nazwa urzadzenia


#define DEVICE_VERSION "?.? (2019.01.1)"
// Tekst opisujący wersje urządzenia


#define BOARD_LED_LIFE_PIN 5
// GPIO (np 5 dla WEMOS LOLIN D32 PRO) do którego podłączony jest np LED blikujący co sekundę.
// Równocześnie co sekundę jest wysyłany message IM_LIVE_BLINK do wszystkich zadań bez względu 
// czy zdefiniowany pin BOARD_LED_LIFE_PIN.


#define BOARD_LED_TX_PIN ?
#define BOARD_LED_RX_PIN ?
// GPIO do którego podłączone są ledy informujące że coś jest odbierane oraz wysyłane z urządzenia.
// Można pominąć lub też zdefiniować tylko  BOARD_LED_TX_PIN  lub  BOARD_LED_RX_PIN.
// Za wyzwolenie tej funkcjonalności odpowiadają funkcje Blink_RX() oraz Blink_TX(), funkcje oprócz
// zapalaniu LEDów wcześniej  zdefiniowanych wysyłają messaga IM_RX_BLINK, IM_TX_BLINK do wszystkich 
// zadań. Można podać w argumencie ID w celu rozpoznania w messagu o które RX TX nam chodzi.

#define BOARD_LED_TX_STATUS_OFF LOW
#define BOARD_LED_RX_STATUS_OFF LOW
// Definiowanie czy wygaszenie LEDów jest przy stanie LOW czy HIGH pinów wcześniej zdefiniowanych.

#define TICK_LED_BLINK 250
// Ustalenie czasu jak długo ma LED_TX LED_RX świecić po wyzwoleniu funkcjami Blink_RX() oraz Blink_TX()


#define XB_GUI
// Jeśłi zdefiniowano to cały system zacznie korzystać z biblioteki xb_gui.h


#define XB_PREFERENCES
// Jeśli zdefiniowano to biblioteka zacznie korzystać do obsługi konfiguracji w pamięci flash z 
// biblioteki Preferences.h (ESP32) .


// Jeśli chcemy aby funkcja Log() biblioteki wysyłała na UART jakieś komunikaty oraz żeby 
// biblioteka pobierała kody z RX jako wciskanie klawiszy, należy najpierw dodać do projektu
// biblioteke XB_SERIAL. Podstawowa konfiguracja XB_SERIAL polega na ustawieniu takich makr:

#define Serial0Board_BAUD 115200
// Określa szybkość pierwszego UARTa w urządzeniu, oraz równocześnie go uruchamia

#define SERIAL0_SizeRXBuffer 1024
// Definicja wielkości bufora odbiorczego

#define Serial0Board_UseKeyboard
// Jeśli zdefiniujemy to zostanie dodany stream z którego korzystać będzie klawiatura jako kody 
// klawiszy nadchodzące w RX

#define Serial0Board_UseLog
// Jeśli zdefiniowane to biblioteka XB_BOARD a konkretnie funkcja Log() zacznie korzystać poprzez
// stream do wysyłania komunikatów na TX uarta pierwszego

#define Serial0Board_UseGui
// Jeśli zdefiniowane to na pierwszy uart będzie rysowane GUI

#define Serial0Board_RX_PIN ?
#define Serial0Board_TX_PIN ?
// Dla ESP32 możemy zdefiniować na których pinach zostanie uruchomiony pierwszy UART.
// Jeśli nie podamy tych definicji to użyte zostaną standardowe przyporządkowania pinów.

#define Serial0Board_UseDebugOutPut
// Jeśli zdefiniujemy to na pierwszy UART zostaną skierowane komunikaty diagnostyczne frameworku 
// arduino dla ESP32.

