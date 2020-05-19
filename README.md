# PlayStation 1 3D graphics demo

Dipendenze:
- Psy-Q (http://www.psxdev.net/)
- Wine (oppure compatibile con Windows 95/98/ME/XP 32bit)

Configurazione:
<pre>
  Scaricare Psy-Q e creare una cartella psyq dentro al drive C di wine con le relative cartelle include, lib e bin.
  >winecfg
  impostare versione su Windows XP
  >wine regedit
  importare il file pspath.reg nel registro e creare una cartella TMP nel drive C di wine "C:\TEMP"
  >cd src
  >make
</pre>

![preview](https://user-images.githubusercontent.com/8449266/82376024-c6e95480-9a21-11ea-9827-915922b95ac5.gif)
