// A check on the sources: the firmware's own idea of who it is and where it
// updates from. The GitHub self-updater asks REPO_OWNER/REPO_NAME for the
// newest release and downloads UPDATE_ASSET from it, so an identity left on the
// upstream project (RockBase-lab/NM-SmallTV-Pro) makes "Update now" offer an upstream
// release and, installed, replace this image with one that lacks the fork's
// features. A merge from upstream can quietly restore exactly those lines, and
// a release asset renamed on one side only breaks the download, so both are
// checked here rather than discovered on the device.
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');

const root=process.env.SRC_ROOT||'src';
const repoRoot=path.dirname(path.resolve(root));
const workflowPath=process.env.WORKFLOW||path.join(repoRoot,'.github/workflows/build.yml');
const cfg=fs.readFileSync(path.join(root,'config.h'),'utf8');
const html=fs.readFileSync(path.join(root,'webui.html'),'utf8');
const workflow=fs.readFileSync(workflowPath,'utf8');

// `#define NAME "value"` pairs, so a value is read as the compiler sees it.
const defs={};
for (const m of cfg.matchAll(/^#define\s+(\w+)\s+"([^"]*)"/gm)) {
  if (!(m[1] in defs)) defs[m[1]]=m[2];
}
// All UPDATE_ASSET values, one per target in the #if defined() chain.
const assets=[...cfg.matchAll(/^\s*#define\s+UPDATE_ASSET\s+"([^"]+)"/gm)].map(m=>m[1]);

// 1. The update repo is the fork.
assert.equal(defs.REPO_OWNER,'RockBase-lab','REPO_OWNER is the fork, not the upstream');
assert.equal(defs.REPO_NAME,'NM-SmallTV-Pro','REPO_NAME is the fork, not the upstream');
assert.equal(defs.REPO_URL,'https://github.com/RockBase-lab/NM-SmallTV-Pro',
  'REPO_URL matches REPO_OWNER/REPO_NAME (the web UI links to it)');
assert.ok(defs.REPO_URL.endsWith('/'+defs.REPO_OWNER+'/'+defs.REPO_NAME),
  'REPO_URL is built from the same owner and name the API call uses');

// 2. The version the device compares releases against is this fork's current
//    release, and the README says the same, so an update cannot be offered
//    forever or be reported as already installed when it is not.
assert.match(defs.FW_VERSION,/^\d+\.\d+\.\d+$/,'FW_VERSION is a plain a.b.c string');
assert.equal(defs.FW_NAME,'nm-smalltv-pro');
assert.match(html,/<title>NMMiner SmallTV Pro<\/title>/);
assert.match(html,/<h1>NMMiner SmallTV Pro<\/h1>/);
const readme=fs.readFileSync(path.join(repoRoot,'README.md'),'utf8');
const readmeTitle=readme.split('\n')[0];
const rm=/\(v(\d+\.\d+\.\d+)\)/.exec(readmeTitle);
assert.ok(rm,'the README title carries the release version');
assert.equal(defs.FW_VERSION,rm[1],'FW_VERSION matches the release the README announces');
// The upstream this was forked from last shipped 3.6.0; that value here would
// make every fork release look like a pending update, forever.
assert.ok(defs.FW_VERSION!=='3.6.0','FW_VERSION is not the upstream version it was forked at');

// 3. The image this board pulls is the name CI publishes, and the workflow
//    stages it into dist/, so a tag build produces a release asset the device
//    can actually download.
const proAsset='nm-smalltv-pro-firmware-esp32-pro.bin';
assert.ok(cfg.includes('#define SMALLTV_ESP32_PRO')||cfg.includes('SMALLTV_ESP32_PRO'),
  'the pro target is selected in config.h');
assert.ok(assets.includes(proAsset),'the SmallTV Pro image is named '+proAsset);
const staged=new Set([...workflow.matchAll(/^\s*copy\s+\S+\s+dist\/(\S+)\s*$/gm)].map(m=>m[1]));
assert.ok(staged.has(proAsset),
  proAsset+' is staged into dist/ by '+workflowPath+'; a release without it leaves this device unable to self-update');
assert.ok(staged.has(proAsset.replace(/\.bin$/,'.factory.bin')),
  'the matching .factory.bin is staged too (clean install from the same release)');

// 4. No page in the web UI sends the device anywhere near the upstream repo:
//    the update and manual-upload hints and the footer all name the fork.
assert.ok(!html.includes('giovi321'),'src/webui.html links to no upstream repository');
assert.ok(html.includes('GitHub: RockBase-lab/NM-SmallTV-Pro'),'the footer names the fork');
assert.ok(/id="repoLink" href="https:\/\/github\.com\/RockBase-lab\/NM-SmallTV-Pro\/releases"/.test(html),
  'the update hint falls back to the fork\'s releases');
assert.ok(/id="relLink" href="https:\/\/github\.com\/RockBase-lab\/NM-SmallTV-Pro\/releases"/.test(html),
  'the manual-upload hint falls back to the fork\'s releases');
// The live page rewrites those links from the status reply, label included, so
// the footer cannot keep showing the compiled-in name after the firmware moves.
assert.ok(/fr\.textContent='GitHub: '\+s\.repo/.test(html),'the footer label follows the status reply');
assert.ok(/var rp=\$\(\'relLink\'\); if\(rp\)rp\.href=s\.repo\+'\/releases'/.test(html),
  'the manual-upload link follows the status reply');

console.log('update identity: all passed');
