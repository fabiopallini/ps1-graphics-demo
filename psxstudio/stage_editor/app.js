import { LIMITS, clone, project, projectPolygon, unproject, corners, rectangle, inside, validate, encodeTim, serialize } from './model.js';
const $ = selector => document.querySelector(selector);
const canvas = $('#scene'), ctx = canvas.getContext('2d');
const colors = { planes: '#7ddbb1', zones: '#ef9b89', spawns: '#8fbef6' };
const titles = { planes: 'Piattaforma', zones: 'Zona', spawns: 'Ingresso' };
const state = { stages: [], sid: 0, selection: null, view: 'perspective', tool: 'select', layers: { planes:true, zones:true, spawns:true }, backgrounds:new Map(), catalog:{files:[],imported:[]}, history:[], future:[], version:0, savedVersion:0, zoom:1, top:{x:0,z:0,scale:0.1}, drag:null, revision:'', token:'', saving:false, conflict:false };
const esc = text => String(text).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const stage = () => state.stages[state.sid];
const selected = () => state.selection && stage()?.[state.selection.kind]?.[state.selection.index];
const round = n => Math.round(n / Number($('#snap').value)) * Number($('#snap').value);
const key = s => `${s.tim_0}|${s.tim_1}`;
function status(text, error=false) { $('#save-state').textContent=text; $('#save-state').classList.toggle('error',error); }
function report(error) { status(error.message || error,true); }
async function api(url, data) {
  if (window.offlineEditor) return window.offlineEditor.api(url, data);
  const response = await fetch(url,data ? { method:'POST', headers:{'Content-Type':'application/json','X-Editor-Token':state.token}, body:JSON.stringify(data) } : undefined);
  const result = await response.json();
  if (!response.ok) throw Object.assign(new Error(result.error || 'Operazione non riuscita.'),{status:response.status});
  return result;
}
function image(url) { return new Promise((resolve,reject) => { const im = new Image(); im.onload=()=>resolve(im);im.onerror=()=>reject(new Error('Immagine non disponibile.'));im.src=url; }); }
async function background(s) {
  const id=key(s); if(state.backgrounds.has(id)) return state.backgrounds.get(id);
  const job=(async()=>{
    const imported=[...state.catalog.imported].reverse().find(i=>i.tim0===s.tim_0 && i.tim1===s.tim_1);
    if(imported) return { image:await image(imported.url), imported };
    const files=state.catalog.files;
    const prefix=s.tim_0.replace(/_[01]\.TIM$/,'');
    const full=files.find(f=>f.name.toUpperCase()===`${prefix}.PNG` && f.width===320 && f.height>=240);
    if(full) return {image:await image(full.url)};
    const parts=[s.tim_0,s.tim_1].map(name=>files.find(f=>f.name.toUpperCase()===name.replace(/\.TIM$/,'.PNG')));
    if(parts.some(p=>!p)) return null;
    const images=await Promise.all(parts.map(p=>image(p.url)));
    // Asset 2 stores the two pages in reverse order. Identify pages by width.
    const left=images.find(im=>im.width===256), right=images.find(im=>im.width===64);
    if(!left || !right) return null;
    const surface=document.createElement('canvas');surface.width=320;surface.height=240;
    const c=surface.getContext('2d');c.imageSmoothingEnabled=false;c.drawImage(left,0,0);c.drawImage(right,255,0);
    return {image:surface};
  })().catch(()=>null);
  state.backgrounds.set(id,job);
  const result=await job;state.backgrounds.set(id,result);return result;
}
async function load() {
  const result=await api('/api/project');
  state.stages=result.stages;state.catalog=result.catalog;state.revision=result.revision;state.token=result.token;
  state.sid=Math.min(state.sid,state.stages.length-1);state.selection=null;state.history=[];state.future=[];
  state.version=0;state.savedVersion=0;state.conflict=false;state.backgrounds.clear();
  status('Sfondi in caricamento…');await Promise.all(state.stages.map(background));
  fitTop();render();fitZoom();status(window.offlineEditor ? 'JSON caricato · salvataggio tramite download' : 'Collegato a stages.json · tutto salvato');
}
function changed(before) {
  if(JSON.stringify(before)===JSON.stringify(state.stages)) {render();return;}
  state.history.push(before);if(state.history.length>60)state.history.shift();state.future=[];
  state.version++;render();status('Modifiche da salvare');
}
function mutate(action) {
  const before=clone(state.stages);
  try { if(action()===false)return;changed(before); } catch(error) {state.stages=before;render();report(error);}
}
async function save() {
  if(!stage() || state.saving || state.conflict)return;
  const checks=validate(state.stages);
  if(checks.errors.length) {status('Salvataggio sospeso: correggi i dati evidenziati',true);renderMessages();return;}
  const snapshot=clone(state.stages), version=state.version;
  state.saving=true;status('Salvataggio…');$('#save').disabled=true;
  try {
    const result=await api('/api/stages',{stages:snapshot,revision:state.revision});
    state.revision=result.revision;state.savedVersion=version;
    status(state.version===version ? (window.offlineEditor ? 'Download di stages.json avviato · ' : 'Salvato in stages.json · ')+new Date().toLocaleTimeString('it-IT',{hour:'2-digit',minute:'2-digit',second:'2-digit'}) : 'Modifiche da salvare');
  } catch(error) {if(error.status===409)state.conflict=true;report(error);} finally {
    state.saving=false;$('#save').disabled=false;
  }
}
function fitTop() {
  const s=stage();if(!s)return;
  const points=[...s.planes.flatMap(p=>corners(p)),...s.zones.flatMap(p=>corners(p)),...s.spawns];
  const xs=points.map(p=>p.x),zs=points.map(p=>p.z);
  const xmin=Math.min(-100,...xs),xmax=Math.max(100,...xs),zmin=Math.min(-100,...zs),zmax=Math.max(100,...zs);
  state.top={x:(xmin+xmax)/2,z:(zmin+zmax)/2,scale:Math.min(280/(xmax-xmin),200/(zmax-zmin))};
}
function fitZoom() {
  const viewport=$('#viewport');state.zoom=Math.max(.5,Math.min((viewport.clientWidth-52)/320,(viewport.clientHeight-52)/240));resizeCanvas();
}
function resizeCanvas() {
  canvas.style.width=`${Math.round(320*state.zoom)}px`;canvas.style.height=`${Math.round(240*state.zoom)}px`;
  $('#zoom-label').textContent=`${Math.round(state.zoom*100)}%`;
  const viewport=$('#viewport');viewport.style.justifyContent=320*state.zoom>viewport.clientWidth-40?'flex-start':'center';viewport.style.alignItems=240*state.zoom>viewport.clientHeight-40?'flex-start':'center';draw();
}
function toScreen(p) {return state.view==='perspective'?project(stage(),p):{x:160+(p.x-state.top.x)*state.top.scale,y:120-(p.z-state.top.z)*state.top.scale};}
function toWorld(p,y=0) {return state.view==='perspective'?unproject(stage(),p,y):{x:state.top.x+(p.x-160)/state.top.scale,y,z:state.top.z-(p.y-120)/state.top.scale};}
function screenPolygon(p) {return state.view==='perspective'?projectPolygon(stage(),corners(p,true)):corners(p,true).map(toScreen);}
function draw() {
  const s=stage();if(!s)return;
  ctx.setTransform(3,0,0,3,0,0);ctx.clearRect(0,0,320,240);ctx.fillStyle='#101a1c';ctx.fillRect(0,0,320,240);
  const bg=state.backgrounds.get(key(s));const present=bg && !(bg instanceof Promise);
  if(state.view==='perspective' && present) {ctx.imageSmoothingEnabled=false;ctx.drawImage(bg.image,0,0,320,240);}
  $('#missing-image').hidden=state.view==='top' || Boolean(present);
  if(state.view==='top') {
    const grid=Math.max(50,Math.pow(10,Math.floor(Math.log10(40/state.top.scale))));
    const a=toWorld({x:0,y:240}),b=toWorld({x:320,y:0});ctx.lineWidth=.3;ctx.strokeStyle='#33443e';ctx.beginPath();
    for(let x=Math.ceil(a.x/grid)*grid;x<=b.x;x+=grid){const p=toScreen({x,y:0,z:0});ctx.moveTo(p.x,0);ctx.lineTo(p.x,240);}
    for(let z=Math.ceil(a.z/grid)*grid;z<=b.z;z+=grid){const p=toScreen({x:0,y:0,z});ctx.moveTo(0,p.y);ctx.lineTo(320,p.y);}ctx.stroke();
    ctx.fillStyle='#8da69a';ctx.font='5px system-ui';ctx.fillText(`Griglia ${grid} unità · Z ↑`,6,10);
  }
  for(const kind of ['planes','zones']) {
    if(!state.layers[kind])continue;
    s[kind].forEach((p,index)=>{
      const poly=screenPolygon(p);if(!poly)return;
      const isSelected=state.selection?.kind===kind && state.selection.index===index;
      paintPolygon(poly,colors[kind],isSelected);
      const center=toScreen({x:p.x+p.w/2,y:p.y,z:p.z+p.d/2});
      if(center && center.x>-30 && center.x<350 && center.y>-20 && center.y<260) label(`${kind==='planes'?'P':'Z'}${index}${kind==='zones'?' → '+p.stage_id:''}`,center,colors[kind]);
      if(isSelected) {
        ctx.fillStyle='#e7fff4';ctx.strokeStyle='#102c22';ctx.lineWidth=.8;
        corners(p).map(toScreen).filter(Boolean).forEach(pt=>{ctx.fillRect(pt.x-2,pt.y-2,4,4);ctx.strokeRect(pt.x-2,pt.y-2,4,4);});
      }
    });
  }
  if(state.layers.spawns) s.spawns.forEach((sp,index)=>{
    const p=toScreen(sp);if(!p)return;
    ctx.beginPath();ctx.arc(p.x,p.y,state.selection?.kind==='spawns' && state.selection.index===index?3.2:2.5,0,Math.PI*2);ctx.fillStyle=colors.spawns;ctx.fill();ctx.strokeStyle='#18272a';ctx.lineWidth=.7;ctx.stroke();
    const heading=sp.ry*Math.PI/2048,end=toScreen({x:sp.x+Math.sin(heading)*60,y:sp.y,z:sp.z+Math.cos(heading)*60});
    if(end){ctx.beginPath();ctx.moveTo(p.x,p.y);ctx.lineTo(end.x,end.y);ctx.strokeStyle=colors.spawns;ctx.stroke();}
    label(`I${index}`,{x:p.x,y:p.y-7},colors.spawns);
  });
  for(const group of s.npcs||[])for(const npc of Object.values(group)){
    const p=toScreen(npc);if(!p)continue;ctx.fillStyle='#ecd389';ctx.fillRect(p.x-1.5,p.y-1.5,3,3);label('NPC',{x:p.x,y:p.y+8},'#ecd389');
  }
  if(state.drag?.type==='create' && state.drag.preview) paintPolygon(screenPolygon(state.drag.preview),colors[state.tool],true);
}
function paintPolygon(poly,color,selected=false) {
  if(!poly)return;ctx.beginPath();poly.forEach((p,i)=>i?ctx.lineTo(p.x,p.y):ctx.moveTo(p.x,p.y));ctx.closePath();ctx.fillStyle=color+(selected?'55':'2d');ctx.fill();ctx.strokeStyle=selected?'#edfff4':color;ctx.lineWidth=selected?1.1:.7;ctx.stroke();
}
function label(text,p,color) {
  ctx.font='bold 5px system-ui';const width=ctx.measureText(text).width;
  ctx.fillStyle='#091c1ed9';ctx.fillRect(p.x-width/2-2,p.y-4.5,width+4,7);ctx.fillStyle=color;ctx.fillText(text,p.x-width/2,p.y+.5);
}
function renderStages() {
  const term=$('#search').value.toLowerCase();$('#stage-count').textContent=state.stages.length;
  $('#stages').replaceChildren();
  state.stages.forEach((s,i)=>{
    if(!(`${i} ${s.name}`.toLowerCase().includes(term)))return;
    const button=document.createElement('button');button.className='stage-item'+(i===state.sid?' active':'');button.dataset.stage=i;
    const bg=state.backgrounds.get(key(s));let src='';
    if(bg && !(bg instanceof Promise)) {if(bg.image instanceof HTMLCanvasElement)src=bg.image.toDataURL();else src=bg.image.src;}
    button.innerHTML=`${src?`<img class="thumbnail" src="${esc(src)}" alt="">`:'<span class="thumbnail"></span>'}<span><span class="name">${esc(s.name.replace(/^\d+_/,'').replaceAll('_',' '))}</span><small>${String(i).padStart(2,'0')} · ${s.planes.length} piattaforme · ${s.zones.length} zone</small></span>`;
    button.addEventListener('click',()=>{if(state.drag)cancelDrag();state.sid=i;state.selection=null;fitTop();render();});$('#stages').append(button);
  });
}
function field(label,key,value,scope,options='') {
  return `<label${options==='wide'?' class="wide"':''}>${label}<input data-scope="${scope}" data-key="${key}" value="${esc(value)}" ${typeof value==='number'?'type="number" step="1"':'type="text"'}></label>`;
}
function renderInspector() {
  const s=stage(),p=selected();
  if(!p) $('#inspector').innerHTML='<div class="empty-selection"><span>↖</span>Seleziona una forma sullo sfondo<br>per vedere le sue proprietà.</div>';
  else {
    const {kind,index}=state.selection;
    let fields=['x','y','z'].map(k=>field(k.toUpperCase(),k,p[k],'object')).join('');
    if(kind==='spawns') fields+=['rx','ry','rz'].map(k=>field(k.toUpperCase(),k,p[k],'object')).join('');
    else fields+=field('Larghezza · W','w',p.w,'object')+field('Profondità · D','d',p.d,'object')+field('Altezza · H','h',p.h,'object');
    if(kind==='zones') {
      fields+=`<label class="wide">Schermata di destinazione<select data-scope="object" data-key="stage_id">${state.stages.map((area,i)=>`<option value="${i}"${i===p.stage_id?' selected':''}>${i} · ${esc(area.name)}</option>`).join('')}</select></label>`;
      fields+=`<label class="wide">Ingresso di destinazione<select data-scope="object" data-key="spawn_id">${(state.stages[p.stage_id]?.spawns||[]).map((sp,i)=>`<option value="${i}"${i===p.spawn_id?' selected':''}>${i} · X ${sp.x}, Z ${sp.z}</option>`).join('')}</select></label>`;
    }
    $('#inspector').innerHTML=`<div class="selection-title"><span class="swatch ${kind==='planes'?'green':kind==='zones'?'red':'blue'}"></span>${titles[kind]} ${index}</div><div class="field-grid">${fields}</div>${kind!=='spawns'?'<p class="muted">W positivo · D negativo. I lati restano allineati agli assi X/Z.</p>':''}<div class="object-actions"><button id="duplicate"${s[kind].length>=LIMITS[kind]?' disabled':''}>Duplica</button><button id="delete" class="danger">Elimina</button></div>`;
    $('#duplicate').onclick=()=>mutate(()=>{const copy=clone(selected());copy.x+=Number($('#snap').value)*2;copy.z+=Number($('#snap').value)*2;s[kind].push(copy);state.selection.index=s[kind].length-1;});
    $('#delete').onclick=deleteObject;
  }
  $('#stage-settings').innerHTML=`<div class="field-grid">${field('Nome','name',s.name,'stage','wide')}${field('Texture sinistra','tim_0',s.tim_0,'stage')}${field('Texture destra','tim_1',s.tim_1,'stage')}</div>`;
  $('#camera-fields').innerHTML=`<div class="field-grid">${['cam_x','cam_y','cam_z','cam_rx','cam_ry','cam_rz'].map(k=>field(k.slice(4).toUpperCase(),k,s[k],'stage')).join('')}</div>`;
  const bg=state.backgrounds.get(key(s));$('#asset-export').hidden=!bg?.imported;
  if(bg?.imported) {
    if(window.offlineEditor) { $('#asset-export').href='#'; $('#asset-export').onclick=e=>{e.preventDefault();window.offlineEditor.exportAssets(bg.imported.id);}; }
    else $('#asset-export').href=`/api/export/${bg.imported.id}`;
  }
  $('#object-list').innerHTML=['planes','zones','spawns'].map(kind=>`<div class="section-label">${{planes:'PIATTAFORME',zones:'ZONE',spawns:'INGRESSI'}[kind]} · ${s[kind].length}/${LIMITS[kind]}</div>`+s[kind].map((p,i)=>`<button class="object-row${state.selection?.kind===kind&&state.selection.index===i?' active':''}" data-kind="${kind}" data-index="${i}"><span><span class="swatch ${kind==='planes'?'green':kind==='zones'?'red':'blue'}"></span>${titles[kind]} ${i}</span><small>${kind==='zones'?`→ ${p.stage_id} / ${p.spawn_id}`:`${p.x}, ${p.z}`}</small></button>`).join('')).join('');
}
function renderMessages() {
  const checks=validate(state.stages),localWarnings=checks.warnings.filter(w=>w.startsWith(`Area ${state.sid}:`));
  const bg=state.backgrounds.get(key(stage()));
  if(bg?.imported)localWarnings.push('Sfondo importato: scarica il pacchetto PNG/TIM e aggiungi le voci CTI prima di ricostruire la ISO.');
  $('#messages').replaceChildren();
  for(const message of [...checks.errors,...localWarnings]){const row=document.createElement('div');row.textContent=message;$('#messages').append(row);}
}
function render() {
  if(!stage())return;$('#scene-name').textContent=stage().name.replace(/^\d+_/,'').replaceAll('_',' ');$('#scene-id').textContent=`AREA ${String(state.sid).padStart(2,'0')}`;
  $('#undo').disabled=!state.history.length;$('#redo').disabled=!state.future.length;
  renderStages();renderInspector();renderMessages();draw();
}
function deleteObject() {
  if(!selected())return;
  const {kind,index}=state.selection;
  mutate(()=>{
    if(kind==='spawns') {
      if(stage().spawns.length===1)throw new Error('Ogni area deve avere almeno un ingresso.');
      if(state.stages.some(s=>s.zones.some(z=>z.stage_id===state.sid&&z.spawn_id===index)))throw new Error('Questo ingresso è usato da una zona: cambia prima la sua destinazione.');
      state.stages.forEach(s=>s.zones.forEach(z=>{if(z.stage_id===state.sid && z.spawn_id>index)z.spawn_id--;}));
    }
    stage()[kind].splice(index,1);state.selection=null;
  });
}
function chooseTool(tool) {
  state.tool=tool;document.querySelectorAll('[data-tool]').forEach(b=>b.classList.toggle('active',b.dataset.tool===tool));
  $('#tool-hint').textContent=tool==='select'?'Trascina una forma o i suoi angoli.':tool==='spawns'?'Fai clic sul terreno per creare un ingresso.':'Trascina due angoli sul terreno.';
  canvas.style.cursor=tool==='select'?'default':'crosshair';
}
function pointer(event) {const r=canvas.getBoundingClientRect();return {x:(event.clientX-r.left)*320/r.width,y:(event.clientY-r.top)*240/r.height};}
function pick(screen) {
  const p=selected();
  if(p && state.selection.kind!=='spawns' && state.layers[state.selection.kind]) {
    const poly=corners(p).map(toScreen);
    const index=poly.findIndex(pt=>pt && Math.hypot(pt.x-screen.x,pt.y-screen.y)<5);
    if(index>=0)return {selection:state.selection,corner:index};
  }
  if(state.layers.spawns)for(let i=stage().spawns.length-1;i>=0;i--){const pt=toScreen(stage().spawns[i]);if(pt && Math.hypot(pt.x-screen.x,pt.y-screen.y)<5)return {selection:{kind:'spawns',index:i}};}
  for(const kind of ['zones','planes'])if(state.layers[kind])for(let i=stage()[kind].length-1;i>=0;i--){const poly=screenPolygon(stage()[kind][i]);if(poly && inside(screen,poly))return {selection:{kind,index:i}};}
  return null;
}
canvas.addEventListener('pointerdown',event=>{
  if(!stage() || event.button!==0)return;canvas.focus();const screen=pointer(event);
  if(state.tool==='select') {
    const hit=pick(screen);state.selection=hit?{...hit.selection}:null;renderInspector();draw();if(!hit)return;
    const p=selected(),world=toWorld(screen,p.y);if(!world){report('Punto fuori dal piano visibile della camera.');return;}
    state.drag={type:hit.corner===undefined?'move':'resize',corner:hit.corner,start:world,original:clone(p),before:clone(state.stages)};
  } else {
    const kind=state.tool,world=toWorld(screen,0);if(!world){report('Qui il raggio non incontra il terreno. Usa la vista dall’alto.');return;}
    if(stage()[kind].length>=LIMITS[kind]){report(`Limite raggiunto: ${LIMITS[kind]} ${kind==='planes'?'piattaforme':kind==='zones'?'zone':'ingressi'} per area.`);return;}
    if(kind==='spawns') {mutate(()=>{stage().spawns.push({x:round(world.x),y:0,z:round(world.z),rx:0,ry:0,rz:0});state.selection={kind,index:stage().spawns.length-1};});chooseTool('select');return;}
    state.drag={type:'create',start:world,screenStart:screen,preview:null,before:clone(state.stages)};
  }
  canvas.setPointerCapture(event.pointerId);
});
canvas.addEventListener('pointermove',event=>{
  const screen=pointer(event),p=selected(),world=toWorld(screen,state.drag?.original?.y??p?.y??0);
  $('#coordinates').textContent=world?`X ${Math.round(world.x)} · Z ${Math.round(world.z)}`:'Nessuna intersezione col terreno';
  const drag=state.drag;if(!drag || !world)return;
  if(drag.type==='create')drag.preview=rectangle(drag.start,world,0,Number($('#snap').value));
  else if(drag.type==='move'){p.x=drag.original.x+round(world.x-drag.start.x);p.z=drag.original.z+round(world.z-drag.start.z);}
  else {
    const opposite=corners(drag.original)[(drag.corner+2)%4],r=rectangle(opposite,world,drag.original.y,Number($('#snap').value));
    Object.assign(p,{x:r.x,z:r.z,w:r.w,d:r.d});
  }
  draw();
});
canvas.addEventListener('pointerup',event=>{
  const drag=state.drag;if(!drag)return;state.drag=null;
  if(drag.type==='create') {
    if(!drag.preview || Math.hypot(pointer(event).x-drag.screenStart.x,pointer(event).y-drag.screenStart.y)<2){draw();return;}
    const kind=state.tool,p=drag.preview;
    if(kind==='zones')Object.assign(p,{stage_id:state.sid?state.sid-1:Math.min(1,state.stages.length-1),spawn_id:0});
    stage()[kind].push(p);state.selection={kind,index:stage()[kind].length-1};chooseTool('select');
  }
  changed(drag.before);
});
function cancelDrag() {if(state.drag){state.stages=state.drag.before;state.drag=null;render();}}
canvas.addEventListener('pointercancel',cancelDrag);
$('.inspector-sidebar').addEventListener('change',async event=>{
  const input=event.target;if(!input.dataset.scope)return;
  const target=input.dataset.scope==='object'?selected():stage();if(!target)return;
  const k=input.dataset.key;let value=['name','tim_0','tim_1'].includes(k)?input.value:Number(input.value);
  if(!['name','tim_0','tim_1'].includes(k) && (input.value==='' || !Number.isInteger(value))){input.value=target[k];return;}
  if(k.startsWith('tim_'))value=value.trim().toUpperCase();
  mutate(()=>{target[k]=value;if(k==='stage_id')target.spawn_id=0;});
  if(k.startsWith('tim_')) {await background(stage());render();}
});
$('#object-list').addEventListener('click',event=>{const row=event.target.closest('[data-kind]');if(row){state.selection={kind:row.dataset.kind,index:Number(row.dataset.index)};renderInspector();draw();}});
function undo(redo=false) {
  cancelDrag();const from=redo?state.future:state.history,to=redo?state.history:state.future;if(!from.length)return;
  to.push(clone(state.stages));state.stages=from.pop();state.sid=Math.min(state.sid,state.stages.length-1);state.selection=null;state.version++;render();status('Modifiche da salvare');
}
$('#undo').onclick=()=>undo();$('#redo').onclick=()=>undo(true);$('#save').onclick=()=>save();
$('#search').oninput=renderStages;
$('#snap').onchange=draw;
for(const b of document.querySelectorAll('[data-tool]'))b.onclick=()=>{cancelDrag();chooseTool(b.dataset.tool);};
for(const b of document.querySelectorAll('[data-view]'))b.onclick=()=>{cancelDrag();state.view=b.dataset.view;document.querySelectorAll('[data-view]').forEach(button=>button.classList.toggle('active',button===b));draw();};
for(const check of document.querySelectorAll('[data-layer]'))check.onchange=()=>{state.layers[check.dataset.layer]=check.checked;draw();};
$('#fit').onclick=()=>{fitTop();fitZoom();};$('#zoom-in').onclick=()=>{state.zoom=Math.min(6,state.zoom+.25);resizeCanvas();};$('#zoom-out').onclick=()=>{state.zoom=Math.max(.5,state.zoom-.25);resizeCanvas();};
$('#reload').onclick=async()=>{
  if(state.saving){report('Attendi la fine del salvataggio.');return;}
  if(state.version!==state.savedVersion && !confirm('Rileggere stages.json e scartare le modifiche non salvate? Puoi prima scaricarne una copia.'))return;
  try{await load();}catch(e){report(e);}
};
function download(name,data,type) {const link=document.createElement('a'),url=URL.createObjectURL(new Blob([data],{type}));link.href=url;link.download=name;link.click();setTimeout(()=>URL.revokeObjectURL(url),5000);}
$('#export-json').onclick=()=>download('stages.json',serialize(state.stages),'application/json');
function nextNames() {
  const used=new Set(state.stages.flatMap(s=>[s.tim_0,s.tim_1]));
  for(let n=state.stages.length;n<10000;n++) {
    const names=n<10?[`BG${n}_0.TIM`,`BG${n}_1.TIM`]:n<100?[`B${n}_0.TIM`,`B${n}_1.TIM`]:[`N${n}A.TIM`,`N${n}B.TIM`];
    if(names.every(name=>name.length<=9&&!used.has(name)))return names;
  }
  throw new Error('Nomi TIM disponibili esauriti.');
}
$('#new-stage').onclick=()=>{
  if(!stage()){report('Apri prima stages.json.');return;}
  if(state.saving || state.drag){report('Attendi il completamento dell’operazione corrente.');return;}
  const [a,b]=nextNames();$('#new-form').reset();$('#new-name').value=`${state.stages.length}_nuova_area`;$('#new-tim0').value=a;$('#new-tim1').value=b;$('#dialog-error').textContent='';$('#new-dialog').showModal();
};
$('#cancel-new').onclick=()=>$('#new-dialog').close();
const base64 = data => {
  let text='';for(let i=0;i<data.length;i+=8192)text+=String.fromCharCode(...data.subarray(i,i+8192));return btoa(text);
};
async function prepareBackground(file,tim0,tim1,name) {
  if(!file || file.type!=='image/png')throw new Error('Scegli uno sfondo PNG.');
  const bitmap=await createImageBitmap(file);
  const full=document.createElement('canvas');full.width=320;full.height=240;
  const c=full.getContext('2d');c.fillStyle='#000';c.fillRect(0,0,320,240);c.drawImage(bitmap,0,0,320,240);bitmap.close();
  const padded=document.createElement('canvas');padded.width=320;padded.height=256;
  const pc=padded.getContext('2d');pc.fillStyle='#000';pc.fillRect(0,0,320,256);pc.drawImage(full,0,0);
  const parts=[{width:256,start:0,x:320},{width:64,start:255,x:576}].map(spec=>{
    const surface=document.createElement('canvas');surface.width=spec.width;surface.height=256;
    const cx=surface.getContext('2d');cx.drawImage(padded,spec.start,0,spec.width,256,0,0,spec.width,256);
    const pixels=cx.getImageData(0,0,spec.width,256).data;
    return {png:surface.toDataURL('image/png').split(',')[1],tim:base64(encodeTim(pixels,spec.width,256,spec.x,0))};
  });
  const item=await api('/api/backgrounds',{name,tim0,tim1,image:full.toDataURL('image/png').split(',')[1],parts});
  state.catalog.imported.push(item);state.backgrounds.delete(`${tim0}|${tim1}`);return item;
}
$('#new-form').onsubmit=async event=>{
  event.preventDefault();const name=$('#new-name').value.trim(),a=$('#new-tim0').value.trim().toUpperCase(),b=$('#new-tim1').value.trim().toUpperCase();
  if(!/^[A-Z0-9_]{1,5}\.TIM$/.test(a)||!/^[A-Z0-9_]{1,5}\.TIM$/.test(b)||a===b){$('#dialog-error').textContent='Due nomi TIM distinti, maiuscoli, massimo 9 caratteri (BG9_0.TIM).';return;}
  if(state.stages.some(s=>[s.tim_0,s.tim_1].some(t=>t===a||t===b))){$('#dialog-error').textContent='Questi nomi TIM sono già usati. Scegli nomi nuovi.';return;}
  $('#create-stage').disabled=true;
  try {
    await prepareBackground($('#new-image').files[0],a,b,name);
    const camera=Object.fromEntries(['cam_x','cam_y','cam_z','cam_rx','cam_ry','cam_rz'].map(k=>[k,stage()[k]]));
    const spawn=clone(stage().spawns[0]);
    mutate(()=>{state.stages.push({name,tim_0:a,tim_1:b,...camera,planes:[],zones:[],spawns:[spawn],npcs:[]});state.sid=state.stages.length-1;state.selection=null;});
    await background(stage());fitTop();render();$('#new-dialog').close();
  }catch(error){$('#dialog-error').textContent=error.message;}finally{$('#create-stage').disabled=false;}
};
$('#replace-background').onclick=()=>{if(stage())$('#replace-file').click();else report('Apri prima stages.json.');};
$('#replace-file').onchange=async()=>{
  const file=$('#replace-file').files[0];if(!file)return;
  const target=stage();$('#replace-background').disabled=true;
  try {
    await prepareBackground(file,target.tim_0,target.tim_1,target.name);await background(target);render();status(window.offlineEditor ? 'Sfondo associato · scarica PNG e TIM prima di chiudere' : 'Sfondo associato · asset pronti nella cartella editor');
  } catch(error){report(error);}finally{$('#replace-background').disabled=false;$('#replace-file').value='';}
};
window.addEventListener('keydown',event=>{
  if($('#new-dialog').open || ['INPUT','SELECT','TEXTAREA'].includes(document.activeElement.tagName))return;
  const command=event.ctrlKey||event.metaKey;
  if(command&&event.key.toLowerCase()==='z'){event.preventDefault();undo(event.shiftKey);}
  else if(command&&event.key.toLowerCase()==='s'){event.preventDefault();save();}
  else if(event.key==='Delete'||event.key==='Backspace'){event.preventDefault();deleteObject();}
  else if(event.key==='Escape'){cancelDrag();state.selection=null;chooseTool('select');renderInspector();draw();}
  else if(!command && {v:'select',p:'planes',z:'zones',i:'spawns'}[event.key.toLowerCase()])chooseTool({v:'select',p:'planes',z:'zones',i:'spawns'}[event.key.toLowerCase()]);
});
window.addEventListener('beforeunload',event=>{if(state.version!==state.savedVersion || state.drag || window.offlineEditor?.pendingAssets()){event.preventDefault();event.returnValue='';}});
new ResizeObserver(()=>{if(state.stages.length)fitZoom();}).observe($('#viewport'));
if(window.offlineEditor) {
  $('#reload').onclick=()=>$('#open-json').click();
  $('#export-json').onclick=()=>$('#open-backgrounds').click();
  $('#open-json').onchange=async event=>{
    const file=event.target.files[0];if(!file)return;
    try {
      const stages=JSON.parse(await file.text()), checks=validate(stages);
      if(checks.errors.length)throw new Error(checks.errors.join(' · '));
      if(state.version!==state.savedVersion && !confirm('Aprire un altro JSON e scartare le modifiche non scaricate?'))return;
      window.offlineEditor.stages=stages;await load();
    }catch(error){report(error);}finally{event.target.value='';}
  };
  $('#open-backgrounds').onchange=async event=>{
    try {
      await window.offlineEditor.addImages(event.target.files);
      state.catalog=window.offlineEditor.catalog;state.backgrounds.clear();
      await Promise.all(state.stages.map(background));render();status('Sfondi PNG caricati');
    }catch(error){report(error);}finally{event.target.value='';}
  };
  $('#save').disabled=true;$('#new-stage').disabled=true;$('#replace-background').disabled=true;
  const originalLoad=load;
  load=async()=>{await originalLoad();$('#save').disabled=false;$('#new-stage').disabled=false;$('#replace-background').disabled=false;};
  $('#scene-name').textContent='Apri stages.json';
  $('#missing-image').hidden=false;$('#missing-image').textContent='Apri stages.json, poi carica gli sfondi PNG da data/stages.';
} else {
  $('#reload').textContent='Rileggi stages.json';$('#export-json').textContent='Scarica una copia JSON';
  $('#save-state').textContent='Connessione…';
  $('.dialog-note').textContent='Camera iniziale copiata dall’area corrente. Le risorse importate sono conservate nella cartella psxstudio/stage_editor/assets.';
  load().catch(report);
}
