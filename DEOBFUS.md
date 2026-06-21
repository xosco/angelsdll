# Deobfuscation Analysis: `pnevma.lua`

**Tool used:** [LuaObfuscator.com](https://luaobfuscator.com) (Alpha 0.10.9)  
**Target:** Roblox vehicle suspension controller — "KryptonZRP"  
**Repository:** `C:\Users\Admin\Downloads\lua\`

---

## 1. Reconnaissance

### 1.1 Initial Inspection

The obfuscated file consists of 12 lines:

- **Lines 1–10:** ASCII art banner and obfuscator credit (`--[[ ... ]]--`)
- **Line 11:** Empty
- **Line 12:** ~78 KB of heavily transformed Lua code — a single expression comprising variable bindings, inner function definitions, and the obfuscated payload

The first 10 lines provide an immediate signature:

```
--[[
 .____                  ________ ___.    _____                           __
 |    |    __ _______   \_____  \\_ |___/ ____\_ __  ______ ____ _____ _/  |_  ___________
 |    |   |  |  \__  \   /   |   \| __ \   __\  |  \/  ___// ___\\__  \\   __\/  _ \_  __ \
 |    |___|  |  // __ \_/    |    \ \_\ \  | |  |  /\___ \\  \___ / __ \|  | (  <_> )  | \/
 |_______ \____/(____  /\_______  /___  /__| |____//____  >\___  >____  /__|  \____/|__|
         \/          \/         \/    \/                \/     \/     \/
          \_Welcome to LuaObfuscator.com   (Alpha 0.10.9) ~  Much Love, Ferib
]]--
```

**Attribution:** The file was obfuscated by **Ferib's LuaObfuscator.com**, version Alpha 0.10.9. This immediately tells us the obfuscation strategy: string encoding + custom bytecode VM (v28/v29 pattern typical of this tool).

### 1.2 Structural Overview

After stripping comments, the executable code on line 12 follows this architecture:

```
local v0 = tonumber        -- type coercion
local v1 = string.byte     -- byte extraction
local v2 = string.char     -- byte → char
local v3 = string.sub      -- substring
local v4 = string.gsub     -- pattern replacement
local v5 = string.rep      -- string repetition
local v6 = table.concat    -- table flattening
local v7 = table.insert    -- table insertion
local v8 = math.ldexp      -- floating-point construction
local v9 = getfenv         -- environment capture (or _ENV fallback)
local v10 = setmetatable   -- metatable assignment
local v11 = pcall          -- protected call
local v12 = select         -- vararg selection
local v13 = unpack         -- table expansion

local function v15(v16, v17, ...) ... end  -- gsub decoder + embedded VM
```

The function `v15` is the main container. Inside it:

- **v20–v25**: low-level binary reader functions (1, 2, 4, 8 byte reads, length-prefixed strings)
- **v28**: binary format parser (constant table → instruction table → register section)
- **v29**: custom bytecode interpreter that executes the parsed instructions
- **v53**: register-assignment helper

---

## 2. String Encoding (gsub Layer)

### 2.1 The Encoded Payload

The variable `v14` (defined later in line 12) holds a 53,354-character string beginning with `LOL!B23Q0003043Q0067616D65...`. This is the **encoded binary payload**.

### 2.2 Decoding Mechanism

The decoder is `v15`:

```lua
local function v15(v16, ...)
    local v18 = 1           -- stream position
    local v19                -- repeat counter
    v16 = v4(v3(v16, 5), "..", function(v30)
        -- v30 is each 2-char match from the gsub pattern ".."
        if v1(v30, 2) == 81 then       -- second byte == 'Q'
            v19 = v0(v3(v30, 1, 1))    -- first byte is repeat count
            return ""                   -- remove 'dQ' from output
        else
            local v83 = v2(v0(v30, 16)) -- interpret pair as hex byte
            if v19 then
                local v99 = v5(v83, v19) -- repeat char v19 times
                v19 = nil
                return v99
            else
                return v83
            end
        end
    end)
    ...
end
```

**How it works:**

| Step | Input | Action |
|------|-------|--------|
| 1 | `LOL!` prefix | Stripped by `string.sub(s, 5)` — 4 chars removed (Lua 1-indexed, so chars 1–4 are dropped) |
| 2 | Pair `dQ` | `d` is a digit (0–9). The `Q` in position 2 triggers repeat mode: the *next* hex pair is emitted `d` times |
| 3 | Pair `XY` (any hex) | `tonumber("XY", 16)` → integer → `string.char(int)` → emitted once |

**Example trace** for `LOL!B23Q00043Q00`:

```
LOL!  → stripped
B2    → char(0xB2) = 178
3Q    → repeat = 3
00    → char(0x00) × 3  (three null bytes)
43    → char(0x43) = 'C'
3Q    → repeat = 3
00    → char(0x00) × 3
```

### 2.3 Critical Off-by-One

Lua's `string.sub(s, 5)` returns **from index 5 onward** (1-indexed). In Python:

```python
# Correct (removes first 4 chars):
s = s[4:]

# Wrong (skips first 5 chars):
s = s[5:]  # DOES NOT MATCH Lua behavior
```

This off-by-one error is a common pitfall when reimplementing Lua string operations in Python. Verification was done by comparing output byte-for-byte against Lua 5.5 (via `lupa`).

### 2.4 Implementation

```python
def gsub_decode(s: str) -> bytes:
    s = s[4:]                     # strip "LOL!"
    result = bytearray()
    repeat = 0
    for i in range(0, len(s), 2):
        pair = s[i:i+2]
        if len(pair) < 2:
            break
        if pair[1] == 'Q':
            repeat = int(pair[0])
        else:
            char_val = int(pair, 16)
            if repeat > 0:
                result.extend([char_val] * repeat)
                repeat = 0
            else:
                result.append(char_val)
    return bytes(result)
```

**Output:** 30,209 bytes of raw binary (`corrected_binary.bin`).

---

## 3. Binary Format (v28 Parser)

### 3.1 Reader Functions

All defined inside `v15`, reading from the shared stream variable `v16` (the decoded binary) using position counter `v18`:

| Function | Reads | Returns |
|----------|-------|---------|
| `v21()` | 1 byte | `uint8` |
| `v22()` | 2 bytes LE | `uint16` |
| `v23()` | 4 bytes LE | `uint32` |
| `v24()` | 8 bytes LE | `double` (IEEE 754) |
| `v25()` | 4-byte length prefix + data | UTF-8 string |

### 3.2 Constant Table

```python
count = v23()  # uint32: number of constants
for i in range(1, count + 1):
    const_type = v21()  # 1=bool, 2=number, 3=string
    if const_type == 1:
        value = v21() != 0          # 1 byte, non-zero = true
    elif const_type == 2:
        value = v24()               # 8-byte IEEE 754 double
    elif const_type == 3:
        value = v25()               # length-prefixed string
```

**Results:**

| Type | Count | Examples |
|------|-------|---------|
| String | ~160 | `"game"`, `"GetService"`, `"Players"`, `"FL"`, `"https://discord.gg/RP29yRPXcp"`, Russian UI text |
| Number | ~16 | `1.83`, `0.1`, `340`, `440`, `12`, `16`, RGB values |
| Boolean | 2 | `true`, `false` |

### 3.3 Decoded Constants (Selection)

```
Const[  1] STR  = game
Const[  2] STR  = GetService
Const[  3] STR  = Players
Const[  4] STR  = RunService
Const[  5] STR  = UserInputService
Const[  6] STR  = TweenService
Const[  7] STR  = LocalPlayer
Const[  9] NUM  = 1.83            (default suspension value)
Const[ 12] STR  = FL
Const[ 13] STR  = FR
Const[ 14] STR  = RL
Const[ 15] STR  = RR
Const[ 17] STR  = https://discord.gg/RP29yRPXcp
Const[ 23] NUM  = 12              (CARD R)
Const[ 24] NUM  = 16              (CARD G=B?)
Const[ 26] NUM  = 18              (ACCENT R)
Const[ 27] NUM  = 24              (ACCENT G=B?)
Const[ 29] NUM  = 70              (TEXT R)
Const[ 30] NUM  = 80              (TEXT G=B?)
Const[ 35] NUM  = 120             (GREEN R)
Const[ 38] NUM  = 200             (RED R)
Const[ 55] STR  = new
Const[ 56] STR  = ScreenGui
Const[ 59] STR  = KryptonZRP
Const[ 62] STR  = UDim2
Const[ 78] NUM  = 9999            (ZIndex)
Const[ 91] STR  = GothamMedium
Const[104] STR  = KRYPTON.ZRP
Const[119] STR  = BOUNCE (прыжки)
Const[122] STR  = WAVE (волна)
Const[124] STR  = SIDE 2 SIDE (качание)
Const[126] STR  = DANCE (случайно)
Const[133] STR  = MouseButton1Click
Const[136] STR  = ПНЕВМА
Const[168] STR  = ПРАВЫЙ SHIFT — СВЕРНУТЬ
Const[177] STR  = KRYPTON.ZRP - LOW-RIDER загружен!
```

The Russian text indicates the script targets a Russian-speaking Roblox community (low-rider suspension scene).

### 3.4 Instruction Table

After constants, the parser reads:

```python
flag = v21()        # 1 byte (always 0 in this payload)
instr_count = v23() # uint32: number of instructions
```

Each instruction is encoded with a variable-length opcode:

```
opcode byte = v21()
bits_1_3 = opcode & 7       # v20(opcode, 1, 3) — non-zero → non-standard
bits_2_3 = (opcode >> 1) & 3  # v114
bits_4_6 = (opcode >> 3) & 7  # v115
```

**Non-standard** (bits_1_3 ≠ 0): consume only the opcode byte. These 339 instructions are skipped by the parser (they appear to be padding/noise inserted by the obfuscator).

**Standard** (bits_1_3 = 0): operand format determined by `v114`:

| v114 | Format | Bytes |
|------|--------|-------|
| 0 | `A = uint16, B = uint16` | 4 |
| 1 | `C = uint32` | 4 |
| 2 | `C = int32` (signed, offset by 2¹⁶) | 4 |
| 3 | `C = int32, D = uint16` | 6 |

Additional operands based on v115:

- Bit 0 of `v115` set → read 1 byte (register index)
- Bit 1 of `v115` set → read 2 bytes (another operand)

Total: **768 standard + 339 non-standard = 1,107 instructions**.

### 3.5 Remaining Data

After the instruction table, 23,547 bytes remain. These contain:

- A second structure parsed during VM execution (register initialization values)
- Inline string/number literals (e.g., `"PlaceId"`, `"FreeLength"`, `"Humanoid"`, font names)
- Additional instruction operands consumed by v29 during interpretation

---

## 4. The v29 Virtual Machine

### 4.1 Architecture

The v29 function implements a register-based VM:

```lua
local function v29(v60, v61, v62)
    -- v60 = parser output {constants, instructions, flag, ...}
    -- v61 = interpreter state (empty table passed in)
    -- v62 = environment (Roblox globals)
```

Internally it maintains:

- **v77**: register table (numeric and string keys)
- **v79**: constant table reference
- **v71**: program counter

The VM loops through instructions, dispatching based on opcode type (v115). Each instruction performs one operation:

- Load constant into register
- Call method on object
- Index table
- Perform arithmetic
- Branch/jump
- Create Roblox instances
- Connect events

### 4.2 Execution Flow

```
1. v28() parses binary → {constants_table, instructions_table, nil, other}
2. v29(parsed_data, {}, roblox_env) → returns a function
3. That function is called with (...) — the remaining varargs from v15
4. v29 executes instructions, interacting with Roblox services via v62
```

### 4.3 Why Full Decompilation Failed

The v29 VM was **not designed to be decompiled** — it's a bytecode interpreter that translates opcodes into Roblox API calls at runtime. Reconstructing the original Lua source would require either:

1. **A complete Roblox environment** to execute v29 and trace all operations, or
2. **A custom decompiler** that maps each opcode back to its Lua equivalent and handles register allocation, control flow reconstruction, and SSA form

Both approaches are beyond the scope of practical deobfuscation for this file.

---

## 5. Execution Attempts

### 5.1 Lua 5.5 Const Variable Issue

Attempting to run the obfuscated code through Lua 5.5 (via `lupa`) fails with:

```
attempt to assign to const variable 'v102'
```

The obfuscator generates for-loops like:

```lua
for v102 = 1, v23() do
    v55, v102, v28 = some_function()
end
```

In Lua 5.1–5.4, assigning to a for-loop variable inside the body is allowed (though the assignment is reset at the next iteration). **Lua 5.5 made for-loop variables const**, causing this to error.

**Fix:** Replace the multi-return assignment that writes to the loop variable:

```lua
-- Before:
v55, v102, v28 = some_function()

-- After:
v55, _, v28 = some_function()
```

Since `some_function()` returns `v102` unchanged via `v53`, the assignment was a no-op. The patch preserves correct behavior.

### 5.2 Roblox Dependency

Even after patching, execution fails because the script depends on Roblox globals:
- `game` (with `:GetService()`)
- `Instance` (with `.new()`)
- `Color3`, `UDim2`, `UDim`, `Enum`
- `setclipboard`
- `script` object

Providing minimal mocks gets past the loading phase but fails during instruction execution because v29 expects specific object types and method signatures from Roblox.

---

## 6. Manual Reconstruction

### 6.1 Approach

Using the extracted constants and structural analysis, the original script was reconstructed:

1. **Identify Roblox services** from string constants: `Players`, `RunService`, `UserInputService`, `TweenService`, `MarketplaceService`
2. **Identify UI structure** from property names: `ScreenGui`, `Frame`, `TextLabel`, `TextButton`, `UICorner`, `UDim2`, `ZIndex`
3. **Identify color theme** from RGB values mapped to keys: `CARD`, `ACCENT`, `TEXT`, `TEXT2`, `GREEN`, `RED`, `BTN`, `SLIDER`, `FILL`, `BLUE`
4. **Identify modes** from string constants: `BOUNCE`, `WAVE`, `SIDE 2 SIDE`, `DANCE`
5. **Identify events** from method names: `MouseButton1Click`, `InputBegan`, `InputChanged`, `InputEnded`, `RenderStepped`
6. **Reconstruct slider logic** from strings like `AbsolutePosition`, `AbsoluteSize`, `UIScale`, `Offset`
7. **Identify suspension logic** from strings: `SpringConstraint`, `FreeLength`, `SuspensionGeometry`, `Wheels`, `math.sin`

### 6.2 Result

The deobfuscated script (`pnevma_deobfuscated.lua`) produces a Roblox GUI that:

- Creates a dark-themed ScreenGui with 340×440 frame
- Shows mode selection buttons (Bounce, Wave, Side 2 Side, Dance)
- Provides per-wheel suspension sliders (FL, FR, RL, RR)
- Includes a Discord button that copies invite link to clipboard
- Displays a live date/time clock
- Toggles visibility with RightShift
- Animates suspension via `SpringConstraint.FreeLength` based on the selected mode
- Uses `RunService.RenderStepped` for real-time updates

---

## 7. Tools & Files

| File | Description |
|------|-------------|
| `pnevma.lua` | Original obfuscated script (79 KB) |
| `pnevma_patched.lua` | Obfuscated script with for-loop patch for Lua 5.5 |
| `pnevma_deobfuscated.lua` | Manual reconstruction (~500 lines) |
| `encoded_string.txt` | Extracted gsub-encoded payload (53,354 chars) |
| `corrected_binary.bin` | Decoded binary (30,209 bytes) |
| `parse_full.py` | Python v28 constant parser |
| `reconstruct.py` | Instruction dump + partial reconstruction |
| `verify_decoder.py` | Python vs Lua gsub verification |
| `reparse_full.py` | Full instruction parser with v115 operands |

---

## 8. Key Takeaways

1. **LuaObfuscator.com** uses a two-layer approach: gsub-based string encoding + custom register-based VM
2. The gsub encoding pairs hex characters into bytes, with `dQ` sequences for run-length encoding of null bytes
3. The binary format stores constants (strings, numbers, booleans), then instructions, then register initialization data
4. The v29 VM is a Roblox-specific interpreter — decompilation requires either a complete Roblox environment or a custom opcode decompiler
5. Manual reconstruction using extracted constants is the most practical approach for scripts of this complexity
6. Lua 5.5's const for-loop variables breaks obfuscated code that reassigns loop variables — this must be patched for non-Roblox execution

---

## 9. References

- [LuaObfuscator.com](https://luaobfuscator.com)
- [Lua 5.5 Reference Manual — for-loop semantics change](https://www.lua.org/manual/5.5/)
- [Roblox Instance documentation](https://create.roblox.com/docs/reference/engine/classes/Instance)
- `lupa` — Python bindings for Lua 5.5 (used for verification)
