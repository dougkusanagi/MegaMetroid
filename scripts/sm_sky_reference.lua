-- Read original BG2 scroll by scanline; rendering registers are never changed.
-- High/upper probes freeze position and velocity; documented in the cache.
local state=assert(io.open(os.getenv('MM_SM_STATE'),'rb'));local data=state:read('a');state:close()
local WR=emu.memType.snesWorkRam
local function r(a)return emu.read16(a,WR,false)end
local out=assert(io.open(os.getenv('MM_SM_OUT'),'w'))
out:write('scenario,frame,camera_y,bg2_lines\n')
local loaded=false;local ticks=-1;local probe=os.getenv("MM_SM_SCENARIO")=="upper" and 4 or 1;local lines={};local prev=-1
local names={'ship','ground','high','upper'}
local function load()
 local ref;ref=emu.addMemoryCallback(function()
  emu.removeMemoryCallback(ref,emu.callbackType.exec,0,0xFFFFFF)
  emu.loadSavestate(data);loaded=true;ticks=-1;lines={};prev=-1
 end,emu.callbackType.exec,0,0xFFFFFF)
end
emu.addEventCallback(function()
 if not loaded then return end
 emu.setInput({right=probe==2 and ticks<175},0)
end,emu.eventType.inputPolled)
emu.addMemoryCallback(function()
 if not loaded then return end
 local target=probe==1 and 3 or probe==2 and 215 or 80
 if ticks<target-1 then return end
 local s=emu.getState();local line=s['ppu.scanline']
 if line~=prev and line>=0 and line<225 then
  lines[#lines+1]=line..':'..s['ppu.layers[1].vscroll']..':'..s['ppu.layers[1].hscroll'];prev=line
 end
end,emu.callbackType.exec,0,0xFFFFFF)
emu.addEventCallback(function()
 if not loaded then return end
 ticks=ticks+1
 if probe>=3 then
  emu.write16(0x0AFA,probe==3 and 800 or 155,WR);
  if probe==4 then emu.write16(0x0AF6,2150,WR) end; emu.write16(0x0B2E,0,WR);emu.write16(0x0B2C,0,WR)
 end
 local target=probe==1 and 3 or probe==2 and 215 or 80
 if ticks>=target then
  for name,size in pairs({snesVideoRam=65536,snesCgRam=512}) do
   local raw=assert(io.open(os.getenv('MM_SM_OUT')..'.'..names[probe]..'.'..name,'wb'))
   for a=0,size-1 do raw:write(string.char(emu.read(a,emu.memType[name],false))) end;raw:close()
  end
  out:write(names[probe]..',1,'..r(0x0915)..','..table.concat(lines,' ')..'\n')
  local f=assert(io.open(os.getenv('MM_SM_OUT')..'.'..names[probe]..'.png','wb'));f:write(emu.takeScreenshot());f:close()
  loaded=false;probe=probe+1
  if probe>4 then out:close();emu.stop(0) else load() end
 end
 lines={};prev=-1
end,emu.eventType.endFrame)
load()
