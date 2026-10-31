#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import hashlib,json,pathlib,sys,zipfile
root=pathlib.Path(__file__).resolve().parent.parent
out=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else root/'build').resolve()
manifest=json.loads((root/'azmod.json').read_text())
files={'LICENSE':root/'LICENSE','azmod.json':root/'azmod.json','icon.png':root/'icon.png','teof-sound-rec.so':out/'teof-sound-rec.so'}
out.mkdir(exist_ok=True,parents=True);target=out/f"teof-sound-rec-{manifest['version']}.azmod"
with zipfile.ZipFile(target,'w',compression=zipfile.ZIP_DEFLATED) as z:
 for name,p in files.items():
  info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;z.writestr(info,p.read_bytes())
print(json.dumps({'file':target.name,'sha256':hashlib.sha256(target.read_bytes()).hexdigest(),'size':target.stat().st_size},indent=2))
