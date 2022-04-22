# PandOSPlus

Evoluzione di Kaya OS, PandOSPlus è un sistema operativo per architettura uMPS3

## Requisiti

I seguenti pacchetti sono necessari per la compilazione:

- mipsel-linux-gnu-gcc
- umps3
- make

## Installazione dei pacchetti

Per installare mipsel-linux-gnu-gcc (Testato su Debian 11, potrebbe variare in base alla [distribuzione](https://github.com/virtualsquare/umps3#how-to-install)):
```bash
$ sudo apt install gcc-mipsel-linux-gnu
```
Per installare umps3:
```bash
$ sudo apt install umps3
```
Per installare make:
```bash
$ sudo apt install make
```

## Istruzioni per la compilazione

Viene fornito un file Makefile, per compilare aprire un terminale nella cartella e digitare:
```bash
$ make
```

Per eliminare i file creati da make eseguire il comando:

```bash
$ make clean
```

## Esecuzione 

Avviare la macchina già presente (machine), togliere la spunta da "Exceptions" ed eseguire il programma.
