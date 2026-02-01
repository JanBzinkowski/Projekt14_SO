# Temat projektu: Bar Mleczny


## Autor: Jan Bzinkowski

## Nr. albumu: 157193

### [Repozytorium github](https://github.com/JanBzinkowski/Projekt14_SO/)

### Środowisko: Zorin OS 17.3

### Kompilator: g++ version 11.4.0

### CMake: CMake VERSION 3.20

---

## 1. Opis tematu

Tematem projektu jest stworzenie symulacji baru mlecznego z wieloma współbieżnymi uczestnikami: kierownikiem, pracownikami obsługi, kasjerem oraz klientami (indywidualnymi lub w grupach). Symulacja odwzorowuje rzeczywiste zasady funkcjonowania lokalu, w tym obsługę zamówień, zajmowanie stolików, ograniczenia miejsc, sytuacje wyjątkowe oraz reakcje na sygnały sterujące.

Klienci są generowani przez dodatkowy program ```generator_klientow```. Około 5% z nich nie składa zamówienia i w związku z tym nie może zająć miejsca przy stoliku. Pozostali klienci zamawiają gorące dania, płacą w kasie i odbierają zamówienie od pracownika obsługi. Niedopuszczalne jest oczekiwanie na wolny stolik z wydanym daniem – klient może usiąść wyłącznie wtedy, gdy dostępne jest odpowiednie miejsce.

W barze znajdują się stoliki 1-, 2-, 3- i 4-osobowe w liczbie odpowiednio ```X1, X2, X3, X4,``` co determinuje maksymalną liczbę klientów jednocześnie spożywających posiłek. Klienci przychodzą pojedynczo lub w grupach 2-, 3- lub 4-osobowych. Ze względu na konflikty między klientami, przy jednym stoliku mogą siedzieć tylko grupy o tej samej liczebności.

Po zakończeniu posiłku naczynia są zwracane zbiorczo przez jednego przedstawiciela grupy.

Z programu kierownika można wysłać następujące sygnały:

- jednorazowo (**SIGRTMIN+1**) podwoić liczbę stolików 3-osobowych (*+1 jeśli początkowa liczba stolików wynolsiła 0*);

- na żądanie (**SIGRTMIN+2**) zarezerwować określoną liczbę stolików lub miejsc, wyłączając je z użytku klientów;

- w sytuacji pożaru (**SIGRTMIN**) natychmiast następuje ewakuacja klientów, zakończenie pracy obsługi i zamknięcie kasy;

- dodana została również obsłyga sygnału **SIGINT**, aby móc szybko zamykać restaurację (zabijać procesy) z poporawnym czyszczeniem zasobów.

Przy zamknięciu kasy kasjer liczy utarg z symulacji.

## 2. Uruchomienie programu

#### Kompilacja projektu
Będąc w głównym folderze projektu wpisać następujące komendy:
```
mkdir build
cd build
cmake ..
cmake --build .
```
Wszystkie potrzebne programy powinny się pojawić w folderze ```/bin```

#### Uruchomienie programu

Aby uruchomić program główny należy uruchomić znajdujący się w folderze ```/bin``` program ```mainprog```. Aby poprawnie uruchomić program kierownika należy w osobnej konsoli uruchomić znajdujący się w folderze ```/bin``` program ```kierownik``` podając mu w argumencie wywołania pid programu głównego (```mainprog```). Przykład wywołania: ```./kierownik 1234```

#### Ustawianie zmiennych uruchomienia

Program posiada możliwości łatwego zmieniania warunków uruchomienia. Lista zmiennych uruchomienia:

```MAX_KLIENTOW``` - ([Shared_memory.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L8)) Zarządza maksymmalną ilością grup klientów aktywnych w programie w tym samym czasie (podstawowo: 10000)

```MAX_KLIENTOW_W_RRESTAURACJI``` - ([Shared_memory.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L9)) Zarządza maksymalną ilością grup klientów znajdujących się w lokalu, zarówmo przy stolikach jak i tych stojących w kolejce. (podstawowo: 120)

```X1``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L7)) Liczba stolików 1-osobowych w restauracji. (podstawowo: 5)

```X2``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L8)) Liczba stolików 2-osobowych w restauracji. (podstawowo: 5)

```X3``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L9)) Liczba stolików 3-osobowych w restauracji. (podstawowo: 5)

```X4``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L10)) Liczba stolików 4-osobowych w restauracji. (podstawowo: 5)

```MENU_NAPOJE``` ([Zamowienie.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Zamowienie.h#L9)) Liczba możliwych do wyboryu napojów. Nr. napoju mówi o tym ile będzie on kosztował i jaki czas zajmie klientowi spożywanie go (sekundy) (podstawowo: 4)

```MENU_NAPOJE``` ([Zamowienie.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Zamowienie.h#L10)) Liczba możliwych do wyboryu dań. Nr. dania mówi o tym ile będzie on kosztował i jaki czas zajmie klientowi spożywanie go (sekundy) (podstawowo: 4)

```SLEEP``` ([klient.cpp](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/klient.cpp#L16)) Decyduje o tym czy klient ma używać sleep podczas jedzenia. (podstawowo: sleep, w razie potrzeby zmienić na //sleep)

```CUSTOM_SLEEP_TIME``` ([klient.cpp](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/klient.cpp#L17)) Decyduje o czasie snu klientów. 0 jeśli chcemy używać podstawowych czasów wykonywania czynności przez klientów, > 0 jeśli chcemy ustawić własny czas. (podstawowo 0)

## 3. Struktura programu

- ```mainprog``` program "rodzic". Tworzy wszystykie potrzebne struktury IPC. Tworzy programy potomne za pomocą ```fork()```. Tworzone programy: [```generator_klientow```](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/mainprog.cpp#L98), [```kasjer```](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/mainprog.cpp#L109), [```pracownik```](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/mainprog.cpp#L120) i [```logger```](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/mainprog.cpp#L131). Po zakończeniu symulacji usuwa utworzone wcześniej struktury IPC.

- ```kasjer``` odpowiada za przyjmowanie zamówień od klientów oraz za przydzielanie ich do stolików. Po zakończeniu symulacji zamyka kasę oraz sprawdza utarg zyskany podczas symulacji.

- ```pracownik``` odpowiada za wydawanie zamówień. Na polecenie ```kierownika``` może zarezerwować odpowiednią ilość stołów lub donieść dodatkowe stoliki 3-osobowe (czynność jednorazowa!)

- ```generator_klientow``` odpowiada za generowanie klientów, oraz za asynchroniczne usuwanie procesów zombie.

- ```logger``` odpowiada za wypisywanie logów wysyłanych przez inne programy w konsoli a także do pliku ```log.txt```.

- ```kierownik``` zarządza restauracją. Umożliwia poprawne wydawanie sygnałów do programów potomnych ```mainprog```.

## 4. Opis plików

- Folder ```include```:
  - ```Shared_memory.h``` - zawiera zmienne zarządzające ilością klientów w lokalu oraz strukturę [```SharedMem```]([https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L11](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L8))
 
  - ```Tables.h``` - zawiera zmienne zarządzające ilością stolików, a także strukturę [```Table```](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L17)
 
  - ```Wrappers.h``` - zawiera deklaracje wrapperów funkcji System V oraz struktury odpowiadające za [wysyłanie wiadomości do loggera](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Wrappers.h#L20) oraz za [rezerwację stolików i komunikację kierownik-pracownik](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Wrappers.h#L25)
 
  - ```Zamowienie.h``` - zawiera zmienne informujące o ilości pozycji w menu, a także struktury odpowiadające za [komunikację klient-kasjer-pracownik-klient](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Zamowienie.h#L12)
 
- Folder ```src```:
  - ```Tables.cpp``` - zawiera inicjalizację zmiennych [```table_count```](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Tables.cpp#L3) oraz [```table_count_max```](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Tables.cpp#L5).
 
  - ```Wrappers.cpp``` - zawiera [definicje](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L8) wrapperów funkcji System V. 
 
  - ```mainprog.cpp``` - zawiera kod programu ```mainprog```
 
  - ```kasjer.cpp``` - zawiera kod programu ```kasjer```
 
  - ```pracownik.cpp``` - zawiera kod programu ```pracownik```
   
  - ```generator_klientow.cpp``` - zawiera kod programu ```generator_klientow```
 
  - ```klient.cpp``` - zawiera kod programu ```klient```
 
  - ```logger.cpp``` - zawiera kod programu ```logger```
 
  - ```kierownik.cpp``` - zawiera kod programu ```kierownik```


## 5. Pseudokody wybranych algorytmów

#### [Algorytm przeszukiwania stolików](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/kasjer.cpp#L31):
```
FUNKCJA sprawdz_stolik(&index, table_array, zam, mem_flags)

    JEŚLI zam.liczba_osob <= 2
        //ustaw poprawny idndex startowy w zależności od rozmiaru grupy (minimalizacja zużycia zasobów)
        JEŚLI zam.liczba_osob == 1
            index ← X1
        INACZEJ
            index ← X1 + X2 + X3
        KONIEC JEŚLI
        //szukaj możliwości dosiadania się
        WHILE index < mem_flags.table_count - (jeśli zam.liczba_osob == 2 i mem_flags.new_tables to X3 inaczej 0)
            ZABLOKUJ semid_prac

            ok ← (stolik nie jest zarezerwowany) I (typ_grupy == zam.liczba_osob)

            ODBLOKUJ semid_prac

            JEŚLI ok = FAŁSZ
                index ← index + 1
                CONTINUE
            KONIEC JEŚLI

            JEŚLI nie da się zająć stolika (sem_op IPC_NOWAIT)
                index ← index + 1
                CONTINUE
            KONIEC JEŚLI

            ZAPISZ LOG: znaleziono stolik
            ZWRÓĆ PRAWDA
        KONIEC WHILE
    KONIEC JEŚLI

    // szukaj w całej tablicy
    DLA index OD 0 DO mem_flags.table_count-1
        ZABLOKUJ semid_prac

        ok ← (stolik nie jest zarezerwowany) I (stolik jest pusty)

        ODBLOKUJ semid_prac

        JEŚLI ok = FAŁSZ
            CONTINUE
        KONIEC JEŚLI

        JEŚLI nie da się zająć stolika (sem_op IPC_NOWAIT)
            CONTINUE
        KONIEC JEŚLI

        table_array[index].typ_grupy ← zam.liczba_osob

        ZAPISZ LOG: znaleziono stolik
        ZWRÓĆ PRAWDA
    KONIEC DLA

    index ← -1
    ZWRÓĆ FAŁSZ
KONIEC FUNKCJI
```

#### [Algorytm rezerwacji stolików przez pracownika](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/pracownik.cpp#L33):
```
FUNKCJA rezerwacje(rezerwacja, table, mem_flags)

    ZABLOKUJ semafor semid
    JEŚLI sem_op zwróci błąd:
        ZWRÓĆ

    // rezerwacja stolików 1-osobowych
    DLA i OD 0 DO min(rezerwacja.reserved.x1, X1) - 1
        table[i].zarezerwowany_pzez_kierownika ← true
    KONIEC DLA

    // rezerwacja stolików 2-osobowych
    DLA i OD X1 DO min(X1 + rezerwacja.reserved.x2, X1 + X2) - 1
        table[i].zarezerwowany_pzez_kierownika ← true
    KONIEC DLA

    start3 ← X1 + X2

    JEŚLI stoliki niedoniesione
        // rezerwacja stolików 3-osobowych z podstawowej puli
        DLA i OD start3 DO min(start3 + rezerwacja.reserved.x3, start3 + X3) - 1
            table[i].zarezerwowany_pzez_kierownika ← true
        KONIEC DLA

    INACZEJ
        added3 ← (X3 == 0 ? 1 : X3)

        // rezerwacja stolików 3-osobowych z podstawowej puli
        reserved_base3 ← min(rezerwacja.reserved.x3, X3)
        DLA i OD start3 DO start3 + reserved_base3 - 1
            table[i].zarezerwowany_pzez_kierownika ← true
        KONIEC DLA

        // rezerwacja stolików 3-osobowych z dodatkowej puli
        reserved_extra3 ← rezerwacja.reserved.x3 - reserved_base3
        JEŚLI reserved_extra3 > 0
            start3_extra ← mem_flags->table_count - added3
            DLA i OD start3_extra DO start3_extra + min(reserved_extra3, added3) - 1
                table[i].zarezerwowany_pzez_kierownika ← true
            KONIEC DLA
        KONIEC JEŚLI
    KONIEC JEŚLI

    start4 ← X1 + X2 + X3

    // rezerwacja stolików 4-osobowych
    DLA i OD start4 DO min(start4 + rezerwacja.reserved.x4, start4 + X4) - 1
        table[i].zarezerwowany_pzez_kierownika ← true
    KONIEC DLA

    ODBLOKUJ semafor semid
KONIEC FUNKCJI
```

#### [Ustawienie unikatowego ziarna generatora dla każdego wątku klienta (zapewnienie najlepszej losowości)](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/klient.cpp#L30):
```
FUNKCJA make_seed() ZWRACA uint64

    seed ← losowa wartość z random_device
    seed ← (seed << 32) XOR kolejna losowa wartość z random_device

    seed ← seed XOR aktualny czas w nanosekundach

    seed ← seed XOR identyfikator procesu (PID)

    seed ← seed XOR identyfikator wątku (pthread_self)

    ZWRÓĆ seed
KONIEC FUNKCJI
```

## 6. Użyte funkcje systemowe:

- **Tworzenie procesów**

  - ```fork()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/mainprog.cpp#L98)

  - ```exit()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L10)
 
- Tworzenie wątków:
  - ```pthred_create()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/klient.cpp#L177)
    
  - ```pthred_join()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/klient.cpp#L184)
    
  - ```std::thread``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/generator_klientow.cpp#L97)
 
  - ```std::mutex``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/generator_klientow.cpp#L19)

- **Obsługa sygnałów**
  - ```kill()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/generator_klientow.cpp#L36)
    
  - ```sigaction()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/generator_klientow.cpp#L80)
 
- **Synchronizacja procesów**
  - ```ftok()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/mainprog.cpp#L44)
 
  - ```semget()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L41)
 
  - ```semctl()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L88)
 
  - ```semop()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L66)
 
- **Kolejki komunikatów**
  - ```msgget()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L102)
 
  - ```msgsend()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L111)
 
  - ```msgrcv()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L124)
 
  - ```msgctl()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L142)
 
- **Segmenty pamięci dzielonej**
  - ```shmget()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L15)
 
  - ```shmat()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L22)
 
  - ```shmdt()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L29)
 
  - ```shmctl()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/Wrappers.cpp#L34)
 
- **Tworzenie i obsługa plików**
  - ```std::ofstream``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/logger.cpp#L47)
    
  - ```flush()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/logger.cpp#L66)
 
  - ```close()``` - [przykład użycia](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/logger.cpp#L72)
 
## 7. Przeprowadzone testy

#### Test 1 - 1000 klientów w lokalu, 1 stolik - 4-osobowy. 

Zmieniamy zmiene:
```MAX_KLIENTOW``` - ([Shared_memory.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L9)) zmiana na: 10000

```MAX_KLIENTOW_W_RRESTAURACJI``` - ([Shared_memory.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L9)) zmiana na: 1000

```X1``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L7)) zmiana na: 0

```X2``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L8)) zmiana na: 0

```X3``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L9)) zmiana na: 0

```X4``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L10)) zmiana na: 1

```CUSTOM_SLEEP_TIME``` ([klient.cpp](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/klient.cpp#L17)) zmiana na 0

test ma na celu sprawdzenie poprawnego działania programu przy dużym obciążeniu.

<details>
<summary>Logi testu: (rozwiń aby zobaczyć)</summary>
  
```
Logger uruchomiony. Odbieranie wiadomości...
[2026-01-27 12:59:14.287][mtype=1] Kasjer rozpoczyna prace
[2026-01-27 12:59:14.287][mtype=3] Pracownik rozpoczyna prace
[2026-01-27 12:59:14.287][mtype=4] Utworzono klienta: [188830]
[2026-01-27 12:59:14.287][mtype=4] Utworzono klienta: [188831]
[2026-01-27 12:59:14.287][mtype=4] Utworzono klienta: [188832]
[2026-01-27 12:59:14.287][mtype=4] Watek do czyszczenia procesow zombie uruchomiony.
[2026-01-27 12:59:14.287][mtype=4] Utworzono klienta: [188833]
[2026-01-27 12:59:14.287][mtype=4] Utworzono klienta: [188834]
[2026-01-27 12:59:14.290][mtype=1] Kasjer rozpoczyna obsluge klienta
[2026-01-27 12:59:14.290][mtype=1] Kasjer znalazl stolik nr: 0 dla grupy. Stolik 4 osobowy
[2026-01-27 12:59:14.290][mtype=1] Kasjer skonczyl obsluge klienta. Przydzielony stolik: 0
[2026-01-27 12:59:14.290][mtype=2] Grupa klientow zostala utworzona, rozmiar grupy: 1
[2026-01-27 12:59:14.290][mtype=2] Grupa klientow [188830]zlozyla zamowienie: 1 osob, Klient 1: pozycja: 3, napoj: 4
[2026-01-27 12:59:14.290][mtype=2] Grupa klientow zostala utworzona, rozmiar grupy: 4
[2026-01-27 12:59:14.290][mtype=2] Grupa klientow [188833]zlozyla zamowienie: 4 osob, Klient 1: pozycja: 3, napoj: 1, Klient 2: pozycja: 1, napoj: 1, Klient 3: pozycja: 1, napoj: 1, Klient 4: pozycja: 2, napoj: 1
[2026-01-27 12:59:14.290][mtype=2] Grupa klientow zostala utworzona, rozmiar grupy: 4
[2026-01-27 12:59:14.290][mtype=2] Grupa klientow [188855]zlozyla zamowienie: 4 osob, Klient 1: pozycja: 4, napoj: 3, Klient 2: pozycja: 2, napoj: 3, Klient 3: pozycja: 3, napoj: 3, Klient 4: pozycja: 4, napoj: 3
[2026-01-27 12:59:14.290][mtype=3] Pracownik rozpoczyna obsluge klienta. Przydzielony stolik: stolik nr. 0
[2026-01-27 12:59:14.290][mtype=3] Pracownik wydal zamowienie, stolik nr: 0

. . .

[2026-01-27 13:03:14.393][mtype=4] Zabito proces [202054]
[2026-01-27 13:03:14.393][mtype=4] Zabito proces [202058]
[2026-01-27 13:03:14.393][mtype=4] Zabito proces [202063]
[2026-01-27 13:03:14.393][mtype=4] Zabito proces [202070]
[2026-01-27 13:03:14.393][mtype=4] Zabito proces [202075]
[2026-01-27 13:03:14.393][mtype=4] Zabito proces [203815]
[2026-01-27 13:03:14.394][mtype=4] Watek do czyszczenia procesow zombie zakonczony.
[2026-01-27 13:03:14.394][mtype=4] Klienci opuscili lokal, generator klientow konczy dzialanie
[2026-01-27 13:03:14.394][mtype=5] Pracownik konczy prace
[2026-01-27 13:03:14.394][mtype=5] Kasjer zamyka kase
[2026-01-27 13:03:14.394][mtype=5] Dzisiejszy utarg to: 636 zl
[2026-01-27 13:03:14.394][mtype=5] Kasjer konczy prace
Logger zakończony.
```
</details>

W logach widać że przez 4 min pracy przy takim obciążeniu klientów program nie został zablokowany oraz zakończył się prawidłowo (wszyscy klienci opuścili restaurację, struktury IPC zostały poprawnie usunięte). 

***Test udany.***

--- 

#### Test 2 - Test obciążenia procesora przy małym ruchu
Zmieniamy zmienne:
```MAX_KLIENTOW``` - ([Shared_memory.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L9)) zmiana na: 50

```MAX_KLIENTOW_W_RRESTAURACJI``` - ([Shared_memory.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L9)) zmiana na: 10

```X1``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L7)) zmiana na: 10

```X2``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L8)) zmiana na: 10

```X3``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L9)) zmiana na: 10

```X4``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L10)) zmiana na: 10

```CUSTOM_SLEEP_TIME``` ([klient.cpp](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/klient.cpp#L17)) zmiana na 10

Test ten ma sprawdzać czy symulacja poprawnie się usypia (nie marnuje zasobów procesora) jeśli ruch jest mały.

<details>
<summary> top przy rególarnym użytkowaniu: (rozwiń aby zobaczyć)</summary>

```
top - 13:29:19 up  4:42,  1 user,  load average: 0,75, 1,41, 4,23
Zadania:razem: 356, działających:   1, śpiących: 353, zatrzymanych:   0, zombie:   2
%CPU:  5,5 uż,  0,8 sy,  0,0 ni, 93,6 be,  0,1 io,  0,0 hi,  0,0 si,  0,0 sk
MiB RAM :  15832,0 razem,   3350,4 wolne,   8335,1 użyte,   4146,5 buf/cache
MiB Swap:   1950,0 razem,    325,5 wolne,   1624,5 użyte.   5944,0 dost. RAM 

    PID UŻYTK.    PR  NI    WIRT    REZ    WSP S  %CPU  %PAM     CZAS+ KOMENDA
```
</details>

<details>

<summary> top przy braku ruchu: (rozwiń aby zobaczyć)</summary>

```
top - 13:28:04 up  4:41,  1 user,  load average: 1,94, 1,74, 4,56
Zadania:razem: 412, działających:   1, śpiących: 409, zatrzymanych:   0, zombie:   2
%CPU:  6,7 uż,  0,8 sy,  0,0 ni, 92,2 be,  0,2 io,  0,0 hi,  0,0 si,  0,0 sk
MiB RAM :  15832,0 razem,   3297,8 wolne,   8397,9 użyte,   4136,3 buf/cache
MiB Swap:   1950,0 razem,    325,2 wolne,   1624,8 użyte.   5890,1 dost. RAM 

    PID UŻYTK.    PR  NI    WIRT    REZ    WSP S  %CPU  %PAM     CZAS+ KOMENDA  
```
</details>

Jak widać przy braku ruchu zasoby cpu nie są nadmiernie zużywane. Wszystkie procesy śpią. 

***Test udany.***

---

#### Test 3 - Rezerwacja wszystkich dostępnych stolików w restauracji:
Zmieniamy zmienne:
```MAX_KLIENTOW``` - ([Shared_memory.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L9)) zmiana na: 10000

```MAX_KLIENTOW_W_RRESTAURACJI``` - ([Shared_memory.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L9)) zmiana na: 120

```X1``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L7)) zmiana na: 10

```X2``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L8)) zmiana na: 10

```X3``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L9)) zmiana na: 10

```X4``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L10)) zmiana na: 10

```CUSTOM_SLEEP_TIME``` ([klient.cpp](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/klient.cpp#L17)) zmiana na 0

Test ma na celu sprawdzenie poprawności działania rezerwacji stolików w lokalu. (Po zarezerwowaniu miejsc nie powinny się już pojawiać  wiadomości o przydzielaniu nowych klientów do stolika)


W menu kierownika wybieramy ```1```:
```
Witaj kierowniku. Co chcialbys zrobic?
1. Wydac sygnal
2. Wylaczyc tworzenie nowych klientow
1
```

Następnie wybieramy sygnał ```2```:
```
Wybierz sygnal:
1. Zwieksz liczbe stolikow
2. Rezerwuj miejsca
3. Pozar!
2
```
I wpisujemy odpowiednie lioczby stolików do zarezerwowania:
```
Ile X1 zarezerwowac?
10
Ile X2 zarezerwowac?
10
Ile X3 zarezerwowac?
10
Ile X4 zarezerwowac?
10
```

<details>
<summary>Logi testu: (rozwiń aby zobaczyć)</summary>

```
Logger uruchomiony. Odbieranie wiadomości...
[2026-01-27 14:15:31.965][mtype=1] Kasjer rozpoczyna prace
[2026-01-27 14:15:31.965][mtype=3] Pracownik rozpoczyna prace
[2026-01-27 14:15:31.965][mtype=4] Utworzono klienta: [235613]
[2026-01-27 14:15:31.965][mtype=4] Watek do czyszczenia procesow zombie uruchomiony.
[2026-01-27 14:15:31.965][mtype=4] Utworzono klienta: [235614]
[2026-01-27 14:15:31.965][mtype=4] Utworzono klienta: [235615]

...

[2026-01-27 14:15:52.017][mtype=1] Kasjer znalazl stolik nr: 32 dla grupy. Stolik 4 osobowy
[2026-01-27 14:15:52.017][mtype=1] Kasjer skonczyl obsluge klienta. Przydzielony stolik: 32
[2026-01-27 14:15:52.017][mtype=3] Pracownik rozpoczyna obsluge klienta. Przydzielony stolik: stolik nr. 32
[2026-01-27 14:15:52.017][mtype=3] Pracownik wydal zamowienie, stolik nr: 32
[2026-01-27 14:15:52.017][mtype=2] Klient odebral zamowienie, stolik nr: 32[236018]
[2026-01-27 14:15:52.054][mtype=3] Pracownik zarezerwowal stoly
[2026-01-27 14:15:52.307][mtype=2] Klient skonczyl swoj posilek
[2026-01-27 14:15:52.309][mtype=2] Klient skonczyl swoj posilek
[2026-01-27 14:15:52.309][mtype=2] Klient skonczyl swoj posilek
[2026-01-27 14:15:52.309][mtype=2] Klient skonczyl swoj posilek

...

[2026-01-27 14:16:01.023][mtype=2] Grupa klientow oddala naczynia, stolik nr: 20
[2026-01-27 14:16:01.023][mtype=2] Grupa klientow opuscila restauracje, stolik nr: 20, rozmiar grupy: 3
[2026-01-27 14:16:01.023][mtype=4] Zabito proces [236431]
[2026-01-27 14:16:01.023][mtype=4] Utworzono klienta: [246787]
[2026-01-27 14:16:01.024][mtype=2] Grupa klientow zostala utworzona, rozmiar grupy: 1
[2026-01-27 14:16:01.034][mtype=2] Grupa klientow [236254]zlozyla zamowienie: 1 osob, Klient 1: pozycja: 3, napoj: 1
[2026-01-27 14:16:01.112][mtype=1] Kasjer rozpoczyna obsluge klienta
[2026-01-27 14:16:01.115][mtype=1] Kasjer nie znalazl stolika dla grupy
[2026-01-27 14:16:02.121][mtype=1] Kasjer rozpoczyna obsluge klienta
[2026-01-27 14:16:02.124][mtype=1] Kasjer nie znalazl stolika dla grupy
[2026-01-27 14:16:03.126][mtype=1] Kasjer rozpoczyna obsluge klienta
[2026-01-27 14:16:03.130][mtype=1] Kasjer nie znalazl stolika dla grupy
[2026-01-27 14:16:04.131][mtype=1] Kasjer rozpoczyna obsluge klienta
[2026-01-27 14:16:04.139][mtype=1] Kasjer nie znalazl stolika dla grupy
[2026-01-27 14:16:05.143][mtype=1] Kasjer rozpoczyna obsluge klienta
[2026-01-27 14:16:05.146][mtype=1] Kasjer nie znalazl stolika dla grupy
[2026-01-27 14:16:06.149][mtype=1] Kasjer rozpoczyna obsluge klienta
[2026-01-27 14:16:06.154][mtype=1] Kasjer nie znalazl stolika dla grupy

...

[2026-01-27 14:18:44.934][mtype=4] Zabito proces [246775]
[2026-01-27 14:18:44.934][mtype=4] Zabito proces [246786]
[2026-01-27 14:18:44.934][mtype=4] Zabito proces [246787]
[2026-01-27 14:18:44.947][mtype=4] Watek do czyszczenia procesow zombie zakonczony.
[2026-01-27 14:18:44.947][mtype=4] Klienci opuscili lokal, generator klientow konczy dzialanie
[2026-01-27 14:18:44.947][mtype=5] Pracownik konczy prace
[2026-01-27 14:18:44.947][mtype=5] Kasjer zamyka kase
[2026-01-27 14:18:44.947][mtype=5] Dzisiejszy utarg to: 1495 zl
[2026-01-27 14:18:44.947][mtype=5] Kasjer konczy prace
Logger zakończony.
```
</details>

Jak widać Po rezerwacji stołu i skończeniu przez klientów którzy już tam siedzieli posiłków kasjer nie może znaleźć miejsca dla żadnych innych klientów. Następnie po zamknięciu restauracji wszyscy klienci opuszczają lokal poprawnie. 

***Test udany.***

---

#### Test 4 - Klienci jedzą swoje dania natychmiastowo. (usunięcie sleep)
Zmieniamy zmienne:
```MAX_KLIENTOW``` - ([Shared_memory.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L9)) zmiana na: 10000

```MAX_KLIENTOW_W_RRESTAURACJI``` - ([Shared_memory.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L9)) zmiana na: 120

```X1``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L7)) zmiana na: 5

```X2``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L8)) zmiana na: 5

```X3``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L9)) zmiana na: 5

```X4``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L10)) zmiana na: 5

```CUSTOM_SLEEP_TIME``` ([klient.cpp](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/klient.cpp#L17)) zmiana na 0

```SLEEP``` ([klient.cpp](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/klient.cpp#L16)) zmiana na //sleep

Test ma na celu udowodnienei iż funkcja ```sleep()``` nie jest traktowana jako mechanizm synchronizacji międzyprocesowej. 

<details>
<summary>Logi testu: (rozwiń aby zobaczyć)</summary>

```
Logger uruchomiony. Odbieranie wiadomości...
[2026-01-27 14:36:44.942][mtype=1] Kasjer rozpoczyna prace
[2026-01-27 14:36:44.942][mtype=3] Pracownik rozpoczyna prace
[2026-01-27 14:36:44.942][mtype=4] Watek do czyszczenia procesow zombie uruchomiony.
[2026-01-27 14:36:44.942][mtype=4] Utworzono klienta: [389438]
[2026-01-27 14:36:44.942][mtype=4] Utworzono klienta: [389439]
[2026-01-27 14:36:44.942][mtype=4] Utworzono klienta: [389440]
[2026-01-27 14:36:44.942][mtype=4] Utworzono klienta: [389441]

...

[2026-01-27 14:37:17.581][mtype=4] Zabito proces [481567]
[2026-01-27 14:37:17.581][mtype=4] Zabito proces [481568]
[2026-01-27 14:37:17.581][mtype=4] Zabito proces [481569]
[2026-01-27 14:37:17.581][mtype=4] Zabito proces [481570]
[2026-01-27 14:37:17.581][mtype=4] Zabito proces [481571]
[2026-01-27 14:37:17.581][mtype=4] Zabito proces [481572]
[2026-01-27 14:37:17.581][mtype=4] Zabito proces [481573]
[2026-01-27 14:37:17.581][mtype=4] Zabito proces [481574]
[2026-01-27 14:37:17.591][mtype=4] Watek do czyszczenia procesow zombie zakonczony.
[2026-01-27 14:37:17.591][mtype=4] Klienci opuscili lokal, generator klientow konczy dzialanie
[2026-01-27 14:37:17.591][mtype=5] Pracownik konczy prace
[2026-01-27 14:37:17.591][mtype=5] Kasjer zamyka kase
[2026-01-27 14:37:17.591][mtype=5] Dzisiejszy utarg to: 167918 zl
[2026-01-27 14:37:17.591][mtype=5] Kasjer konczy prace
Logger zakończony.
```
</details>

Logi pokazują iż w krótkim czasie symulacji (ok 45s) wygenerowany został dochód: ```167918zł```. Oznacza to że po usunięciu funkcji sleep z programu pracownika symulacja znacznie przyspiesza oraz nie blokuje się (brak problemów z synchronizacją).

## 8. Elementy wyróżniające:
- konsola w programie ```kierownik``` pozwalająca na obsługę sygnałów oraz rezerwację określonej, podanej w konsoli ilości stolików.
- funkcja zapewniająca unikalne ziarno generatora umożliwiające pełną losowość w wyborach poszczególnych wątków ```klientów``` mimo znacznej szybkości działania programu.

## 9. Problemy napotkane podczas realizacji projektu i zastosowane rozwiązania
- Procesy zombie (pozostałości po klientach którzy już opuścili bar)
  - Problem: po opuszczeniu przez klienta baru zostawał on procesem zombie (zakończonym procesem, który jednak zajmował dalej miejsce w systemie)
  - Rozwiązanie: asynchroniczny wątek sprzątający w programie genratora, który na bieżąco czyści procesy zombie funkcją ```waitpid()```
 
- Synchronizacja procesów podczas zamykania restauracji/ewakuacji
  - Problem: procesy pracownika i kasjera powinny się kończyć dopiero po wyjściu wszystkich klientów z lokalu.
  - Rozwiązanie: zastosowanie semafora blokującego procesy pracownika i kasjera do czasu wyjścia klientów.
 
- Race condition
  - Problem: różne procesy/wątki mogą chcieć dostęp do tych samych zasobów w tym samym czasie.
  - Rozwiązanie: zastosowanie mutexów w postaci semaforów oraz ```std::mutex```
