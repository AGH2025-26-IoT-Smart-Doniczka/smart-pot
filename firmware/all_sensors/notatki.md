# Spotkanie 11.01.26
## Ustalenia
- dodatkowy topic na podlewanie

- co mozna zmienic przez serwer:
swiatlo, wilgotnosc, temperatura, podlewanie (ile sekund)

## Dane z sensorów - konfiguracja

swiatlo: duzo/srednio/malo: (0/1/2)
wilgotnosc: 15/30/60/80 - 4 progi
temperatura - 5/25 - 2 progi
ciśnienie - tylko wysyła

Kalibracja czujnika wilgotności (pojemnościowy, analog, ADC_BITWIDTH_9):
- stan dry: czujnik w powietrzu, zapis soil_adc_dry
- BTN2 krótki -> stan wet: czujnik w wodzie, zapis soil_adc_wet
- BTN2 krótki w wet -> przejście do provisioning
- BTN1 krótki w wet -> powrót do dry

## Protokół komunikacji

mqtt config -> esp, json
{
    "lux": (int), // (duzo/srednio/malo: (0/1/2))
    "moi": (int[4]), //progi wilgotnosci gleby 
    "tem": (number[2]) //próg dolny i górny
    "sle": uint16_t // sleep duration w sekundach
}

esp -> mqtt data, json
{
    "potId": string, // 12 chars
    "timestamp": number, //(unix time)
    "data": {
        "lux": number (int),
        "tem": number (float),
        "moi": number (int),
        "pre": number (float)
    }
}

user app -> esp: uint8_t[11], kodowanie:
{
    0: (uint8_t), naświetlenie (duzo/srednio/malo: (0/1/2))
    1-4: (uint8_t[4]), 4 progi, zakres 0-100, bardzo sucho/sucho/idealnie/przelana
    5-8: (uint16_t[2]), 2 progi, podane w Kelwinach*10, zimno/odpowiednio/za ciepło
    9-10: (uint16_t), sleep duration w sekundach
}

id/podlewanie
(int) (liczba sekund)

## struktury C
(Typy do dopisania)
struct plant_config_t {
    swiatlo: duzo/srednio/malo: (0/1/2)
    wilgotnosc: 15/30/60/80 - 4 progi
    temperatura - 5/25 - 2 progi
    ciśnienie - tylko wysyła
} plant_config_t;

stuct config {
    char[32] SSID;
    char[64] passwd;
    plant_config_t plant_config;
    int sleep_duration;
};

## BLE UUID

UUID BLE:
service:
    a2909447-7a7f-d8b8-d140-68a237aa735c
    
    ssid (write):
        88971243-7282-049b-d14a-e951055fc3a3

    password (write):
        ee0bfcd4-bdce-6898-0c4a-72321e3c6f45

    config (write):
        2934e2ce-26f9-4705-bd1d-dfb343f63d04

    pot_id (read):
        752ff574-058c-4ba3-8310-b6daa639ee4d


# Peripherals
## I2C
Oprogramowanie wykorzystuje 2 szyny I2C - jedna obsługująca ekran, druga obsługująca sensory.
### Ekran
Ekral oled o rozdzielczości 128x64. Korzysta ze sterownika **SSD1306**
Sterownik:

### Sensory
- Cyfrowe:
  - **BMP280** sensor temperatury i ciśnienia 
  - **VEML7700** sensor naświetlenia, obslugiwany autorskim sterownikiem
- Analogowe:
  - pojemnościowy sensor wilgotności gleby