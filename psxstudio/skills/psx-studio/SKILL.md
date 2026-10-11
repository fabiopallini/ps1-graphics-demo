---
name: psx-studio
description: Crea o modifica scenari giocabili, personaggi e modelli low poly per PS1 reale in questo progetto Psy-Q, usando gli strumenti PSX Studio e i loro formati stages.json, OBJ e texture TIM.
---

# PSX Studio: scenari e modelli PS1

Questa skill è locale al progetto, in `psxstudio/skills/psx-studio`.
Lavora nel repository con `stages/stages.json`, `src/game.c`, `cdrom/CONF.CTI`
e `psxstudio`. Leggi `AGENTS.md` e il [README degli strumenti](../../README.md).

Scegli la guida in base al lavoro richiesto:

- **Fondali, piattaforme, zone e ingressi:** leggi [references/stages.md](references/stages.md).
  Il programma è l'editor nativo `psxstudio/stage_editor`, in GTK 4, C11 e OpenGL.
  Per un esempio integrato consulta [references/sanctuary.md](references/sanctuary.md).
- **Personaggi, mesh OBJ, UV e texture dei modelli:** leggi
  [references/models.md](references/models.md). Il modellatore è
  `psxstudio/3dmodel_editor/index.html`, utilizzabile direttamente nel browser.

Carica entrambe le guide solo quando il compito comprende entrambi i tipi di asset.
Usa l'editor oppure modifica direttamente gli stessi dati, secondo il metodo più
efficace; verifica i formati e controlla visivamente il risultato.

Rispetta lo stile, la scala e i limiti effettivi del caricatore PS1. Per una
rifinitura leggi prima gli asset e il codice che li carica. Conserva i dati non
coinvolti, dialoghi, campi sconosciuti e topologia delle animazioni esistenti.
Non adattare il motore per aggirare i limiti degli asset.

Per generare o modificare immagini raster usa la skill `imagegen` quando
disponibile; altrimenti usa asset forniti o un metodo locale adatto, dichiarando
la provenienza. Una richiesta di sola anteprima, fondale o modello non richiede
integrazione o compilazione del gioco. Una richiesta di nuova area giocabile
collegata comprende dati, asset e integrazione, salvo un limite esplicito.

Integra nel gioco solo nell'ambito richiesto: verifica nomi, VRAM, CTI e percorsi
runtime come descritto nella guida pertinente. Il C del gioco resta ANSI C89/Psy-Q;
l'editor nativo usa C11. Riporta file, collegamenti o conteggi di geometria,
verifiche effettuate e limiti reali. Distingui anteprima da prova su emulatore
o PS1, senza dichiarare prove non eseguite.
