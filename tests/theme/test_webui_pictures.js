// NM-TV-154 Pictures: only picture packages are listed, selection updates the
// device immediately, cadence is clamped, and the visible text is bilingual.
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const html=fs.readFileSync(process.env.WEBUI_HTML||'src/webui.html','utf8');
const between=(a,b)=>{const i=html.indexOf(a),k=html.indexOf(b,i);assert.ok(i>=0&&k>i,'page text not found: '+a);return html.slice(i,k)};

function makeSelect(id){
 const options=[];
 return {id,options,value:'',disabled:false,
  set textContent(v){assert.equal(v,'');options.length=0;this.value=''},
  appendChild(o){options.push(o);if(!this.value)this.value=o.value}};
}
function makePage(){
 const els={};
 for(const id of ['themeSelect','pictureSelect'])els[id]=makeSelect(id);
 for(const id of ['themeUseBtn','pictureUseBtn'])els[id]={disabled:false};
 for(const id of ['themeDetails','pictureDetails','themeStatus','pictureStatus'])els[id]={textContent:''};
 els.pictureAuto={checked:true};els.pictureRotateSec={value:'10'};els.mode={value:'timer'};
 const document={getElementById:id=>els[id]||null,createElement:tag=>({tag,value:'',textContent:''})};
 const requests=[];
 const context={document,C:{pictureId:'ultra-02'},console,requests,$:id=>els[id]||null,
  gv:id=>els[id]&&els[id].value!=null?els[id].value:'',gc:id=>!!(els[id]&&els[id].checked),
  sv:(id,v)=>{if(els[id])els[id].value=v},numIn:(v,fallback,min,max)=>{const n=parseInt(v,10);return Number.isFinite(n)?Math.max(min,Math.min(max,n)):fallback},
  toast:()=>{},modeChanged:()=>{},
  j:path=>{assert.equal(path,'/api/themes');return Promise.resolve({selected:'classic-24',freeBytes:28148,themes:[
   {id:'classic-24',name:'Classic 24',version:'1.0.0',bytes:416,valid:true},
   {id:'classic-12',name:'Classic 12',version:'1.0.0',bytes:422,valid:true},
   {id:'pixel-motion',name:'Pixel Motion',version:'1.0.0',bytes:4653,valid:true},
   {id:'ultra-01',name:'Ultra Picture 01',version:'1.0.0',bytes:21672,valid:true},
   {id:'ultra-02',name:'Ultra Picture 02',version:'1.0.0',bytes:19106,valid:true},
   {id:'picture-custom',name:'Custom',version:'1.0.0',bytes:1000,valid:false,error:'test'}
  ]})},
  fetch:(path,options)=>{requests.push({path,options,body:JSON.parse(options.body)});return Promise.resolve({ok:true,json:()=>Promise.resolve({id:options.body})})}
 };
 vm.createContext(context);
 vm.runInContext(between('// Theme and picture package selection','function deleteTheme()'),context);
 return {context,els,requests};
}

(async()=>{
 const {context,els,requests}=makePage();
 await context.loadThemes();
 assert.deepEqual(Array.from(els.pictureSelect.options,o=>o.value),
  ['pixel-motion','ultra-01','ultra-02','picture-custom'],'clock faces must not enter Pictures: '+JSON.stringify({entries:context.pictureEntries,themeOptions:els.themeSelect.options,pictureOptions:els.pictureSelect.options}));
 assert.equal(els.pictureSelect.value,'ultra-02','saved picture is selected');
 assert.equal(els.pictureUseBtn.disabled,false);
 assert.equal(els.pictureStatus.textContent,'4 pictures · 27 KiB free');

 els.pictureSelect.value='pixel-motion';els.pictureAuto.checked=false;els.pictureRotateSec.value='17';
 await context.selectPicture();
 assert.equal(requests.length,1);assert.equal(requests[0].path,'/api/pictures/select');
 assert.deepEqual(requests[0].body,{id:'pixel-motion',pictureAuto:false,pictureRotateSec:17});
 assert.equal(context.C.mode,'pictures');assert.equal(context.C.pictureId,'pixel-motion');
 assert.equal(context.C.pictureAuto,false);assert.equal(context.C.pictureRotateSec,17);
 assert.equal(els.mode.value,'pictures','selection switches the device to Pictures');

 els.pictureRotateSec.value='1';await context.selectPicture();
 assert.equal(requests[1].body.pictureRotateSec,2,'minimum cadence is enforced');
 els.pictureSelect.value='picture-custom';context.pictureChoiceChanged();
 assert.equal(els.pictureUseBtn.disabled,true,'invalid packages cannot be shown');
 const before=requests.length;await context.selectPicture();assert.equal(requests.length,before);

 const i18nContext={localStorage:{getItem:()=>null}};
 vm.createContext(i18nContext);
 vm.runInContext(between('var UI_LANG=','// null-safe field helpers'),i18nContext);
 assert.equal(i18nContext.UI_LANG,'en');assert.equal(i18nContext.i18n('Pictures'),'Pictures');
 i18nContext.UI_LANG='zh';assert.equal(i18nContext.i18n('Pictures'),'图片屏保');
 assert.equal(i18nContext.i18n('Auto-rotate pictures'),'自动轮播图片');
 assert.equal(i18nContext.i18n('Switch picture every (seconds)'),'图片切换间隔（秒）');
 assert.match(html,/carouselPictures/);assert.match(html,/pictures:'pictures'/);
 assert.equal(context.pictureError('layers[0].loop: expected true or false'),'Outdated animation package. Install the full-color picture library.');
 assert.equal(context.pictureError('Invalid image'),'Picture package cannot be loaded. Reinstall it.');
 assert.equal(i18nContext.i18n(context.pictureError('layers[0].loop')),'动画包版本过旧，请安装新版全彩图片库。');
 console.log('Pictures web UI tests passed');
})().catch(e=>{console.error(e);process.exitCode=1});
