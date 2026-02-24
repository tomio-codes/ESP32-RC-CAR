# RC Crawler Pro - Firmware Architektura a Bezpečnost

## 1. Architektonické vysvětlení a vrstvy
Firmware implementuje plně neblokující modulární architekturu, která se dělí do následujících logických vrstev:

- **Network Layer (`NetworkLayer.h`, `WebUI.h`)**:
  Spravuje WiFi AP, statický HTTP server a WebSocket server. Dekóduje přijatý JSON z klientského telefonu, aplikuje striktní saturaci hranic (± 100 %). Výstupem je bezpečná unifikovaná struktura `ControlInput`. Žádné objekty typu `String` nejsou po startu alokovány do heapu, pracuje se statickým bufferem `ArduinoJson` (ochrana proti fragmentaci paměti). Celé Web UI (`index.html`) je pro tyto účely integrováno jako C-string.

- **Safety Layer (`SafetyLayer.h`)**:
  Separátní bezpečnostní middleware. Aplikuje 5% hardwarový Deadband. Provádí Slew Rate limiting nezávisle na komunikačním zpoždění – max 5% změna plynu na každých 10 ms (brání prudkým změnám zrychlení a mechanickému zničení převodovky `MIG 280`). Samostatně kontroluje Lockout pro ochranu před okamžitým reverzem proudu elektromotoru.

- **Motion Layer (`MotionLayer.h`)**:
  Provádí závěrečný generátor PWM profilu pomocí nativního driveru `driver/ledc.h` bez abstrakční režie knihovny Servo z Arduina. Aplikuje Expo (nelineární křivku s typickým faktorem 1.5) na logický plyn, což zjemňuje preciznost ovládání pro typické "crawler" překážky v nejnižších otáčkách.
  
- **RCStateMachine (`StateMachine.h`)**:
  Mozek celého firmware. Garantuje spouštění v 10ms přesných taktech. Agreguje moduly dohromady a provádí změny tak, jak mu striktně dovolí pravidla přechodů.

## 2. Detailní popis State Machine
Systém implementuje stavový automat `RCState` pomocí silně typovaného enumerátoru. Explicitní přechody zcela eliminují nedefinované chování.

*   `BOOT`: Původní start mikrokontroléru. Odsud okamžitě přechází na `INIT_ESC`.
*   `INIT_ESC`: Odesílá signál Neutrálu po dobu 2000 ms, čímž zajištuje správnou armovací stabilizační sekvenci pro moderní i staré regulátory ESC, aniž by došlo k tzv. runaway stavu při bootu ESP32.
*   `IDLE_NO_CLIENT`: Bezpečný idle stav při čekání na připojení klienta, motor fyzicky zablokován na Neutrálu.
*   `CLIENT_CONNECTED`: Klient je nově připojený po Websocketu. Z bezpečnostních důvodů (glitch ochrana) se čeká na přijetí vůbec prvního packetu, který má plynovou páku prokazatelně poblíž 0 % bodu úvrati. Teprve po potvrzení čistého stavu dovolí akceleraci.
*   `ACTIVE_CONTROL`: Běžný aktivní provoz. Systém běží naplno.
*   `FAILSAFE`: Vstupní bod, stane-li se, že paket od klienta nedorazil za stanovený Watchdog limit. Auto okamžitě převede plyn plynulejší cestou na nulu.
*   `LOCKOUT_DIRECTION_CHANGE`: Stav trvající přesně 400 ms. Zajišťuje fixní pauzu na hardwarovém neutrálu po rychlé snaze jezdce přejít z plné Jízdy-vpřed (> 20%) do Zpátečky (< -20%), chránící tak elektroniku vozu a omezující poškození převodovky.

## 3. Popis bezpečnostních opatření (Safety Protocol)
*   **Watchdog (100 ms)** - Měří latenci z mobilu ven, auto neustále posuzuje srovnávání časových relací. Neobjeví-li se 100ms packet, vozidlo přejde do bezpečí.
*   **Hard Timeout (500 ms)** - Fyzické zablokování výstupu ESC generátoru pulzů (`disableEscOutput()`) v případě enormního výpadku komunikace.
*   **Lockout Timer (400 ms)** - Pravidlo přemosťující rychlé snahy o změnu směru do nutné zastavovací pauzy (aktivní Dead-time).
*   **Heartbeat Mechanism** - Klient (uživatel v mobilu) odesílá do svého JSON payloadu lokální Timestamp mobilu. ESP32 mu jej bleskově navrací (echo `pong`), čehož prohlížeč využívá k permanentnímu výdeji RTT Pingu zobrazeného na obrazovce pro hlubší diagnostiku dosahu WiFi.

## 4. Analýza Failure scénářů a řešení

| Failure Scénář | Hardwarové a Softwarové Opatření |
| :--- | :--- |
| **Klient ztratí spojení s WiFi / odejde z dosahu** | Skrz `WStype_DISCONNECTED` event ESP32 ihned spadne do `IDLE` nebo `FAILSAFE` state podle délky trvání. ESC se zablokuje. |
| **Lag v síti / paket drop** | Zásah aplikovaného limitu (100 ms). Vozidlo provede soft neutral, dokud nová stabilní data znovu nepřijdou. |
| **Připojení dalšího klientálního telefonu** | Systém má nastavený limit `connectedClientNum` pro jedno zařízení naráz, duplicitní `WStype_CONNECTED` session nekompromisně odpojí (`disconnect(num)`). |
| **Při inicializaci/zapnutí podrží uživatel plný plyn** | Zachyceno proaktivní restrikcí ve stavu `CLIENT_CONNECTED`. Přepnutí dovolí výlučně tehdy, když obdrží neutrál (`<= 5 %`). Auto proto nikam omylem nevyletí. |
| **Extrémně rychlá změna směru prstem na pultu** | Značně škodlivé - eliminováno na úrovni dvou vrstev: Jednak Slew Rate zpomalením (soft stop do neutrálu limitované na 5 % změny za 10 ms), zadruhé 400 ms `LOCKOUT` dead-timem pro úplné zabrzdění. |
| **Poškozený či malformovaný JSON ve WebSocket packetu** | Přístup `deserializeJson()` používající buffer na stacku s prevencí overflow. Neexistující atribut json mapy je ošetřen bitovým OR maskováním fallback na hrubou nulu, chrání tak proti nekontrolovatelným skokům uninitialized registrů. |
| **Neznámý PWM Glitch při startu/restartu** | Explicitní vyřešení skrze přítomnost `BOOT` a `INIT_ESC` state. Tři vteřiny po napájení je fixně generován nezastavitelný neutrální kód. |
