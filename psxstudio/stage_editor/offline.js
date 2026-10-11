/* Local-file browser adapter. No fetch, server, filesystem API or dependencies. */
window.offlineEditor = location.protocol === 'file:' ? (() => {
  const catalog={files:[],imported:[]}, packages=new Map(), downloaded=new Set();
  const bytes=text=>Uint8Array.from(atob(text),c=>c.charCodeAt(0));
  const utf8=text=>new TextEncoder().encode(text);
  function crc32(data) {
    let crc=0xffffffff;
    for(const b of data){crc^=b;for(let i=0;i<8;i++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}
    return (crc^0xffffffff)>>>0;
  }
  function zip(files) {
    const entries=[],records=[];let position=0;
    for(const [filename,contents] of Object.entries(files)) {
      const name=utf8(filename),data=typeof contents==='string'?utf8(contents):contents,crc=crc32(data);
      const header=new Uint8Array(30),h=new DataView(header.buffer);
      h.setUint32(0,0x04034b50,true);h.setUint16(4,20,true);h.setUint32(14,crc,true);h.setUint32(18,data.length,true);h.setUint32(22,data.length,true);h.setUint16(26,name.length,true);
      entries.push(header,name,data);
      const record=new Uint8Array(46),r=new DataView(record.buffer);
      r.setUint32(0,0x02014b50,true);r.setUint16(4,20,true);r.setUint16(6,20,true);r.setUint32(16,crc,true);r.setUint32(20,data.length,true);r.setUint32(24,data.length,true);r.setUint16(28,name.length,true);r.setUint32(42,position,true);
      records.push(record,name);position+=header.length+name.length+data.length;
    }
    const end=new Uint8Array(22),e=new DataView(end.buffer),size=records.reduce((n,b)=>n+b.length,0),count=Object.keys(files).length;
    e.setUint32(0,0x06054b50,true);e.setUint16(8,count,true);e.setUint16(10,count,true);e.setUint32(12,size,true);e.setUint32(16,position,true);
    return new Blob([...entries,...records,end],{type:'application/zip'});
  }
  const adapter={stages:[],catalog,
    async api(url,data) {
      if(url==='/api/project')return {stages:adapter.stages,catalog,revision:'offline',token:''};
      if(url==='/api/stages') {
        download('stages.json',serialize(data.stages),'application/json');
        return {revision:'offline'};
      }
      if(url==='/api/backgrounds') {
        const id=String(Date.now())+'-'+catalog.imported.length;
        const item={id,name:data.name,tim0:data.tim0,tim1:data.tim1,url:'data:image/png;base64,'+data.image};
        const files={};
        [data.tim0,data.tim1].forEach((name,i)=>{
          files['data/stages/'+name.slice(0,-4)+'.png']=bytes(data.parts[i].png);
          files['data/stages/'+name.slice(0,-4)+'.tim']=bytes(data.parts[i].tim);
        });
        files['CONF_ENTRIES.TXT']=[data.tim0,data.tim1].map(name=>`File ${name}\r\n    XAFileAttributes Form1 Data\r\n    Source [root]\\data\\stages\\${name.slice(0,-4)}.tim\r\nEndFile\r\n`).join('\r\n');
        files['README.txt']='Copy data/stages into the project. Add the CONF_ENTRIES.TXT entries at the root of cdrom/CONF.CTI without duplicates.\nReplace stages/stages.json with the downloaded JSON. Run make run in stages, then bash build.sh from the repository root.\n16-bit TIM: VRAM (320,0) and (576,0).\n';
        packages.set(id,zip(files));return item;
      }
      throw new Error('Operation unavailable offline.');
    },
    async addImages(files) {
      for(const file of files) {
        if(!/\.png$/i.test(file.name))continue;
        const url=URL.createObjectURL(file);
        try {
          const im=await image(url),entry={name:file.name,width:im.width,height:im.height,url};
          const index=catalog.files.findIndex(f=>f.name.toUpperCase()===file.name.toUpperCase());
          if(index>=0){URL.revokeObjectURL(catalog.files[index].url);catalog.files[index]=entry;}else catalog.files.push(entry);
        }catch(error){URL.revokeObjectURL(url);throw error;}
      }
    },
    exportAssets(id) {
      const blob=packages.get(id),item=catalog.imported.find(i=>i.id===id);
      if(blob){download((item?.name||'background')+'.zip',blob,'application/zip');downloaded.add(id);}
      else report('Package already downloaded: assign the PNG again to regenerate it.');
    },
    pendingAssets:()=>[...packages.keys()].some(id=>!downloaded.has(id))
  };
  return adapter;
})() : null;
