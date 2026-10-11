# Dati, fondali e integrazione PS1

## Fonti nel repository

- `stages/stages.json`: dati modificabili anche dall’editor.
- `src/data.h`: capacità e layout StageData.
- `src/game.c`: stage_load, movimento, zones_collision.
- `src/psx.c` e `src/utils.c`: mesh_on_plane, camera e background_init.
- `psxstudio/stage_editor/model.js`: proiezione, collisioni di controllo, encoder TIM e validazione.
- `psxstudio/README.md`: avvio degli strumenti; `psxstudio/stage_editor/app.js` e `server.py`: editor browser, esportazione e server Python.
- `stages/Makefile`, `build.sh`, `cdrom/CONF.CTI`: generazione e integrazione.

Controlla questi file se sono cambiati rispetto alla guida. Non applicare le
regole dei fondali di Crystal Fantasy agli asset di Neon Ranger/SHOOTER.

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

Proiezione come `psxstudio/stage_editor/model.js`: R=Rx*Ry*Rz, punto camera=R*(p+camera.pos),
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
`walkable` in model.js replica i limiti stretti di mesh_on_plane:
x > plane.x-1, x < plane.x+w, z > plane.z+d, z < plane.z+1.
La sagoma usata dai trigger estende x e z del punto del personaggio di mesh.size;
`zoneHit` usa 100 come valore predefinito, da verificare sul modello corrente.
Uno spawn che appare fuori dal rettangolo del trigger può comunque attivarlo.
Evita il rimbalzo immediato fra aree e non spostare ingressi utilizzati da altre zone.

## Preparazione del fondale

Crea o normalizza una PNG 320×240. Dopo il layout preliminare sovrapponi
piattaforme, zone e ingressi per verificare che il pavimento rappresenti il percorso.
L’SVG può essere aperto nel browser; per strumenti che richiedono raster usa
una cattura del browser o esporta una PNG dal canvas dell’editor.

Le due pagine PS1 sono:

- sinistra: 256×256, colonne 0–255, VRAM (320,0);
- destra: 64×256, colonne 255–318, VRAM (576,0).

La seconda pagina parte dalla colonna **255**, come lo sprite del motore:
non cambiare a 256; le immagini condividono una colonna. Le righe 240–255 sono
padding nero opaco. Entrambe sono TIM a 16 bit e sostituiscono le texture del
fondale precedente: non assegnare loro slot residenti aggiuntivi.

L’editor importa il PNG completo e prepara correttamente PNG/TIM. Con il server
li conserva in `psxstudio/stage_editor/assets`; l’esportazione ZIP include quattro asset,
CONF_ENTRIES.TXT e README.txt. Copia gli asset runtime in `data/stages`, conserva
anche il PNG completo per anteprime future se prodotto separatamente.
Usa nomi non già assegnati; dopo BG9, B10_0.TIM/B10_1.TIM rispettano il limite.

In alternativa, da `data`, dopo avere preparato le pagine PNG:

```bash
wine img2tim.exe -bpp 16 -usealpha -org 320 0 stages/BG9_0.png
wine img2tim.exe -bpp 16 -usealpha -org 576 0 stages/BG9_1.png
```

Conserva il nero opaco: 0x8000; 0x0000 è trasparente sulla PS1.
Non eliminare pixel neri opachi di ombre o oggetti. Per un fondale opaco normalizza
prima l’alpha; l’encoder JS lo fa componendo su nero.

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
Dalla cartella `stages` esegui `make run`. Il Makefile usa **-m32**:
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
riporta chiaramente che questa verifica resta da fare. Tutto il C eventualmente
modificato deve restare ANSI C89/Psy-Q; normalmente questo flusso non richiede C.
