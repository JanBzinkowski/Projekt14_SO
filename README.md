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

Klienci pojawiają się w losowych momentach czasu. Około 5% z nich nie składa zamówienia i w związku z tym nie może zająć miejsca przy stoliku. Pozostali klienci zamawiają gorące dania, płacą w kasie i odbierają zamówienie od pracownika obsługi. Niedopuszczalne jest oczekiwanie na wolny stolik z wydanym daniem – klient może usiąść wyłącznie wtedy, gdy dostępne jest odpowiednie miejsce.

W barze znajdują się stoliki 1-, 2-, 3- i 4-osobowe w liczbie odpowiednio ```X1, X2, X3, X4,``` co determinuje maksymalną liczbę klientów jednocześnie spożywających posiłek. Klienci przychodzą pojedynczo lub w grupach 2-, 3- lub 4-osobowych. Ze względu na konflikty między klientami, przy jednym stoliku mogą siedzieć tylko grupy o tej samej liczebności.

Po zakończeniu posiłku naczynia są zwracane zbiorczo przez jednego przedstawiciela grupy.

Z programu kierownika można wysłać następujące sygnały:

- jednorazowo (**SIGRTMIN+1**) podwoić liczbę stolików 3-osobowych (*+1 jeśli początkowa liczba stolików wynolsiła 0*);

- na żądanie (**SIGRTMIN+2**) zarezerwować określoną liczbę stolików lub miejsc, wyłączając je z użytku klientów;

- w sytuacji pożaru (**SIGRTMIN**) natychmiast następuje ewakuacja klientów, zakończenie pracy obsługi i zamknięcie kasy;

- dodana została również obsłyga sygnału **SIGINT**, aby móc szybko zamykać restaurację (zabijać procesy) z poporawnym czyszczeniem zasobów.

Przy zamknięciu kasy kasjer liczy utarg z symulacji.

## Uruchomienie programu

### Jak poprawnie uruchomić program

#### Kompilacja projektu
Będąc w głównym folderze projektu wpisać następujące komendy:

