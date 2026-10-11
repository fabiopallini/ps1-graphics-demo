# Dati, fondali e integrazione PS1

## Fonti nel repository

- `stages/stages.json`: dati modificabili anche dall’editor.
- `src/data.h`: capacità e layout StageData.
- `src/game.c`: stage_load, movimento, zones_collision.
- `src/psx.c` e `src/utils.c`: mesh_on_plane, camera e background_init.
- `psxstudio/stage_editor/model.c`: proiezione, collisioni di controllo, validazione e gestione del JSON.
- `psxstudio/stage_editor/assets.c`: importazione PNG, encoder TIM ed esportazione ZIP.
- `psxstudio/stage_editor/render.c` e `ui.c`: rendering OpenGL e interfaccia GTK.
- `psxstudio/README.md`: avvio degli strumenti.
- `stages/Makefile`, `build.sh`, `cdrom/CONF.CTI`: generazione e integrazione.

Controlla questi file se sono cambiati rispetto alla guida. Non applicare le
regole dei fondali di Crystal Fantasy agli asset di Neon Ranger/SHOOTER.

## Editor e dati

L'editor nativo si compila e si avvia dalla radice con:

```bash
make -C psxstudio/stage_editor build
make -C psxstudio/stage_editor run
```

Legge `stages/stages.json` e i fondali in `data/stages`. Il salvataggio è manuale
con Save JSON o Ctrl+S: mantiene la versione precedente in `stages.json~` e
controlla la revisione del file. In caso di conflitto rileggi e riapplica soltanto
le modifiche pertinenti; evita di sovrascrivere una copia più recente dell'utente.
Aggiungi le nuove aree in fondo: gli indici di stage e spawn sono gli ID delle
zone. Conserva NPC, dialoghi e campi sconosciuti; modifica l'area di partenza
solo quanto serve per collegare uscita e ingresso di ritorno.

Per validare senza aprire la finestra, dopo `make build`:

```bash
psxstudio/stage_editor/build/stage_editor --validate stages/stages.json
```

Lo script indipendente `psxstudio/stage_editor/preview_stage.py` genera una
guida SVG del layout, anche prima che esista un fondale:

```bash
python3 psxstudio/stage_editor/preview_stage.py 8 --output /tmp/area-8.svg
```

Scegli camera e percorsi prima di creare l'immagine e usa la guida proiettata
come riferimento. Mantieni margini rispetto a muri, parapetti e decorazioni:
un pavimento con prospettiva incompatibile non si corregge aggiungendo rettangoli.

## Geometria e camera

Ogni area ha name, tim_0, tim_1, cam_x/y/z, cam_rx/ry/rz e gli array planes,
zones, spawns, npcs (opzionale). Un plane è `{x,y,z,w,h,d}`: W positivo,
D negativo, lati allineati a X/Z. Il motore carica Y come quota; H non crea scale
né collisioni verticali. Per i percorsi normali usa Y=0 e H=0.
Non sostituire questi rettangoli con poligoni arbitrari o mesh 3D.

Limiti per area: 10 planes, 5 zones, 5 spawns (almeno uno), 5 NPC complessivi.
Coordinate/dimensioni sono interi a 32 bit; rotazioni sono short a 16 bit.
I nomi TIM devono entrare in char[10], terminatore incluso: ad esempio BG9_0.TIM.
L’editor accetta basename maiuscoli di 1–5 caratteri, numeri e underscore.
Le pagine dei dialoghi possono contenere al massimo 77 byte UTF-8, senza NUL.

Proiezione come `psxstudio/stage_editor/model.c`: R=Rx*Ry*Rz, punto camera=R*(p+camera.pos),
centro (160,120), focale 512. Angoli: 4096 unità/giro. La camera in coordinate
mondo è -camera.pos; non interpretare cam_x/y/z come posizione diretta.
L’anteprima usa floating point, il GTE usa arrotondamenti fissi.
Taglia i poligoni al piano vicino, invece di scartare interi percorsi che escono
dalla camera. `preview_stage.py` supporta tutte e tre le rotazioni.

## Collegamenti e raggiungibilità

Ogni zona aggiunge stage_id e spawn_id. Per un passaggio bidirezionale:

1. Aggiungi un ingresso nella nuova area, sul terreno e lontano dalla sua uscita.
2. Nell’area di partenza aggiungi una zona raggiungibile che punta al nuovo ingresso.
3. Aggiungi un ingresso di ritorno nell’area originale senza spostare quelli esistenti.
4. Nella nuova area aggiungi una zona che punta a questo ingresso di ritorno.

Controlla ID, continuità delle piattaforme e raggiungibilità dei trigger da ciascun
ingresso interessato. Non basta che zona e piattaforma si tocchino nel disegno.
Il controllo di percorribilità in `model.c` replica i limiti stretti di mesh_on_plane:
x > plane.x-1, x < plane.x+w, z > plane.z+d, z < plane.z+1.
La sagoma usata dai trigger estende x e z del punto del personaggio di mesh.size;
la validazione in `model.c` usa 100 come valore predefinito, da verificare sul modello corrente.
Uno spawn che appare fuori dal rettangolo del trigger può comunque attivarlo.
Evita il rimbalzo immediato fra aree e non spostare ingressi utilizzati da altre zone.

## Preparazione del fondale

Crea o normalizza una PNG 320×240. Dopo il layout preliminare sovrapponi
piattaforme, zone e ingressi per verificare che il pavimento rappresenti il percorso.
Il PNG 320×240 è la guida dell’editor; completa le righe PAL come indicato sotto.
L’SVG può essere aperto nel browser; per strumenti che richiedono raster usa
una cattura del rendering o una conversione dell'SVG a PNG.

Le due pagine PS1 sono:

- sinistra: 256×256, colonne 0–255, VRAM (320,0);
- destra: 64×256, colonne 255–318, VRAM (576,0).

La seconda pagina parte dalla colonna **255**, come lo sprite del motore:
non cambiare a 256; le immagini condividono una colonna. Controlla `SCREEN_HEIGHT` e PAL/NTSC in `src/psx.h`: il gioco attuale è
PAL 320×256, quindi le righe 240–255 sono visibili e devono contenere il
fondale, mai padding nero. L’editor mostra soltanto le prime 240 righe:
per l’integrazione PAL completa anche le 16 righe inferiori senza spostare
il contenuto o le coordinate delle prime 240. In NTSC 320×240 le righe
aggiuntive possono invece essere padding. Entrambe sono TIM a 16 bit e sostituiscono le texture del
fondale precedente: non assegnare loro slot residenti aggiuntivi.

L'editor conserva tutte le righe di un PNG 320×256; per gli altri formati scala
a 320×240 e riflette le ultime 16 righe per completare il fondale PAL.
Controlla comunque che l'estensione sia coerente con il pavimento. Il programma
conserva gli import in `psxstudio/stage_editor/assets`; l'esportazione ZIP include quattro asset,
CONF_ENTRIES.TXT e README.txt. Copia gli asset runtime in `data/stages`, conserva
anche il PNG completo per anteprime future se prodotto separatamente.
Usa nomi non già assegnati; dopo BG9, B10_0.TIM/B10_1.TIM rispettano il limite.

In alternativa, da `data`, dopo avere preparato le pagine PNG:

```bash
wine img2tim.exe -bpp 16 -usealpha -org 320 0 stages/BG9_0.png
wine img2tim.exe -bpp 16 -usealpha -org 576 0 stages/BG9_1.png
```

Conserva il nero opaco: 0x8000; 0x0000 è trasparente sulla PS1.
Con `-usealpha` prepara PNG con alpha esplicitamente opaco. Verifica anche i
pixel molto scuri: la quantizzazione RGB555 può produrre zero; se il pixel
sorgente è opaco, correggilo a 0x8000 nel TIM.
Non eliminare pixel neri opachi di ombre o oggetti. Per un fondale opaco normalizza
prima l'alpha; l'encoder nativo lo fa componendo su nero.

## CTI, generazione e build

Registra entrambi i TIM **alla radice della gerarchia esistente** in
`cdrom/CONF.CTI`, con File uppercase e Source corrispondente al file reale.
Non inserirli nella directory SHOOTER e non duplicare voci già presenti.
Esempio (ripetere per la seconda pagina):

```text
File BG9_0.TIM
    XAFileAttributes Form1 Data
    Source [root]\data\stages\BG9_0.tim
EndFile
```

Preserva le terminazioni di riga esistenti del CTI. Solo i file registrati sono
presenti nell’ISO: averli in data non basta.

Valida prima di eseguire il generatore: non controlla tutti i limiti degli array.
Dalla cartella `stages` esegui `make build`. Il Makefile usa **-m32**:
StageData contiene puntatori NPC; compilare il generatore a 64 bit altera il layout.
Questo aggiorna `data/stages/STAGES.BIN` e `src/stages.h`, anche se ignorati da git.
La taglia della parte fissa è attualmente 684 byte; le pagine dialogo aggiungono
blocchi variabili. Non scrivere offset basati su 684 senza verificare il layout
corrente. Controlla tabella offsets, quantità di record e destinazioni.

Dalla radice esegui `bash build.sh`, poi ispeziona `src/BUILD.LOG` per errori
Psy-Q, linker, BUILDCD e generazione ISO. Il successo di un comando shell da solo
non prova che tutta la compilazione DOS sia riuscita. Se necessario verifica
la presenza dei nuovi TIM e STAGES.BIN nella ISO. Per un compito solo grafico
senza integrazione fermati agli asset richiesti, indicando cosa resta da integrare.

Controlla in emulatore/hardware allineamento del personaggio, uscite, spawn e
continuità visiva quando questi strumenti sono disponibili. Se non lo sono,
riporta chiaramente che questa verifica resta da fare. Il C del gioco deve
restare ANSI C89/Psy-Q; l'editor nativo usa C11. Normalmente questo flusso
non richiede modifiche al C.
