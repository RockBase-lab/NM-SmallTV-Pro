const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = 'src/webui-search.js';
assert.ok(fs.existsSync(path), 'shared searchable configuration module must exist');
const timers = new Map(); let serial = 0;
const scope = {console, AbortController, Promise, Date, JSON, Map, Set, Number,
  setTimeout(fn) {const id=++serial;timers.set(id,fn);return id},
  clearTimeout(id) {timers.delete(id)},
  localStorage:{getItem(){throw Error('blocked')},setItem(){throw Error('blocked')}}};
vm.createContext(scope); vm.runInContext(fs.readFileSync(path,'utf8'),scope);
const S=scope.SearchUI;
assert.ok(S,'module exports SearchUI');
const plain=value=>JSON.parse(JSON.stringify(value));
(async()=>{
  assert.ok(S.cities('上海').some(c=>c.timezone==='Asia/Shanghai'));
  assert.ok(S.cities('Seoul').some(c=>c.name.includes('Seoul')));
  assert.equal(S.cities('nonexistent-city-xyz').length,0);
  assert.equal(S.timezoneMatches('Asia/Shanghai','中国'),true);
  assert.equal(S.timezoneMatches('America/New_York','纽约'),true);
  assert.equal(S.timezoneMatches('Europe/London','Tokyo'),false);
  const directory=S.spotPairs({symbols:[
    {symbol:'BTCUSDT',baseAsset:'BTC',quoteAsset:'USDT',status:'TRADING',isSpotTradingAllowed:true},
    {symbol:'ETHUSDT',baseAsset:'ETH',quoteAsset:'USDT',status:'BREAK',isSpotTradingAllowed:true},
    {symbol:'FOOUSDT',baseAsset:'FOO',quoteAsset:'USDT',status:'TRADING',isSpotTradingAllowed:false}]});
  assert.deepEqual(plain(directory),[{symbol:'BTCUSDT',baseAsset:'BTC',quoteAsset:'USDT'}]);
  assert.equal(S.cryptoMatches(directory[0],'比特币'),true);
  assert.equal(S.cryptoMatches(directory[0],'btc/usdt'),true);
  assert.equal(S.cryptoMatches(directory[0],'eth'),false);
  assert.ok(S.stocks('三星').some(s=>s.symbol==='005930'&&s.source==='naver'));
  assert.ok(S.stocks('苹果').some(s=>s.symbol==='AAPL'&&s.source==='yahoo'));
  assert.ok(S.stocks('英伟达').some(s=>s.symbol==='NVDA.O'&&s.source==='naver'));
  assert.equal(S.merge([{symbol:'AAPL',source:'yahoo'}],[{symbol:'AAPL',source:'yahoo'},{symbol:'AAPL',source:'naver'}],x=>x.source+':'+x.symbol).length,2);
  assert.equal(S.readCache('pairs',1000),null,'blocked browser storage is harmless');
  S.writeCache('pairs',directory);
  const writes={};scope.localStorage={getItem:k=>writes[k]||null,setItem:(k,v)=>writes[k]=v};
  S.writeCache('pairs',directory);assert.deepEqual(plain(S.readCache('pairs',10000)),plain(directory));
  Object.keys(writes).forEach(k=>writes[k]=JSON.stringify({time:0,data:directory}));
  assert.equal(S.readCache('pairs',1000),null,'expired catalog is not current');
  let finishFirst,finishSecond;const states=[];
  const lookup=S.lookup({local:q=>[q],remote:q=>new Promise(resolve=>q==='first'?finishFirst=resolve:finishSecond=resolve),show:(items,state)=>states.push({items:plain(items),state})});
  const first=lookup.run('first');const second=lookup.run('second');await Promise.resolve();
  finishSecond(['new']);await second;finishFirst(['old']);await first;
  assert.deepEqual(states.at(-1).items,['second','new'],'late response cannot replace latest results');
  const third=lookup.run('first');await Promise.resolve();lookup.input('next');finishFirst(['obsolete']);await third;
  assert.equal(states.at(-1).items.includes('obsolete'),false,'typing invalidates in-flight work before debounce');
  const before=timers.size;lookup.input('拼',true);assert.equal(timers.size,before-1,'IME composition cancels pending debounce');
  const unavailable=[];await S.lookup({local:()=>['offline'],remote:async()=>{throw Error('offline')},show:(items,state)=>unavailable.push({items,state})}).run('x');
  assert.equal(unavailable.at(-1).state,'unavailable');assert.deepEqual(plain(unavailable.at(-1).items),['offline']);
  const empty=[];await S.lookup({local:()=>[],remote:async()=>[],show:(items,state)=>empty.push(state)}).run('x');
  assert.equal(empty.at(-1),'empty');
  console.log('Search aliases, Spot filtering, cache failures/expiry, debounce/IME and stale requests passed');
})().catch(e=>{console.error(e);process.exitCode=1});
