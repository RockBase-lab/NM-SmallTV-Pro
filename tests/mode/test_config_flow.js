const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const html=fs.readFileSync('src/webui.html','utf8');
function between(start,end){const first=html.indexOf(start),last=html.indexOf(end,first);assert.ok(first>=0&&last>first,start);return html.slice(first,last)}
const apps={ticker:'stocks',ticker2:'stocks2',crypto:'crypto',weather:'weather',usage:'usage',radar:'radar',ha:'ha',theme:'theme',pictures:'pictures'};

function page(mode,features={ticker:true,crypto:true,usage:true,radar:true,ha:true,theme:true,pictures:true}){
 const elements={};
 function element(tag='div'){
  let selected='',identity='';
  const item={tag,children:[],options:[],style:{},checked:false,textContent:'',className:'',
   appendChild(child){this.children.push(child);if(child.tag==='option')this.options.push(child);return child},
   remove(){delete elements[this.id]},closest(){return this},querySelector(){return null},
   replaceChildren(){this.children=[];this.options=[]},set innerHTML(value){assert.equal(value,'');this.replaceChildren()}};
  Object.defineProperty(item,'id',{get:()=>identity,set:value=>{identity=value;elements[value]=item}});
  Object.defineProperty(item,'value',{get:()=>tag==='select'&&!item.options.some(option=>option.value===selected)?'':selected,set:value=>{selected=String(value)}});
  return item;
 }
 for(const match of html.matchAll(/<(\w+)\b[^>]*\bid="([^"]+)"[^>]*>/g)){const item=element(match[1]);item.id=match[2]}
 for(const match of html.matchAll(/<select\b[^>]*\bid="([^"]+)"[^>]*>([\s\S]*?)<\/select>/g)){
  const select=elements[match[1]];
  for(const option of match[2].matchAll(/<option value="([^"]+)"/g)){const item=element('option');item.value=option[1];select.appendChild(item)}
 }
 const configuration={mode,features,brightness:80,rotation:0,hostname:'test',apSsid:'test',
  carouselSec:30,clock:{},ticker:{},ticker2:{},crypto:{},radar:{},timers:[],display:{},
  carouselTicker:true,carouselTicker2:true,carouselCrypto:true,carouselWeather:true,
  carouselUsage:true,carouselRadar:true,carouselHa:true,carouselTheme:true,carouselPictures:true};
 const document={getElementById:id=>elements[id]||null,createElement:element,
  querySelector(selector){const option=/^#mode option\[value="([^"]+)"\]$/.exec(selector);return option?elements.mode.options.find(item=>item.value===option[1])||null:null},
  querySelectorAll(selector){if(selector==='#carouselApps input')return Object.values(elements).filter(item=>item.id.startsWith('carousel')&&item.tag==='input');return []}};
 const context={document,C:{},TZMAP:{'':'UTC0'},parseInt,parseFloat,Object,Math,console,
  j:async()=>configuration,collectWifi:()=>[],toast:()=>{}};
 for(const name of ['renderWifi','setGain','fillTz','loadThemes','burninToggled','burninTriggerChanged','renderSyms','symHintFor','radarLocChanged','radarSrcChanged','renderAps','authShow','apOpenChanged'])context[name]=()=>{};
 vm.createContext(context);
 vm.runInContext(between('function $(id)','// sun begin')+between('function numIn(','// sun end')+
  between('var T_TEXT=','// IANA -> POSIX')+between('var MODEOPT=','// The AP password')+
  between('function loadConfig()','function esc(s)')+between('function collect(){','function saveAll(){'),context);
 return {context,elements,configuration};
}

(async()=>{
 for(const mode of [...Object.values(apps),'timer','carousel']){
  const {context,elements}=page(mode);await context.loadConfig();
  assert.equal(elements.mode.value,mode,'saved mode must be represented: '+mode);
  assert.equal(context.collect().mode,mode,'ordinary save preserves '+mode);
  for(let index=0;index<4;index++)for(const value of Object.values(apps))
   assert.ok(elements['timerApp'+index].options.some(option=>option.value===value),'timer app '+value);
 }
 const {context,elements}=page('carousel');await context.loadConfig();
 for(const feature of Object.keys(apps)){
  const field=context.CAROPT[feature];assert.ok(elements[field],'compiled app has a visible checkbox: '+feature);
  elements[field].checked=feature==='weather'||feature==='theme';
 }
 const saved=context.collect();
 for(const feature of Object.keys(apps))assert.equal(saved[context.CAROPT[feature]],feature==='weather'||feature==='theme');
 const unavailable=page('future-mode');await unavailable.context.loadConfig();
 assert.equal(unavailable.elements.mode.value,'');
 assert.equal('mode' in unavailable.context.collect(),false,'unknown saved mode must never become Timer');
 const lean=page('timer',{ticker:false,crypto:false,usage:false,radar:false,ha:false,theme:false,pictures:false});
 lean.configuration.timers=[{enabled:true,time:'08:00',app:'radar'}];
 await lean.context.loadConfig();
 assert.equal(lean.elements.timerApp0.value,'');
 assert.equal(lean.context.collect().timers[0].app,'radar','ordinary saves preserve unavailable timer apps');
 assert.equal(lean.elements.carouselRadar,undefined);
 assert.deepEqual(lean.elements.mode.options.map(option=>option.value),['timer','carousel','weather']);
 console.log('Real page mode loading, timer choices and carousel save flow passed');
})().catch(error=>{console.error(error);process.exitCode=1});
