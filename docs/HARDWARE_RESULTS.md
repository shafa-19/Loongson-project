# Real lövhədə alınan nəticələr (Loongson 1B, LS1B200)

Tarix: 16 noyabr 2026. Alət: LoongIDE 1.1 + SDE Lite (mips-sde-elf-gdb 7.9),
EJTAG proxy `127.0.0.2:50000`, lövhə PMON-dan sonra EJTAG ilə idarə olunur.

---

## §5 — Assembly: 1+3+5+... cəmi (TƏSDİQLƏNDİ)

Metod: `example_7_2.exe` yükləndi, `save_result` sətrinə qədər icra edildi.

```
pc: 0x8020012c        <-- sw t0, 0(t5)
t0: 0xfffe0001        (4294836225)  cəm
t1: 0x0001ffff        (131071)      daşmağa səbəb olacaq növbəti tək ədəd
t2: 0xffffffff        (4294967295)  maksimum 32-bit unsigned
t3: 0x0001fffd        (131069)      son qəbul edilmiş tək ədəd
t4: 0x0001fffe        (131070)      qalan boş yer
t5: 0x80030000                     təyinat ünvanı

stepi (sw icra edildi) sonrası:
0x80030000: 0xfffe0001
```

Nəticə bələdçinin Cədvəl 5-1-i ilə **tam üst-üstə düşür**.
Qəbul edilən hədlərin sayı 65535, 65535² = 0xFFFE0001.

---

## §6 — UART3/LED idarəetməsi (TƏSDİQLƏNDİ)

### 6a. Vizual test
Proqram `main()`-ə çatır, `GPIO_OUT = 0xFFFFFFFF` (bütün LED-lər sönük).
`running = 1`, `speed_ms = 300` verildikdə **8 LED növbə ilə yanıb-sönür**
(istifadəçi tərəfindən təsdiqləndi).

### 6b. Əmr parserinin testi (özünütest build-i)
Eyni funksiyalar (`parse_command`, `led_only`, `led_pins[]`) saxlanılaraq `main()`
əvəzinə ardıcıl əmr testi işlədildi; hər əmrdən sonra `LS1B_GPIO_OUT(0)`,
`speed_ms`, `running` yaddaşa (0x80230000) yazıldı və gdb ilə oxundu.

| Əmr | GPIO_OUT | speed_ms | running | Nəticə |
|---|---|---|---|---|
| (başlanğıc) | 0xFFFFFFFF | 500 | 0 | hamısı sönük |
| `ON 3` | 0xFFFF7FFF | 500 | 0 | bit15 = 0 → **LED 3 yandı** (GPIO15) |
| `OFF` | 0xFFFFFFFF | 500 | 0 | hamısı söndü |
| `ON 8` | 0xFFFFFF7F | 500 | 0 | bit7 = 0 → **LED 8 yandı** (GPIO7) |
| `SPEED 5000` | 0xFFFFFF7F | **500** | 0 | **rədd edildi** (100..1000 xaricində) |
| `SPEED 250` | 0xFFFFFF7F | **250** | 0 | qəbul edildi |
| `BADCMD` | 0xFFFFFF7F | 250 | 0 | tanınmayan əmr, vəziyyət dəyişmədi |
| `START` | 0xFFFFFF7F | 250 | **1** | running light işə düşdü |
| `STOP` | 0xFFFFFF7F | 250 | **0** | dayandı |

Bit xəritəsi: LED1=GPIO9, LED2=11, LED3=15, LED4=17, LED5=20, LED6=22, LED7=6, LED8=7.
Aktiv aşağı səviyyə: `gpio_write(pin, 0)` = yanır.

---

## Şəbəkə/kompilyasiya yoxlamaları

| Bölmə | Yoxlama | Nəticə |
|---|---|---|
| §5 | `cs-make -f Makefile.mk all` | 0 xəta, 0 xəbərdarlıq, .bin = 2840 bayt |
| §6 | LoongIDE Compile & Build | 0 xəta (18 xəbərdarlıq — BSP başlıqları), .bin = 81 568 bayt |
| §7 | `mips-sde-elf-gcc -Wall -c` (real RT-Thread başlıqları) | **0 xəta, 0 xəbərdarlıq** |
| §8 | eyni üsul, lwIP 1.4.1 başlıqları | 5 fayl 0 xəbərdarlıq; `tcp_server.c`-də 1 xəbərdarlıq (`EDEADLK` yenidən təyini — vendor başlığında) |

---

## Mühitdə aşkar edilən və həll edilən problemlər

| Problem | Səbəb | Həll |
|---|---|---|
| `wait for pracc of LS232 fail` | CPU əvvəlki kəsilmiş sessiyadan debug rejimində qalır | layihə parametri **`ResetAtStart=1`** — IDE hər sessiyadan əvvəl lövhəni reset edir; xəta keçicidir, ikinci F9 qoşulur |
| IDE pəncərəsi 0×0 açılır | yenidən başladılanda layout bərpa olunmur | `MoveWindow` ilə ölçü verilir |
| GUI-yə klaviatura/klik getmir | Windows `SetForegroundWindow`-a icazə vermir | `SwitchToThisWindow` + həmin skript daxilində input |
| `call parse_command(...)` MIPS-də ilişir | gdb-nin MIPS çağırış mexanizmi etibarsız | özünütest build-i (yuxarıdaki 6b) |
| Serial konsol yoxdur | COM3/COM4 adapteri lövhəyə qoşulmayıb (lokal loopback) | testlər gdb ilə aparılır |

---

## Təkrar icra üçün

```bash
# lövhəyə yükləmək və nəticəni oxumaq (§5)
cd D:/example_7_2/example_7_2/Build
D:/LoongIDE/mips-2015.05/bin/mips-sde-elf-gdb.exe -q -batch \
  -ex "target remote 127.0.0.2:50000" -ex "load" -ex "set \$pc = start" \
  -ex "advance *0x8020012c" -ex "info registers t0 t3" \
  -ex "stepi" -ex "x/1xw 0x80030000" example_7_2.exe
```

```bash
# GPIO registrini canlı oxumaq (§6)
x/1xw 0xBFD010F0      # GPIO_OUT: 0 = LED yanır
```

Qeyd: proxy yalnız LoongIDE işləyərkən (`127.0.0.2:50000`) mövcuddur; IDE bağlananda
əvvəlcə F9 ilə sessiya başladılmalıdır.


---

## §7 — RT-Thread: yuxulu bərbər (TƏSDİQLƏNDİ)

Layihə: `D:\loongson1B_example\example_7_1` (RT-Thread 4.0.2, 227 fayl, `RTT4/` ağacı).
Yığım: `example_7_1.exe` 643 689 bayt, `.bin` 258 152 bayt, **0 xəta**.

Lövhədə icra (gdb breakpoint jurnalı, hər hadisə real vaxtda çap olunub):

```
[MUSTERI OTURDU] freeseats = 3      customer 1: freeseats 3 -> 2
[MUSTERI OTURDU] freeseats = 2      customer 2: 2 -> 1
[MUSTERI OTURDU] freeseats = 1      customer 3: 1 -> 0
[MUSTERI GEDIR]  freeseats = 0      customer 4: bos stul yoxdur -> cixir
[MUSTERI GEDIR]  freeseats = 0      customer 5: bos stul yoxdur -> cixir
[BERBER KESIR]   served = 0         berber 1-ci musterini kesir
[BERBER KESIR]   served = 1         2-ci
[BERBER KESIR]   served = 2         3-cu
```

Nəticə: **3 müştəri oturdu, 2-si getdi, 3 saç kəsimi** — `FREESEATS_INIT = 3` və
5 müştəri üçün gözlənilən davranış. Sıralama da düzgündür: müştərilər (prioritet
5..9) bərbərdən (prioritet 10) əvvəl işləyir, bərbər yalnız onlar bloklandıqdan
sonra növbə ilə xidmət edir.

### Layihəni wizard olmadan yaratmaq

Wizard bu maşında açılmadığı üçün layihə `Template.ini`-dəki reseptə əsasən
`tools/make_project.py` ilə quruldu (wizard-ın etdiyi işin eynisi):
9 qovluq kopyası + 4 fayl + `.lxp` generasiyası (53 qovluq, 227 unit).
Əlavə düzəlişlər: `[Assembler]` bölməsində `OS_RTTHREAD` və RT-Thread include
yolları, `[Linker] Libraries=m;c;gcc` (əks halda `memset/strlen` tapılmırdı).

---

## §7 — YEKUN NƏTİCƏ (tam təsdiqlənmiş)

Lövhədə, PuTTY konsolunda (COM5, 115200) alınan **real çıxış**:

```
Welcome to RT-Thread.

[init] barber thread started (priority 10).
[init] customer 1 started (priority 5).
[init] customer 2 started (priority 6).
[init] customer 3 started (priority 7).
[init] customer 4 started (priority 8).
[init] customer 5 started (priority 9).
[customer 1] sits down, freeseats=2
[customer 2] sits down, freeseats=1
[customer 3] sits down, freeseats=0
[customer 4] no free seat, leaves.
[customer 5] no free seat, leaves.
[barber] find customer 1, begin to cut hair.
[customer 1] haircut done, leaves.
[barber] finish cut hair.
[barber] find customer 2, begin to cut hair.
[customer 2] haircut done, leaves.
[barber] finish cut hair.
[barber] find customer 3, begin to cut hair.
[customer 3] haircut done, leaves.
[barber] finish cut hair.
[barber] no customer, go to sleep.
```

Sayğac: **3 oturdu / 2 getdi / 3 kəsim / 1 yuxuya getdi** — bələdçinin gözlədiyi nəticə.

### Demo parametrləri (kodda `#define` ilə)

| Define | Dəyər | Nə edir |
|---|---|---|
| `DEMO_SLOW` | 1 | gəliş 2000 ms, kəsim 4000 ms (görünən demo); 0 = bələdçidəki 50/40 ms |
| `DEMO_LEDS` | 1 | LED 1..5 = gözləyən müştəri, LED 8 = bərbər işləyir; 0 = yalnız konsol |

### Yolda tapılıb düzəldilən 4 problem

1. **Konsolu `console_init(9600)` etmək qəza verirdi** — `Clock_initialize`-dəki `rt_kprintf` 36 ms çəkir, interrupt-lar bağlı qalır, sonra yığılmış tick-lər `TLB Load` istisnası yaradır. Konsol **115200-də saxlanıldı**.
2. **Vaxt nisbəti ssenarini pozurdu** — gəlişlər 400 ms aralıqla olanda bərbər stulu tez azad edir və 4 müştəri otururdu. Gəlişlər **bərabər (2000 ms)** edildi → 3 oturur, 2 gedir.
3. **`led_set(id, 1)` koda düşməmişdi** — yalnız başlanğıc işişi yanırdı; əlavə edildi.
4. **Wizard işləmədiyi üçün layihə əl ilə quruldu** (`Template.ini` resepti): 227 fayl, `[Assembler]` bölməsində `OS_RTTHREAD`, linkerdə `Libraries=m;c;gcc`.

---

## §8 — Şəbəkə ötürülməsi: 4 test də KEÇDİ (17 noyabr 2026)

Lövhə `192.168.1.123`, PC `192.168.1.111/24`, birbaşa kabel, `ping` 6/6 (0% itki).

| Test | Nəticə | Sübut |
|---|---|---|
| 8.2.12 TCP **server** (lövhə :9060) | ✅ | PC-dən 3 mesaj → 3 echo |
| 8.2.13 TCP **client** (lövhə → PC :9061) | ✅ | `board connected from 192.168.1.123:49153`, 5 mesaj |
| 8.2.14 UDP **server** (lövhə :9062) | ✅ | 3 datagram → 3 echo |
| 8.2.15 UDP **client** (lövhə → PC :9063) | ✅ | `FROM 192.168.1.123:49153 -> client ticks = 1804.` |

Tam çıxış: `04_lwip_network/EVIDENCE_all_tests.txt`, hesabatda **Şəkil 5-3**.

### Bu bölmədə tapılan 3 vacib səhv

1. **`-DUSE_LWIP -DBSP_USE_GMAC0` verilmirdi** → lwIP/GMAC kodu kompilyasiyadan
   kənar qalırdı, lövhə şəbəkədə səssiz idi. `.lxp`-dəki `DefinedSymbols`-ı
   kompilyatora ötürmək lazımdır.
2. **`console_init(9600)` boot-da qəza verirdi** (`TLB Load`) — `Clock_initialize`-dəki
   `rt_kprintf` uzun çəkir, interrupt-lar bağlı qalır, tick-lər yığılır.
   Konsol boot-da **115200**, sonra `main()`-də 9600-ə keçir.
3. **UDP client bir dəstək göndərib `recvfrom`-da bloklanır** — dinləyici (PC)
   proqram başlamazdan **əvvəl** açıq olmalıdır, əks halda test “0 datagram” görünür.

---

## §9 — Linux portinqi (yalnız hesabat sənədi, lövhədə icra edilməyib)

§9 bu maşında **lövhədə icra edilmədi** (VMware/Ubuntu VM, kernel mənbələri, buildroot
və rootfs bu kompüterdə yoxdur). Hesabatın **6-cı fəsli** kurs materialındakı real
nəticələrə əsaslanır:

| Addım | Nəticə |
|---|---|
| Cross compiler `gcc-4.3.6` (`/opt/gcc-4.3-ls232`) | ✅ quraşdırılıb, `--with-arch=loongson232` |
| `vmlinux` (12 723 580 B) + System.map | ✅ hazır (arxivdə prebuilt) |
| buildroot → `rootfs.ext2` (87 410 688 B) | ✅ `exit=0`, `e2fsck` təmiz |
| TFTP + PMON (`ifconfig syn0 192.168.1.11`, ping 0% itki) | ✅ |
| `devcp ... /dev/mtd0` (kernel) | ✅ PMON `12723580` bayt yazdı |
| `devcp ... /dev/mtd1` (rootfs) | ❌ 87 MB image → MTD partitiondan böyükdür |
| İlk Linux boot | ⏳ əvvəlki addımdan asılıdır |

**Səbəb və həll:** PMON-un verdiyi erase ünvanları (`mtd0=0x00E00000`, `mtd1=0x07200000`)
partitionun 87 MB-dan kiçik olduğunu göstərir. Həll: `BR2_TARGET_ROOTFS_EXT2_SIZE` ilə
image-i partitiona sığdırmaq, və ya flaşı EJTAG ilə yazmaq.

Bu, hesabatın 6.5 bölməsində açıq şəkildə yazılıb — "port tam bitdi" kimi göstərilməyib.

---
