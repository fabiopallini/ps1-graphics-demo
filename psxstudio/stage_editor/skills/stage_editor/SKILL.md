---
name: stage_editor
description: Crea o modifica fondali e aree giocabili di Crystal Fantasy nel progetto Psy-Q per PS1 reale, con piattaforme, zone di passaggio, ingressi, texture TIM e integrazione nel gioco. Usa questa skill per richieste di nuove schermate collegate o modifiche alla loro navigazione, anche tramite l’editor browser del progetto.
---

# Crystal Fantasy: scenari giocabili

Lavora nel repository che contiene `stages/stages.json`, `src/game.c`,
`cdrom/CONF.CTI` e `psxstudio`. Se la cartella corrente non è questo progetto,
individua il repository prima di scrivere. Leggi le sue istruzioni `AGENTS.md`.
La copia mantenuta di questa skill è `psxstudio/stage_editor/skills/stage_editor`.

Una richiesta come «aggiungi un bosco collegato al santuario» comprende una nuova
area navigabile e integrata nel gioco, salvo una limitazione esplicita dell’utente.
Una richiesta di sola anteprima o solo fondale non richiede compilazione o nuove
zone. Non cambiare il motore per adattarlo a un’immagine.

## Prima di creare il fondale

Leggi [references/workflow.md](references/workflow.md) per geometria, risorse e
verifiche. Per un esempio già integrato leggi
[references/sanctuary.md](references/sanctuary.md); i valori dell’area 8 sono un
esempio, non una configurazione obbligatoria.

Rileggi i dati correnti: gli indici delle aree e degli ingressi sono gli ID usati
dalle zone. Conserva le aree esistenti, NPC, dialoghi e campi sconosciuti; aggiungi
le aree nuove in fondo. Modifica l’area di partenza solo quanto necessario per
collegare l’uscita e il punto di ritorno.

Scegli camera e geometria prima dell’immagine. Produci una guida proiettata dei
percorsi e usala per generare il fondale; un pavimento disegnato con una prospettiva
incompatibile non si corregge semplicemente aggiungendo poligoni. Mantieni spazi
percorribili leggibili e margini rispetto a muri, parapetti e decorazioni.
Per generare o modificare raster usa la skill `imagegen` se disponibile; altrimenti
usa gli asset forniti o un metodo locale adatto, dichiarandone la provenienza.
Non generare nuove immagini se basta correggere geometria o collegamenti.

## Editor e scrittura dei dati

Puoi usare l’editor browser oppure modificare direttamente gli stessi dati; scegli
il metodo più efficace e controlla visivamente il risultato. L’editor con server
si avvia dalla radice con `python3 psxstudio/stage_editor/server.py`, porta predefinita 8000.
Il salvataggio è **manuale**, tramite Salva JSON o Ctrl+S. Usalo per ispezionare
camera, piattaforme, zone, ingressi e messaggi di validazione.

Le API HTTP sono `/api/project`, `/api/stages`, `/api/backgrounds` e
`/api/export/<id>`; per scrivere occorrono token e revisione ottenuti dal progetto.
Se il server segnala un conflitto, rileggi e riapplica solo le modifiche pertinenti:
non forzare la sovrascrittura. Non inviare una copia obsoleta del JSON mentre
l’utente sta lavorando nell’editor.

Per verifiche automatizzate senza browser puoi importare `validate` e `serialize`
da `psxstudio/stage_editor/server.py`; sono funzioni pure. Lo script
`psxstudio/stage_editor/preview_stage.py` genera SVG senza server e senza dipendenze esterne:

```bash
python3 psxstudio/stage_editor/preview_stage.py 8 --output /tmp/area-8.svg
```

Lo script serve anche per verificare la proiezione del layout prima di scegliere
l’immagine finale. Il rendering dell’editor e l’SVG sono anteprime, non catture
PS1: controlla il risultato nel gioco quando è possibile.

## Completamento

Integra gli asset nelle cartelle runtime e nel CTI solo quando richiesto dal
compito. Per una nuova area giocabile completa il flusso di generazione e build
illustrato nella guida, controllando i log e i collegamenti di andata e ritorno.
Non considerare sufficienti i soli asset salvati sotto `psxstudio/stage_editor/assets`.

Riporta aree collegate, file creati, verifiche eseguite e limiti effettivi della
verifica, distinguendo anteprima da prova nel gioco. Non inventare un test su
emulatore o hardware se hai verificato solo dati, binario o compilazione.
