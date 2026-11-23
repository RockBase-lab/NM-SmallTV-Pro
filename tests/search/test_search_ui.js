const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),zlib=require('node:zlib');
const html=fs.readFileSync('src/webui.html','utf8');
function element(tag='div') {
  return {tag,value:'',textContent:'',children:[],style:{},events:{},
    appendChild(c){this.children.push(c);return c},replaceChildren(){this.children=[];this.value=''},
    addEventListener(name,fn){this.events[name]=fn},setAttribute(){}};
}
async function check(code){
  const nodes={};for(const id of ['locQ','locRes','locMsg','wLocQ','wLocRes','wLocMsg','radarCityQ','radarCityRes','radarSearchMsg','tz','tzQuery','timezoneMsg'])nodes[id]=element();
  const rows=[];let writes=0,city,radar;const requests=[];
  const table=element();table.parentNode={insertBefore(box){box.children.forEach(n=>{if(n.id)nodes[n.id]=n})}};nodes.symTable=table;
  function row(item){return {fields:{'.s':{value:item.symbol||''},'.n':{value:item.name||''},'.src':{value:item.source||''}},querySelector(k){return this.fields[k]}}}
  rows.push(row({}));
  const ctx={console,setTimeout,clearTimeout,AbortController,UI_LANG:'en',TZMAP:{'':'UTC0',UTC:'UTC0','Asia/Shanghai':'CST-8','Europe/London':'GMT0BST,M3.5.0/1,M10.5.0'},C:{clock:{tz:'Custom/Stored',tzPosix:'TEST-3'}},
    document:{createElement:element,querySelectorAll:()=>rows},$:id=>nodes[id]||null,gv:id=>nodes[id]?nodes[id].value:'',sv:(id,v)=>{if(nodes[id])nodes[id].value=v},
    symTableId:n=>n===1?'symTable':'absent',addRow:o=>rows.push(row(o)),symHintFor(){},toast(){},
    fillTz(){nodes.tz.replaceChildren();Object.keys(ctx.TZMAP).forEach(k=>{const o=element('option');o.value=k;nodes.tz.appendChild(o)})},
    setLoc:(...values)=>city=values,radarSetCenter:(...values)=>radar=values,
    fetch:async(url,opt)=>{requests.push(url);if(opt&&opt.method==='POST')writes++;return {ok:true,json:async()=>url.startsWith('/api/search')?{results:[{symbol:'AAPL',name:'Apple Inc.',source:'yahoo',market:'NASDAQ'}]}:
      {results:[{name:'Berlin',admin1:'Berlin',country:'Germany',latitude:52.52437,longitude:13.41053,timezone:'Europe/Berlin'}]}}}};
  vm.createContext(ctx);vm.runInContext(code,ctx);
  vm.runInContext(html.slice(html.indexOf('function presetAdd('),html.indexOf('function addMarketPreset(')),ctx);
  ctx.SearchUI.init();
  nodes.locQ.value='Berlin';await ctx.SearchUI.searchCity('locQ');
  assert.equal(city,undefined,'searching never auto-selects the first result');
  assert.equal(nodes.locRes.value,'');assert.equal(nodes.locRes.children.length,3);
  assert.match(nodes.locRes.children[2].textContent,/Germany.*52.5244.*Europe\/Berlin/);
  nodes.locRes.value='1';nodes.locRes.onchange();assert.equal(city[0],'52.5244');
  nodes.radarCityQ.value='Berlin';await ctx.SearchUI.searchCity('radarCityQ');nodes.radarCityRes.value='0';nodes.radarCityRes.onchange();assert.equal(radar[0],52.52437);
  nodes.locQ.value='';await ctx.SearchUI.searchCity('locQ');assert.equal(city[0],'52.5244');
  nodes.tz.value='Europe/London';assert.equal(ctx.SearchUI.selectTimezone('Mars/Unknown'),false);assert.equal(nodes.tz.value,'Europe/London');
  assert.match(nodes.timezoneMsg.textContent,/no built-in/);
  assert.equal(ctx.SearchUI.selectTimezone('Asia/Shanghai'),true);assert.equal(nodes.tz.value,'Asia/Shanghai');
  nodes.tz.value='Custom/Stored';nodes.tzQuery.value='上海';ctx.SearchUI.filterTimezone();
  assert.equal(nodes.tz.value,'Custom/Stored','filter preserves saved custom timezone and rule');
  assert.ok(nodes.tz.children.some(c=>c.value==='Asia/Shanghai'));
  nodes.symbolQuery1.value='苹果';nodes.symbolQuery1.events.input({isComposing:true});
  assert.equal(rows[0].fields['.s'].value,'');
  nodes.symbolResults1.value='0';nodes.symbolResults1.onchange();assert.equal(rows[0].fields['.s'].value,'AAPL');assert.equal(rows[0].fields['.src'].value,'yahoo');
  nodes.symbolResults1.onchange();assert.equal(rows.length,1,'selecting the same provider/symbol does not duplicate rows');
  for(const query of ['ON','CAT','苹果']){
    nodes.symbolQuery1.value=query;nodes.symbolQuery1.events.keydown({key:'Enter',isComposing:false,preventDefault(){}});
    await new Promise(setImmediate);
    assert.equal(requests.at(-1),'/api/search/stocks?q='+encodeURIComponent(query==='苹果'?'AAPL':query),'short real tickers must not turn into a partial company-name match');
  }
  assert.equal(writes,0,'lookup and picking only edit unsaved form fields');
}
(async()=>{
  await check(fs.readFileSync('src/webui-search.js','utf8'));
  if(process.argv.includes('--generated')){
    const header=fs.readFileSync('src/webui.h','utf8').split('PROGMEM = {')[1];
    const served=zlib.gunzipSync(Buffer.from([...header.matchAll(/0x([a-f0-9]{2})/g)].map(m=>parseInt(m[1],16)))).toString();
    assert.ok(!served.includes('src="webui-search.js"'),'search module is bundled for offline device UI');
    for(const match of served.matchAll(/<script>([\s\S]*?)<\/script>/g))new vm.Script(match[1]);
    await check([...served.matchAll(/<script>([\s\S]*?)<\/script>/g)][0][1]);
  }
  console.log('Search form selection, independent radar, timezone preservation and unsaved provider rows passed');
})().catch(e=>{console.error(e);process.exitCode=1});
