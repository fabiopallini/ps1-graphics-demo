(() => {
  'use strict';
  const C=window.PSX3DStudioCore,$=s=>document.querySelector(s);
  const canvas=$('#scene'),ctx=canvas.getContext('2d'),uvCanvas=$('#uv-canvas'),uvCtx=uvCanvas.getContext('2d');
  const state={model:C.primitive('cube'),mode:'model',tool:'select',selection:new Set(),history:[],future:[],version:0,saved:0,
    texture:null,textureName:'',textureData:'',view:'perspective',camera:{yaw:.65,pitch:.35,center:[0,0,0],distance:4,span:3},drag:null,uvDrag:null,width:800,height:600,points:[],faceTriangles:[]};
  state.model.name='MODEL';
  const esc=t=>String(t).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  function message(text,error=false){$('#message').textContent=text;$('#message').classList.toggle('error',error);}
  function snapshot(){return C.copy(state.model);}
  function record(before){
    if(JSON.stringify(before)===JSON.stringify(state.model)){render();return;}
    state.history.push(before);if(state.history.length>60)state.history.shift();state.future=[];state.version++;render();
  }
  function edit(action,text='Modello modificato.'){
    if(state.drag||state.uvDrag){message('Completa prima il trascinamento.',true);return;}
    const before=snapshot();
    try{action();C.validate(state.model);record(before);message(text);}catch(e){state.model=before;state.selection.clear();render();message(e.message,true);}
  }
  function download(name,data,type){const url=URL.createObjectURL(new Blob([data],{type})),a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),10000);}
  function numbers(ids){const values=ids.map(id=>Number($(id).value));if(values.some(v=>!Number.isFinite(v)))throw new Error('Inserisci valori numerici validi.');return values;}
  const selectedIds=()=>C.selectedVertices(state.model,state.mode,state.selection);
  const selectedFaces=()=>state.mode==='model'?state.model.faces.map((_,i)=>i):state.mode==='faces'?[...state.selection]:[];
  const selectedFace=()=>state.mode==='faces'&&state.selection.size===1?state.model.faces[[...state.selection][0]]:null;
  function fit(){const b=C.bounds(state.model);state.camera.center=b.center;state.camera.distance=b.radius*3.6;state.camera.span=b.radius*2.8;draw();}
  function basis(){
    const {yaw:y,pitch:p}=state.camera,sign=$('#y-down').checked?-1:1;
    const adjust=v=>[v[0],v[1]*sign,v[2]];
    return {right:adjust([Math.cos(y),0,-Math.sin(y)]),up:adjust([-Math.sin(p)*Math.sin(y),Math.cos(p),-Math.sin(p)*Math.cos(y)]),forward:adjust([Math.sin(y)*Math.cos(p),Math.sin(p),Math.cos(p)*Math.cos(y)])};
  }
  function project(point){
    const b=basis(),d=C.sub(point,state.camera.center),depth=state.camera.distance-C.dot(d,b.forward);
    const scale=state.view==='perspective'?Math.min(state.width,state.height)*1.1/depth:Math.min(state.width,state.height)/state.camera.span;
    if(state.view==='perspective'&&depth<.001)return null;
    return {x:state.width/2+C.dot(d,b.right)*scale,y:state.height/2-C.dot(d,b.up)*scale,depth,scale};
  }
  function drawLine(a,b,color,width=1){const p=project(a),q=project(b);if(!p||!q)return;ctx.beginPath();ctx.moveTo(p.x,p.y);ctx.lineTo(q.x,q.y);ctx.strokeStyle=color;ctx.lineWidth=width;ctx.stroke();}
  function triangleTexture(points,uvs){
    const im=state.texture;if(!im)return;
    const a=uvs.map(p=>[p[0]*(im.width-1),(1-p[1])*(im.height-1)]),[p,q,r]=a;
    const determinant=(q[0]-p[0])*(r[1]-p[1])-(r[0]-p[0])*(q[1]-p[1]);if(Math.abs(determinant)<1e-7)return;
    const coefficients=axis=>{
      const d1=points[1][axis]-points[0][axis],d2=points[2][axis]-points[0][axis];
      const u=(d1*(r[1]-p[1])-d2*(q[1]-p[1]))/determinant,v=((q[0]-p[0])*d2-(r[0]-p[0])*d1)/determinant;
      return [u,v,points[0][axis]-u*p[0]-v*p[1]];
    };
    const x=coefficients('x'),y=coefficients('y');ctx.save();ctx.beginPath();points.forEach((p,i)=>i?ctx.lineTo(p.x,p.y):ctx.moveTo(p.x,p.y));ctx.closePath();ctx.clip();
    ctx.transform(x[0],y[0],x[1],y[1],x[2],y[2]);ctx.imageSmoothingEnabled=false;ctx.drawImage(im,0,0);ctx.restore();
  }
  function draw(){
    const w=state.width,h=state.height,ratio=window.devicePixelRatio||1;ctx.setTransform(ratio,0,0,ratio,0,0);ctx.clearRect(0,0,w,h);ctx.fillStyle='#505050';ctx.fillRect(0,0,w,h);
    const model=state.model,wire=$('#shading').value==='wire',textured=$('#shading').value==='texture'&&state.texture;
    if($('#grid').checked){
      const radius=Math.max(.5,state.camera.span),step=10**Math.floor(Math.log10(radius/4)),limit=Math.ceil(radius/step)*step;
      const center=state.camera.center;
      const axes=state.view==='front'?[0,1]:state.view==='side'?[2,1]:[0,2];
      for(let v=-limit;v<=limit+step*.1;v+=step){
        const a=[...center],b=[...center],c=[...center],d=[...center];
        const perpendicular=[0,1,2].find(axis=>!axes.includes(axis));a[perpendicular]=b[perpendicular]=c[perpendicular]=d[perpendicular]=0;
        a[axes[0]]=center[axes[0]]+v;b[axes[0]]=a[axes[0]];a[axes[1]]-=limit;b[axes[1]]+=limit;
        c[axes[1]]=center[axes[1]]+v;d[axes[1]]=c[axes[1]];c[axes[0]]-=limit;d[axes[0]]+=limit;
        drawLine(a,b,'#666');drawLine(c,d,'#666');
      }
      const size=C.bounds(model).radius*.6;
      [[1,0,0],[0,1,0],[0,0,1]].forEach((v,i)=>{drawLine([0,0,0],C.mul(v,size),['#ff7070','#72df72','#7bafff'][i],2);const p=project(C.mul(v,size));if(p){ctx.fillStyle=['#ff7070','#72df72','#7bafff'][i];ctx.font='bold 11px Tahoma, sans-serif';ctx.fillText('XYZ'[i],p.x+4,p.y-4);}});
    }
    state.points=model.vertices.map(project);const faces=[];state.faceTriangles=[];
    model.faces.forEach((face,index)=>{
      let triangles;try{triangles=C.triangulate(model,face);}catch(_){triangles=face.v.length===4?[[0,1,2],[0,2,3]]:[[0,1,2]];}
      const projected=triangles.map(tri=>({points:tri.map(k=>state.points[face.v[k]]),uv:tri.map(k=>model.uvs[face.uv[k]])})).filter(t=>t.points.every(Boolean));
      state.faceTriangles[index]=projected;
      if(!projected.length)return;
      const depths=projected.flatMap(t=>t.points.map(p=>p.depth));faces.push({face,index,triangles:projected,depth:depths.reduce((a,b)=>a+b,0)/depths.length});
    });
    faces.sort((a,b)=>b.depth-a.depth);
    for(const {face,index,triangles} of faces){
      const selected=state.mode==='faces'&&state.selection.has(index),n=C.normal(model,face),shade=.35+.65*Math.abs(C.dot(n,C.unit([.3,-.8,.6]))),v=Math.round(shade*195);
      if(!wire)for(const tri of triangles){
        ctx.beginPath();tri.points.forEach((p,i)=>i?ctx.lineTo(p.x,p.y):ctx.moveTo(p.x,p.y));ctx.closePath();ctx.fillStyle=`rgb(${v},${v},${Math.max(0,v-8)})`;ctx.fill();
        if(textured)triangleTexture(tri.points,tri.uv);
        if(selected){ctx.fillStyle='rgba(32,100,255,.35)';ctx.fill();}
      }
      if(wire||$('#edges').checked||selected){
        const points=face.v.map(i=>state.points[i]);if(points.every(Boolean)){ctx.beginPath();points.forEach((p,i)=>i?ctx.lineTo(p.x,p.y):ctx.moveTo(p.x,p.y));ctx.closePath();ctx.strokeStyle=selected?'#ffff00':wire?'#ddd':'#242424';ctx.lineWidth=selected?2:1;ctx.stroke();}
      }
    }
    if(state.mode==='vertices')state.points.forEach((p,i)=>{
      if(!p)return;const selected=state.selection.has(i);ctx.fillStyle=selected?'#ffff00':'#fff';ctx.strokeStyle='#000';const size=selected?6:4;ctx.fillRect(p.x-size/2,p.y-size/2,size,size);ctx.strokeRect(p.x-size/2,p.y-size/2,size,size);
    });
    if(state.drag?.type==='box'){
      const a=state.drag.start,b=state.drag.last;ctx.fillStyle='#00008022';ctx.strokeStyle='#fff';ctx.setLineDash([3,3]);ctx.fillRect(a.x,a.y,b.x-a.x,b.y-a.y);ctx.strokeRect(a.x,a.y,b.x-a.x,b.y-a.y);ctx.setLineDash([]);
    }
    $('#view-label').textContent=({perspective:'Prospettiva',front:'Frontale · X/Y',side:'Laterale · Z/Y',top:'Dall’alto · X/Z'}[state.view]);
  }
  function drawUV(){
    const w=256;uvCtx.clearRect(0,0,w,w);
    for(let y=0;y<w;y+=16)for(let x=0;x<w;x+=16){uvCtx.fillStyle=(x/16+y/16)%2?'#a0a0a0':'#d0d0d0';uvCtx.fillRect(x,y,16,16);}
    if(state.texture){uvCtx.imageSmoothingEnabled=false;uvCtx.drawImage(state.texture,0,0,w,w);}
    const face=selectedFace();if(!face)return;
    const points=face.uv.map(i=>state.model.uvs[i]).map(([u,v])=>[u*255,(1-v)*255]);
    uvCtx.beginPath();points.forEach(([x,y],i)=>i?uvCtx.lineTo(x,y):uvCtx.moveTo(x,y));uvCtx.closePath();uvCtx.strokeStyle='#ffff00';uvCtx.lineWidth=2;uvCtx.stroke();
    points.forEach(([x,y],i)=>{uvCtx.fillStyle='#ffff00';uvCtx.fillRect(x-3,y-3,6,6);uvCtx.strokeStyle='#000';uvCtx.strokeRect(x-3,y-3,6,6);uvCtx.font='bold 12px Tahoma, sans-serif';uvCtx.strokeText(String(i+1),x+6,y-6);uvCtx.fillText(String(i+1),x+6,y-6);});
  }
  function inspect(){
    const ids=selectedIds(),face=selectedFace(),center=C.centroid(state.model,ids),single=state.mode==='vertices'&&state.selection.size===1;
    $('#selection-title').textContent=state.mode==='model'?'Modello intero':state.mode==='vertices'?'Vertici selezionati':'Facce selezionate';
    $('#selection-info').textContent=`${ids.length} vertici${state.mode==='faces'?' · '+state.selection.size+' facce':''}${ids.length?' · Centro '+center.map(v=>C.format(v)).join(', '):''}`;
    $('#vertex-fields').innerHTML=single?'<div class="triplet labeled">'+state.model.vertices[ids[0]].map((v,i)=>`<label>${'XYZ'[i]}<input data-coordinate="${i}" type="number" step="0.01" value="${C.format(v)}"></label>`).join('')+'</div>':'';
    $('#face-info').textContent=face?'Vertici (ordine): '+face.v.map(i=>i+1).join(' → '):'';
    $('#uv-fields').innerHTML=face?'<table><thead><tr><th>Angolo</th><th>U</th><th>V</th></tr></thead><tbody>'+face.uv.map((id,k)=>'<tr><td>'+(k+1)+'</td>'+state.model.uvs[id].map((v,a)=>`<td><input data-uv-corner="${k}" data-uv-axis="${a}" type="number" step="0.01" value="${C.format(v)}" aria-label="UV angolo ${k+1} ${'UV'[a]}"></td>`).join('')+'</tr>').join('')+'</tbody></table><p class="note">Trascina i punti sulla texture per regolare le UV.</p>':'<p class="note">Seleziona una sola faccia per modificarne le UV.</p>';
    $('#extrude').disabled=!face;$('#flip-face').disabled=!selectedFaces().length;$('#make-face').disabled=state.mode!=='vertices'||![3,4].includes(state.selection.size);
    $('#merge').disabled=state.mode!=='vertices'||state.selection.size<2;$('#duplicate').disabled=!selectedFaces().length;$('#map-uv').disabled=!selectedFaces().length;
    $('#delete').disabled=state.mode!=='model'&&!state.selection.size;$('#apply-transform').disabled=!ids.length;
    $('#connected').disabled=state.mode!=='vertices'||!state.selection.size;
    $('#export-mtl').disabled=!state.texture;$('#remove-texture').disabled=!state.texture;
    $('#texture-info').textContent=state.texture?`${state.textureName} · ${state.texture.width} × ${state.texture.height}`:'Nessuna texture';
    const m=state.model,warnings=[];
    if(m.faces.length>256)warnings.push('Superato il limite di 256 quad.');
    if(m.vertices.length>2048)warnings.push('Superato il limite di 2048 vertici.');
    if(m.uvs.length>2048)warnings.push('Superato il limite di 2048 UV: prova Pulisci.');
    if(m.vertices.some(p=>p.some(v=>v*100<-32768||v*100>32767)))warnings.push('Coordinate fuori da SVECTOR con scala ×100.');
    if(m.uvs.some(p=>p.some(v=>v<0||v>1)))warnings.push('UV fuori dall’intervallo 0–1.');
    $('#ps1-info').textContent=warnings.length?warnings.join(' '):`${m.faces.length}/256 quad · ${m.vertices.length}/2048 vertici · ${m.uvs.length}/2048 UV`;
    $('#ps1-info').classList.toggle('warning',Boolean(warnings.length));
    drawUV();
  }
  function render(){
    $('#document-title').textContent=state.model.name+(state.version!==state.saved?' *':'');$('#model-name').value=state.model.name;
    $('#counts').textContent=`${state.model.vertices.length} vertici · ${state.model.faces.length} facce · ${state.model.uvs.length} UV`;
    $('#selection-count').textContent=`${selectedIds().length} vertici selezionati`;$('#file-state').textContent=state.version!==state.saved?'Modifiche non salvate':'Pronto';
    $('#undo').disabled=!state.history.length;$('#redo').disabled=!state.future.length;inspect();draw();
  }
  function setMode(mode){cancelDrag();state.mode=mode;state.selection.clear();document.querySelectorAll('[data-mode]').forEach(b=>b.classList.toggle('active',b.dataset.mode===mode));render();}
  function setTool(tool){state.tool=tool;document.querySelectorAll('[data-tool]').forEach(b=>b.classList.toggle('active',b.dataset.tool===tool));canvas.style.cursor=tool==='move'?'move':'default';}
  function setView(view){
    state.view=view;
    if(view==='front'){state.camera.yaw=0;state.camera.pitch=0;}
    if(view==='side'){state.camera.yaw=Math.PI/2;state.camera.pitch=0;}
    if(view==='top'){state.camera.yaw=0;state.camera.pitch=Math.PI/2;}
    if(view==='perspective'){state.camera.yaw=.65;state.camera.pitch=.35;}
    document.querySelectorAll('[data-view]').forEach(b=>b.classList.toggle('active',b.dataset.view===view));draw();
  }
  function selectAll(){state.selection=new Set((state.mode==='faces'?state.model.faces:state.model.vertices).map((_,i)=>i));render();}
  function undo(redo=false){
    cancelDrag();const from=redo?state.future:state.history,to=redo?state.history:state.future;if(!from.length)return;
    to.push(snapshot());state.model=from.pop();state.selection.clear();state.version++;render();message(redo?'Operazione ripetuta.':'Operazione annullata.');
  }
  function deleteSelection(){
    if(state.mode==='model'&&!confirm('Eliminare tutta la mesh?'))return;
    edit(()=>{
      if(state.mode==='model')state.model=C.empty(state.model.name);
      else if(state.mode==='vertices')C.removeVertices(state.model,[...state.selection]);
      else state.model.faces=state.model.faces.filter((_,i)=>!state.selection.has(i));
      state.selection.clear();
    },'Selezione eliminata.');
  }
  function hitTriangle(p,tri){
    const [a,b,c]=tri,den=(b.y-c.y)*(a.x-c.x)+(c.x-b.x)*(a.y-c.y);if(Math.abs(den)<1e-8)return null;
    const u=((b.y-c.y)*(p.x-c.x)+(c.x-b.x)*(p.y-c.y))/den,v=((c.y-a.y)*(p.x-c.x)+(a.x-c.x)*(p.y-c.y))/den,t=1-u-v;
    if(u<0||v<0||t<0)return null;
    return state.view==='perspective'?1/(u/a.depth+v/b.depth+t/c.depth):u*a.depth+v*b.depth+t*c.depth;
  }
  function pick(p){
    if(state.mode==='vertices'){
      let hit=null,best=9,near=Infinity;state.points.forEach((q,i)=>{if(!q)return;const d=Math.hypot(q.x-p.x,q.y-p.y);if(d<9&&(d<best-.2||(Math.abs(d-best)<.2&&q.depth<near))){best=d;near=q.depth;hit=i;}});return hit;
    }
    let hit=null,best=Infinity;state.faceTriangles.forEach((triangles,i)=>{for(const tri of triangles){const depth=hitTriangle(p,tri.points);if(depth!==null&&depth<best){best=depth;hit=i;}}});return hit;
  }
  function pointer(event,target=canvas){const r=target.getBoundingClientRect();return {x:(event.clientX-r.left)*(target===canvas?state.width:256)/r.width,y:(event.clientY-r.top)*(target===canvas?state.height:256)/r.height};}
  function cancelDrag(){
    if(state.drag?.before)state.model=state.drag.before;if(state.uvDrag?.before)state.model=state.uvDrag.before;
    state.drag=null;state.uvDrag=null;render();
  }
  canvas.addEventListener('contextmenu',e=>e.preventDefault());
  canvas.addEventListener('pointerdown',event=>{
    if(event.button>2)return;canvas.focus();const p=pointer(event);
    if(event.button===2||event.altKey){state.drag={type:'orbit',start:p,camera:C.copy(state.camera)};}
    else if(event.button===1){event.preventDefault();state.drag={type:'pan',start:p,camera:C.copy(state.camera)};}
    else {
      const hit=pick(p);
      if(state.mode==='model'){
        if(hit===null){state.drag={type:'box',start:p,last:p,add:event.shiftKey};}
        else if(state.tool==='move')state.drag={type:'move',start:p,before:snapshot(),ids:selectedIds(),center:C.centroid(state.model,selectedIds()),basis:basis()};
      }else if(hit===null){if(!event.shiftKey)state.selection.clear();state.drag={type:'box',start:p,last:p,add:event.shiftKey};}
      else {
        if(event.shiftKey){if(state.selection.has(hit))state.selection.delete(hit);else state.selection.add(hit);}
        else if(!state.selection.has(hit)||state.tool==='select')state.selection=new Set([hit]);
        if(state.tool==='move'&&state.selection.has(hit))state.drag={type:'move',start:p,before:snapshot(),ids:selectedIds(),center:C.centroid(state.model,selectedIds()),basis:basis()};
      }
      render();
    }
    canvas.setPointerCapture(event.pointerId);
  });
  canvas.addEventListener('pointermove',event=>{
    const d=state.drag;if(!d)return;const p=pointer(event),dx=p.x-d.start.x,dy=p.y-d.start.y;
    if(d.type==='orbit'){
      state.view='perspective';document.querySelectorAll('[data-view]').forEach(b=>b.classList.toggle('active',b.dataset.view==='perspective'));
      state.camera.yaw=d.camera.yaw-dx*.009;state.camera.pitch=Math.max(-Math.PI/2+.01,Math.min(Math.PI/2-.01,d.camera.pitch+dy*.009));draw();
    }else if(d.type==='pan'){
      const b=basis(),scale=state.view==='perspective'?Math.min(state.width,state.height)*1.1/state.camera.distance:Math.min(state.width,state.height)/state.camera.span;
      state.camera.center=C.add(d.camera.center,C.add(C.mul(b.right,-dx/scale),C.mul(b.up,dy/scale)));draw();
    }else if(d.type==='box'){d.last=p;draw();}
    else if(d.type==='move'){
      const center=project(d.center);if(!center)return;let delta=C.add(C.mul(d.basis.right,dx/center.scale),C.mul(d.basis.up,-dy/center.scale));
      const constraint=$('#constraint').value;
      if(constraint!=='free'){
        const axis=Number(constraint),vector=[0,0,0];vector[axis]=1;
        const sx=C.dot(vector,d.basis.right)*center.scale,sy=-C.dot(vector,d.basis.up)*center.scale,den=sx*sx+sy*sy;
        if(den<1e-5){message('Asse perpendicolare alla vista: cambia vista.',true);return;}
        delta=C.mul(vector,(dx*sx+dy*sy)/den);
      }
      const snap=Math.max(0,Number($('#snap').value)||0);if(snap)delta=delta.map(v=>Math.round(v/snap)*snap);
      for(const i of d.ids)state.model.vertices[i]=C.add(d.before.vertices[i],delta);draw();
    }
  });
  canvas.addEventListener('pointerup',event=>{
    const d=state.drag;if(!d)return;state.drag=null;
    if(d.type==='move'){record(d.before);message('Selezione spostata.');}
    else if(d.type==='box'&&state.mode!=='model'){
      const a=d.start,b=pointer(event),xmin=Math.min(a.x,b.x),xmax=Math.max(a.x,b.x),ymin=Math.min(a.y,b.y),ymax=Math.max(a.y,b.y);
      if(!d.add)state.selection.clear();
      const inside=p=>p&&p.x>=xmin&&p.x<=xmax&&p.y>=ymin&&p.y<=ymax;
      if(state.mode==='vertices')state.points.forEach((p,i)=>{if(inside(p))state.selection.add(i);});
      else state.model.faces.forEach((f,i)=>{const p=project(C.centroid(state.model,f.v));if(inside(p))state.selection.add(i);});
      render();
    }else draw();
  });
  canvas.addEventListener('pointercancel',cancelDrag);
  canvas.addEventListener('wheel',event=>{event.preventDefault();const factor=Math.exp(event.deltaY*.001);state.camera.distance=Math.max(.01,Math.min(1e7,state.camera.distance*factor));state.camera.span=Math.max(.01,Math.min(1e7,state.camera.span*factor));draw();},{passive:false});
  uvCanvas.addEventListener('pointerdown',event=>{
    if(event.button!==0)return;const face=selectedFace();if(!face)return;
    const p=pointer(event,uvCanvas);let corner=-1,best=12;
    face.uv.forEach((id,i)=>{const [u,v]=state.model.uvs[id],distance=Math.hypot(u*255-p.x,(1-v)*255-p.y);if(distance<best){best=distance;corner=i;}});
    if(corner<0)return;
    const before=snapshot();face.uv=face.uv.map(id=>{state.model.uvs.push([...state.model.uvs[id]]);return state.model.uvs.length-1;});
    state.uvDrag={corner,before};uvCanvas.setPointerCapture(event.pointerId);uvCanvas.focus();
  });
  uvCanvas.addEventListener('pointermove',event=>{
    if(!state.uvDrag)return;const p=pointer(event,uvCanvas),face=selectedFace();
    state.model.uvs[face.uv[state.uvDrag.corner]]=[Math.max(0,Math.min(1,p.x/255)),Math.max(0,Math.min(1,1-p.y/255))];drawUV();draw();
  });
  uvCanvas.addEventListener('pointerup',()=>{if(state.uvDrag){const before=state.uvDrag.before;state.uvDrag=null;record(before);message('UV aggiornate.');}});
  uvCanvas.addEventListener('pointercancel',cancelDrag);
  $('.properties').addEventListener('change',event=>{
    const input=event.target;
    if(input.dataset.coordinate!==undefined){
      const value=Number(input.value),id=[...state.selection][0];if(input.value===''||!Number.isFinite(value)){inspect();return;}
      edit(()=>{state.model.vertices[id][Number(input.dataset.coordinate)]=value;},'Vertice aggiornato.');
    }else if(input.dataset.uvCorner!==undefined){
      const face=selectedFace(),corner=Number(input.dataset.uvCorner),axis=Number(input.dataset.uvAxis),value=Number(input.value);
      if(!face||input.value===''||!Number.isFinite(value)){inspect();return;}
      edit(()=>{const uv=[...state.model.uvs[face.uv[corner]]];uv[axis]=value;state.model.uvs.push(uv);face.uv[corner]=state.model.uvs.length-1;},'UV aggiornate.');
    }
  });
  for(const b of document.querySelectorAll('[data-mode]'))b.onclick=()=>setMode(b.dataset.mode);
  for(const b of document.querySelectorAll('[data-tool]'))b.onclick=()=>setTool(b.dataset.tool);
  for(const b of document.querySelectorAll('[data-view]'))b.onclick=()=>{cancelDrag();setView(b.dataset.view);};
  for(const id of ['#grid','#edges','#y-down','#shading'])$(id).onchange=draw;
  $('#fit').onclick=fit;$('#select-all').onclick=selectAll;$('#select-none').onclick=()=>{state.selection.clear();render();};
  $('#connected').onclick=()=>{state.selection=new Set(C.connected(state.model,[...state.selection]));render();};
  $('#undo').onclick=()=>undo();$('#redo').onclick=()=>undo(true);$('#delete').onclick=deleteSelection;
  $('#model-name').onchange=()=>edit(()=>{state.model.name=C.safeName($('#model-name').value);},'Nome aggiornato.');
  for(const b of document.querySelectorAll('[data-primitive]'))b.onclick=()=>edit(()=>{
    const size=numbers(['#primitive-size'])[0];if(size<=0||size>10000)throw new Error('Dimensione richiesta: fra 0 e 10000.');
    const offset=numbers(['#add-x','#add-y','#add-z']),part=C.primitive(b.dataset.primitive,size,Number($('#segments').value)||6),added=C.append(state.model,part,offset);
    state.selection=new Set(state.mode==='faces'?added.faces:added.vertices);fit();
  },'Forma aggiunta.');
  $('#add-vertex').onclick=()=>edit(()=>{
    const p=numbers(['#add-x','#add-y','#add-z']);state.model.vertices.push(p);setMode('vertices');state.selection=new Set([state.model.vertices.length-1]);
  },'Vertice aggiunto. Seleziona tre o quattro punti per creare una faccia.');
  $('#make-face').onclick=()=>edit(()=>{
    const v=[...state.selection];if(state.mode!=='vertices'||![3,4].includes(v.length))throw new Error('Seleziona tre o quattro vertici nell’ordine del bordo.');
    const face={v,uv:[],group:''};if(C.length(C.normal(state.model,face))<.5)throw new Error('I vertici non definiscono una faccia: controlla ordine e posizione.');
    C.triangulate(state.model,face);C.mapUV(state.model,face);state.model.faces.push(face);setMode('faces');state.selection=new Set([state.model.faces.length-1]);
  },'Faccia creata.');
  $('#flip-face').onclick=()=>edit(()=>{for(const i of selectedFaces()){state.model.faces[i].v.reverse();state.model.faces[i].uv.reverse();}},'Ordine delle facce invertito.');
  $('#extrude').onclick=()=>edit(()=>{const id=[...state.selection][0];C.extrude(state.model,id,numbers(['#extrude-distance'])[0]);},'Faccia estrusa.');
  $('#merge').onclick=()=>edit(()=>{C.merge(state.model,[...state.selection]);state.selection.clear();},'Vertici uniti al centro della selezione.');
  $('#cleanup').onclick=()=>edit(()=>{C.cleanup(state.model);state.selection.clear();},'Vertici e UV non utilizzati rimossi.');
  $('#duplicate').onclick=()=>edit(()=>{const added=C.duplicate(state.model,selectedFaces());state.selection=new Set(state.mode==='faces'?added.faces:added.vertices);setTool('move');},'Copia creata nella stessa posizione: spostala con il mouse o i campi numerici.');
  $('#transform-type').onchange=()=>{const value=$('#transform-type').value==='scale'?1:0;for(const id of ['#transform-x','#transform-y','#transform-z'])$(id).value=value;};
  $('#apply-transform').onclick=()=>edit(()=>{
    const ids=selectedIds(),type=$('#transform-type').value,values=numbers(['#transform-x','#transform-y','#transform-z']);
    if(!ids.length)throw new Error('Seleziona prima i vertici o le facce da trasformare.');
    if(type==='scale'&&values.some(v=>Math.abs(v)<1e-8))throw new Error('La scala zero appiattisce la mesh. Usa un fattore diverso da zero.');
    C.transform(state.model,ids,type,values,$('#pivot').value==='origin'?[0,0,0]:C.centroid(state.model,ids));
    if(type==='scale'&&values.reduce((a,b)=>a*b,1)<0){const set=new Set(ids);for(const face of state.model.faces)if(face.v.every(i=>set.has(i))){face.v.reverse();face.uv.reverse();}}
  },'Trasformazione applicata.');
  $('#map-uv').onclick=()=>edit(()=>{
    const plane=$('#uv-plane').value,axes={xy:[0,1],xz:[0,2],zy:[2,1]}[plane];for(const i of selectedFaces())C.mapUV(state.model,state.model.faces[i],axes);
  },'UV mappate: ciascuna faccia usa l’intera texture.');
  function readDataURL(file){return new Promise((resolve,reject)=>{const reader=new FileReader();reader.onload=()=>resolve(reader.result);reader.onerror=()=>reject(new Error('Impossibile leggere il file.'));reader.readAsDataURL(file);});}
  function loadImage(url){return new Promise((resolve,reject)=>{const im=new Image();im.onload=()=>resolve(im);im.onerror=()=>reject(new Error('Texture PNG non valida.'));im.src=url;});}
  $('#load-texture').onclick=()=>$('#texture-file').click();
  $('#texture-file').onchange=async event=>{
    const file=event.target.files[0];if(!file)return;
    try{
      if(file.size>8*1024*1024)throw new Error('Texture troppo grande: massimo 8 MB.');
      const header=new Uint8Array(await file.slice(0,8).arrayBuffer());if(header.join(',')!=='137,80,78,71,13,10,26,10')throw new Error('Scegli una texture PNG.');
      const data=await readDataURL(file),im=await loadImage(data);if(im.width>2048||im.height>2048)throw new Error('Texture troppo grande: massimo 2048 × 2048.');
      state.texture=im;state.textureData=data;state.textureName=file.name.replace(/[\r\n]/g,'_');state.version++;$('#shading').value='texture';render();message('Texture caricata. Le UV esistenti sono conservate.');
    }catch(e){message(e.message,true);}finally{event.target.value='';}
  };
  $('#remove-texture').onclick=()=>{state.texture=null;state.textureData='';state.textureName='';state.version++;render();message('Texture rimossa dalla sessione.');};
  $('#export-mtl').onclick=()=>{if(state.texture)download(C.safeName(state.model.name)+'.mtl',C.exportMTL(state.textureName),'text/plain');};
  $('#export').onclick=()=>{
    try{
      const ps1=$('#export-format').value==='ps1',text=C.exportOBJ(state.model,{ps1,texture:state.texture});
      download(C.safeName(state.model.name)+'.obj',text,'text/plain');
      message(ps1?'OBJ PS1 esportato: facce uniformi, UV e assi originali.':state.texture?'OBJ standard esportato. Scarica anche l’MTL e mantieni la PNG accanto all’OBJ.':'OBJ standard esportato.');
    }catch(e){message(e.message,true);}
  };
  function saveProject(){
    if(state.drag||state.uvDrag){message('Completa prima il trascinamento.',true);return;}
    const data={format:'psx3dstudio-1',model:state.model,texture:state.textureData?{name:state.textureName,data:state.textureData}:null,camera:state.camera,view:state.view,yDown:$('#y-down').checked};
    download(C.safeName(state.model.name)+'.model.json',JSON.stringify(data,null,2)+'\n','application/json');state.saved=state.version;render();message('Progetto scaricato: mesh, UV, texture e vista.');
  }
  $('#save-project').onclick=saveProject;
  function reset(model){state.model=model;state.history=[];state.future=[];state.selection.clear();state.version=0;state.saved=0;state.texture=null;state.textureName='';state.textureData='';fit();render();}
  const discard=()=>state.version===state.saved||confirm('Ci sono modifiche non salvate nel progetto. Continuare?');
  $('#new').onclick=()=>{if(!discard())return;cancelDrag();reset(C.empty('MODEL'));message('Nuovo modello. Aggiungi una forma o dei vertici.');};
  $('#open').onclick=()=>$('#open-file').click();
  $('#open-file').onchange=async event=>{
    const file=event.target.files[0];if(!file)return;
    try{
      if(file.size>16*1024*1024)throw new Error('File troppo grande: massimo 16 MB.');
      const text=await file.text();let model,projectData=null,texture=null,warnings=[];
      if(/\.obj$/i.test(file.name)){const result=C.parseOBJ(text,file.name);model=result.model;warnings=result.warnings;}
      else {
        projectData=JSON.parse(text);if(!['psx3dstudio-1','crystal-modeler-1'].includes(projectData.format))throw new Error('Non è un progetto PSX 3D Studio.');
        model=C.validate(projectData.model);
        if(projectData.texture){
          if(typeof projectData.texture.data!=='string'||!/^data:image\/png;base64,/.test(projectData.texture.data))throw new Error('Texture del progetto non valida.');
          texture=await loadImage(projectData.texture.data);if(texture.width>2048||texture.height>2048)throw new Error('Texture del progetto troppo grande.');
        }
      }
      if(!discard())return;cancelDrag();model.name=C.safeName(model.name||file.name.replace(/\.[^.]+$/,''));reset(model);
      if(projectData){
        if(texture){state.texture=texture;state.textureData=projectData.texture.data;state.textureName=String(projectData.texture.name||'TEX.png');$('#shading').value='texture';}
        $('#y-down').checked=Boolean(projectData.yDown);
        setView(['perspective','front','side','top'].includes(projectData.view)?projectData.view:'perspective');
        const cam=projectData.camera;
        if(cam&&Array.isArray(cam.center)&&cam.center.length===3&&cam.center.every(Number.isFinite)&&['yaw','pitch','distance','span'].every(k=>Number.isFinite(cam[k]))&&cam.distance>0&&cam.span>0)state.camera=C.copy(cam);
      }
      render();message('Aperto '+file.name+(warnings.length?' · '+warnings.join(' '):' · UV e coordinate conservate.'));
    }catch(e){message(e.message,true);}finally{event.target.value='';}
  };
  $('#help').onclick=()=>$('#help-dialog').showModal();$('#close-help').onclick=()=>$('#help-dialog').close();
  window.addEventListener('keydown',event=>{
    if($('#help-dialog').open)return;
    const cmd=event.ctrlKey||event.metaKey,key=event.key.toLowerCase();
    if(cmd&&key==='s'){event.preventDefault();document.activeElement.blur();saveProject();return;}
    if(['INPUT','SELECT','TEXTAREA'].includes(document.activeElement.tagName))return;
    if(cmd&&key==='z'){event.preventDefault();undo(event.shiftKey);}
    else if(cmd&&key==='s'){event.preventDefault();saveProject();}
    else if(event.key==='Escape'){cancelDrag();state.selection.clear();render();}
    else if(event.key==='Delete'||event.key==='Backspace'){event.preventDefault();deleteSelection();}
    else if(!cmd&&key==='v')setTool('select');
    else if(!cmd&&key==='g')setTool('move');
    else if(!cmd&&key==='a')selectAll();
    else if(!cmd&&key==='f')fit();
  });
  window.addEventListener('beforeunload',event=>{if(state.version!==state.saved){event.preventDefault();event.returnValue='';}});
  new ResizeObserver(()=>{
    const r=$('#viewport').getBoundingClientRect(),ratio=window.devicePixelRatio||1;
    state.width=Math.max(1,r.width);state.height=Math.max(1,r.height);canvas.width=Math.round(state.width*ratio);canvas.height=Math.round(state.height*ratio);draw();
  }).observe($('#viewport'));
  fit();render();
})();
