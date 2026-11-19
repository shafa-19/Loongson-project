# §7 — Müəllim üçün demo təlimatı (RT-Thread: yuxulu bərbər)

Layihə: `D:\loongson1B_example\example_7_1\example_7_1.lxp`

## Addımlar (dəqiq ardıcıllıq — vacibdir)

| # | Kim | Nə etmək |
|---|---|---|
| 1 | **Lövhə** | Enerjini **tam kəs**, 10 saniyə gözlə, yenidən tax. *(Bu addım vacibdir — təmiz başlanğıc olmadan EJTAG proxy `pracc` xətası verir.)* |
| 2 | PC | LoongIDE-ni aç, `Project → Open Project` → `example_7_1.lxp` |
| 3 | PC | **`Ctrl + F9`** (Compile & Build) → `Project "example_7_1" compile successful` (0 xəta) |
| 4 | PC | **`F9`** (Debug → Run) → proqram lövhəyə yüklənir və **`main.c:119`** sətrində dayanır. Sağda `CPU Registers` dolur. |
| 5 | PC | **`Debug → Continue`** → proqram işləyir |
| 6 | PC | PuTTY: **UART5**, `115200 8N1` → RT-Thread çıxışı görünür |

## Gözlənilən konsol çıxışı

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
[barber] finish cut hair.
[barber] find customer 2, begin to cut hair.
[barber] finish cut hair.
[barber] find customer 3, begin to cut hair.
[barber] finish cut hair.
[barber] no customer, go to sleep.
```

Bu ardıcıllıq lövhədə **artıq yoxlanılıb** (gdb breakpoint jurnalı:
3 müştəri oturdu, 2-si getdi, 3 saç kəsimi).

## Əgər `F9` ilişsə (ehtiyat variant)

Simptom: `Message` panelində `wait for pracc of LS232(1B/1C) fail`, registr paneli boş.

1. `Debug → Stop` (F10)
2. Lövhənin enerjisini kəs-aç (10 s)
3. Yenidən `F9`

Hələ də olmasa — **bir kliklə işləyən alternativ**:

```
RUN_ON_BOARD.bat
```

Bu fayl LoongIDE-ni açır, sessiya başladır, ilişmiş gdb-ni dayandırır və proqramı
**birbaşa gdb ilə** yükləyib işə salır (bu yol 4 dəfə sınaqdan keçib).

## Kodun qısa izahı (müdafiə üçün)

| Obyekt | Tip | Başlanğıc | Nə üçün |
|---|---|---|---|
| `customers` | counting semaphore | 0 | gözləyən müştərilərin sayı; bərbər burada bloklanır |
| `barber` | counting semaphore | 0 | "bərbər hazırdır" siqnalı |
| `mutex` | mutex | 1 | `freeseats` sayğacını qoruyur |
| `freeseats` | `volatile int` | 3 | boş stulların sayı |

Thread-lər: bərbər (prioritet 10) + 5 müştəri (5..9). RT-Thread-də **kiçik rəqəm =
yüksək prioritet**, ona görə müştərilər ardıcıl (1→5) işləyir və nəticə təkrarlanandır.

Kritik hissə: müştəri `mutex`-i götürür → `freeseats` yoxlayır → stul varsa azaldır,
`customers` semaforunu buraxır (bərbəri oyadır) → mutex-i buraxır → `barber`
semaforunda gözləyir. Bərbər `customers`-də gözləyir, oyananda `freeseats`-i artırıb
`barber`-i **həmin kritik bölmədə** buraxır — beləliklə nə "itirilmiş oyanma",
nə də deadlock olur.

---

## Konsol (PuTTY) haqqında vacib qeyd

Bu maşındaki **CH340 USB-RS232 adapteri qüsurludur**:
- `SetCommState` yalnız ≤ 19200 baud üçün işləyir, 38400+ üçün `ERROR_GEN_FAILURE` verir
- portdan mənalı siqnal əvəzinə 5.5 MB/s "zibil" (0x6A) gəlir
- driver yenilənməsi (3.8 → 4.0.2026.2) problemi həll etmədi

Buna görə **konsol çıxışını bu adapterlə almaq mümkün deyil**. Layihə buna bağlı deyil:

| Sübut | Necə alındı |
|---|---|
| `F9` işləyir, registrlər dolur | LoongIDE debug sessiyası (ekran görüntüsü) |
| Proqram lövhədə işləyir | `run7.log` — 258 148 bayt yükləndi, `continue` |
| 3 oturdu / 2 getdi / 3 kəsim | gdb breakpoint jurnalı (`fig_hw_s7.png`) |

**Konsol lazımdırsa**: FTDI (FT232/FT2232) əsaslı USB-RS232 adapterlə 5 dəqiqədə alınar —
`serread3.ps1` skripti hazırdır, sadəcə port nömrəsini vermək kifayətdir.
