const fs=require('node:fs');
const {minify}=require('terser');
(async()=>{
  const html=fs.readFileSync(0,'utf8');
  const scripts=[...html.matchAll(/<script>([\s\S]*?)<\/script>/g)];
  let out=html;
  for(const script of scripts){
    const result=await minify(script[1],{ecma:2018,toplevel:false,mangle:{toplevel:false},format:{comments:false,inline_script:true}});
    out=out.replace(script[0],()=>'<script>'+result.code+'</script>');
  }
  process.stdout.write(out.replace(/<!--(?!\[if)[\s\S]*?-->/g,'').replace(/^[ \t]+/gm,'').replace(/\n{2,}/g,'\n'));
})().catch(error=>{console.error(error);process.exitCode=1});
