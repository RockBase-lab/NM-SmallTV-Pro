// Exercise the real market-preset, Crypto-row and collect() JavaScript with a
// small DOM stand-in. Crypto saves must always use Binance, uppercase pairs,
// and retain both the per-pair rotation and the app-carousel flag.
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const html=fs.readFileSync(process.env.WEBUI_HTML||'src/webui.html','utf8');
const between=(a,b)=>{const i=html.indexOf(a),k=html.indexOf(b,i);assert.ok(i>=0&&k>i,'page text not found: '+a);return html.slice(i,k)};
const helpers=between('function $(id)','// sun begin')+between('function numIn(','// sun end');
const featureCode=between('var MODEOPT=','function modeChanged()');
const tickerVars=between('var T_TEXT=','// IANA -> POSIX');
const collectCode=between('function collect(){','function saveAll(){');
const escCode=between('function esc(s)','function symHintFor');
const symbolCode=between('// symbols','// airports');

function input(value){return {value:value==null?'':String(value),checked:false}}
function row(){
 const fields={'.s':input(),'.n':input(),'.src':input(),'.q':input(),'.c':input()};
 let markup='';
 return {className:'',
  set innerHTML(v){
   markup=v;
   // The real row template puts symbol/name/qty/cost in value attributes.
   for(const cls of ['s','n','q','c']){
    const m=new RegExp('class="'+cls+'"[^>]*value="([^"]*)"').exec(v);
    fields['.'+cls].value=m?m[1]:'';
   }
  },get innerHTML(){return markup},
  querySelector:sel=>fields[sel]||null};
}
function table(){
 const rows=[];
 return {rows,
  set innerHTML(v){if(v==='')rows.length=0},get innerHTML(){return ''},
  appendChild(r){rows.push(r)},get lastChild(){return rows[rows.length-1]}};
}
function makePage(){
 const els={};
 const add=(id,props)=>{els[id]=Object.assign({id,value:'',checked:false,style:{}},props)};
 for(const id of ['symTable','symTable2','symTable3'])add(id,table());
 add('crypto',{});add('mode',{value:'crypto'});add('carouselCrypto',{checked:true});
 add('carouselMin',{value:'1'});add('brightness',{value:'80'});add('rotation',{value:'0'});
 add('hostname',{value:'smalltv-test'});add('apSsid',{value:'SmallTV-Test'});
 add('marketPreset_1',{value:'005930.KS|Samsung Elec'});
 add('cryptoPreset',{value:'ETHUSDT|Ethereum'});
 const values={webhookUrl_3:'',range_3:'5m',chartStyle_3:'candle',rotateSec_3:'17',pollSec_3:'45',points_3:'30',colorInverted_3:'false'};
 for(const [id,value] of Object.entries(values))add(id,{value});
 for(const id of ['showName_3','showPrice_3','showChange_3','showChart_3','showRangeLabel_3','showUpdatedAgo_3','showPageDots_3','showPortfolio_3'])add(id,{checked:true});
 const document={
  getElementById:id=>els[id]||null,
  createElement:tag=>{assert.equal(tag,'tr');return row()},
  querySelector:()=>null,
  querySelectorAll:sel=>{
   const m=/^#(symTable3|symTable2|symTable) tr$/.exec(sel);
   return m?els[m[1]].rows:[];
  }
 };
 const ctx={document,C:{},parseInt,parseFloat,Object,Math,collectWifi:()=>[],toast:()=>{},symHintFor:()=>{}};
 vm.createContext(ctx);
 vm.runInContext(helpers+featureCode+tickerVars+escCode+collectCode+symbolCode,ctx);
 return {ctx,els};
}

const {ctx,els}=makePage();
ctx.renderSyms([],1);
ctx.addMarketPreset(1);
assert.equal(els.symTable.rows.length,1);
assert.equal(els.symTable.rows[0].querySelector('.s').value,'005930.KS');
assert.equal(els.symTable.rows[0].querySelector('.n').value,'Samsung Elec');
assert.equal(els.symTable.rows[0].querySelector('.src').value,'yahoo');

ctx.renderSyms([{symbol:'btcusdt',name:'Bitcoin',source:'binance',qty:1.25,cost:50000}],3);
assert.equal(els.symTable3.rows.length,1);
assert.equal(els.symTable3.rows[0].querySelector('.src').value,'binance');
assert.equal(els.symTable3.rows[0].querySelector('.q').value,'1.25');
ctx.addCryptoPreset();
assert.equal(els.symTable3.rows.length,2);
assert.equal(els.symTable3.rows[1].querySelector('.s').value,'ETHUSDT');
assert.equal(els.symTable3.rows[1].querySelector('.src').value,'binance');
// Even a stale or manually altered source selector cannot switch this page's API.
els.symTable3.rows[1].querySelector('.s').value='ethusdt';
els.symTable3.rows[1].querySelector('.src').value='yahoo';

const out=ctx.collect();
assert.equal(out.mode,'crypto');
assert.equal(out.carouselCrypto,true);
assert.equal(out.crypto.range,'5m');
assert.equal(out.crypto.chartStyle,'candle');
assert.equal(out.crypto.rotateSec,17);
assert.equal(out.crypto.pollSec,45);
assert.equal(out.crypto.points,30);
assert.equal(out.crypto.changeOnRange,true);
assert.deepEqual(Array.from(out.crypto.symbols,s=>({symbol:s.symbol,source:s.source})),[
 {symbol:'BTCUSDT',source:'binance'},
 {symbol:'ETHUSDT',source:'binance'}
]);
assert.equal(out.crypto.symbols[0].qty,1.25);
assert.equal(out.crypto.symbols[0].cost,50000);
console.log('market preset and crypto page tests passed');
