-- One visual reference capture from the Landing Site save; run with sm_reference.ps1 -Script.
local f = assert(io.open(os.getenv('MM_SM_STATE'), 'rb'))
local data = f:read('a'); f:close()
local loaded = false
local ref
ref = emu.addMemoryCallback(function()
 if loaded then return end
 loaded = true
 emu.removeMemoryCallback(ref, emu.callbackType.exec, 0, 0xFFFFFF)
 emu.loadSavestate(data)
end, emu.callbackType.exec, 0, 0xFFFFFF)
local ticks = 0
emu.addEventCallback(function()
 if not loaded then return end
 ticks = ticks + 1
 if ticks < 2 then return end
 local out = assert(io.open(os.getenv('MM_SM_OUT'), 'w'))
 local function dump(t, prefix)
  for k,v in pairs(t) do
   if type(v) == 'table' then dump(v,prefix..k..'.') else out:write(prefix..k..'='..tostring(v)..'\n') end
  end
 end
 dump(emu.getState(),'state.')
 dump(emu.memType,'mem.')
 out:close()
 for name,size in pairs({snesVideoRam=65536,snesCgRam=512,snesWorkRam=131072,snesSpriteRam=544}) do
  local raw=assert(io.open(os.getenv('MM_SM_OUT')..'.'..name,'wb'))
  for a=0,size-1 do raw:write(string.char(emu.read(a,emu.memType[name],false))) end
  raw:close()
 end
 local shot = emu.takeScreenshot()
 local sf = assert(io.open(os.getenv('MM_SM_OUT')..'.png','wb'))
 sf:write(shot); sf:close()
 emu.stop(0)
end, emu.eventType.endFrame)
