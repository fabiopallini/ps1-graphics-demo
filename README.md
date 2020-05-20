# PlayStation 1 3D graphics demo

Dipendenze:
- Psy-Q (http://www.psxdev.net/)
- Wine (oppure compatibile con Windows 95/98/ME/XP 32bit)

Configurazione:
  1. Scaricare Psy-Q e creare una cartella psyq dentro al drive C di wine.
  2. Inserire le relative cartelle include, lib e bin dentro alla cartella psyq.
  > winecfg
  3. Impostare versione su Windows XP
  > wine regedit
  4. Importare il file pspath.reg nel registro e creare una cartella TMP nel drive C di wine "C:\TEMP"
  > make

![preview](https://user-images.githubusercontent.com/8449266/82489341-ca441500-9ae1-11ea-85ec-e7d0b059174b.gif)
