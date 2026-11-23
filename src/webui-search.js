var SearchUI = (function () {
  'use strict';
  var cityData = [
    ['Shanghai 上海 中国',31.2304,121.4737,'Asia/Shanghai','China 中国'],
    ['Beijing 北京 中国',39.9042,116.4074,'Asia/Shanghai','China 中国'],
    ['Shenzhen 深圳 中国',22.5431,114.0579,'Asia/Shanghai','China 中国'],
    ['Guangzhou 广州 中国',23.1291,113.2644,'Asia/Shanghai','China 中国'],
    ['Chengdu 成都 中国',30.5728,104.0668,'Asia/Shanghai','China 中国'],
    ['Hangzhou 杭州 中国',30.2741,120.1551,'Asia/Shanghai','China 中国'],
    ['Hong Kong 香港',22.3193,114.1694,'Asia/Hong_Kong','Hong Kong 香港'],
    ['Taipei 台北 臺北',25.033,121.5654,'Asia/Taipei','Taiwan 台湾'],
    ['Seoul 首尔 서울',37.5665,126.978,'Asia/Seoul','South Korea 韩国'],
    ['Busan 釜山 부산',35.1796,129.0756,'Asia/Seoul','South Korea 韩国'],
    ['Tokyo 东京 東京',35.6762,139.6503,'Asia/Tokyo','Japan 日本'],
    ['Osaka 大阪',34.6937,135.5023,'Asia/Tokyo','Japan 日本'],
    ['Singapore 新加坡',1.3521,103.8198,'Asia/Singapore','Singapore 新加坡'],
    ['London 伦敦',51.5074,-0.1278,'Europe/London','United Kingdom 英国'],
    ['Paris 巴黎',48.8566,2.3522,'Europe/Paris','France 法国'],
    ['Berlin 柏林',52.52,13.405,'Europe/Berlin','Germany 德国'],
    ['New York 纽约',40.7128,-74.006,'America/New_York','United States 美国'],
    ['Los Angeles 洛杉矶',34.0522,-118.2437,'America/Los_Angeles','United States 美国'],
    ['Sydney 悉尼',-33.8688,151.2093,'Australia/Sydney','Australia 澳大利亚']
  ];
  var stockData = [
    ['AAPL','Apple 苹果','US / NASDAQ','yahoo'],['NVDA','NVIDIA 英伟达','US / NASDAQ','yahoo'],
    ['MSFT','Microsoft 微软','US / NASDAQ','yahoo'],['TSLA','Tesla 特斯拉','US / NASDAQ','yahoo'],
    ['AMZN','Amazon 亚马逊','US / NASDAQ','yahoo'],['GOOGL','Alphabet 谷歌','US / NASDAQ','yahoo'],
    ['META','Meta 脸书','US / NASDAQ','yahoo'],['SPY','S&P 500 标普500','US / NYSE','yahoo'],
    ['005930','Samsung 三星 삼성전자','Korea / KRX','naver'],['000660','SK hynix 海力士','Korea / KRX','naver'],
    ['035420','NAVER','Korea / KRX','naver'],['AAPL.O','Apple 苹果','US / NASDAQ','naver'],
    ['NVDA.O','NVIDIA 英伟达','US / NASDAQ','naver'],['005930.KS','Samsung 三星','Korea / KRX','yahoo'],
    ['0700.HK','Tencent 腾讯','Hong Kong / HKEX','yahoo'],['9988.HK','Alibaba 阿里巴巴','Hong Kong / HKEX','yahoo'],
    ['600519.SS','Moutai 贵州茅台','China / Shanghai','yahoo'],['300750.SZ','CATL 宁德时代','China / Shenzhen','yahoo'],
    ['7203.T','Toyota 丰田','Japan / Tokyo','yahoo'],['6758.T','Sony 索尼','Japan / Tokyo','yahoo']
  ];
  var coinNames = {BTC:'Bitcoin 比特币',ETH:'Ethereum 以太坊',BNB:'BNB 币安币',SOL:'Solana 索拉纳',
    XRP:'XRP 瑞波币',DOGE:'Dogecoin 狗狗币',ADA:'Cardano 艾达币',AVAX:'Avalanche 雪崩',DOT:'Polkadot 波卡',LTC:'Litecoin 莱特币'};
  var recent = (readCache('cities',365*86400000)||[]).filter(validCity).slice(0,8);
  var pairInfo={},pairs=readCache('pairs',86400000,pairInfo),pairExpires=(pairInfo.time||0)+86400000,pairRequest,pairRetryAt=0;
  if(pairs)pairs=pairs.filter(validPair);
  var controllers = {};
  function text(en,zh) {return typeof UI_LANG!=='undefined' && UI_LANG==='zh' ? zh : en}
  function normalized(value) {return String(value||'').toLowerCase().replace(/[\s/_-]/g,'')}
  function matches(value,query) {return normalized(value).indexOf(normalized(query))>=0}
  function merge(a,b,key) {var seen=new Set();return a.concat(b).filter(function(x){var k=key(x);if(seen.has(k))return false;seen.add(k);return true})}
  function cityKey(c) {return c.name+':'+Number(c.latitude).toFixed(3)+':'+Number(c.longitude).toFixed(3)}
  function validCity(c) {return c&&typeof c.name==='string'&&c.name.length<150&&Number.isFinite(c.latitude)&&Math.abs(c.latitude)<=90&&Number.isFinite(c.longitude)&&Math.abs(c.longitude)<=180}
  function validPair(p) {return p&&typeof p.symbol==='string'&&/^[A-Z0-9]{5,23}$/.test(p.symbol)&&typeof p.baseAsset==='string'&&typeof p.quoteAsset==='string'}
  function cities(query) {
    var common=cityData.map(function(c){return {name:c[0],latitude:c[1],longitude:c[2],timezone:c[3],country:c[4]}});
    return merge(recent,common,cityKey).filter(function(c){return matches([c.name,c.admin1,c.country].join(' '),query)}).slice(0,12);
  }
  function rememberCity(city) {recent=merge([city],recent,cityKey).slice(0,8);writeCache('cities',recent)}
  function timezoneMatches(zone,query) {
    var aliases=cityData.filter(function(c){return c[3]===zone}).map(function(c){return c[0]+' '+c[4]}).join(' ');
    var extra={UTC:'协调世界时 世界标准时间',
      'Europe/Dublin':'Ireland 爱尔兰 都柏林','Europe/Lisbon':'Portugal 葡萄牙 里斯本','Europe/Rome':'Italy 意大利 罗马',
      'Europe/Madrid':'Spain 西班牙 马德里','Europe/Amsterdam':'Netherlands 荷兰 阿姆斯特丹','Europe/Brussels':'Belgium 比利时 布鲁塞尔',
      'Europe/Zurich':'Switzerland 瑞士 苏黎世','Europe/Vienna':'Austria 奥地利 维也纳','Europe/Warsaw':'Poland 波兰 华沙',
      'Europe/Prague':'Czech 捷克 布拉格','Europe/Stockholm':'Sweden 瑞典 斯德哥尔摩','Europe/Oslo':'Norway 挪威 奥斯陆',
      'Europe/Copenhagen':'Denmark 丹麦 哥本哈根','Europe/Athens':'Greece 希腊 雅典','Europe/Helsinki':'Finland 芬兰 赫尔辛基',
      'Europe/Bucharest':'Romania 罗马尼亚 布加勒斯特','Europe/Kyiv':'Ukraine 乌克兰 基辅','Europe/Istanbul':'Turkey 土耳其 伊斯坦布尔',
      'Europe/Moscow':'Russia 俄罗斯 莫斯科','America/Toronto':'Canada 加拿大 多伦多','America/Chicago':'USA 美国 芝加哥',
      'America/Denver':'USA 美国 丹佛','America/Phoenix':'USA 美国 凤凰城','America/Anchorage':'USA 美国 安克雷奇 阿拉斯加',
      'America/Sao_Paulo':'Brazil 巴西 圣保罗','America/Mexico_City':'Mexico 墨西哥 墨西哥城','America/Bogota':'Colombia 哥伦比亚 波哥大',
      'America/Argentina/Buenos_Aires':'Argentina 阿根廷 布宜诺斯艾利斯','Asia/Dubai':'UAE 阿联酋 迪拜',
      'Asia/Karachi':'Pakistan 巴基斯坦 卡拉奇','Asia/Kolkata':'India 印度 加尔各答','Asia/Dhaka':'Bangladesh 孟加拉 达卡',
      'Asia/Bangkok':'Thailand 泰国 曼谷','Asia/Jakarta':'Indonesia 印度尼西亚 雅加达','Australia/Perth':'Australia 澳大利亚 珀斯',
      'Australia/Adelaide':'Australia 澳大利亚 阿德莱德','Australia/Brisbane':'Australia 澳大利亚 布里斯班',
      'Pacific/Auckland':'New Zealand 新西兰 奥克兰','Pacific/Honolulu':'USA 美国 夏威夷 檀香山'};
    return matches(zone+' '+aliases+' '+(extra[zone]||''),query);
  }
  function stocks(query) {return stockData.map(function(s){return {symbol:s[0],name:s[1],market:s[2],source:s[3]}})
    .filter(function(s){return matches(s.symbol+' '+s.name+' '+s.market,query)}).slice(0,16)}
  function spotPairs(data) {return (Array.isArray(data.symbols)?data.symbols:[]).filter(function(s){return s.status==='TRADING'&&s.isSpotTradingAllowed===true&&/^[A-Z0-9]{5,23}$/.test(s.symbol)})
    .slice(0,8000).map(function(s){return {symbol:s.symbol,baseAsset:s.baseAsset,quoteAsset:s.quoteAsset}})}
  function cryptoMatches(pair,query) {return matches(pair.symbol+' '+(coinNames[pair.baseAsset]||'')+' '+pair.baseAsset+'/'+pair.quoteAsset,query)}
  function readCache(key,age,meta) {try {var item=JSON.parse(localStorage.getItem('smalltv-search-'+key));
    if(item&&Array.isArray(item.data)&&item.data.length<=8000&&item.time<=Date.now()&&Date.now()-item.time<age){if(meta)meta.time=item.time;return item.data}
    return null}catch(e){return null}}
  function writeCache(key,data) {try {localStorage.setItem('smalltv-search-'+key,JSON.stringify({time:Date.now(),data:data}))}catch(e){}}
  function request(url,timeout) {
    var abort=typeof AbortController!=='undefined'?new AbortController():null, timer;
    return Promise.race([fetch(url,abort?{signal:abort.signal}:{}).then(function(r){if(!r.ok)throw Error('HTTP '+r.status);return r.json()}),
      new Promise(function(resolve,reject){timer=setTimeout(function(){if(abort)abort.abort();reject(Error('timeout'))},timeout||8000)})])
      .finally(function(){clearTimeout(timer)});
  }
  function lookup(options) {
    var generation=0,timer;
    function run(query) {
      clearTimeout(timer);var id=++generation,local=options.local(query);options.show(local,'loading');
      if(!query.trim()){options.show(local,'idle');return Promise.resolve()}
      return Promise.resolve().then(function(){return options.remote(query)}).then(function(items){
        if(id!==generation)return;var result=options.merge?options.merge(local,items):local.concat(items);
        options.show(result,result.length?'ready':'empty');
      }).catch(function(){if(id===generation)options.show(local,'unavailable')});
    }
    function input(query,composing) {
      ++generation;clearTimeout(timer);options.show(options.local(query),'idle');
      if(!composing&&query.trim().length>=2)timer=setTimeout(function(){run(query)},350);
    }
    return {run:run,input:input};
  }
  function message(state) {
    return {loading:text('Searching online… Local results can be selected.','正在联网检索… 可先选择本地结果。'),
      ready:text('Choose a result, then Save settings.','请选择结果，然后保存设置。'),
      idle:text('Type at least 2 characters to search; choose a result, then save.','输入至少 2 个字符检索；选择结果后请保存。'),
      empty:text('No matches. Try another spelling or enter manually.','没有匹配结果，请换个名称或手动输入。'),
      unavailable:text('Online search unavailable. Use local results or enter manually.','在线检索暂不可用，请选择本地结果或手动输入。')}[state];
  }
  function bind(inputId,selectId,messageId,local,remote,label,pick,key,authoritative) {
    var input=$(inputId),select=$(selectId),hint=$(messageId);if(!input||!select||!hint)return;
    var results=[];
    var control=lookup({local:local,remote:remote,merge:function(a,b){return (authoritative?b:merge(a,b,key)).slice(0,24)},show:function(items,state){
      var previous=select.value,chosen=results[Number(previous)],chosenKey=previous!==''&&chosen?key(chosen):null;
      results=items;select.replaceChildren();var placeholder=document.createElement('option');placeholder.value='';
      placeholder.textContent=text('Choose a result…','请选择结果…');select.appendChild(placeholder);
      items.forEach(function(item,i){var option=document.createElement('option');option.value=String(i);option.textContent=label(item);select.appendChild(option)});
      var index=chosenKey===null?-1:items.findIndex(function(item){return key(item)===chosenKey});select.value=index<0?'':String(index);
      select.style.display=items.length?'block':'none';hint.textContent=message(state);
    }});
    input.addEventListener('input',function(e){control.input(input.value,e.isComposing)});
    input.addEventListener('compositionstart',function(){control.input(input.value,true)});
    input.addEventListener('compositionend',function(){control.input(input.value,false)});
    input.addEventListener('keydown',function(e){if(e.key==='Enter'&&!e.isComposing){e.preventDefault();control.run(input.value)}});
    select.onchange=function(){if(select.value!==''&&results[Number(select.value)])pick(results[Number(select.value)])};
    controllers[inputId]=control;control.input('',true);
  }
  function cityLabel(c) {return [c.name,c.admin1,c.country].filter(Boolean).join(', ')+' · '+Number(c.latitude).toFixed(4)+', '+Number(c.longitude).toFixed(4)+(c.timezone?' · '+c.timezone:'')}
  function cityRemote(q) {return request('https://geocoding-api.open-meteo.com/v1/search?name='+encodeURIComponent(q)+'&count=8&language='+text('en','zh')+'&format=json')
    .then(function(d){return (d.results||[]).filter(validCity)})}
  function cityPick(city,radar) {rememberCity(city);if(radar)radarSetCenter(city.latitude,city.longitude);
    else {setLoc(Number(city.latitude).toFixed(4),Number(city.longitude).toFixed(4),[city.name,city.admin1,city.country].filter(Boolean).join(', '),city.timezone);
      if(city.timezone&&!(city.timezone in TZMAP))toast(text('Location selected. Check the unsupported timezone in Display before saving.','地点已选择；该时区尚不支持，请在显示设置中核对后保存。'));}}
  function searchCity(id) {return controllers[id]?controllers[id].run(gv(id)):Promise.resolve()}
  function filterTimezone() {
    var select=$('tz');if(!select)return;var current=select.value,q=gv('tzQuery');
    var custom=typeof C!=='undefined'&&C.clock&&C.clock.tz,keys=Object.keys(TZMAP).filter(function(k){return k!==''});
    if(custom&&keys.indexOf(custom)<0)keys.push(custom);
    select.replaceChildren();keys.sort().filter(function(k){return k===current||timezoneMatches(k,q)}).forEach(function(k){
      var o=document.createElement('option');o.value=k;o.textContent=k;select.appendChild(o)});
    if(current===''){var utc=document.createElement('option');utc.value='';utc.textContent='UTC';select.appendChild(utc)}
    select.value=current;
  }
  function selectTimezone(zone) {
    var hint=$('timezoneMsg');
    if(zone&&Object.prototype.hasOwnProperty.call(TZMAP,zone)) {
      sv('tzQuery','');fillTz();sv('tz',zone);if(hint)hint.textContent=text('City timezone selected; save to apply.','已选择城市时区，保存后生效。');return true;
    }
    if(hint)hint.textContent=text('This city’s timezone has no built-in device rule. Check the timezone below before saving.','该城市时区暂无内置设备规则，请在保存前确认下面的时区。');
    return false;
  }
  function commonPairs(q) {if(Date.now()>=pairExpires)pairs=null;
    var list=pairs||Object.keys(coinNames).map(function(base){return {symbol:base+'USDT',baseAsset:base,quoteAsset:'USDT',preset:true}});
    return list.filter(function(pair){return cryptoMatches(pair,q)}).slice(0,24)}
  function loadPairs() {
    if(pairs&&Date.now()<pairExpires)return Promise.resolve(pairs);if(pairRequest)return pairRequest;if(Date.now()<pairRetryAt)return Promise.reject(Error('retry later'));
    pairRequest=request('https://data-api.binance.vision/api/v3/exchangeInfo?permissions=SPOT&symbolStatus=TRADING&showPermissionSets=false',12000).then(function(data){
      var active=spotPairs(data);if(!active.length)throw Error('no active pairs');pairs=active;pairExpires=Date.now()+86400000;writeCache('pairs',pairs);return pairs;
    }).catch(function(e){pairRetryAt=Date.now()+60000;throw e}).finally(function(){pairRequest=null});return pairRequest;
  }
  function shortName(name) {var out='';Array.from(name).some(function(c){if(unescape(encodeURIComponent(out+c)).length>19)return true;out+=c;return false});return out}
  function addSymbol(which,item) {presetAdd(which,item.symbol,shortName(item.name||item.baseAsset||item.symbol),item.source||'binance')}
  function marketControls(which) {
    var table=$(symTableId(which));if(!table)return;
    var box=document.createElement('div'),label=document.createElement('label'),input=document.createElement('input'),select=document.createElement('select'),hint=document.createElement('small');
    label.htmlFor='symbolQuery'+which;label.textContent=which===3?text('Find a Spot pair','检索现货交易对'):text('Find a stock or ETF','检索股票或 ETF');
    input.id=label.htmlFor;input.type='search';input.autocomplete='off';input.placeholder=which===3?'BTC / 比特币 / BTCUSDT':'Apple / 苹果 / AAPL';
    select.id='symbolResults'+which;select.setAttribute('aria-label',label.textContent);hint.id='symbolMessage'+which;hint.className='hint';hint.setAttribute('role','status');
    box.appendChild(label);box.appendChild(input);box.appendChild(select);box.appendChild(hint);table.parentNode.insertBefore(box,table);
    bind(input.id,select.id,hint.id,which===3?commonPairs:stocks,
      which===3?function(q){return loadPairs().then(function(all){return all.filter(function(pair){return cryptoMatches(pair,q)}).slice(0,24)})}:
      function(q){var aliases=stocks(q).filter(function(s){return s.source==='yahoo'&&s.name.split(' ').some(function(alias){return normalized(alias)===normalized(q)})});var query=aliases.length===1?aliases[0].symbol:q;
        return request('/api/search/stocks?q='+encodeURIComponent(query)).then(function(d){return (d.results||[]).filter(function(s){return s.source==='yahoo'&&typeof s.symbol==='string'&&s.symbol.length<24})})},
      which===3?function(p){return p.baseAsset+'/'+p.quoteAsset+' · '+(coinNames[p.baseAsset]||p.baseAsset)+' · Binance Spot'+(p.preset?text(' (preset; verify availability)','（预设，请确认可交易）'):'')}:
      function(s){return s.name+' · '+s.symbol+' · '+s.market+' · '+s.source},
      function(item){addSymbol(which,item)},function(s){return (s.source||'binance')+':'+s.symbol},which===3);
  }
  function init() {
    [['locQ','locRes','locMsg',false],['wLocQ','wLocRes','wLocMsg',false],['radarCityQ','radarCityRes','radarSearchMsg',true]].forEach(function(ids){
      bind(ids[0],ids[1],ids[2],cities,cityRemote,cityLabel,function(c){cityPick(c,ids[3])},cityKey)});
    [1,2,3].forEach(marketControls);var query=$('tzQuery');if(query)query.addEventListener('input',filterTimezone);
  }
  return {cities:cities,stocks:stocks,timezoneMatches:timezoneMatches,spotPairs:spotPairs,cryptoMatches:cryptoMatches,
    merge:merge,readCache:readCache,writeCache:writeCache,lookup:lookup,request:request,init:init,
    searchCity:searchCity,filterTimezone:filterTimezone,selectTimezone:selectTimezone};
}());
