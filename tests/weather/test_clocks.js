const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const html=fs.readFileSync('src/webui.html','utf8');
const nodes=new Proxy({}, {get:(items,id)=>items[id]||(items[id]={value:'',checked:false,style:{},textContent:'',innerHTML:'',appendChild(){},replaceChildren(){},remove(){},closest(){return {remove(){}}},querySelector(){return null}})});
const context={document:{getElementById:id=>nodes[id],querySelectorAll:()=>[],querySelector:()=>null,createElement:()=>({appendChild(){}})},console,Math,JSON,Number,parseFloat,parseInt,isNaN,Promise,setTimeout,clearTimeout,C:{},collectWifi:()=>[],toast:()=>{},loadStatus:()=>{},loadThemes:()=>{}};
vm.createContext(context);
for(const [start,end] of [['function $(id)','// tabs'],['// field groups','function loadConfig()'],['function loadConfig()','function esc('],['function collect()','// wifi networks']])
 vm.runInContext(html.slice(html.indexOf(start),html.indexOf(end,html.indexOf(start))),context);
context.renderWifi=context.renderSyms=context.renderAps=context.symHintFor=context.radarSrcChanged=()=>{};
context.authShow=()=>{};
(async()=>{
 context.j=async()=>({chip:'esp32',features:{ticker:true,usage:true,radar:true,ha:true,theme:true,wireguard:true},clock:{},weatherStyle:'forecast',weatherAuto:true,weatherRotateSec:25});
 await context.loadConfig();
 assert.equal(nodes.weatherStyle.value,'forecast');
 assert.equal(nodes.weatherAuto.checked,true);
 assert.equal(nodes.weatherRotateSec.value,25);
 nodes.weatherStyle.value='stacked';nodes.weatherAuto.checked=false;nodes.weatherRotateSec.value='30';
 const config=context.collect();
 assert.equal(config.weatherStyle,'stacked');assert.equal(config.weatherAuto,false);assert.equal(config.weatherRotateSec,30);
 assert.match(html,/Open-Meteo/);assert.match(html,/Simple weather clock/);assert.match(html,/Three-day forecast/);
 console.log('Weather clock page tests passed');
})().catch(error=>{console.error(error);process.exitCode=1});
