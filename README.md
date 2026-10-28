# Polmar Easy 2 Converter

[![Windows build](https://github.com/Paliddo86/polmar-converter/actions/workflows/build-windows.yml/badge.svg)](https://github.com/Paliddo86/polmar-converter/actions/workflows/build-windows.yml)
[![Latest release](https://img.shields.io/github/v/release/Paliddo86/polmar-converter)](https://github.com/Paliddo86/polmar-converter/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Convertitore con interfaccia grafica per Windows che trasferisce i canali da un
vecchio file `.dat` **Polmar Easy / XW-679 / KT-680** a un file `.dat`
**Polmar Easy 2**.

[**Download ultima release**](https://github.com/Paliddo86/polmar-converter/releases/latest)

## Caratteristiche

- GUI Win32 nativa, nessuna installazione e nessuna DLL aggiuntiva.
- Eseguibili Windows 64-bit e 32-bit.
- Gestione di 128 canali con frequenze RX/TX, toni CTCSS/DCS, Narrow e Busy Lock.
- Tre modalità di conversione: `merge-full`, `replace-all`, `freq-only`.
- Migrazione automatica delle impostazioni globali old dove esiste un equivalente.
- Scelta del livello SQ, del separatore decimale e sovrascrittura dell'output.
- Log dettagliato al termine della conversione.
- Preserva schema e impostazioni del file nuovo/template.

## Requisiti

- Windows 10 o Windows 11 (64-bit o 32-bit).
- Windows 7: se Windows segnala una DLL UCRT mancante, installare
  l'aggiornamento **KB2999226**.
- Non servono DLL di libstdc++/libgcc accanto all'eseguibile.
- Gli eseguibili non sono firmati: Windows SmartScreen può chiedere conferma
  al primo avvio.

## Download

Dalla pagina [Releases](https://github.com/Paliddo86/polmar-converter/releases/latest):

| File | Descrizione |
|---|---|
| `polmar-convert-windows.zip` | pacchetto completo: exe 64/32-bit, README, LEGGIMI, licenza e checksum |
| `polmar-convert-win64.exe` | eseguibile Windows 64-bit |
| `polmar-convert-win32.exe` | eseguibile Windows 32-bit |
| `SHA256SUMS.txt` | hash SHA-256 degli eseguibili |

Link diretti:

- [polmar-convert-windows.zip](https://github.com/Paliddo86/polmar-converter/releases/latest/download/polmar-convert-windows.zip)
- [polmar-convert-win64.exe](https://github.com/Paliddo86/polmar-converter/releases/latest/download/polmar-convert-win64.exe)
- [polmar-convert-win32.exe](https://github.com/Paliddo86/polmar-converter/releases/latest/download/polmar-convert-win32.exe)
- [SHA256SUMS.txt](https://github.com/Paliddo86/polmar-converter/releases/latest/download/SHA256SUMS.txt)

## Uso rapido

1. Estrai il pacchetto e copia `polmar-convert-win64.exe` (o `win32`) nella
   cartella che contiene `vecchio.dat` e `nuovo.dat`.
2. Avvia l'eseguibile. I percorsi di `vecchio.dat`, `nuovo.dat` e `new.dat`
   vengono compilati automaticamente se i file sono nella cartella dell'exe.
3. Controlla o seleziona il **file vecchio**, il **file nuovo/template** e il
   **file output**.
4. Scegli la **modalità** di conversione.
5. Lascia **SQ Level** su `1 (consigliato)`.
6. Lascia attivo **Migra impostazioni old**.
7. Se `new.dat` esiste già, spunta **Sovrascrivi output**.
8. Premi **Converti** e controlla il log. Il file output viene creato nella
   posizione indicata.

> Conserva sempre una copia dei file originali prima di sovrascrivere o
> caricare un nuovo profilo sulla radio.

## Interfaccia

| Campo | Default | Note |
|---|---|---|
| File vecchio (.dat) | `vecchio.dat` accanto all'exe | file originale a 128 canali |
| File nuovo/template (.dat) | `nuovo.dat` accanto all'exe | file Polmar Easy 2 usato come base |
| File output (.dat) | `new.dat` | può essere qualsiasi nome |
| Modalità | `merge-full` | vedi tabella seguente |
| Decimali | `Auto` | usa `Punto` o `Virgola` solo se il template lo richiede |
| SQ Level | `1` | `0` = Off, `1..9` = livello; `Mantieni nuovo file` lascia il valore del template |
| Migra impostazioni old | attivo | migra Squelch, Beep, Battery Save, VOX, TOT, ecc. |
| Sovrascrivi output | disattivo | necessario se il file output esiste già |

## Modalità di conversione

| Modalità | Comportamento |
|---|---|
| `merge-full` | i canali old popolati sostituiscono l'intera riga; i canali old vuoti lasciano invariata la riga del template |
| `replace-all` | i canali old popolati sostituiscono la riga; i canali old vuoti svuotano anche la riga del template |
| `freq-only` | sostituisce solo le frequenze RX/TX; toni, potenza, banda e Busy Lock restano dal template |

## Impostazioni migrate

Con **Migra impostazioni old** attivo:

| Vecchia impostazione | Nuova impostazione |
|---|---|
| FM Scan Mode: Manual | `fmsel=0` |
| Squelch Level: 1 | `sqlevel=1` (sovrascrivibile con SQ Level) |
| Beep: Off | `keybeep=0` |
| Scan Mode: Carry | `scanmode=1` (CO) |
| Time-out Timer: 5 s | `tot=1` (15 s) |
| Battery Save: On | `powersave=1` |
| VOX: Off | `voxlevel=0` |
| Auto Backlight: Off | `light=0` |

Le impostazioni old senza equivalente restano quelle del file nuovo/template.

## Note pratiche

- **Fruscio sui canali senza tono**: il template può avere SQ Level `0` (Off).
  Il convertitore imposta `1` per impostazione predefinita; se il fruscio
  rimane, seleziona `2..9` e riconverti.
- **File vecchio non valido**: il file old deve essere un `.dat` KT-680 da
  128 canali (2096 byte).
- **Output già esistente**: spunta **Sovrascrivi output** oppure scegli un
  altro nome.
- **Dopo la conversione**: carica il file output con il software Polmar Easy 2
  e verifica i canali e le impostazioni sulla radio.

## Build

Dipendenze: C++17 e MinGW-w64.

### Windows con MSYS2

Apri il terminale **MSYS2 UCRT64** e installa il toolchain:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc
```

Poi, dalla cartella del repository:

```sh
sh build_win.sh
```

Per la versione 32-bit usa il terminale **MSYS2 MINGW32**:

```sh
pacman -S --needed mingw-w64-i686-gcc
sh build_win.sh
```

In alternativa, con MinGW nel `PATH` e un prompt dei comandi:

```bat
build_win.bat
```

### macOS / Linux (cross-compile)

```sh
# macOS
brew install mingw-w64

# Debian/Ubuntu
sudo apt install g++-mingw-w64-x86-64 g++-mingw-w64-i686
```

Poi:

```sh
sh build_win.sh
```

Gli eseguibili vengono creati in `dist/`:

- `dist/polmar-convert-win64.exe`
- `dist/polmar-convert-win32.exe`

### Test del core

```sh
sh build_core_test.sh
./dist/polmar-core-test vecchio.dat nuovo.dat output.dat
```

Il test è uno strumento di sviluppo: non fa parte della GUI e richiede i file
`.dat` di input.

## Release

Il workflow `.github/workflows/build-windows.yml`:

- su push e pull request verso `main` compila le versioni 64-bit e 32-bit e
  carica gli artifact;
- su tag `v*` compila, crea `SHA256SUMS.txt`, prepara lo ZIP e pubblica la
  Release;
- può essere avviato manualmente dalla scheda **Actions**.

Per pubblicare una nuova versione:

```sh
git tag -a v1.0.0 -m "Polmar Easy 2 Converter v1.0.0"
git push origin v1.0.0
```

## Licenza

Distribuito con licenza [MIT](LICENSE).

## Disclaimer

Progetto indipendente, non affiliato a Polmar. "Polmar" e "Polmar Easy 2" sono
marchi dei rispettivi titolari. Usa il software a tuo rischio e conserva sempre
una copia dei file originali.
