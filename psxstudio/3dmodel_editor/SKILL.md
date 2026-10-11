---
name: psx-3d-models
description: Crea o modifica personaggi e modelli low poly per PS1 reale, con geometria OBJ, UV e texture compatibili con PSX 3D Studio e con il caricatore Psy-Q di questo progetto.
---

# Modelli per PSX 3D Studio

Leggi `AGENTS.md` del repository e [README.md](../README.md) per avvio e panoramica degli strumenti; consulta `index.html` e `app.js` per i controlli del modellatore. Il programma è `psxstudio/3dmodel_editor/index.html`: funziona direttamente nel browser. Questa skill resta un file locale del progetto.

## Creazione e modifica

Parti dalla richiesta: personaggio, oggetto, scala, stile e destinazione. Per rifinire un modello esistente leggi prima OBJ, PNG e codice che lo carica. Per Crystal Fantasy confronta gli asset in `data/chars/char1`; per altri modelli scegli lo stile richiesto.

Privilegia silhouette leggibile, pochi vertici e dettagli dipinti sulla texture. I limiti del caricatore sono soglie di sicurezza, non budget da riempire. Mantieni capelli, spalle e altre parti riconoscibili anche nelle viste laterali. Controlla normali, facce mancanti e superfici collassate.

Puoi usare il browser oppure generare dati e verificarli tramite `core.js`, che espone `PSX3DStudioCore`: usa `validate`, `parseOBJ` ed `exportOBJ` per controllare ed esportare la mesh. Consulta le firme nel file, senza ricreare un esportatore incompatibile. Il programma gestisce una posa, senza rig o timeline.

## Compatibilità del gioco

Il caricatore in `src/psx.c` contiene 2048 vertici, 2048 UV e 2048 indici di faccia: ogni quad usa 8 indici, quindi massimo 256 quad. Esporta nel formato PS1: sole facce `v/vt` con quattro coppie; per i triangoli ripeti l'ultima coppia. Non mescolare triangoli e quad, non aggiungere normali o materiali all'OBJ PS1.

Le coordinate devono essere decimali senza esponenti, entro i 9 caratteri del parser, con righe inferiori a 100 caratteri. Usa l'export del programma. Controlla la conversione SVECTOR alla scala effettiva letta dal codice: RUN e ATT possono usare scale differenti.

In Crystal Fantasy Y è verso il basso, piedi circa a Y=0 e testa a Y negativo. Il controllo Y del modellatore modifica la vista, non le coordinate esportate.

Per animazioni esistenti preserva topologia, ordine dei vertici e indici UV fra tutti i frame. Le pose sono OBJ separati: modificare un solo file non modifica l'animazione intera. Le UV `nan` di alcuni asset vengono normalizzate a zero come nel parser del gioco. Il personaggio attuale ha circa 240 quad, quindi poco margine per estrusioni.

## Texture e consegna

Usa un atlante PNG piccolo, coerente con la mesh, e conserva le UV nelle rifiniture. Il comando Mappa UV dell'interfaccia assegna l'intera texture a ciascuna faccia: non usarlo per sostituire indiscriminatamente un atlante esistente. Per nuove texture pianifica quali regioni servono alle parti del modello. Conserva l'alpha e verifica i bordi trasparenti.

Consegna OBJ PS1, progetto `.model.json` riapribile e PNG quando richiesta, con un'anteprima e conteggi di geometria. Il JSON usa `format: psx3dstudio-1`, `model`, `texture`, `camera`, `view`, `yDown`; consulta `app.js` per costruire valori validi. Scegli la cartella indicata dall'utente; se manca, conserva i nuovi asset in una sottocartella del modellatore. Non sostituire asset esistenti senza una richiesta che includa la sostituzione.

Verifica apertura del progetto e OBJ nel browser, texture, viste frontale e laterale e round-trip degli indici. Il canvas è una vista di lavoro, non un'emulazione PS1. Per modifiche al programma usa anche `tests.html`.

Quando è richiesta l'integrazione nel gioco, segui `AGENTS.md` per Wine/img2tim, slot VRAM liberi e registrazione in `cdrom/CONF.CTI`. Usa nomi ISO9660 compatibili, verifica percorsi runtime e scala, compila con `bash build.sh` e leggi `src/BUILD.LOG`. La sola creazione di un modello non richiede modifiche al motore o compilazione.
