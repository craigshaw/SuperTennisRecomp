-- MesenCE calibration adapter for the synthetic input-poll proof.

local config = INPUT_SYNC_CONFIG
if type(config) ~= "table" then
  error("INPUT_SYNC_CONFIG is required")
end

local output, open_error = io.open(config.output_path, "w")
if not output then
  error("unable to open poll log: " .. tostring(open_error))
end

local function write_line(line)
  output:write(line, "\n")
  output:flush()
end

local function fail(detail)
  write_line(
    string.format(
      '{"schema_version":1,"record_type":"error","detail":%q}',
      tostring(detail)
    )
  )
  output:close()
  emu.stop(64)
end

if type(emu.getInput) ~= "function" or
   type(emu.setInput) ~= "function" or
   type(emu.getMasterClock) ~= "function" or
   type(emu.read) ~= "function" or
   emu.eventType.inputPolled == nil or
   emu.eventType.endFrame == nil or
   emu.memType.snesWorkRam == nil then
  fail("required MesenCE input and timing APIs are unavailable")
  return
end

local buttons = {
  { name = "b", mask = 0x001 },
  { name = "y", mask = 0x002 },
  { name = "select", mask = 0x004 },
  { name = "start", mask = 0x008 },
  { name = "up", mask = 0x010 },
  { name = "down", mask = 0x020 },
  { name = "left", mask = 0x040 },
  { name = "right", mask = 0x080 },
  { name = "a", mask = 0x100 },
  { name = "x", mask = 0x200 },
  { name = "l", mask = 0x400 },
  { name = "r", mask = 0x800 },
}

local function input_from_mask(mask)
  local input = {}
  for _, button in ipairs(buttons) do
    input[button.name] = mask % (button.mask * 2) >= button.mask
  end
  return input
end

local function mask_from_input(input)
  local mask = 0
  for _, button in ipairs(buttons) do
    if input[button.name] == true then
      mask = mask + button.mask
    end
  end
  return mask
end

local poll_count = 0
local frame_count = 0
local finished = false

write_line(
  '{"schema_version":1,"record_type":"start",' ..
  '"poll_semantics":"state_applied_during_inputPolled",' ..
  '"frame_semantics":"completed_frames_since_script_start"}'
)

local function finish()
  if finished then return end
  finished = true

  local sample_bytes = emu.read(0, emu.memType.snesWorkRam) +
    emu.read(1, emu.memType.snesWorkRam) * 256
  local sample_count = math.floor(sample_bytes / 2)
  local words = {}
  for index = 0, sample_count - 1 do
    local address = 0x100 + index * 2
    words[#words + 1] = emu.read(address, emu.memType.snesWorkRam) +
      emu.read(address + 1, emu.memType.snesWorkRam) * 256
  end

  write_line(
    '{"schema_version":1,"record_type":"complete",' ..
    '"poll_count":' .. poll_count ..
    ',"frame_count":' .. frame_count ..
    ',"guest_sample_count":' .. sample_count ..
    ',"guest_words":[' .. table.concat(words, ",") .. "]}"
  )
  output:close()
  emu.stop(0)
end

local function on_input_polled(cpu_type)
  if finished or cpu_type ~= emu.cpuType.snes then return end
  if poll_count >= #config.masks then
    fail("Mesen requested an unexpected input poll after the supplied sequence")
    return
  end

  local supplied_mask = config.masks[poll_count + 1]
  local ok, detail = pcall(emu.setInput, input_from_mask(supplied_mask), 0, 0)
  if not ok then
    fail("unable to set controller 1 input: " .. tostring(detail))
    return
  end

  local observed = emu.getInput(0, 0)
  if type(observed) ~= "table" then
    fail("getInput did not return a controller state")
    return
  end
  local observed_mask = mask_from_input(observed)
  write_line(
    '{"schema_version":1,"record_type":"poll",' ..
    '"poll":' .. poll_count ..
    ',"frame":' .. frame_count ..
    ',"master_cycle":' .. tostring(emu.getMasterClock()) ..
    ',"supplied_mask":' .. supplied_mask ..
    ',"observed_mask":' .. observed_mask .. '}'
  )
  poll_count = poll_count + 1
end

emu.addEventCallback(on_input_polled, emu.eventType.inputPolled)

emu.addEventCallback(function(cpu_type)
  if finished or cpu_type ~= emu.cpuType.snes then return end
  frame_count = frame_count + 1
  local sample_bytes = emu.read(0, emu.memType.snesWorkRam) +
    emu.read(1, emu.memType.snesWorkRam) * 256
  if poll_count == #config.masks and
     math.floor(sample_bytes / 2) == #config.masks then
    finish()
  elseif frame_count >= config.max_frames then
    fail("frame budget expired before every guest joypad sample was stored")
  end
end, emu.eventType.endFrame)
