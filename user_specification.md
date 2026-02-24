Jsi senior embedded systems architekt se specializací na real-time řízení, ESP32 (konkrétně ESP32-C3), bezpečnost výkonových DC systémů a návrh spolehlivých RC platforem pro závodní a crawler aplikace.
Nechci hobby řešení.Chci architekturu na úrovni produkčního embedded firmware.
Navrhni kompletní firmware pro Arduino IDE pro následující hardware:

HARDWARE
--------
*   ESP32-C3 Super Mini   
*   Brushed DC motor MIG 280   
*   Převodovka 4.5:1    
*   20A brushed ESC - PIN 5 na ESP32-C3   
*   Servo MG90S (servo PWM 1000–2000 µs) - PIN 4 na ESP32-C3
*	Světla, 2 bílé LED vepředu, dvě červené LED vzadu - PIN 6 na ESP32-C3
*	Klakson/buzzer - PIN 7 na ESP32-C3
*   2S LiPo pro ESC 
*   5V regulátor pro ESP32 + servo + LED + buzzer
*   Android Xiaomi 15 telefon s HyperOS, Chrome browser
    
ARCHITEKTURA (POVINNÉ)
======================
Implementuj vícevrstvou architekturu:

### 1 Network Layer
*   WiFi AP mó  d  
*   Max 1 klient   
*   WebSocket (žádný polling)   
*   Heartbeat mechanismus   
*   Detekce výpadku klienta   
*   Latence optimalizovaná
    
### 2 Control Input Layer
*   Validace vstupních dat   
*   Saturace rozsahu (-100 až 100)   
*   Ochrana proti malformed JSO    
*   Rate limiting vstupů
    
### 3 Safety Layer (samostatná logická vrstva)
Implementuj explicitní bezpečnostní logiku:
*   Fail-safe při ztrátě spojení   
*   Watchdog timeout 100 ms (neutral)   
*   Hard timeout 500 ms (disable ESC output)   
*   Direction change lockout   
*   Povinný neutral brake interval před reverzem   
*   Soft start / soft stop (slew rate limiter) 
*   Ochrana proti PWM glitchům při bootu
    
### 4 Motion Control Layer
*   Převod throttle → ESC PWM  
*   Převod steering → servo PWM 
*   Nelineární throttle křivka (expo) vhodná pro crawler  
*   Možnost omezení maximální rychlosti
    
### 5 State Machine (POVINNÉ)
Implementuj explicitní stavový automat:
Stavy:
*   BOOT  
*   INIT\_ESC  
*   IDLE\_NO\_CLIENT  
*   CLIENT\_CONNECTED   
*   ACTIVE\_CONTROL   
*   FAILSAFE  
*   LOCKOUT\_DIRECTION\_CHANGE
    
Každý stav musí mít:
*   Entry akci  
*   Exit akci  
*   Jasně definované přechody
    
Žádná implicitní logika v loop().
BEZPEČNOSTNÍ POŽADAVKY (STRIKTNÍ)
=================================
### ESC Initialization
*   2 s stabilizace   
*   Pevné nastavení neutrálu   
*   Zabránění náhodnému pulzu při startu
    
### Zákaz okamžité změny směru
Pokud:forward > +20 %a požadavek reverse < -20 %
Pak:
1.  Přepni do LOCKOUT\_DIRECTION\_CHANGE  
2.  Nastav neutrál   
3.  400 ms blokuj reverz   
4.  Poté povol reverz
    
### Slew Rate Limiter
Max změna:
*   5 % každých 10 ms
    
### Deadband
Implementuj 5% deadband kolem neutrálu.
### ESC Output Disable
Pokud není klient:
*   ESC signál musí být fyzicky deaktivován nebo fixně neutrál   
*   Žádné náhodné PWM
    
VÝKON
=====
*   Žádné delay()    
*   Pouze millis() nebo mikro    
*   Nezablokovaný loop   
*   Minimalizuj heap alokace   
*   Žádné String třídy (používej pevné buffery)   
*   Minimalizuj fragmentaci paměti
    
WEB UI
======
Minimalistické:
*   Dva virtuální joysticky (canvas), vlevo vertikální throttle, vpravo horizontální steering
*	Držení mobilu na šířku
*	Labely s vyjádřením % throttle a steering
*	Switch na Lights
*	Button na Horn
*   WebSocket přímé odesílání   
*   Odesílání každých 20 ms   
*   Heartbeat
    
Žádné externí CDN.
PWM SPECIFIKACE
===============
ESC:
*   50 Hz   
*   1000 µs reverse   
*   1500 µs neutral   
*   2000 µs forward
    
Servo:
*   Mapuj -100 až 100 na bezpečný rozsah MG90S
    
Použij LEDC.
DODATEČNÉ OCHRANY
=================
Implementuj návrh pro budoucí rozšíření:
*   LiPo low-voltage cutoff (architektura připravena)   
*   Proudový senzor (placeholder)
*   Teplotní ochrana (placeholder)
    
VÝSTUP
======
Vygeneruj:
1.  Kompletní produkční .ino firmware    
2.  Architektonické vysvětlení (blokové rozdělení)    
3.  Detailní popis state machine    
4.  Detailní popis bezpečnostních opatření    
5.  Seznam možných failure scénářů a jak jsou řešeny
6.  Samostatný klon použitého HTML a CSS řešení pro telefon, abych si ho mohl zkontrolovat předem bokem
    
Nechci zjednodušené demo. Nechci pseudokód. Nechci akademický příklad.
Chci robustní embedded řešení, které bych mohl dát do reálného RC crawleru bez rizika zničení převodovky nebo ohrožení lidí kolem. A zároveň se to musí vlézt na ESP32-C3 a utáhnout to.
