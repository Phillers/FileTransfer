# Projekt zaliczeniowy - laboratorium programowania sieciowego

Temat: Narzędzie kopiowania plików i katalogów wykorzystujące protokoły (do wyboru) Ethernet II, IP, UDP/IP, TCP/IP.

Projekt składa się z dwóch programów: `sender` i `receiver`.
Programy odtwarzają wskazaną strukturę plików i katalogów z komputera gdzie uruchomiony jest `sender` na komputerze gdzie uruchomiony jest `receiver`.

## Argumenty programu `sender`

W przypadku wyboru protokołów IP, TCP/IP lub UDP/IP:
* nazwa pliku lub katalogu źródłowego,
* adres IP komputera docelowego,
* numer portu, na którym nasłuchuje program `receiver` (numer protokołu IP w przypadku `IP`),
* nazwa protokołu (`IP`, `TCPIP` lub `UDPIP`).

W przypadku wyboru protokołu Ethernet:
* nazwa pliku lub katalogu źródłowego,
* adres sprzętowy urządzenia odbiorczego,
* EtherType (dziesiętnie),
* nazwa protokołu (`ETHERNET`),
* nazwa interfejsu wysyłającego dane.

## Argumenty programu `receiver`

W przypadku wyboru protokołów IP, TCP/IP lub UDP/IP:
* numer portu, na którym nasłuchuje program (numer protokołu IP w przypadku `IP`),
* nazwa katalogu, gdzie zostaną umieszczone odebrane pliki,
* nazwa protokołu (`IP`, `TCPIP` lub `UDPIP`),

W przypadku wyboru protokołu Ethernet:
* EtherType (dziesiętnie),
* nazwa katalogu, gdzie zostaną umieszczone odebrane pliki,
* nazwa protokołu (`ETHERNET`),
* nazwa interfejsu odbierającego dane.

## Przykłady wywołań programów
~~~~
$ ./receiver 1234 destination UDPIP
$ ./sender source 10.0.0.1 1234 UDPIP
# ./receiver 253 destination IP
# ./sender source 192.168.122.177 253 IP
# ./receiver 34952 destination ETHERNET ens3
# ./sender source 52:54:00:31:e9:f3 34952 ETHERNET virbr0
~~~~

## Przykład działania
Uruchomienie programu `./sender source 192.168.122.177 1234 TCPIP` w katalogu o następującej strukturze...
~~~~
.
├── source
│   ├── a
│   │   ├── b
│   │   │   ├── c
│   │   │   └── d
│   │   ├── d
│   │   └── e
│   ├── b
│   └── e
│       ├── d
│       └── e
└── sender
~~~~
podczas gdy na komputerze o adresie IP 192.168.122.177 jest uruchomiony program `./receiver 1234 destination TCPIP` spowoduje przesłanie plików przy pomocy protokołu TCP/IP i umieszczenie ich w odpowiednich katalogach:
~~~~
.
├── destination
│   └── source
│       ├── a
│       │   ├── b
│       │   │   ├── c
│       │   │   └── d
│       │   ├── d
│       │   └── e
│       ├── b
│       └── e
│           ├── d
│           └── e
└── receiver
~~~~

## Kompilacja i zawartość plików
Aby skompilować program należy uruchomić program `make` w głównym katalogu projektu.
Zawartość plików źródłowych:
 * `Sender.cpp` ustanawia połączenie odpowiedniego typu z odbiorcą oraz przegląda strukturę katalogu źródłowego, wywołując na każdym pliku metodę wysyłającą.
 * `Receiver.cpp` ustanawia połączenie odpowiedniego typu z nadawcą oraz wywołuje metodę oczekującą na transmisję plików.
 * `Connection.cpp` implementuje klasy `SenderConnection` i `ReceiverConnection`, które zawierają metody odpowiednio wysyłające i odbierające pliki i katalogi. Metody te wykorzystują metody `send_bytes`, `send_string`, `receive_bytes` i `receive_string`, które muszą być zaimplementowane przez klasy potomne. I tak:
   * `IPConnection.cpp` implementuje klasy `SenderIPConnection` i `ReceiverIPConnection` posiadające metody, które korzystają z protokołu IP,
   * `TCPIPConnection.cpp` implementuje klasy `SenderTCPIPConnection` i `ReceiverTCPIPConnection` posiadające metody, które korzystają z protokołu TCP/IP,
   * `UDPIPConnection.cpp` implementuje klasy `SenderUDPIPConnection` i `ReceiverUDPIPConnection` posiadające metody, które korzystają z protokołu UDP/IP,
   * `EthernetConnection.cpp` implementuje klasy `SenderEthernetConnection` i `ReceiverEthernetConnection` posiadające metody, które korzystają z protokołu Ethernet.

Kompilatorem użytym do testów był g++ 6.3.0 na systemie operacyjnym Debian GNU/Linux 9.
