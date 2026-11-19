# BOMB DEFUSAL GAME — Loongson 1B (MIPS32)

**Layihə:** `D:\loongson1B_example\example_10_bomb`
**Baza:** `example_6_1` (bare-metal, UART + GPIO) — BSP-yə heç bir dəyişiklik edilmədi.
**Yeni yazılan:** yalnız `main.c` (oyun). Bütün BSP, linker skripti və `libc` olduğu kimi qaldı.

---

## 1. Oyunun ideyası

Bomba 8 "teldən" (LED) ibarətdir. Hər raundda bomba öz **kodunu** LED-ləri bir-bir
yanıb-sönməklə göstərir. Oyunçu həmin nömrələri **eyni ardıcıllıqla** terminaldan
yazaraq teli kəsməlidir. 90 saniyə, 5 mərhələ, 3 səhv hüququ var.

| Mərhələ | Kodun uzunluğu |
|---|---|
| 1 | 2 tel |
| 2 | 3 tel |
| 3 | 4 tel |
| 4 | 5 tel |
| 5 | 6 tel |

**Qaydalar**

* düzgün tel → həmin LED qısa yanır, növbəti tələb olunan mövqe açılır;
* səhv tel → **strike** (3 dəfə) və **−10 saniyə**, kod yenidən göstərilir;
* `D` (scan) → kodu terminalda göstərir, **−15 saniyə**;
* `R` (replay) → kodu LED-lərdə təkrar göstərir, **−5 saniyə**;
* mərhələ heç səhv olmadan keçilirsə **+5 saniyə**;
* 3 strike və ya vaxt bitməsi → **BOOM** (bütün LED-lər yanır);
* 5 mərhələ bitərsə → **DEFUSED** (qələbə işıq effekti).

---

## 2. İstifadə olunan hardware (hamısı mövcud layihədə var)

| Element | Detal | Haradan |
|---|---|---|
| 8 LED | GPIO **9, 11, 15, 17, 20, 22, 6, 7**, **aktiv LOW** (0 = yanır) | `example_6_1` ilə eyni |
| UART3 | bələdçinin tələb etdiyi port | `ls1x_uart_*` |
| UART5 | konsol portu (`bsp.h` → `BSP_USE_UART5`) | `ls1x_uart_*` |
| Taymer | `get_clock_ticks()` — 1 tick = 1 ms | `core/tick.c` |
| Terminal | 9600 8N1 (`CMD_BAUD_CODE`), PuTTY və ya `tools\serial_terminal.ps1` | — |

> **Fiziki düymə yoxdur.** BSP-də (`ls1x-drv`, `core`) heç bir key/button driveri
> olmadığı üçün klaviatura **UART terminalıdır** — bu, `example_6_1`-in də işləmə
> üsuludur. Uydurma pin istifadə edilməyib.

---

## 3. Komandalar (terminal)

| Giriş | Nə edir |
|---|---|
| `1` … `8` | tel `n`-i kəs (hər sətirdə bir rəqəm) |
| `D` / `SCAN` | kodu göstər (−15 s) |
| `R` / `REPLAY` | kodu LED-lərdə təkrar göstər (−5 s) |
| `S` / `STATUS` | cari vəziyyət |
| `N` / `NEW` | yeni oyun |
| `H` / `HELP` | komanda siyahısı |
| `ENTER` | boş sətir = oyunu başlat / davam et |

Nümunə sessiya:

```
Type N (or just press ENTER) to arm the bomb
>

=== stage 1/5: the bomb will show 2 wires ===
  the bomb is arming ...

[88s] stage 1/5  wire 1/2  strikes 0/3 > 5

  ok, wire 5 correct

[87s] stage 1/5  wire 2/2  strikes 0/3 > 3

  fuse 1 cut
  clean stage: +5 seconds
```

---

## 4. Kodun quruluşu (`main.c`)

| Bölmə | Nə var |
|---|---|
| konfiqurasiya | `LEVELS`, `SEQ_MAX`, `BOMB_TIME_MS`, cərimə/bonus sabitləri |
| `struct channel` | hər UART üçün öz sətir buferi və entropiya sayğacı (2 port paralel) |
| LED köməkçiləri | `led_set`, `led_all`, `led_pulse`, `led_strike_flash`, `led_win_pattern`, `led_boom_pattern` |
| RNG | öz LCG-si (`rnd_next`) — `rand()` asılılığı yoxdur |
| ekranlar | `banner`, `help_screen`, `status_line`, `prompt` |
| oyun məntiqi | `game_new`, `stage_start`, `show_code`, `strike`, `stage_cleared`, `game_over`, `game_win` |
| giriş | `poll_input` → `handle_line` (rəqəm / komanda) |
| `main` | sürücüləri qur, LED-ləri çıxış et, iki portu aç, əbədi dövr: `poll_input` + countdown |

**Dizayn qərarları**

* `printf`/`malloc` istifadə olunmur — yalnız `out()` və öz `out_num()`; kod ölçüsü
  kiçik qalır və `-msoft-float`/bare-metal mühitdə risk yoxdur.
* Status sətri `\r` ilə **yerində yeniləndiyi** üçün terminalda canlı geri sayım görünür;
  oyunçu yazarkən (`any_typing()`) sətir pozulmur.
* Kod göstərilərkən (`show_code`) daxil olan düymələr `flush_port()` ilə atılır —
  animasiya zamanı təsadüfi basmalar oyunu pozmur.
* Countdown **mütləq vaxta** (`deadline`) bağlıdır, ona görə bloklayan animasiyalar
  vaxtı dayandırmır.

---

## 5. Yığma (build)

```bat
cd /d D:\loongson1B_example\example_10_bomb
python <deliverables>\tools\build_proj.py D:\loongson1B_example\example_10_bomb example_10_bomb
```

Nəticə (yoxlanılıb):

```
compiled: 84 / 84
   text    data     bss     dec     hex  filename
  59504    3608   17968   81080   13cb8  Build\example_10_bomb.exe
```

> Qeyd: bu layihə **bare-metal** olduğu üçün `-DOS_NONE` ilə yığılmalıdır
> (IDE-də `DefinedSymbols=LS1B;OS_NONE`). `tools\build_proj.py` bunu avtomatik edir.

---

## 6. Lövhədə işə salma

1. `LoongIDE` → `Project → Open Project` → `example_10_bomb.lxp`
2. **F9** (Debug → Run). `.lxp`-də `AutoRebuild=0` olduğu üçün IDE yenidən
   yığmır, hazır `Build\example_10_bomb.exe`-ni yükləyir.
3. Terminal: **COM5, 9600 8N1** (və ya `tools\serial_terminal.ps1 -Port COM5 -Baud 9600`).
4. `ENTER` → oyun başlayır.

**Diaqnostika:** `F9` "wait for pracc of LS232(1B/1C) fail" verərsə — lövhənin
enerjisini 10 saniyə kəs, sonra yenidən `F9`. CH340 adaptoru açılmırsa —
USB-dən çıxarıb yenidən tax.

---

## 7. Test nəticələri

### 7.1 Host simulyasiyası — `TEST_RESULTS_host.txt`

`example_10_bomb_test/` qovluğunda `main.c` **dəyişdirilmədən** (`-Dmain=game_main`)
kompilyasiya olunur, BSP funksiyaları stub-larla əvəz olunur, virtual 1 ms saat və
UART FIFO verilir. Bot oyunu yalnız **klaviatura ilə** oynayır.

| Ssenari | Yoxlanan | Nəticə |
|---|---|---|
| `win` | 5 mərhələ düzgün cavab → `DEFUSED` | **PASS** (7/7) |
| `strike` | 3 səhv cavab → `BOOM`, "three wrong wires" | **PASS** (8/8) |
| `timeout` | heç nə etməmək → `BOOM`, "the time ran out" | **PASS** (7/7) |
| `commands` | `H`, `S`, `D`, `R`, səhv komanda | **PASS** (8/8) |

Cəmi **30 yoxlama, 0 xəta**.

### 7.2 Real lövhə

Yükləmə və oyun ssenarisi üçün `tools\board_game_test.ps1` hazırdır: skript
konsolu oxuyur, `D` ilə kodu öyrənir, rəqəmləri göndərir və nəticəni
(`DEFUSED` / `BOOM`) jurnala yazır.

```powershell
powershell -ExecutionPolicy Bypass -File tools\board_game_test.ps1 -Port COM5 -Baud 9600
```

**Real lövhə nəticəsi (17 noyabr 2026, COM5 @9600):**

| Cəhd | Nəticə |
|---|---|
| 1-ci | Bot səhv kod göndərdi (köhnə scan sətrini oxudu) → 3 strike → **BOOM** `fuses cut: 1/5` |
| 2-ci | Düzgün oynadı: 5 mərhələ, **0 strike** → **DEFUSED**, `time left: 13 s` |

Son mərhələnin kodu `3-5-2-5-6-3` idi (6 tel) — tam jurnal: `board_test_log.txt` (3 287 bayt).

Bu, oyunun real hardware-də (LED-lər, UART, 1 ms taymer, countdown, cərimə/bonus,
strike və hər iki sonluq) işlədiyini sübut edir.
