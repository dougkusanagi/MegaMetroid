-- Captura a abertura de uma porta azul no Super Metroid original (Mesen 2, --testrunner).
-- A Samus corre para a esquerda até a porta fechada do Landing Site (mesma corrida do cenário
-- wall_open), atira uma vez e o script grava uma captura de tela por quadro, além de uma linha de
-- CSV com a posição da Samus e os projéteis. Rode por scripts/sm_reference.ps1 -Script.
local STATE = os.getenv("MM_SM_STATE")
local OUT = os.getenv("MM_SM_OUT")
local WR = emu.memType.snesWorkRam
local function r16(a) return emu.read16(a, WR, false) end

local RUN, SHOT_AT, TOTAL = 300, 316, 396 -- corre 300 quadros, atira no 316, observa até o 396
local loaded, load_ref = false, nil
local f = assert(io.open(STATE, "rb")); local state_data = f:read("a"); f:close()
local csv = assert(io.open(OUT, "w"))
csv:write("frame,buttons,samus_x,samus_y,projectiles,shots\n")

load_ref = emu.addMemoryCallback(function()
  if loaded then return end
  loaded = true
  emu.removeMemoryCallback(load_ref, emu.callbackType.exec, 0x000000, 0xFFFFFF)
  emu.loadSavestate(state_data)
end, emu.callbackType.exec, 0x000000, 0xFFFFFF)

local frame = -1
-- MM_DOOR_HOLD=1: depois do tiro continua apertando para a esquerda, para ver em que quadro a
-- tampa deixa de bloquear a Samus e quando a transição de sala começa.
local HOLD = os.getenv("MM_DOOR_HOLD") == "1"
local function buttons(n)
  if n <= RUN then return "left run" end
  if n == SHOT_AT then return "left shot" end
  return (n < SHOT_AT or HOLD) and "left" or ""
end

emu.addEventCallback(function()
  if not loaded or frame < 0 then return end
  local pad = {a = false, b = false, x = false, y = false, l = false, r = false,
               up = false, down = false, left = false, right = false, start = false, select = false}
  local map = {left = "left", run = "b", shot = "x"}
  for word in buttons(frame + 1):gmatch("%a+") do pad[map[word]] = true end
  emu.setInput(pad, 0)
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
  if not loaded then return end
  frame = frame + 1
  if frame == 0 then return end
  local live = {}
  for i = 0, 4 do
    if r16(0x0C18 + i * 2) ~= 0 then
      live[#live + 1] = string.format("%.2f:%.2f", r16(0x0B64 + i * 2), r16(0x0B78 + i * 2))
    end
  end
  csv:write(string.format("%d,%s,%d,%d,%s,%d\n", frame, (buttons(frame):gsub(" ", "+")),
    r16(0x0AF6), r16(0x0AFA), table.concat(live, " "), r16(0x008B)))
  if frame >= SHOT_AT - 4 then
    local png = assert(io.open(string.format("%s.f%03d.png", OUT, frame), "wb"))
    png:write(emu.takeScreenshot()); png:close()
  end
  if frame >= TOTAL then csv:close(); emu.stop(0) end
end, emu.eventType.endFrame)
