(() => {
  const C=PSX3DStudioCore,checks=[];
  const ok=(condition,message='Verifica fallita')=>{if(!condition)throw new Error(message);};
  const near=(a,b)=>ok(Math.abs(a-b)<1e-6,`${a} ≠ ${b}`);
  const throws=fn=>{let failed=false;try{fn();}catch(_){failed=true;}ok(failed,'Era atteso un errore.');};
  const test=(name,fn)=>{try{fn();checks.push({name,pass:true});}catch(e){checks.push({name,pass:false,error:e.message});}};
  test('Primitive valide e low poly',()=>{
    for(const kind of ['cube','plane','pyramid','tetra','cylinder']){const m=C.primitive(kind);C.validate(m);ok(m.faces.length<=18);ok(m.faces.every(f=>f.uv.length===f.v.length));}
    const cube=C.primitive('cube');ok(cube.vertices.length===8&&cube.faces.length===6);
  });
  test('Le normali delle primitive chiuse puntano verso l’esterno',()=>{
    for(const kind of ['cube','pyramid','tetra','cylinder']){
      const m=C.primitive(kind),center=C.centroid(m,m.vertices.map((_,i)=>i));
      for(const f of m.faces)ok(C.dot(C.normal(m,f),C.sub(C.centroid(m,f.v),center))>0,kind+' ha una normale invertita');
    }
  });
  test('OBJ con indici negativi, UV e normali',()=>{
    const result=C.parseOBJ('v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nvn 0 0 1\nf -3/-3/1 -2/-2/1 -1/-1/1\n');
    ok(JSON.stringify(result.model.faces[0].v)==='[0,1,2]');ok(JSON.stringify(result.model.faces[0].uv)==='[0,1,2]');
  });
  test('OBJ senza UV riceve una mappatura valida',()=>{
    const m=C.parseOBJ('v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n').model;C.validate(m);ok(m.uvs.length===3);
  });
  test('UV NaN legacy diventano zero come nel parser del gioco, senza cambiare indici',()=>{
    const parsed=C.parseOBJ('v 0 0 0\nv 1 0 0\nv 0 1 0\nvt nan nan\nvt 1 0\nvt 0 1\nf 1/1 2/2 3/3\n');
    ok(parsed.model.uvs[0][0]===0&&parsed.model.uvs[0][1]===0);ok(parsed.model.faces[0].uv[0]===0);ok(parsed.warnings.length===1);
  });
  test('Un poligono concavo viene triangolato senza riempire il vuoto',()=>{
    const m=C.parseOBJ('v 0 0 0\nv 2 0 0\nv 2 2 0\nv 1 1 0\nv 0 2 0\nf 1 2 3 4 5\n').model;
    ok(m.faces.length===3);const area=m.faces.reduce((sum,f)=>sum+C.length(C.cross(C.sub(m.vertices[f.v[1]],m.vertices[f.v[0]]),C.sub(m.vertices[f.v[2]],m.vertices[f.v[0]])))/2,0);near(area,3);
  });
  test('Round trip OBJ PS1 conserva coordinate, indici e UV dei quad',()=>{
    const m=C.primitive('cube'),out=C.exportOBJ(m),parsed=C.parseOBJ(out).model;
    ok(JSON.stringify(m.vertices)===JSON.stringify(parsed.vertices));ok(JSON.stringify(m.uvs)===JSON.stringify(parsed.uvs));
    ok(JSON.stringify(m.faces.map(f=>[f.v,f.uv]))===JSON.stringify(parsed.faces.map(f=>[f.v,f.uv])));
    ok(!out.includes('vn '));ok(out.endsWith('\n'));
  });
  test('L’export PS1 promuove i triangoli a quad uniformi',()=>{
    const m=C.primitive('pyramid'),out=C.parseOBJ(C.exportOBJ(m)).model;
    ok(out.faces.every(f=>f.v.length===4));ok(out.faces.slice(1).every(f=>f.v[2]===f.v[3]));
  });
  test('I limiti reali del parser PS1 bloccano l’export senza troncare',()=>{
    const m=C.primitive('cube');m.faces=Array.from({length:257},()=>C.copy(m.faces[0]));throws(()=>C.exportOBJ(m));ok(C.exportOBJ(m,{ps1:false}).includes('f '));
    for(const v of [-123.123456,0.000001,1234.56789]){const s=C.format(v,true);ok(s.length<=9);ok(!s.includes('e'));}
  });
  test('Estrusione: aggiunge quattro vertici, cap e pareti senza alterare la base',()=>{
    const m=C.primitive('cube'),original=C.copy(m.vertices),n=C.normal(m,m.faces[0]),old=C.copy(m.faces[0].v);C.extrude(m,0,.2);C.validate(m);
    ok(m.vertices.length===12&&m.faces.length===10);ok(JSON.stringify(m.vertices.slice(0,8))===JSON.stringify(original));
    old.forEach((id,i)=>m.vertices[m.faces[0].v[i]].forEach((v,k)=>near(v,original[id][k]+n[k]*.2)));
  });
  test('Trasformazioni numeriche attorno al centro della selezione',()=>{
    const m=C.primitive('cube'),ids=m.vertices.map((_,i)=>i);C.transform(m,ids,'move',[1,2,3]);near(C.centroid(m,ids)[0],1);
    const p=C.copy(m.vertices[0]);C.transform(m,ids,'scale',[2,1,1]);near(m.vertices[0][0],1+(p[0]-1)*2);
    C.transform(m,ids,'rotate',[0,0,90]);near(C.centroid(m,ids)[0],1);near(C.centroid(m,ids)[1],2);
  });
  test('Duplica solo vertici e UV usati dalle facce selezionate',()=>{
    const m=C.primitive('cube'),result=C.duplicate(m,[0]);C.validate(m);ok(result.vertices.length===4&&result.faces.length===1);ok(m.uvs.length===28);
  });
  test('Unione e pulizia conservano indici validi',()=>{
    const m=C.primitive('cube');m.vertices.push([9,9,9]);C.cleanup(m);ok(m.vertices.length===8);C.merge(m,[0,1]);C.validate(m);ok(m.vertices.length===7);
  });
  test('Eliminare un vertice rimuove le facce che lo usano',()=>{
    const m=C.primitive('cube');C.removeVertices(m,[0]);C.validate(m);ok(m.vertices.length===7&&m.faces.length===3);
  });
  test('Indici e coordinate non validi vengono rifiutati',()=>{
    throws(()=>C.parseOBJ('v 0 0 0\nf 1 2 3\n'));throws(()=>C.parseOBJ('v NaN 0 0\n'));
    const m=C.primitive('cube');m.faces[0].uv[0]=999;throws(()=>C.validate(m));
  });
  const failed=checks.filter(t=>!t.pass);window.modelerTestResults={total:checks.length,failed};
  document.querySelector('#result').textContent=`${checks.length-failed.length}/${checks.length} test superati`;
  for(const check of checks){const row=document.createElement('li');row.textContent=(check.pass?'OK · ':'ERRORE · ')+check.name+(check.error?' — '+check.error:'');row.style.color=check.pass?'#006000':'#800000';document.querySelector('#tests').append(row);}
})();
