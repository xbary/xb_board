#ifndef XB_BOARD_DEF_H
#define XB_BOARD_DEF_H

#define BOARD_DEVICE_NAME "STATION_3"
// Nazwa urzadzenia


#define BOARD_DEVICE_VERSION "0.1 (2020.01.10)"
// Tekst opisuj�cy wersje urz�dzenia


// #define BOARD_LED_LIFE_PIN 5
// GPIO (np 5 dla WEMOS LOLIN D32 PRO) do kt�rego pod��czony jest np LED blikuj�cy co sekund�.
// R�wnocze�nie co sekund� jest wysy�any message IM_LIVE_BLINK do wszystkich zada� bez wzgl�du 
// czy zdefiniowany pin BOARD_LED_LIFE_PIN.


//#define BOARD_LED_TX_PIN ?
//#define BOARD_LED_RX_PIN ?
// GPIO do kt�rego pod��czone s� ledy informuj�ce �e co� jest odbierane oraz wysy�ane z urz�dzenia.
// Mo�na pomin�� lub te� zdefiniowa� tylko  BOARD_LED_TX_PIN  lub  BOARD_LED_RX_PIN.
// Za wyzwolenie tej funkcjonalno�ci odpowiadaj� funkcje Blink_RX() oraz Blink_TX(), funkcje opr�cz
// zapalaniu LED�w wcze�niej  zdefiniowanych wysy�aj� messaga IM_RX_BLINK, IM_TX_BLINK do wszystkich 
// zada�. Mo�na poda� w argumencie ID w celu rozpoznania w messagu o kt�re RX TX nam chodzi.

#define BOARD_LED_TX_STATUS_OFF LOW
#define BOARD_LED_RX_STATUS_OFF LOW
// Definiowanie czy wygaszenie LED�w jest przy stanie LOW czy HIGH pin�w wcze�niej zdefiniowanych.

#define TICK_LED_BLINK 250
// Ustalenie czasu jak d�ugo ma LED_TX LED_RX �wieci� po wyzwoleniu funkcjami Blink_RX() oraz Blink_TX()


#define XB_GUI
// Je��i zdefiniowano to ca�y system zacznie korzysta� z biblioteki xb_gui.h


#define XB_PREFERENCES
// Je�li zdefiniowano to biblioteka zacznie korzysta� do obs�ugi konfiguracji w pami�ci flash z 
// biblioteki Preferences.h (ESP32) .


// Je�li chcemy aby funkcja Log() biblioteki wysy�a�a na UART jakie� komunikaty oraz �eby 
// biblioteka pobiera�a kody z RX jako wciskanie klawiszy, nale�y najpierw doda� do projektu
// biblioteke XB_SERIAL. Podstawowa konfiguracja XB_SERIAL polega na ustawieniu takich makr:

#define Serial0BoardBuf_BAUD 921600
// Okre�la szybko�� pierwszego UARTa w urz�dzeniu, oraz r�wnocze�nie go uruchamia

#define SERIAL0_SizeRXBuffer 1024
// Definicja wielko�ci bufora odbiorczego

#define Serial0BoardBuf_UseKeyboard
// Je�li zdefiniujemy to zostanie dodany stream z kt�rego korzysta� b�dzie klawiatura jako kody 
// klawiszy nadchodz�ce w RX

#define Serial0BoardBuf_UseLog
// Je�li zdefiniowane to biblioteka XB_BOARD a konkretnie funkcja Log() zacznie korzysta� poprzez
// stream do wysy�ania komunikat�w na TX uarta pierwszego

#define Serial0BoardBuf_UseGui
// Je�li zdefiniowane to na pierwszy uart b�dzie rysowane GUI

//#define Serial0Board_RX_PIN ?
//#define Serial0Board_TX_PIN ?
// Dla ESP32 mo�emy zdefiniowa� na kt�rych pinach zostanie uruchomiony pierwszy UART.
// Je�li nie podamy tych definicji to u�yte zostan� standardowe przyporz�dkowania pin�w.

//#define Serial0Board_UseDebugOutPut
// Je�li zdefiniujemy to na pierwszy UART zostan� skierowane komunikaty diagnostyczne frameworku 
// arduino dla ESP32.

#endif
