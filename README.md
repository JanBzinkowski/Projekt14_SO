# Temat projektu: Bar Mleczny


## Autor: Jan Bzinkowski

## Nr. albumu: 157193

### Ares repozytorium github: https://github.com/JanBzinkowski/Projekt14_SO/

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

## Uruchomienie programu

#### Kompilacja projektu
Będąc w głównym folderze projektu wpisać następujące komendy:
```
mkdir buld
cd build
cmake ..
cmake --build .
```
Wszystkie potrzebne programy powinny się pojawić w folderze ```/bin```

#### Uruchomienie programu

Aby uruchomić program główny należy uruchomić znajdujący się w folderze ```/bin``` program ```mainprog```. Aby poprawnie uruchomić program kierownika należy w osobnej konsoli uruchomić znajdujący się w folderze ```/bin``` program ```kierownik``` podając mu w argumencie wywołania pid programu głównego (```mainprog```). Przykład wywołania: ```./kierownik 1234```

#### Ustawianie zmiennych uruchomienia

Program posiada możliwości łatwego zmieniania warunków uruchomienia. Lista zmiennych uruchomienia:

```MAX_KLIENTOW``` - ([Shared_memory.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L8)) Zarządza maksymmalną ilością klientów aktywnych w programie w tym samym czasie (podstawowo: 10000)

```MAX_KLIENTOW_W_RRESTAURACJI``` - ([Shared_memory.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L9)) Zarządza maksymalną ilością klientów znajdujących się w lokalu, zarówmo przy stolikach jak i tych stojących w kolejce. (podstawowo: 120)

```X1``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L7)) Liczba stolików 1-osobowych w restauracji. (podstawowo: 5)

```X2``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L8)) Liczba stolików 2-osobowych w restauracji. (podstawowo: 5)

```X3``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L9)) Liczba stolików 3-osobowych w restauracji. (podstawowo: 5)

```X4``` ([Tables.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L10)) Liczba stolików 4-osobowych w restauracji. (podstawowo: 5)

```MENU_NAPOJE``` ([Zamowienie.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Zamowienie.h#L9)) Liczba możliwych do wyboryu napojów. Nr. napoju mówi o tym ile będzie on kosztował i jaki czas zajmie klientowi spożywanie go (sekundy) (podstawowo: 4)

```MENU_NAPOJE``` ([Zamowienie.h](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Zamowienie.h#L10)) Liczba możliwych do wyboryu dań. Nr. dania mówi o tym ile będzie on kosztował i jaki czas zajmie klientowi spożywanie go (sekundy) (podstawowo: 4)

```SLEEP``` ([klient.cpp](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/klient.cpp#L16)) Decyduje o tym czy klient ma używać sleep podczas jedzenia. (podstawowo: sleep, w razie potrzeby zmienić na //sleep)

## Struktura programu

- ```mainprog``` program "rodzic". Tworzy wszystykie potrzebne struktury IPC. Tworzy programy potomne za pomocą ```fork()```. Tworzone programy: [```generator_klientow```](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/mainprog.cpp#L98), [```kasjer```](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/mainprog.cpp#L109), [```pracownik```](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/mainprog.cpp#L120) i [```logger```](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/src/mainprog.cpp#L131). Po zakończeniu symulacji usuwa utworzone wcześniej struktury IPC.

- ```kasjer``` odpowiada za przyjmowanie zamówień od klientów oraz za przydzielanie ich do stolików. Po zakończeniu symulacji zamyka kasę oraz sprawdza utarg zyskany podczas symulacji.

- ```pracownik``` odpowiada za wydawanie zamówień. Na polecenie ```kierownika``` może zarezerwować odpowiednią ilość stołów lub donieść dodatkowe stoliki 3-osobowe (czynność jednorazowa!)

- ```generator_klientow``` odpowiada za generowanie klientów, oraz za asynchroniczne usuwanie procesów zombie.

- ```logger``` odpowiada za wypisywanie logów wysyłanych przez inne programy w konsoli a także do pliku ```log.txt```.

- ```kierownik``` zarządza restauracją. Umożliwia poprawne wydawanie sygnałów do programów potomnych ```mainprog```.

## Opis plików

- Folder ```include```:
  - ```Shared_memory.h``` - zawiera zmienne zarządzające ilością klientów w lokalu oraz strukturę [```SharedMem```]([https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L11](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Shared_memory.h#L8))
 
  - ```Tables.h``` - zawiera zmienne zarządzające ilością tolików a także strukturę [```Table```](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Tables.h#L17)
 
  - ```Wrappers.h``` - zawiera deklaracje wrapperów funkcji System V oraz struktury odpowiadające za [wysyłanie wiadomości do loggera](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Wrappers.h#L20) oraz za [rezerwację stolików i komunikację kierownik-pracownik](https://github.com/JanBzinkowski/Projekt14_SO/blob/master/include/Wrappers.h#L25-35) 

