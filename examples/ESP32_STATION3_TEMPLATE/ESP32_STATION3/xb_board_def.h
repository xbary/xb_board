#ifndef XB_BOARD_DEF_H
#define XB_BOARD_DEF_H

#define BOARD_DEVICE_NAME "STATION_3"
// Nazwa urzadzenia


#define BOARD_DEVICE_VERSION "0.1 (2020.01.10)"
// Tekst opisuj¹cy wersje urz¹dzenia


// #define BOARD_LED_LIFE_PIN 5
// GPIO (np 5 dla WEMOS LOLIN D32 PRO) do którego pod³¹czony jest np LED blikuj¹cy co sekundê.
// Równoczeœnie co sekundê jest wysy³any message IM_LIVE_BLINK do wszystkich zadañ bez wzglêdu 
// czy zdefiniowany pin BOARD_LED_LIFE_PIN.


//#define BOARD_LED_TX_PIN ?
//#define BOARD_LED_RX_PIN ?
// GPIO do którego pod³¹czone s¹ ledy informuj¹ce ¿e coœ jest odbierane oraz wysy³ane z urz¹dzenia.
// Mo¿na pomin¹æ lub te¿ zdefiniowaæ tylko  BOARD_LED_TX_PIN  lub  BOARD_LED_RX_PIN.
// Za wyzwolenie tej funkcjonalnoœci odpowiadaj¹ funkcje Blink_RX() oraz Blink_TX(), funkcje oprócz
// zapalaniu LEDów wczeœniej  zdefiniowanych wysy³aj¹ messaga IM_RX_BLINK, IM_TX_BLINK do wszystkich 
// zadañ. Mo¿na podaæ w argumencie ID w celu rozpoznania w messagu o które RX TX nam chodzi.

#define BOARD_LED_TX_STATUS_OFF LOW
#define BOARD_LED_RX_STATUS_OFF LOW
// Definiowanie czy wygaszenie LEDów jest przy stanie LOW czy HIGH pinów wczeœniej zdefiniowanych.

#define TICK_LED_BLINK 250
// Ustalenie czasu jak d³ugo ma LED_TX LED_RX œwieciæ po wyzwoleniu funkcjami Blink_RX() oraz Blink_TX()


#define XB_GUI
// Jeœ³i zdefiniowano to ca³y system zacznie korzystaæ z biblioteki xb_gui.h


#define XB_PREFERENCES
// Jeœli zdefiniowano to biblioteka zacznie korzystaæ do obs³ugi konfiguracji w pamiêci flash z 
// biblioteki Preferences.h (ESP32) .


// Jeœli chcemy aby funkcja Log() biblioteki wysy³a³a na UART jakieœ komunikaty oraz ¿eby 
// biblioteka pobiera³a kody z RX jako wciskanie klawiszy, nale¿y najpierw dodaæ do projektu
// biblioteke XB_SERIAL. Podstawowa konfiguracja XB_SERIAL polega na ustawieniu takich makr:

#define Serial0BoardBuf_BAUD 921600
// Okreœla szybkoœæ pierwszego UARTa w urz¹dzeniu, oraz równoczeœnie go uruchamia

#define SERIAL0_SizeRXBuffer 1024
// Definicja wielkoœci bufora odbiorczego

#define Serial0BoardBuf_UseKeyboard
// Jeœli zdefiniujemy to zostanie dodany stream z którego korzystaæ bêdzie klawiatura jako kody 
// klawiszy nadchodz¹ce w RX

#define Serial0BoardBuf_UseLog
// Jeœli zdefiniowane to biblioteka XB_BOARD a konkretnie funkcja Log() zacznie korzystaæ poprzez
// stream do wysy³ania komunikatów na TX uarta pierwszego

#define Serial0BoardBuf_UseGui
// Jeœli zdefiniowane to na pierwszy uart bêdzie rysowane GUI

//#define Serial0Board_RX_PIN ?
//#define Serial0Board_TX_PIN ?
// Dla ESP32 mo¿emy zdefiniowaæ na których pinach zostanie uruchomiony pierwszy UART.
// Jeœli nie podamy tych definicji to u¿yte zostan¹ standardowe przyporz¹dkowania pinów.

//#define Serial0Board_UseDebugOutPut
// Jeœli zdefiniujemy to na pierwszy UART zostan¹ skierowane komunikaty diagnostyczne frameworku 
// arduino dla ESP32.

#endif
