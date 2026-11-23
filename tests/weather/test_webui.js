// Weather location/units and custom NTP in the real page, using a tiny DOM.
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const html=fs.readFileSync('src/webui.html','utf8');

function node(){return {value:'',checked:false,textContent:'',innerHTML:'',placeholder:'',style:{},children:[],
 appendChild(child){this.children.push(child)},replaceChildren(){this.children=[]},remove(){},closest(){return {remove(){}}},querySelector(){return null}}}
const nodes=new Proxy({}, {get:(object,id)=>object[id]||(object[id]=node())});
const document={getElementById:id=>nodes[id],createElement:node,querySelector:()=>null,querySelectorAll:()=>[]};
const context={document,console,Math,JSON,Number,parseFloat,parseInt,isNaN,Promise,setTimeout,clearTimeout,
 C:{},collectWifi:()=>[],loadStatus:()=>{},loadThemes:()=>{},toast:()=>{},confirm:()=>true};
vm.createContext(context);
vm.runInContext(html.slice(html.indexOf('function $(id)'),html.indexOf('// tabs')),context);
vm.runInContext(html.slice(html.indexOf('// field groups'),html.indexOf('function loadConfig()')),context);
vm.runInContext(html.slice(html.indexOf('function loadConfig()'),html.indexOf('function esc(')),context);
vm.runInContext(html.slice(html.indexOf('function authProblem()'),html.indexOf('// password end')),context);
vm.runInContext(html.slice(html.indexOf('function collect()'),html.indexOf('// wifi networks')),context);
// Keep this test focused on the shared settings fields, not ticker/radar tables.
context.renderWifi=()=>{};context.renderSyms=()=>{};context.renderAps=()=>{};
context.symHintFor=()=>{};context.radarSrcChanged=()=>{};
context.toast=()=>{};

async function main(){
 const config={chip:'esp32',features:{ticker:true,usage:true,radar:true,ha:true,theme:true,wireguard:true},
  brightness:80,rotation:0,weatherTempUnit:'f',weatherWindUnit:'mph',
  clock:{tz:'Asia/Shanghai',lang:'en',ntpServer:'ntp.example.com',locationName:'Daejeon, KR',lat:36.35,lon:127.38}};
 context.j=async()=>config;
 await context.loadConfig();
 config.clock.lat=0;config.clock.lon=-0.1278;
 await context.loadConfig();
 assert.equal(nodes.wLat.value,0,'equator must not become the default city');
 assert.equal(nodes.locLat.value,0,'zero is a valid coordinate');
 assert.equal(nodes.weatherTempUnit.value,'f');
 assert.equal(nodes.weatherWindUnit.value,'mph');
 assert.equal(nodes.ntpServer.value,'ntp.example.com');
 assert.equal(nodes.locName.value,'Daejeon, KR');
 assert.equal(nodes.wLocName.value,'Daejeon, KR');

 // A direct edit in the Weather card wins over the old hidden Clock value.
 nodes.weatherTempUnit.value='c';nodes.weatherWindUnit.value='ms';
 nodes.ntpServer.value='  time.example.net  ';nodes.wLocName.value='Seoul, KR';
 const saved=context.collect();
 assert.equal(saved.weatherTempUnit,'c');assert.equal(saved.weatherWindUnit,'ms');
 assert.equal(saved.clock.ntpServer,'time.example.net');
 assert.equal(saved.clock.locationName,'Seoul, KR');

 // Both location pickers feed both the clock and weather forms.
 context.setLoc('52.5244','13.4105','Berlin, Berlin, Germany');
 assert.equal(Number(nodes.wLat.value),52.5244,'Display city selection updates Weather coordinates');
 assert.equal(nodes.locName.value,'Berlin, Berlin, Germany');
 assert.equal(nodes.wLocName.value,'Berlin, Berlin, Germany');
 nodes.wLocName.value='manual';context.weatherCoordEdited();
 assert.equal(nodes.wLocName.value,'');assert.equal(nodes.locName.value,'');
 nodes.wLat.value='35.68';nodes.wLon.value='139.69';context.weatherCoordEdited(true);
 assert.equal(nodes.locLat.value,'35.68');assert.equal(nodes.locLon.value,'139.69');
 nodes.locLat.value='0';nodes.locLon.value='15';context.weatherCoordEdited(false);
 assert.equal(nodes.wLat.value,'0');assert.equal(nodes.wLon.value,'15');
 context.setLoc(37.56,126.97,'Seoul','Asia/Seoul');
 assert.equal(nodes.tz.value,'Asia/Seoul','known city timezone follows location selection');
 console.log('weather web UI tests passed');
}
main().catch(error=>{console.error(error);process.exitCode=1});
