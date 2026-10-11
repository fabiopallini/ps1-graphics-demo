# Cortile del santuario — area 8

Il percorso prosegue dall'area 7_city: andare fino all'estremità della strada
conduce al cortile. Tornando lungo il vialetto in basso si rientra in città,
al secondo spawn, lontano dal trigger di uscita.

La configurazione è in `stages/stages.json`. Il motore e il suo editor rimangono quelli
esistenti: i planes sono rettangoli X/Z sul terreno, con w positivo e d negativo.
Due rettangoli sovrapposti definiscono vialetto centrale e cortile. Il leggero
margine interno rispetto al pavimento evita aiuole, parapetti e lanternine.
Sono usati due dei dieci planes disponibili, una zona di ritorno e uno spawn.

Camera: posizione (0,1055,2410), rotazione (305,0,0), nelle unità del gioco.
Lo sfondo è stato generato da una guida proiettata con questa camera;
SetGeomOffset(160,120) e SetGeomScreen(512) sono quelli del motore.

Asset in data/stages:

- BG8.png: anteprima dello sfondo 320x240.
- BG8_0.png/TIM: parte sinistra, 256x256, VRAM (320,0).
- BG8_1.png/TIM: parte destra, 64x256, VRAM (576,0).
- BG8_WALK.svg: anteprima tecnica con planes verdi, trigger rosso, spawn azzurro.

La versione iniziale aveva padding nelle righe 240–255: queste sono però
visibili nel gioco PAL 320×256. Per nuove esportazioni completa il fondale
in quelle righe, seguendo [stages.md](stages.md). La parte destra inizia
alla colonna 255, come s1.pos.vx in background_init: la colonna condivisa
mantiene la continuità con il disegno del motore esistente.
Le texture sostituiscono quelle della schermata precedente, senza aumentare
il numero di texture residenti. Entrambe sono registrate alla radice della ISO
in CONF.CTI con nomi DOS 8.3, entro i 10 byte dei tims in StageData.

Dopo modifiche a stages.json, dalla cartella stages eseguire:

    make run

Il generatore a 32 bit produce STAGES.BIN e stages.h per il layout Psy-Q.
Non compilarlo a 64 bit: cambierebbe dimensioni e allineamento dei puntatori.

Per aggiornare l'anteprima dei poligoni, dalla radice del progetto:

    python3 psxstudio/stage_editor/preview_stage.py 8 --output data/stages/BG8_WALK.svg

Per riconvertire le texture, dalla cartella data:

    wine img2tim.exe -bpp 16 -usealpha -org 320 0 stages/BG8_0.png
    wine img2tim.exe -bpp 16 -usealpha -org 576 0 stages/BG8_1.png

Infine eseguire bash build.sh dalla radice e leggere src/BUILD.LOG.
Nel gioco SELECT apre l'editor esistente, QUADRATO aggiunge un plane;
le frecce lo spostano e R1 + frecce ne cambiano larghezza/profondità.
Uscendo dall'editor con SELECT vengono stampati i planes per stages.json.

Verificati layout e valori del binario, riferimenti a spawn/aree, raggiungibilità
delle uscite usando mesh_on_plane e la logica dei trigger, separazione dei punti
di ingresso dai trigger, dimensioni e posizioni VRAM dei TIM.
L'immagine SVG è una proiezione di controllo, non una cattura del gioco.
Il posizionamento visivo va ancora controllato nel gioco su emulatore o PS1.
