# This Space Of Mine — Lua scripting documentation

## Table of contents

- [Introduction](#introduction)
- [Script contexts](#script-contexts)
- [Global functions](#global-functions)
- [Math types](#math-types)
- [Entity scripts](#entity-scripts)
- [Components](#components)
- [Physics](#physics)
- [Voxel world (chunks)](#voxel-world-chunks)
- [Planet generators](#planet-generators)
- [Server API](#server-api-server-1)
- [Server commands](#server-commands)
- [Client API](#client-api-client)
- [Constants](#constants)
- [Complete examples](#complete-examples)

## Introduction

The game embeds Lua virtual machines (via [sol2](https://github.com/ThePhD/sol2)) on both the server and the client. Scripts are used to define entity classes, generate planets, load graphics assets and run administration commands.

Two global boolean variables indicate which side the script runs on: `SERVER` and `CLIENT`. A given entity script is executed on both sides; use these variables to branch your code:

```lua
if SERVER then
    -- authoritative game logic
end
if CLIENT then
    -- rendering, models, local interactions
end
```

## Script contexts

Each context loads a subset of the libraries. The `scripts/libraries` folder (Vec2, Vec3, Quaternion, EulerAngles, Box, Color…) is loaded everywhere.

| Folder | Purpose | Side | Available API |
|---|---|---|---|
| `scripts/entities/*.lua` | Entity classes (one file = one class) | Server *and* client | Base, Math, EntityRegistry + components, Physics; server side: full server API; client side: client API (graphics, config…) |
| `scripts/assets/*.lua` | Loading of models, materials and textures | Client only | Base, Math, Mesh/Model/Material/Texture, `AssetLibrary` |
| `scripts/planets/*.lua` | Terrain generators (one file = one generator) | Server (parallel tasks) | Base, Math (PerlinNoise), Chunk/ChunkContainer/BlockLibrary |
| `scripts/commands/*.lua` | Administration commands, run from the server console via `Server.Execute` | Server (a player's console) | Full server API + `CurrentPlayer` |
| `scripts/libraries/*.lua` | Pure-Lua utility libraries | All contexts | — |

> **Note**: the Lua standard libraries (LuaJIT) are opened in every context (`math`, `string`, `table`, `os`…). `table.new` and `table.clear` are also available.

## Global functions

| Function | Description |
|---|---|
| `print(...)` | Prints to the log (`[Lua]`) or to the player's console. |
| `pprint(...)` | Like `print`, but recursively prints table contents (pretty-print). |
| `tostring(v [, extended])` | Extended version of the standard `tostring`: if `extended` is `true`, tables are serialized recursively. |
| `CreateMetatable(name)` | Creates and registers a named metatable (Lua registry). Errors if the name already exists. Used by the `vec3`, `quaternion`, etc. libraries. |
| `GetMetatable(name)` | Retrieves a metatable registered with `CreateMetatable`. |
| `DirectionFromNormal(normal)` | Converts a normal (`Vec3`) into an engine cardinal direction (used for blocks). |

## Math types

Math types are **pure Lua tables** defined in `scripts/libraries/`. The C++ bridge (`ScriptingUtils`) automatically converts `Nz::Vector2/Vector3/Quaternion/EulerAngles/Box/Rect` to/from these tables: any engine function expecting a vector accepts a `{x=…, y=…, z=…}` table with the right metatable, and returns the same type.

### Vec2 / Vec3

```lua
local v = Vec3(1, 2, 3)   -- Vec3(2) == Vec3(2, 2, 2)
local u = Vec2(1, 2)      -- Vec2(2) == Vec2(2, 2)
```

| Member | Description |
|---|---|
| `v.x, v.y, v.z` | Components (`Vec2`: `x`, `y`). |
| `v:DotProduct(w)` | Dot product. |
| `v:CrossProduct(w)` | Cross product (`Vec3` only). |
| `v:GetAbs()` | Component-wise absolute value. |
| `v:GetLength()` | Length. |
| `v:GetNormal()` | Returns `(normalized, length)`. |
| `v:Maximize(w)` / `v:Minimize(w)` | Component-wise max/min. |
| `Vec3.Distance(a, b)` / `Vec2.Distance(a, b)` | Distance between two points. |
| `+ - * / -v tostring` | Operators (with a scalar or a vector). |

### Quaternion / EulerAngles

```lua
local q = Quaternion(w, x, y, z)         -- careful: field order is {x, y, z, w}
local e = EulerAngles(pitch, yaw, roll)  -- degrees
```

- `q:GetConjugate()` — conjugate of the quaternion.
- `q * q2` — rotation composition; `q * v` — rotates a `Vec3`.
- `EulerAngles` is accepted anywhere the engine expects a rotation (e.g. the `vertexRotation` mesh parameter).

### Box / Color

```lua
local b = Box(x, y, z, width, height, depth)
local c = Color(r, g, b [, a])  -- a = 1.0 by default
```

- `b:GetPosition()`, `b:GetLengths()`, `b:GetCenter()` — AABBs returned by the engine (e.g. `mesh:GetAABB()`) are of this type.
- `Color` is used for material properties (`SetValueProperty`).

### Time

Represents a duration (engine type `Nz::Time`).

| Member | Description |
|---|---|
| `Time.Seconds(s)` / `Time.Milliseconds(ms)` / `Time.Microseconds(µs)` | Constructors from a duration. |
| `Time.Second()` / `Time.Nanosecond()` / `Time.Zero()` | Constants. |
| `t:AsSeconds()` / `t:AsMilliseconds()` / `t:AsMicroseconds()` / `t:AsNanoseconds()` | Conversions. |
| `+ - -t == < <=` | Operators. |

### PerlinNoise

```lua
local noise = PerlinNoise()        -- or PerlinNoise(seed)
noise:reseed(seed)
local n = noise:noise3D_01(x, y, z)
```

Complete binding of [siv::PerlinNoise](https://github.com/Reputeless/PerlinNoise):

- `noise1D/2D/3D` (result in [-1, 1]) and `_01` variants (result in [0, 1]).
- `octave1D/2D/3D(x…, octaves, persistence)` and `_11`, `_01` variants.
- `normalizedOctave1D/2D/3D` and `_01` variants.
- `reseed(seed)`.

## Entity scripts

An entity class is declared in `scripts/entities/<name>.lua`. The script is executed on both the server *and* the client; the typical structure is:

```lua
local classData = EntityRegistry.ClassBuilder()

classData:AddProperty("my_prop", { type = "integer", default = 0, isNetworked = true })

classData:On("init", function (self)
    -- self is the entity table
end)

EntityRegistry.RegisterClass("my_entity", classData)
```

### EntityRegistry / ClassBuilder

| Function | Description |
|---|---|
| `EntityRegistry.ClassBuilder()` | Creates an `EntityBuilder` (class builder). |
| `EntityRegistry.RegisterClass(name, builder)` | Registers the class. Call it at the end of the script. |
| `builder:AddProperty(name, {type=…, default=…, isArray=…, isNetworked=…})` | Declares a property (see [types](#property-types)). `isNetworked = true` replicates the value to clients. |
| `builder:AddClientRPC(name)` | Declares a server → client RPC. |
| `builder:On(event, callback)` | Attaches an event callback (see [events](#events)). |
| `builder:OnClientRPC(name, callback)` | Client side: `function(self)` callback invoked when the RPC is received. |
| `builder:OnPropertyUpdate(name, callback)` | `function(self, newValue)` callback invoked when the property changes. |

The builder is indexable: `builder.MyFunction = function(self) … end` adds a method to the class, accessible on `self` inside callbacks.

### Property types

The `type` field is case-insensitive. Add `isArray = true` for an array of that type.

| Type | Lua value |
|---|---|
| `bool` | boolean |
| `float`, `integer` | number |
| `float2`, `floatposition`, `floatsize`, `integer2`, `integerposition`, `integersize` | `Vec2` |
| `float3`, `floatposition3d`, `floatsize3d`, `integer3`, `integerposition3d`, `integersize3d` | `Vec3` |
| `float4`, `integer4` | 4D vector |
| `floatrect`, `integerrect` | rectangle |
| `string` | string |

### Events

| Event | Side | Signature | Description |
|---|---|---|---|
| `init` | shared | `function(self)` | When the entity is created. This is where components are added. |
| `activate` | shared | `function(self)` | When the entity is activated. |
| `tick` | shared | `function(self)` | Every tick (interval adjustable via `self:SetTickInterval`). |
| `interact` | SERVER | `function(self, player)` | When a player interacts with the entity (requires `SetInteractible(true)`). |
| `env_switch` | SERVER | `function(newSelf, previousSelf)` | When the entity changes environment (requires `AllowEnvironmentSwitch()`). |

### Entity methods (self)

In every callback, `self` is the "entity table". Two entity tables pointing to the same entity compare equal with `==`.

| Method | Side | Description |
|---|---|---|
| `self:AddComponent(type [, params])` | shared | Adds a component and returns it (see [components](#components)). |
| `self:GetComponent(type)` | shared | Retrieves an existing component. |
| `self:GetProperty(name)` | shared | Current value of a property. |
| `self:UpdateProperty(name, value)` | shared | Modifies a property (triggers `OnPropertyUpdate` and network replication if `isNetworked`). |
| `self:SetTickInterval(ms)` | shared | Interval of the `tick` callback in milliseconds (requires a `tick` handler). |
| `self:AllowEnvironmentSwitch()` | SERVER | Allows the entity to move from one environment to another (ship ↔ planet). |
| `self:CallClientRPC(name [, targetPlayer])` | SERVER | Triggers an RPC declared with `AddClientRPC`, to one player or all. |
| `self:GetEnvironment()` | SERVER | Returns the entity's environment (`PlanetEnvironment` or `ShipEnvironment`). |
| `self:SetInteractible(bool)` | SERVER / CLIENT | Enables/disables interaction. On the server, requires an `interact` handler. |
| `self:SetInteractibleText(text)` | CLIENT | Text shown to the player aiming at the entity. |

## Components

Component types usable with `AddComponent` / `GetComponent`:

| Type | Side | Add parameters |
|---|---|---|
| `"node"` | shared | — |
| `"rigidbody3d"` | shared | Mandatory table, see below |
| `"physicscharacter3d"` | shared | *Read-only* — cannot be added from Lua |
| `"atmosphere_exchanger"` | SERVER | — |
| `"atmosphere_monitor"` | SERVER | — |
| `"graphics"` | CLIENT | — |
| `"light"` | CLIENT | — |

### rigidbody3d — parameters

```lua
self:AddComponent("rigidbody3d", {
    kind = "dynamic",              -- "dynamic" or "static" (mandatory)
    collider = BoxCollider3D.new(Vec3(0.5)),
    objectLayer = Constants.ObjectLayerDynamic,
    isTrigger = false,
    isSimulationEnabled = true,
    initiallySleeping = false,
    -- "dynamic"-only parameters:
    mass = 10.0,
    friction = 0.5,
    restitution = 0.0,
    gravityFactor = 1.0,
    linearDamping = 0.05,
    angularDamping = 0.05,
    maxLinearVelocity = 500.0,
    maxAngularVelocity = 47.12,
    allowSleeping = true,
})
```

### NodeComponent (`"node"`)

| Method | Description |
|---|---|
| `GetPosition()` / `GetRotation()` | Local position / rotation. |
| `Scale(vec3)` | Multiplies the scale. |
| `ToLocalPosition(vec3)` / `ToLocalDirection(vec3)` / `ToLocalRotation(quat)` / `ToLocalScale(vec3)` | World → local conversion. |

### Rigidbody3DComponent (`"rigidbody3d"`)

Getters: `GetAABB`, `GetAngularDamping`, `GetAngularVelocity`, `GetCollider`, `GetLinearDamping`, `GetLinearVelocity`, `GetMass`, `GetObjectLayer`, `GetPosition`, `GetRotation`.

Setters: `SetAngularDamping`, `SetAngularVelocity`, `SetCollider`, `SetLinearDamping`, `SetLinearVelocity`, `SetMass`, `SetObjectLayer`, `SetPosition`, `SetRotation`, `TeleportTo(pos, rot)`.

### PhysCharacter3DComponent (`"physicscharacter3d"`)

`GetAngularVelocity`, `GetCollider`, `GetLinearVelocity`, `GetPosition`, `GetObjectLayer`, `GetRotation`, `SetAngularVelocity`, `SetLinearVelocity`, `SetObjectLayer`, `SetRotation`, `TeleportTo(pos, rot)`.

### GraphicsComponent (`"graphics"`) — CLIENT

```lua
local gfx = self:AddComponent("graphics")
gfx:AttachRenderable(model, Constants.RenderMask3D)
```

### LightComponent (`"light"`) — CLIENT

`AddPointLight()` (energy 20 by default), `Hide()`, `Show([visible])`.

### AtmosphereExchanger (`"atmosphere_exchanger"`) — SERVER

Injects/consumes gases in the atmosphere at a regular interval.

`GetGasModifier(gasType)`, `SetGasModifier(gasType, amount)`, `GetTickRate()`, `SetTickRate(time)` (takes a `Time`).

### AtmosphereMonitor (`"atmosphere_monitor"`) — SERVER

Read-only property `monitor.Atmosphere`: the [`Atmosphere`](#atmosphere-1) the entity is in (or `nil`).

## Physics

### Colliders

| Function | Description |
|---|---|
| `BoxCollider3D.new(lengths)` | Box of `Vec3` dimensions. |
| `collider:GetBoundingBox()` | AABB of the collider. |
| `collider:GetCenterOfMass()` | Center of mass. |

### PhysWorld

Obtained via `environment:GetPhysWorld()` (SERVER).

```lua
local result = physWorld:RaycastQueryFirst(startPos, endPos, { IgnorePlayers = true })
if result then
    -- result.fraction     : fraction [0,1] of the ray
    -- result.hitPosition  : impact point (world)
    -- result.hitNormal    : normal at the impact point
    -- result.subShapeID   : sub-shape id of the hit collider
    -- result.hitEntity    : hit entity table (or nil)
    -- result.hitChunk     : hit Chunk if the entity is a chunk (or nil)
end
```

Returns `nil` if nothing is hit. The `IgnorePlayers = true` option ignores players and their triggers.

## Voxel world (chunks)

### BlockLibrary

Obtained via `chunk:GetBlockLibrary()`.

- `GetBlockIndex(name)` — numeric index of a block by name. Errors if the name is unknown. The full list is defined in `CookedAssets/BlockData.json`: `empty`, `debug`, `dirt`, `hull`, `snow`, `stone`, `stone_mossy`, `forcefield`, `planks`, `stone_bricks`, `copper_block`, `grass`, `glass`, `water`, `bark`, `cliff_rocks`, `rock`, `wood_floor`, `white_bricks`, `gold`, `metal`, `metal_plates`, `brickswall`, `floor_tiles`.

### Chunk

| Method | Description |
|---|---|
| `GetIndices()` | Indices of the chunk within its container. |
| `GetSize()` / `GetBlockCount()` / `GetBlockSize()` | Dimensions, block count, size of a block. |
| `GetBlockLibrary()` / `GetContainer()` | Block library / container (planet, ship). |
| `GetBlockContent(localIndices \| flatIndex)` | Index of the block at these coordinates. |
| `GetBlockLocalIndex(indices)` / `GetBlockLocalIndices(index)` | Coordinates ↔ flat index conversion. |
| `GetBlockCenterPosition(localIndices)` | Center of the block in chunk-local coordinates. |
| `GetContent()` | 1-indexed table of all the chunk's blocks. |
| `Reset(contentTable)` | Replaces the whole content (the table must contain `GetBlockCount()` block indices; the rest is filled with empty). |
| `UpdateBlock(localIndices, blockIndex)` | Modifies one block. |
| `ComputeHitCoordinates(localPos, localNormal, collider, subShapeID)` | From a raycast hit (converted to local coordinates), returns `{ direction = …, blockIndices = … }` or `nil`. |

### ChunkContainer

The chunk container of a planet or a ship.

| Method | Description |
|---|---|
| `GetChunk(chunkIndices)` | Chunk at these indices (or `nil`). |
| `GetChunkCount()` / `GetChunkOffset()` / `GetTileSize()` / `GetCenterPosition()` | Container info. |
| `GetBlockIndices(chunkIndices, localIndices)` | Global coordinates of a block. |
| `GetChunkIndicesByBlockIndices(blockIndices)` | Returns `(chunkIndices, localIndices)` for global coordinates. |

## Planet generators

A generator is a `scripts/planets/<name>.lua` file returning a table:

```lua
return {
    PlanetType = "planet",   -- planet type ("planet", "torus_planet", …)
    Generator = function (chunk, chunkCount, properties)
        -- chunkCount : world dimensions in chunks (e.g. chunkCount.x)
        -- properties : table of the planet's properties (e.g. properties.Radius)
        local content = {}
        for i = 1, chunk:GetBlockCount() do
            content[i] = emptyBlock
        end
        return content   -- 1-indexed table of block indices
    end
}
```

- The function is called **once per chunk**, potentially in parallel (each task has its own Lua state — no global state shared between chunks).
- The returned table must contain exactly `chunk:GetBlockCount()` entries; invalid indices are replaced with empty along with an error in the log.
- The result is cached in the database with a hash of the script: modifying the script automatically invalidates the cache.
- Available API: chunks ([above](#voxel-world-chunks)), `PerlinNoise`, math.

> **Warning**: some old scripts (`alice.lua`, `test.lua`) directly return a `(chunk, seed, chunkDims)` function and use `Vec3ui` / `SignedDistance` globals which are no longer exposed: this is the old format, non-functional with the current loader. Follow the format of `torus.lua`.

## Server API (SERVER)

### Server

| Function | Description |
|---|---|
| `Server.Execute(command [, params])` | Executes `scripts/commands/<command>.lua` and returns its result (see [commands](#server-commands)). |
| `Server.FindPlayerByName(name)` | Returns a `Player` (handle) by nickname. |
| `Server.FindPlayerByUuid(uuid)` | Returns a `Player` by UUID (string). Errors if the UUID is invalid. |
| `Server.GetAllPlayers()` | Table of all connected players. |
| `Server.ScheduleForNextTick(callback)` | Schedules a function for the next server tick. |
| `Server.RegisterDatabaseEnvironment(databaseId, planetEnv)` | Associates a freshly created `PlanetEnvironment` with a database id. |
| `Server.LinkDatabaseEnvironments(srcDbId, dstDbId, position)` | Links two registered environments (teleporter/spatial link) at a position. |

### Environments

Enum `EnvironmentType`: `Planet`, `Ship`.

#### Environment (common base)

| Method | Description |
|---|---|
| `CreateEntity(className [, {position=…, rotation=…}])` | Instantiates an entity of a registered class and returns its entity table. |
| `GetPhysWorld()` | The environment's [`PhysWorld`](#physics). |
| `GetAtmosphereAtPosition(pos)` | `Atmosphere` at a position (or `nil`). |
| `GetAllAtmospheres()` | Table of all atmospheres. |
| `GetType()` | `EnvironmentType.Planet` or `EnvironmentType.Ship`. |
| `IsRoot()` | `true` if this is the root environment. |

#### PlanetEnvironment

```lua
local env = PlanetEnvironment(databaseId --[[or nil]], generatorName, chunkCount --[[Vec3]], planetType)
```

- `GetDatabaseId()` — database id (or `nil`).
- `GetChunkContainer()` — the planet's [`ChunkContainer`](#voxel-world-chunks).

#### ShipEnvironment

- `GetShipEntity()` — ship entity (interior).
- `GetExteriorShipEntity()` — entity representing the ship in the exterior environment.

### Atmosphere

Enum `GasType`: `Oxygen`, `CarbonDioxyde`, `Nitrogen`.

| Method | Description |
|---|---|
| `atmosphere:GetGasAmount(gasType)` | Gas amount (milliliters). |
| `atmosphere:SetGasAmount(gasType, amount)` | Sets the amount. |
| `atmosphere:Exchange({[GasType.Oxygen] = -100, …})` | Atomic exchange of several gases (positive values = injection, negative = withdrawal). Returns `true` if the exchange was applied, `false` (without modifying anything) if a withdrawal exceeds the available amount. |

### Player

| Method | Description |
|---|---|
| `player:GetName()` | Nickname. |
| `player:GetUuid()` | UUID (string) or `nil` (anonymous account). |
| `player:GetPlayerIndex()` | Player index on the server. |
| `player:GetEntity()` | Entity table controlled by the player. |
| `player:GetController()` | Their `CharacterController`. |
| `player:GetEnvironment()` / `player:GetRootEnvironment()` | Environment of the controlled entity / root environment. |
| `player:PilotShip(shipEntity, shipExteriorEntity, referenceRotation)` | Makes the player pilot a ship. |
| `player:GetControlledShipEntity()` | Entity of the piloted ship. |
| `player:ExitPiloting()` | Stops piloting. |
| `player:IsValid()` | `false` if the player disconnected (`Player` objects are handles). |

#### CharacterController

`GetCameraAngles()`, `GetCameraRotation()`, `GetCharacterPosition()`, `GetCharacterRotation()`, `GetEyePosition()`, `GetReferenceRotation()`, `SetShipController(shipController | nil)`.

#### ShipController

`ShipController.new(shipEntityTable, rotation)` — controller to assign via `CharacterController:SetShipController`.

### ServerDatabase

| Function | Description |
|---|---|
| `ServerDatabase.CreatePlanet({generatorName=…, type=…, chunkCount=…})` | Creates a planet in the database and returns its id. |
| `ServerDatabase.StorePlanetLink({sourcePlanet=…, destinationPlanet=…, position=…})` | Stores a persistent link between two planets. |

## Server commands

A `scripts/commands/<name>.lua` file must **return a function** taking an optional parameter table. It is executed in a player's console via `Server.Execute("name", {…})`; this context has the full server API plus:

- `CurrentPlayer` — the [`Player`](#player) running the command.

```lua
-- scripts/commands/my_command.lua
return function (opt)
    opt = opt or {}
    local radius = opt.radius or 5

    local env = CurrentPlayer:GetEnvironment()
    local physWorld = env:GetPhysWorld()
    -- …
end
```

## Client API (CLIENT)

### AssetLibrary and asset scripts

The `scripts/assets/*.lua` scripts are loaded on connection, before entity scripts. They prepare models and register them in `AssetLibrary` so entities can retrieve them.

| Function | Description |
|---|---|
| `AssetLibrary.RegisterModel(name, model)` | Registers a model under a name. |
| `AssetLibrary.GetModel(name)` | Retrieves a registered model. |

#### Mesh

| Function | Description |
|---|---|
| `Mesh.Load(path [, params])` | Loads a mesh. `params.animated`, `params.center`, `params.texCoordOffset/Scale`, `params.vertexOffset`, `params.vertexRotation` (EulerAngles), `params.vertexScale`. |
| `Mesh.CreateStatic()` / `Mesh.CreateSkeletal(jointCount)` | Creates an empty mesh. |
| `mesh:AddSubMesh(submesh)` / `mesh:AddSubMesh(name, submesh)` | Adds a submesh. |
| `mesh:BuildSubMesh(primitive)` | Builds a submesh from a `Primitive`. |
| `mesh:GetSubMesh(name \| index)` / `mesh:GetSubMeshCount()` | Submesh access (0-based index). |
| `mesh:GetAABB()` / `mesh:Recenter()` / `mesh:Translate(vec3)` | Geometry. |
| `mesh:SetMaterialCount(n)` / `mesh:GetMaterialData(i)` / `mesh:SetMaterialData(i, data)` | Material slots (0-based index). |

#### SubMesh

`GetMaterialIndex()`, `SetMaterialIndex(i)`.

The derived types `SkeletalMesh` and `StaticMesh` are also exposed (returned by `mesh:GetSubMesh` depending on the mesh type) but add no methods of their own.

#### Primitive

`Primitive.Box(lengths)`, `Primitive.Box(lengths, subdivisions, position, rotation)`, `Primitive.IcoSphere(size, recursionLevel, position)`.

> **Warning**: the `rotation` parameter of `Primitive.Box` is accepted but ignored by the engine (only `position` is forwarded).

#### Model

| Function | Description |
|---|---|
| `Model.Load(path [, params])` | Loads a model. `params.loadMaterials` (bool), `params.mesh` = `Mesh.Load` parameters. |
| `Model.BuildFromMesh(mesh)` | Builds a model from a mesh (default materials). |
| `model:SetMaterial(index, materialInstance)` | Assigns a material (0-based index). |
| `model:GetMaterial(index)` / `model:GetMaterialCount()` / `model:GetAABB()` | Inherited from `InstancedRenderable`. |
| `model:UpdateRenderLayer(layer)` | Render layer. |

#### Texture

`Texture.Load(path)` — loads a texture (errors if not found).

### Materials

#### Enums and flags

- `MaterialType`: `Basic`, `Phong`, `PhysicallyBased`
- `MaterialInstancePresetFlags`: `Default`, `NoDepth`, `AlphaBlended` (combinable with `|`)
- `FaceCulling`: `Basic` (back), `Front`, `FrontAndBack`, `None`
- `FaceFilling`: `Fill`, `Line`, `Point`
- `FrontFace`: `Clockwise`, `CounterClockwise`
- `ShaderStageType`: `Vertex`, `Fragment`, `Compute` (combinable with `|`)

#### MaterialInstance

| Function | Description |
|---|---|
| `MaterialInstance.Instantiate(matType [, presetFlags])` | Creates an instance of a predefined material (the ReverseZ preset is applied automatically). |
| `mat:SetTextureProperty(name, texture)` | E.g. `"BaseColorMap"`. |
| `mat:SetValueProperty(name, value)` | Boolean, number or `Color`. |
| `mat:ApplyPreset(presetFlags)` | Applies a preset. |
| `mat:EnablePass(name [, enable])` / `mat:DisablePass(name)` | Enables/disables a pass. |
| `mat:UpdatePassStates(passName, function(renderStates) … end)` | Modifies the render states of a pass. |
| `mat:UpdatePassesStates(function(renderStates) … end)` | Same for all passes. |

#### Material / MaterialSettings (custom materials)

```lua
local settings = MaterialSettings()
settings:AddPredefinedBasicSettings()
settings:AddPass("ForwardPass", {
    shaders = {
        { shader = "my_shader", stages = ShaderStageType.Vertex | ShaderStageType.Fragment }
    },
    -- flags = …, states = RenderStates()
})

local material = Material(settings, "BasicMaterial")
local instance = material:Instantiate()
```

> **Warning**: a `MaterialSettings` object can only be consumed once by `Material(…)`.

#### RenderStates

Created with `RenderStates()` (depth compare `GreaterOrEqual` by default — reverse-Z), or received in the `UpdatePass(es)States` callbacks. Modifiable fields:
`faceCulling`, `faceFilling`, `frontFace`, `blending`, `depthBias`, `depthBuffer`, `depthClamp`, `depthWrite`, `scissorTest`, `stencilTest`, `depthBiasConstantFactor`, `depthBiasSlopeFactor`, `lineWidth`, `pointSize`.

### Config / Scripts

| Function | Description |
|---|---|
| `Config.GetBool/GetFloat/GetInteger/GetString(name)` | Reads a client configuration option. |
| `Config.SetBool/SetFloat/SetInteger/SetString(name, value)` | Modifies an option. |
| `Scripts.Reload()` | Reloads the client-side scripts. |

## Constants

Read-only global table (accessing a non-existent constant raises an error).

| Constant | Side | Description |
|---|---|---|
| `Constants.TickDuration` | shared | Duration of a tick. |
| `Constants.PlayerOxygenConsumption` | shared | Oxygen consumption of a player. |
| `Constants.BroadphaseStatic` / `BroadphaseDynamic` | shared | Physics broadphase layers. |
| `Constants.ObjectLayerStatic` / `ObjectLayerStaticNoPlayer` / `ObjectLayerStaticTrigger` | shared | Static object layers. |
| `Constants.ObjectLayerDynamic` / `ObjectLayerDynamicNoCollision` / `ObjectLayerDynamicNoPlayer` / `ObjectLayerDynamicTrigger` | shared | Dynamic object layers. |
| `Constants.ObjectLayerPlayer` / `ObjectLayerPlayerOnlyTrigger` | shared | Player-related layers. |
| `Constants.RenderMask2D` / `RenderMaskUI` / `RenderMask3D` / `RenderMaskLocalPlayer` / `RenderMaskOtherPlayer` | CLIENT | Render masks for `AttachRenderable`. |

## Complete examples

### Interactive entity (piloting computer) — `scripts/entities/computer.lua`

```lua
local classData = EntityRegistry.ClassBuilder()

classData:On("init", function (self)
    self:AddComponent("rigidbody3d", {
        kind = "static",
        collider = BoxCollider3D.new(Vec3(0.5)),
        objectLayer = Constants.ObjectLayerStatic
    })

    self:SetInteractible(true)

    if CLIENT then
        self:SetInteractibleText("Pilot")
        local gfx = self:AddComponent("graphics")
        gfx:AttachRenderable(AssetLibrary.GetModel("computer"), Constants.RenderMask3D)
    end
end)

if SERVER then
    classData:On("interact", function (self, player)
        local computerNode = self:GetComponent("node")
        local shipEnv = self:GetEnvironment()
        player:PilotShip(shipEnv:GetShipEntity(), shipEnv:GetExteriorShipEntity(),
                         computerNode:GetRotation())
    end)
end

EntityRegistry.RegisterClass("computer", classData)
```

### Entity with replicated properties (atmosphere sensor)

```lua
local classData = EntityRegistry.ClassBuilder()

classData:AddProperty("sensor_o2", { type = "integer", default = 0, isNetworked = true })

classData:On("init", function (self)
    self:AddComponent("rigidbody3d", {
        kind = "dynamic", mass = 10.0,
        collider = BoxCollider3D.new(Vec3(0.75)),
        objectLayer = Constants.ObjectLayerDynamic
    })

    if SERVER then
        self:AllowEnvironmentSwitch()
        self:SetTickInterval(1000)
        self:AddComponent("atmosphere_monitor")
    end
end)

if SERVER then
    classData:On("tick", function (self)
        local monitor = self:GetComponent("atmosphere_monitor")
        if monitor.Atmosphere then
            self:UpdateProperty("sensor_o2", monitor.Atmosphere:GetGasAmount(GasType.Oxygen))
        end
    end)
else
    classData:OnPropertyUpdate("sensor_o2", function (self)
        self:SetInteractibleText(("O2: %dmL"):format(self:GetProperty("sensor_o2")))
    end)
end

EntityRegistry.RegisterClass("atmosphere_sensor", classData)
```

### Asset script — `scripts/assets/computer.lua`

```lua
local computer = Model.Load("CookedAssets/Models/ShipComputer/scifi_computer_1_3.obj", {
    mesh = {
        center = true,
        vertexRotation = EulerAngles(180, 0, 0),
        vertexScale = Vec3(1.0 / 500.0) * Vec3(1, -1, -1)
    },
    loadMaterials = false
})

local screenMat = MaterialInstance.Instantiate(MaterialType.Basic, MaterialInstancePresetFlags.AlphaBlended)
screenMat:SetTextureProperty("BaseColorMap", Texture.Load("CookedAssets/Models/ShipComputer/digital_displays.dds"))
screenMat:UpdatePassesStates(function (renderStates)
    renderStates.faceCulling = FaceCulling.None
end)

computer:SetMaterial(0, screenMat)
AssetLibrary.RegisterModel("computer", computer)
```

### Server command with raycast — excerpt from `scripts/commands/explosion.lua`

```lua
return function (opt)
    opt = opt or {}
    local radius = opt.radius or 5

    local env = CurrentPlayer:GetEnvironment()
    local physWorld = env:GetPhysWorld()
    local controller = CurrentPlayer:GetController()

    local eyePos = controller:GetEyePosition()
    local cameraRot = controller:GetCameraRotation()

    local result = physWorld:RaycastQueryFirst(eyePos, eyePos + cameraRot * Vec3(0, 0, -1000),
                                               { IgnorePlayers = true })
    if not result or not result.hitChunk then return end

    local chunkNode = result.hitEntity:GetComponent("node")
    local chunkBody = result.hitEntity:GetComponent("rigidbody3d")

    local hitBlock = result.hitChunk:ComputeHitCoordinates(
        chunkNode:ToLocalPosition(result.hitPosition),
        chunkNode:ToLocalDirection(result.hitNormal),
        chunkBody:GetCollider(), result.subShapeID)
    if not hitBlock then return end

    local container = result.hitChunk:GetContainer()
    local globalIndices = container:GetBlockIndices(result.hitChunk:GetIndices(), hitBlock.blockIndices)
    -- … modify the blocks around globalIndices via container:GetChunkIndicesByBlockIndices + UpdateBlock
end
```
