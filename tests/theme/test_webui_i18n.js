// NM-TV-154 web UI: English default, complete Korean removal, and reversible Chinese text.
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const html=fs.readFileSync('src/webui.html','utf8');
assert.doesNotMatch(html,/[가-힯㄰-㆏]/,'Korean text must not remain in the web UI');
assert.match(html,/<select id="uiLang"[^>]*>[\s\S]*?<option value="en">English<\/option>[\s\S]*?<option value="zh">中文<\/option>/);

const nodes=[
 {nodeValue:'Status',parentNode:{nodeName:'BUTTON'}},
 {nodeValue:'Brightness: ',parentNode:{nodeName:'LABEL'}},
 {nodeValue:'ignored',parentNode:{nodeName:'SCRIPT'}}
];
const placeholder={nodeType:1,placeholder:'password',hasAttribute:()=>true};
const select={value:''};
const document={
 documentElement:{lang:''},
 getElementById:id=>id==='uiLang'?select:null,
 createTreeWalker:()=>{let i=0;return {nextNode:()=>nodes[i++]||null}},
 querySelectorAll:()=>[placeholder]
};
const stored={};
const context={document,localStorage:{getItem:k=>stored[k]||null,setItem:(k,v)=>stored[k]=v},setTimeout:()=>1,clearTimeout:()=>{}};
vm.createContext(context);
vm.runInContext(html.slice(html.indexOf('var UI_LANG='),html.indexOf('// null-safe field helpers')),context);
assert.equal(context.UI_LANG,'en','first visit defaults to English');
assert.equal(context.i18n('Status'),'Status');
context.setLanguage('zh',true);
assert.equal(nodes[0].nodeValue,'状态');
assert.equal(nodes[1].nodeValue,'亮度： ','surrounding whitespace is preserved');
assert.equal(placeholder.placeholder,'密码');
assert.equal(document.documentElement.lang,'zh-CN');
assert.equal(stored['smalltv-ui-lang'],'zh');
nodes[0].nodeValue='Firmware';context.translateDom(document);
assert.equal(nodes[0].nodeValue,'固件','dynamic English content is translated');
context.setLanguage('en',true);
assert.equal(nodes[0].nodeValue,'Firmware');
assert.equal(nodes[1].nodeValue,'Brightness: ');
assert.equal(placeholder.placeholder,'password');
assert.equal(document.documentElement.lang,'en');

// NM-TV-154 weather/NTP/signal additions must be reversible too.
const additions={
 'Defaults: Samsung (Naver), NVIDIA and Apple (Yahoo). No API key needed; 1-month line charts.':'默认：三星（Naver）、英伟达和苹果（Yahoo），无需 API Key，显示一个月折线图。',
 'Weather Display Units':'天气显示单位','Temperature unit':'温度单位','Wind speed unit':'风速单位',
 'Location name':'地点名称','Custom NTP server (optional)':'自定义 NTP 服务器（可选）',
 'Strong':'强','Medium':'中','Weak':'弱'
};
for(const [en,zh] of Object.entries(additions)){
 context.UI_LANG='zh';assert.equal(context.i18n(en),zh,en);
 context.UI_LANG='en';assert.equal(context.i18n(en),en,en);
}
vm.runInContext(html.slice(html.indexOf('var WIFI_RSSI_STRONG='),html.indexOf('function scan()')),context);
for(const [rssi,level] of [[-59,'Strong'],[-60,'Strong'],[-61,'Medium'],[-75,'Medium'],[-76,'Weak'],[-100,'Weak']])
 assert.equal(context.signalLevel(rssi),level,'RSSI '+rssi);
console.log('web UI i18n tests passed');
