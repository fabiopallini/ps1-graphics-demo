/* Geometry and OBJ operations, independent from the UI. */
(() => {
  'use strict';
  const copy = value => JSON.parse(JSON.stringify(value));
  const add = (a,b) => a.map((v,i)=>v+b[i]);
  const sub = (a,b) => a.map((v,i)=>v-b[i]);
  const mul = (a,s) => a.map(v=>v*s);
  const dot = (a,b) => a.reduce((s,v,i)=>s+v*b[i],0);
  const cross = (a,b) => [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]];
  const length = a => Math.hypot(...a);
  const unit = a => mul(a,1/(length(a)||1));
  const empty = name => ({name:name||'MODEL',vertices:[],uvs:[],faces:[]});
  function normal(model,face) {
    let n=[0,0,0];
    face.v.forEach((index,i)=>{
      const a=model.vertices[index],b=model.vertices[face.v[(i+1)%face.v.length]];
      n=add(n,[(a[1]-b[1])*(a[2]+b[2]),(a[2]-b[2])*(a[0]+b[0]),(a[0]-b[0])*(a[1]+b[1])]);
    });
    return unit(n);
  }
  function centroid(model,indices) {
    const ids=[...indices];return ids.length?mul(ids.reduce((a,i)=>add(a,model.vertices[i]),[0,0,0]),1/ids.length):[0,0,0];
  }
  function mapUV(model,face,axes) {
    if(!axes){const n=normal(model,face).map(Math.abs),axis=n.indexOf(Math.max(...n));axes=axis===0?[2,1]:axis===1?[0,2]:[0,1];}
    const values=face.v.map(i=>axes.map(a=>model.vertices[i][a]));
    const lo=axes.map((_,a)=>Math.min(...values.map(p=>p[a]))),hi=axes.map((_,a)=>Math.max(...values.map(p=>p[a])));
    face.uv=values.map(p=>{model.uvs.push(p.map((v,a)=>(v-lo[a])/(hi[a]-lo[a]||1)));return model.uvs.length-1;});
  }
  function ensureUV(model) {
    for(const face of model.faces)if(!face.uv || face.uv.length!==face.v.length || face.uv.some(i=>!Number.isInteger(i)||!model.uvs[i]))mapUV(model,face);
  }
  function validate(model) {
    if(!model || !Array.isArray(model.vertices)||!Array.isArray(model.uvs)||!Array.isArray(model.faces))throw new Error('Invalid 3D project.');
    if(model.vertices.length>20000 || model.faces.length>20000 || model.uvs.length>80000)throw new Error('Model too large for this low-poly editor.');
    if(model.vertices.some(p=>!Array.isArray(p)||p.length!==3||p.some(v=>!Number.isFinite(v)||Math.abs(v)>1e6)))throw new Error('Invalid vertex coordinates.');
    if(model.uvs.some(p=>!Array.isArray(p)||p.length!==2||p.some(v=>!Number.isFinite(v)||Math.abs(v)>1e6)))throw new Error('Invalid UV coordinates.');
    for(const face of model.faces) {
      if(!face || !Array.isArray(face.v)||![3,4].includes(face.v.length)||face.v.some(i=>!Number.isInteger(i)||!model.vertices[i])||new Set(face.v).size<3)throw new Error('Invalid face: three or four vertices required.');
      if(!Array.isArray(face.uv)||face.uv.length!==face.v.length||face.uv.some(i=>!Number.isInteger(i)||!model.uvs[i]))throw new Error('Invalid face UVs.');
    }
    return model;
  }
  function triangulate(model,face) {
    const ids=face.v.map((_,i)=>i);if(ids.length<=3)return [ids];
    const n=normal(model,face).map(Math.abs),axis=n.indexOf(Math.max(...n));
    const axes=axis===0?[1,2]:axis===1?[0,2]:[0,1],p=face.v.map(i=>axes.map(a=>model.vertices[i][a]));
    const area=p.reduce((s,a,i)=>{const b=p[(i+1)%p.length];return s+a[0]*b[1]-b[0]*a[1];},0),sign=area>=0?1:-1;
    const turn=(a,b,c)=>(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
    const inside=(q,a,b,c)=>[turn(a,b,q),turn(b,c,q),turn(c,a,q)].every(v=>v*sign>=-1e-10);
    const triangles=[];
    while(ids.length>3){
      let found=false;
      for(let k=0;k<ids.length;k++){
        const a=ids[(k+ids.length-1)%ids.length],b=ids[k],c=ids[(k+1)%ids.length];
        if(turn(p[a],p[b],p[c])*sign<=1e-10)continue;
        if(ids.some(i=>i!==a&&i!==b&&i!==c&&inside(p[i],p[a],p[b],p[c])))continue;
        triangles.push([a,b,c]);ids.splice(k,1);found=true;break;
      }
      if(!found) {
        if(face.v.length===4 && new Set(face.v).size===3)return [[0,1,2],[0,2,3]].filter(t=>new Set(t.map(i=>face.v[i])).size===3);
        throw new Error('Degenerate or self-intersecting polygon: fix its vertices.');
      }
    }
    triangles.push(ids);return triangles;
  }
  function parseOBJ(text,name='MODEL') {
    const model=empty(name.replace(/\.obj$/i,'')),warnings=[];let group='';
    const index=(raw,count,line)=>{
      if(!/^-?\d+$/.test(raw||''))throw new Error(`Line ${line}: Invalid OBJ index.`);
      const n=Number(raw),i=n>0?n-1:count+n;
      if(n===0||i<0||i>=count)throw new Error(`Line ${line}: Index out of range.`);return i;
    };
    text.replace(/^\uFEFF/,'').split(/\r?\n/).forEach((raw,line)=>{
      const tokens=raw.split('#')[0].trim().split(/\s+/),kind=tokens.shift();if(!kind)return;
      if(kind==='v'||kind==='vt'){
        const size=kind==='v'?3:2,values=tokens.slice(0,size).map(token=>{
          if(kind==='vt'&&/^[-+]?nan$/i.test(token)){warnings.push('NaN UVs converted to zero, matching the PS1 loader.');return 0;}
          return Number(token);
        });
        if(values.length!==size||values.some(v=>!Number.isFinite(v)))throw new Error(`Line ${line+1}: invalid coordinates.`);
        (kind==='v'?model.vertices:model.uvs).push(values);
      }else if(kind==='o'||kind==='g')group=tokens.join(' ');
      else if(kind==='f'){
        if(tokens.length<3)throw new Error(`Line ${line+1}: Incomplete face.`);
        const face={v:[],uv:[],group};
        for(const token of tokens){const pair=token.split('/');face.v.push(index(pair[0],model.vertices.length,line+1));face.uv.push(pair[1]?index(pair[1],model.uvs.length,line+1):null);}
        if(new Set(face.v).size<3)throw new Error(`Line ${line+1}: Degenerate face.`);
        if(face.uv.some(i=>i===null))mapUV(model,face);
        if(face.v.length>4){
          for(const triangle of triangulate(model,face))model.faces.push({v:triangle.map(i=>face.v[i]),uv:triangle.map(i=>face.uv[i]),group});
          warnings.push('Polygons with more than four sides converted to triangles.');
        }else model.faces.push(face);
      }else if(['l','p','curv','surf'].includes(kind))warnings.push('Lines, points and free-form surfaces are not imported.');
    });
    if(!model.vertices.length)throw new Error('No vertices found in the OBJ file.');
    validate(model);return {model,warnings:[...new Set(warnings)]};
  }
  function format(value,ps1=false) {
    if(!Number.isFinite(value))throw new Error('Invalid coordinates.');
    for(let precision=6;precision>=0;precision--){
      let out=value.toFixed(precision).replace(/(\.\d*?)0+$/,'$1').replace(/\.$/,'');if(out==='-0')out='0';
      if(!ps1 || out.length<=9)return out;
    }
    throw new Error('Coordinates too large for the PS1 parser.');
  }
  const safeName = name => String(name||'MODEL').replace(/[^A-Za-z0-9_-]/g,'_').slice(0,48)||'MODEL';
  function exportOBJ(model,{ps1=true,texture=null}={}) {
    validate(model);
    if(!model.faces.length)throw new Error('Add at least one face before exporting.');
    if(ps1&&(model.vertices.length>2048||model.uvs.length>2048||model.faces.length>256))throw new Error('PS1 loader limits: 2048 vertices, 2048 UVs, 256 quads. Use Standard OBJ or reduce the mesh.');
    const name=safeName(model.name),lines=['# PSX 3D Studio - Crystal Fantasy',`o ${name}`];
    if(!ps1&&texture)lines.push(`mtllib ${name}.mtl`);
    for(const p of model.vertices)lines.push('v '+p.map(v=>format(v,ps1)).join(' '));
    for(const p of model.uvs)lines.push('vt '+p.map(v=>format(v,ps1)).join(' '));
    if(!ps1&&texture)lines.push('usemtl atlas');
    let group='';
    for(const face of model.faces){
      if(!ps1&&face.group&&face.group!==group){group=face.group;lines.push('g '+safeName(group));}
      const pairs=face.v.map((v,i)=>`${v+1}/${face.uv[i]+1}`);
      if(ps1&&pairs.length===3)pairs.push(pairs[2]);
      lines.push('f '+pairs.join(' '));
    }
    return lines.join('\n')+'\n';
  }
  function exportMTL(textureName) {
    return 'newmtl atlas\nKa 1 1 1\nKd 1 1 1\nKs 0 0 0\nd 1\nmap_Kd '+String(textureName).replace(/[\r\n\\/]/g,'_')+'\n';
  }
  function primitive(kind,size=1,segments=6) {
    const m=empty(kind);let vertices=[],faces=[];const h=size/2;
    if(kind==='cube'){
      vertices=[[-h,-h,-h],[h,-h,-h],[h,h,-h],[-h,h,-h],[-h,-h,h],[h,-h,h],[h,h,h],[-h,h,h]];
      faces=[[0,3,2,1],[4,5,6,7],[0,4,7,3],[1,2,6,5],[0,1,5,4],[3,7,6,2]];
    }else if(kind==='plane'){
      vertices=[[-h,0,-h],[h,0,-h],[h,0,h],[-h,0,h]];faces=[[0,1,2,3]];
    }else if(kind==='pyramid'){
      vertices=[[-h,h,-h],[h,h,-h],[h,h,h],[-h,h,h],[0,-h,0]];
      faces=[[0,3,2,1],[0,1,4],[1,2,4],[2,3,4],[3,0,4]];
    }else if(kind==='tetra'){
      vertices=[[0,-h,0],[-h,h,-h],[h,h,-h],[0,h,h]];faces=[[1,3,2],[0,1,2],[0,2,3],[0,3,1]];
    }else if(kind==='cylinder'){
      segments=Math.max(3,Math.min(12,Math.round(segments)));
      for(const y of [-h,h])for(let i=0;i<segments;i++){const a=i*2*Math.PI/segments;vertices.push([Math.cos(a)*h,y,Math.sin(a)*h]);}
      vertices.push([0,-h,0],[0,h,0]);
      for(let i=0;i<segments;i++){
        const j=(i+1)%segments;faces.push([i,i+segments,j+segments,j],[2*segments,i,j],[2*segments+1,j+segments,i+segments]);
      }
    }else throw new Error('Primitive unavailable.');
    m.vertices=vertices;m.faces=faces.map(v=>({v,uv:[],group:kind}));ensureUV(m);return m;
  }
  function append(model,part,offset=[0,0,0]) {
    const vi=model.vertices.length,ui=model.uvs.length,fi=model.faces.length;
    model.vertices.push(...part.vertices.map(p=>add(p,offset)));model.uvs.push(...copy(part.uvs));
    model.faces.push(...part.faces.map(f=>({...f,v:f.v.map(i=>i+vi),uv:f.uv.map(i=>i+ui)})));
    return {vertices:part.vertices.map((_,i)=>i+vi),faces:part.faces.map((_,i)=>i+fi)};
  }
  function selectedVertices(model,mode,selection) {
    if(mode==='model')return model.vertices.map((_,i)=>i);
    if(mode==='vertices')return [...selection];
    return [...new Set([...selection].flatMap(i=>model.faces[i]?.v||[]))];
  }
  function transform(model,ids,type,values,pivot=centroid(model,ids)) {
    if(values.some(v=>!Number.isFinite(v)))throw new Error('Invalid transform values.');
    for(const i of ids){
      const p=model.vertices[i];if(type==='move'){model.vertices[i]=add(p,values);continue;}
      let q=sub(p,pivot);
      if(type==='scale')q=q.map((v,a)=>v*values[a]);
      else if(type==='rotate'){
        const [a,b,c]=values.map(v=>v*Math.PI/180);
        q=[q[0],q[1]*Math.cos(a)-q[2]*Math.sin(a),q[1]*Math.sin(a)+q[2]*Math.cos(a)];
        q=[q[0]*Math.cos(b)+q[2]*Math.sin(b),q[1],-q[0]*Math.sin(b)+q[2]*Math.cos(b)];
        q=[q[0]*Math.cos(c)-q[1]*Math.sin(c),q[0]*Math.sin(c)+q[1]*Math.cos(c),q[2]];
      }
      model.vertices[i]=add(q,pivot);
    }
  }
  function extrude(model,index,distance) {
    const face=model.faces[index];if(!face)throw new Error('Select a face.');
    const n=normal(model,face);if(length(n)<.5)throw new Error('The face has no defined normal.');
    if(!Number.isFinite(distance)||Math.abs(distance)<1e-8)throw new Error('Enter a nonzero extrusion distance.');
    const verts=face.v.map(i=>{model.vertices.push(add(model.vertices[i],mul(n,distance)));return model.vertices.length-1;});
    model.faces[index]={...face,v:verts,uv:[...face.uv]};
    for(let i=0;i<verts.length;i++){
      const j=(i+1)%verts.length,side={v:[face.v[i],face.v[j],verts[j],verts[i]],uv:[],group:face.group};
      mapUV(model,side);model.faces.push(side);
    }
    return index;
  }
  function removeVertices(model,ids) {
    const removed=new Set(ids),map=new Map(),vertices=[];
    model.vertices.forEach((p,i)=>{if(!removed.has(i)){map.set(i,vertices.length);vertices.push(p);}});
    model.faces=model.faces.filter(f=>!f.v.some(i=>removed.has(i))).map(f=>({...f,v:f.v.map(i=>map.get(i))}));
    model.vertices=vertices;
  }
  function merge(model,ids) {
    if(ids.length<2)throw new Error('Select at least two vertices.');
    const set=new Set(ids),keep=ids[0];model.vertices[keep]=centroid(model,ids);
    model.faces=model.faces.map(f=>{
      const v=[],uv=[];f.v.forEach((i,k)=>{const n=set.has(i)?keep:i;if(!v.includes(n)){v.push(n);uv.push(f.uv[k]);}});
      if(v[0]===v[v.length-1]){v.pop();uv.pop();}return {...f,v,uv};
    }).filter(f=>new Set(f.v).size>=3);
    removeVertices(model,ids.slice(1));
  }
  function cleanup(model) {
    const used=new Set(model.faces.flatMap(f=>f.v));removeVertices(model,model.vertices.map((_,i)=>i).filter(i=>!used.has(i)));
    const map=new Map(),uvs=[];
    for(const face of model.faces)face.uv=face.uv.map(i=>{if(!map.has(i)){map.set(i,uvs.length);uvs.push(model.uvs[i]);}return map.get(i);});
    model.uvs=uvs;
  }
  function duplicate(model,faceIds) {
    const ids=[...new Set(faceIds.flatMap(i=>model.faces[i].v))],part=empty('copy'),map=new Map(ids.map((id,i)=>[id,i]));
    part.vertices=ids.map(i=>copy(model.vertices[i]));
    const uvIds=[...new Set(faceIds.flatMap(i=>model.faces[i].uv))],uvMap=new Map(uvIds.map((id,i)=>[id,i]));
    part.uvs=uvIds.map(i=>copy(model.uvs[i]));
    part.faces=faceIds.map(i=>({...copy(model.faces[i]),v:model.faces[i].v.map(v=>map.get(v)),uv:model.faces[i].uv.map(v=>uvMap.get(v))}));
    return append(model,part);
  }
  function connected(model,start) {
    const set=new Set(start);let changed=true;
    while(changed){changed=false;for(const f of model.faces)if(f.v.some(i=>set.has(i)))for(const i of f.v)if(!set.has(i)){set.add(i);changed=true;}}
    return [...set];
  }
  function bounds(model) {
    if(!model.vertices.length)return {center:[0,0,0],radius:1};
    const min=[Infinity,Infinity,Infinity],max=[-Infinity,-Infinity,-Infinity];
    for(const p of model.vertices)for(let k=0;k<3;k++){min[k]=Math.min(min[k],p[k]);max[k]=Math.max(max[k],p[k]);}
    const center=mul(add(min,max),.5),radius=Math.max(.1,...model.vertices.map(p=>length(sub(p,center))));return {center,radius};
  }
  window.PSX3DStudioCore={copy,add,sub,mul,dot,cross,length,unit,empty,normal,centroid,mapUV,ensureUV,validate,triangulate,parseOBJ,format,safeName,exportOBJ,exportMTL,primitive,append,selectedVertices,transform,extrude,removeVertices,merge,cleanup,duplicate,connected,bounds};
})();
