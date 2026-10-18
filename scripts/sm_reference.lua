-- Mede a física do Super Metroid original no Mesen 2 (modo --testrunner).
-- Cada cenário carrega o save state, aplica uma sequência fixa de botões e grava, a cada quadro,
-- posição, velocidade e pose da Samus em CSV. Rode por scripts/sm_reference.ps1.
-- Endereços de RAM: src/memory.asm do disassembly (github.com/InsaneFirebat/sm_disassembly).

local STATE = os.getenv("MM_SM_STATE")
local OUT = os.getenv("MM_SM_OUT")
local ONLY = os.getenv("MM_SM_SCENARIO") -- opcional: roda só este cenário
if ONLY == "" then ONLY = nil end
local WR = emu.memType.snesWorkRam

local function r16(a) return emu.read16(a, WR, false) end
local function s16(a) return emu.read16(a, WR, true) end
local function w16(a, v) emu.write16(a, v, WR) end

-- Controles da Samus no save (padrão: X tiro, A pulo, B corrida). Nomes de botão do Mesen.
-- Os cenários usam: left right up down jump run shot.
local SETUP = {
  -- Dá a Morph Ball (bit 4 em equipados e coletados) para medir agachar e bola.
  morph = function() w16(0x09A2, r16(0x09A2) | 0x0004); w16(0x09A4, r16(0x09A4) | 0x0004) end,
}

-- {quadros, "botões"}: os botões ficam segurados por esse número de quadros.
-- PREP leva a Samus do topo da nave ao chão plano à direita (y 1211, x 1444 a 1691), virada
-- para a direita e parada em x ~1513. As medidas começam no passo seguinte.
local PREP = {{175, "right"}, {40, ""}}
local function after_prep(...)
  local steps = {PREP[1], PREP[2]}
  for _, step in ipairs({...}) do steps[#steps + 1] = step end
  return steps
end

local SCENARIOS = {
  {name = "walk_right", steps = {{900, "right"}}},
  {name = "run_left", steps = {{400, "left run"}}},
  {name = "walk_stop", steps = after_prep({30, "right"}, {40, ""})},
  {name = "turn", steps = after_prep({30, "left"})},
  {name = "jump_stand", steps = after_prep({100, "jump"})},
  {name = "spin_right", steps = after_prep({100, "right jump"})},
  {name = "jump_then_right", steps = after_prep({6, "jump"}, {94, "right jump"})},
  {name = "walk_spin", steps = after_prep({20, "right"}, {100, "right jump"})},
  -- A Samus gira até a parede da esquerda (x 37), encosta logo abaixo de um teto, aperta para longe e pula.
  {name = "wall_jump", steps = {{300, "left run"}, {15, "right"}, {10, "left"}, {14, "left jump"}, {2, "right"}, {80, "right jump"}}},
  {name = "run_jump", steps = {{115, "left run"}, {100, "left run jump"}}},
  {name = "fall_from_ship", steps = {{120, "right jump"}}},
  {name = "crouch_morph", setup = "morph",
   steps = after_prep({1, "down"}, {40, ""}, {1, "down"}, {40, ""}, {1, "up"}, {40, ""}, {1, "up"}, {40, ""})},
  {name = "down_hold", steps = after_prep({30, "down"}, {40, ""})},
  {name = "morph_jump", setup = "morph", steps = after_prep({1, "down"}, {10, ""}, {1, "down"}, {20, ""}, {40, "jump"})},
  {name = "morph_up_hold", setup = "morph", steps = after_prep({1, "down"}, {10, ""}, {1, "down"}, {20, ""}, {30, "up"}, {1, ""}, {10, "up"})},
  {name = "morph_air_jump", setup = "morph", steps = after_prep({10, "jump"}, {1, "down"}, {1, ""}, {1, "down"}, {8, ""}, {1, "jump"}, {60, ""})},
  {name = "spin_release", steps = after_prep({20, "right"}, {10, "right jump"}, {60, "jump"})},
  {name = "spin_reverse", steps = after_prep({20, "right"}, {10, "right jump"}, {60, "left jump"})},
  {name = "jump_release_dir", steps = after_prep({6, "jump"}, {20, "right jump"}, {60, "jump"})},
  {name = "run_jump_release", steps = {{115, "left run"}, {10, "left run jump"}, {60, "jump"}}},
  {name = "jump_reverse", steps = after_prep({6, "jump"}, {12, "right jump"}, {60, "left jump"})},
  {name = "turn_moving", steps = after_prep({30, "right"}, {40, "left"})},
  {name = "release_short", steps = after_prep({5, "right"}, {20, ""})},
  -- Pulos sem direção durante o deslize, empurrando a parede da esquerda e no meio da virada.
  {name = "slide_jump", steps = after_prep({30, "right"}, {2, ""}, {60, "jump"})},
  {name = "wall_push_jump", steps = {{300, "left run"}, {20, "left"}, {60, "left jump"}}},
  {name = "turn_jump", steps = after_prep({30, "right"}, {3, "left"}, {60, "left jump"})},
  -- Virada no ar partindo do repouso: pulo parado e, no meio dele, a direção oposta.
  {name = "air_turn_rest", steps = after_prep({12, "jump"}, {40, "left jump"})},
  {name = "fall_release", steps = {{62, "right"}, {30, ""}}},
  {name = "fall_reverse", steps = {{62, "right"}, {30, "left"}}},
  {name = "shoot_left", steps = after_prep({7, "left"}, {20, ""}, {1, "shot"}, {90, ""})},
  {name = "morph_roll", setup = "morph", steps = after_prep({1, "down"}, {10, ""}, {1, "down"}, {20, ""}, {40, "right"}, {30, ""})},
  {name = "morph_air", setup = "morph", steps = after_prep({10, "jump"}, {1, "down"}, {60, ""})},
  {name = "morph_air_twice", setup = "morph", steps = after_prep({10, "jump"}, {1, "down"}, {1, ""}, {1, "down"}, {60, ""})},
  {name = "shoot_single", steps = after_prep({1, "shot"}, {60, ""})},
  {name = "shoot_hold", steps = after_prep({60, "shot"})},
}
for n = 1, 30 do
  SCENARIOS[#SCENARIOS + 1] = {name = "jump_hold_" .. n, steps = after_prep({n, "jump"}, {90 - n, ""})}
end
-- Andando para a direita, a Samus sai da borda da nave e começa a cair no quadro 55.
for k = 0, 4 do
  SCENARIOS[#SCENARIOS + 1] = {name = "ledge_jump_" .. k, steps = {{54 + k, "right"}, {40, "right jump"}}}
end
local mash = {}
for _ = 1, 30 do mash[#mash + 1] = {1, "shot"}; mash[#mash + 1] = {1, ""} end
SCENARIOS[#SCENARIOS + 1] = {name = "shoot_mash", steps = after_prep(table.unpack(mash))}

local scenario_index = 0
local frame = 0
local inputs = {}
local step_of = {} -- passo de cada quadro, para separar a preparação das medidas
local pending_load = false
local load_ref = nil
local out = nil
local state_data = nil
local load_state

local function buttons_for(frame_number)
  return inputs[frame_number] or ""
end

local function expand(steps)
  local list, index = {}, {}
  for n, step in ipairs(steps) do
    for _ = 1, step[1] do list[#list + 1] = step[2]; index[#index + 1] = n end
  end
  return list, index
end

local function next_scenario()
  repeat scenario_index = scenario_index + 1
  until scenario_index > #SCENARIOS or not ONLY or SCENARIOS[scenario_index].name == ONLY
  if scenario_index > #SCENARIOS then
    out:close()
    emu.stop(0)
    return
  end
  inputs, step_of = expand(SCENARIOS[scenario_index].steps)
  frame = -1 -- o quadro em que o state é carregado não conta
  pending_load = true
  -- emu.loadSavestate só funciona dentro de um callback de execução da CPU principal.
  load_ref = emu.addMemoryCallback(load_state, emu.callbackType.exec, 0x000000, 0xFFFFFF)
end

load_state = function()
  if not pending_load then return end
  pending_load = false
  emu.removeMemoryCallback(load_ref, emu.callbackType.exec, 0x000000, 0xFFFFFF)
  emu.loadSavestate(state_data)
  local setup = SCENARIOS[scenario_index].setup
  if setup then SETUP[setup]() end
end

local MAP = {left = "left", right = "right", up = "up", down = "down", jump = "a", run = "b", shot = "x"}

emu.addEventCallback(function()
  if pending_load or frame < 0 then return end
  local pad = {a = false, b = false, x = false, y = false, l = false, r = false,
               up = false, down = false, left = false, right = false, start = false, select = false}
  for word in buttons_for(frame + 1):gmatch("%a+") do pad[MAP[word]] = true end
  emu.setInput(pad, 0)
end, emu.eventType.inputPolled)

emu.addEventCallback(function()
  if pending_load then return end
  frame = frame + 1
  if frame == 0 then return end
  local s = SCENARIOS[scenario_index]
  local projectiles = {}
  for i = 0, 4 do
    if r16(0x0C18 + i * 2) ~= 0 then
      projectiles[#projectiles + 1] = string.format("%.4f:%d", r16(0x0B64 + i * 2) + r16(0x0B8C + i * 2) / 65536,
                                                    r16(0x0B78 + i * 2))
    end
  end
  out:write(string.format("%s,%d,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%04X,%d,%d\n",
    s.name, frame, step_of[frame], (buttons_for(frame):gsub(" ", "+")),
    r16(0x0AF6), r16(0x0AF8), r16(0x0AFA), r16(0x0AFC),
    r16(0x0B2E), r16(0x0B2C), r16(0x0B36),
    r16(0x0B46), r16(0x0B48), r16(0x0B42), r16(0x0B44),
    r16(0x0A1C), emu.read(0x0A1F, WR, false), r16(0x0AFE), r16(0x0B00),
    r16(0x0CCE), r16(0x0CCC), table.concat(projectiles, " "), r16(0x008B),
    r16(0x0A96), r16(0x0A94)))
  if frame >= #inputs then next_scenario() end
end, emu.eventType.endFrame)

local f = assert(io.open(STATE, "rb"))
state_data = f:read("a")
f:close()
out = assert(io.open(OUT, "w"))
out:write("scenario,frame,step,buttons,x,xsub,y,ysub,vy,vysub,ydir,vx_base,vx_basesub,vx_extra,vx_extrasub," ..
          "pose,movement,xradius,yradius,projectiles,cooldown,shots,pad,anim_frame,anim_timer\n")
next_scenario()
