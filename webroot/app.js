(() => {
'use strict';
const $ = id => document.getElementById(id);
const CONFIG = '/data/adb/simspoof.prop';
const FIELDS = ['model','brand','device','product','manufacturer','board','hardware','bootloader','fingerprint','build_id','build_display','security_patch','locale','country_iso','timezone','serial','soc_model','soc_manufacturer','baseband','network_operator','network_operator_name','sim_operator','sim_operator_name'];
let apps = [], selected = '';
function log(s){ $('output').textContent = String(s); }
function quote(s){ return "'" + String(s).replace(/'/g, "'\\''") + "'"; }
function bridge(command){
  return new Promise((resolve,reject)=>{
    const api = window.ksu || window.apatch || window.kpatch;
    if (!api || typeof api.exec !== 'function') return reject(new Error('No supported root shell bridge detected. Open this page from a manager WebUI that exposes ksu.exec (KernelSU) or a compatible exec API.'));
    let settled=false; const done=(a,b)=>{if(settled)return;settled=true;if(b)reject(new Error(String(b)));else resolve(typeof a==='string'?a:JSON.stringify(a??''));};
    try { const r=api.exec(command, (out,code)=>{if(typeof code==='number'&&code!==0)done(out,'shell exit '+code);else done(out);}); if(r&&typeof r.then==='function')r.then(v=>done(v)).catch(e=>done('',e)); else if(typeof r==='string')done(r); }
    catch(e){done('',e);}
  });
}
function validatePackage(p){return /^[A-Za-z0-9_]+(?:\.[A-Za-z0-9_]+)+$/.test(p);}
function b64(s){return btoa(unescape(encodeURIComponent(s)));}
function decode64(s){try{return decodeURIComponent(escape(atob(s)));}catch(_){return atob(s);}}
async function loadApps(){
  try {
    const raw=await bridge('cmd package list packages -3 2>/dev/null; cmd package list packages -s 2>/dev/null');
    const set=new Set((raw.match(/package:[A-Za-z0-9_.]+/g)||[]).map(x=>x.slice(8)).filter(validatePackage));
    apps=[...set].sort(); renderApps(); $('manager').textContent='Root bridge ready'; log(`Loaded ${apps.length} package IDs. Select a target app.`);
  } catch(e) { $('manager').textContent='Bridge unavailable'; log(e.message); }
}
function renderApps(){const q=$('appSearch').value.toLowerCase();const sel=$('package');const old=selected;sel.innerHTML='<option value="">Select package…</option>';apps.filter(p=>p.toLowerCase().includes(q)).forEach(p=>{const o=document.createElement('option');o.value=p;o.textContent=p;sel.appendChild(o)});if(old&&apps.includes(old))sel.value=old;}
function setPackage(p){selected=p;$('package').value=apps.includes(p)?p:'';$('manualPackage').value=p;$('profileState').textContent=p||'No package selected';}
function resetFields(){FIELDS.forEach(f=>$(f).value='');$('active').checked=false;$('allowed').checked=false;$('native').checked=true;}
function prefix(){if(!validatePackage(selected))throw new Error('Choose a valid package name first.');return 'app.'+selected+'.';}
async function loadProfile(){
  const p=prefix();
  const script=`if [ -r ${quote(CONFIG)} ]; then grep -F ${quote(p)} ${quote(CONFIG)} | sed 's/^[^=]*=//' | base64 2>/dev/null; else echo __NO_CONFIG__; fi`;
  try {
    const raw=(await bridge(script)).trim(); resetFields();
    if(raw==='__NO_CONFIG__'||!raw){log('No saved per-app profile found. This is normal for a new package.');return;}
    // Avoid depending on shell grep output order: request exact key/value pairs encoded as one payload.
    const keys=['active','allowed','hook_mode',...FIELDS];
    const cmd=`awk -F= -v p=${quote(p)} 'index($1,p)==1 { k=substr($1,length(p)+1); v=substr($0,index($0,"=")+1); print k "=" v }' ${quote(CONFIG)} | base64`;
    const payload=(await bridge(cmd)).trim(); if(!payload){log('No saved per-app profile found.');return;}
    const text=decode64(payload.replace(/\s/g,''));const map={};text.split(/\r?\n/).forEach(line=>{const i=line.indexOf('=');if(i>0)map[line.slice(0,i)]=line.slice(i+1)});
    $('active').checked=['true','1'].includes((map.active||'').toLowerCase());$('allowed').checked=['true','1'].includes((map.allowed||'').toLowerCase());$('native').checked=(map.hook_mode||'native')==='native';FIELDS.forEach(f=>$(f).value=map[f]||'');
    log(`Loaded ${Object.keys(map).length} keys for ${selected}. Active=${$('active').checked}, Allowed=${$('allowed').checked}, mode=${map.hook_mode||'unset'}.`);
  } catch(e){log('Profile load failed: '+e.message);}
}
async function saveProfile(disable=false){
  try {
    const p=prefix();
    const values={active:disable?'false':String($('active').checked),allowed:disable?'false':String($('allowed').checked),hook_mode:$('native').checked?'native':'lsposed',scope:'per_app'};
    FIELDS.forEach(f=>{const v=$(f).value.trim();if(v)values[f]=v;});
    const block=Object.entries(values).map(([k,v])=>`${p}${k}=${v.replace(/[\r\n]/g,' ')}`).join('\n')+'\n';
    const enc=b64(block);
    const script=`set -eu; CFG=${quote(CONFIG)}; TMP="${CONFIG}.tmp.$$"; mkdir -p /data/adb; touch "$CFG"; chmod 0600 "$CFG"; awk -v p=${quote(p)} 'index($0,p)!=1 {print}' "$CFG" > "$TMP"; printf %s ${quote(enc)} | base64 -d >> "$TMP"; chmod 0600 "$TMP"; mv "$TMP" "$CFG"; grep -F ${quote(p)} "$CFG" | wc -l`;
    const result=await bridge(script);log(`Saved ${Object.keys(values).length} fields for ${p}. Matching config lines: ${result.trim()}.\n${values.active==='true'&&values.allowed==='true'?'Profile is enabled; force-stop and reopen the target app.':'Profile remains inactive unless both Active and Allowed are checked.'}`);
  }catch(e){log('Save failed: '+e.message);}
}
$('refresh').addEventListener('click',loadApps);$('appSearch').addEventListener('input',renderApps);$('package').addEventListener('change',()=>{setPackage($('package').value);if(selected)loadProfile();});$('useManual').addEventListener('click',()=>{const p=$('manualPackage').value.trim();if(!validatePackage(p)){log('Invalid package name. Example: com.example.app');return;}setPackage(p);loadProfile();});$('load').addEventListener('click',loadProfile);$('save').addEventListener('click',()=>saveProfile(false));$('disable').addEventListener('click',()=>{if(!selected){log('Choose a package first.');return;}saveProfile(true);});
$('diagnose').addEventListener('click',async()=>{try{const r=await bridge(`echo '--- config ---'; ls -l ${quote(CONFIG)} 2>&1; echo '--- module ---'; ls -l /data/adb/modules/universal-samsung-spoof/zygisk/arm64-v8a.so 2>&1; echo '--- recent logs ---'; logcat -d -s UniversalSamsungSpoof:I '*:S' 2>/dev/null | tail -n 45`);log(r||'Diagnostics returned no output.');}catch(e){log('Diagnostics failed: '+e.message);}});
const api=window.ksu||window.apatch||window.kpatch;$('manager').textContent=api&&typeof api.exec==='function'?'Manager bridge detected':'Open in supported manager';
loadApps();
})();
