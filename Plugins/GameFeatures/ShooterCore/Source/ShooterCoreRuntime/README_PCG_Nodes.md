# ShooterCoreRuntime – PCG Nodes

## `Set Point Bounds From Mesh`
Custom PCG node implemented in C++:
- Class: `UPCGSetPointBoundsFromMeshSettings`
- Header: `ShooterCoreRuntime/Public/PCG/PCGSetPointBoundsFromMesh.h`

### What it does
- **Input:** Point Data
- **Parameter:** `ReferenceMesh` (a `UStaticMesh`)
- **Output:** Point Data (same points) with:
  - `BoundsMin/BoundsMax` overwritten to match the reference mesh dimensions (Width/Length/Height in UU)
  - optional Transform scale forced to `1,1,1`

### Typical usage in a PCG Graph
1. Create or import points (e.g., `Create Grid`, `Scatter`, `Sample Spline`).
2. Add node **Set Point Bounds From Mesh**.
3. Set **ReferenceMesh** to the module piece you want (e.g., a 400x400 floor tile).
4. (Optional) Use PCG nodes that rely on point bounds (fit/overlap/packing), then
5. Spawn meshes with `Static Mesh Spawner`.

### Notes
- This node *does not* generate points by itself; it modifies incoming points.
- Bounds are half-size extents: for a 400×400×20 mesh you'll get extents ~ (200,200,10).
