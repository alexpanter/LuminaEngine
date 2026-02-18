<div align="center">

# Contributing to Lumina Engine

**Building the future of game engines, together.**

[![Discord](https://img.shields.io/discord/1193738186892005387?label=Discord&logo=discord)](https://discord.gg/xQSB7CRzQE)
[![License](https://img.shields.io/github/license/mrdrelliot/lumina)](LICENSE)

</div>

---

## Our Philosophy

**Contributing is a right for everyone.** Lumina is a community-driven project built on the principle of shared knowledge. We believe that every contribution, no matter how small, helps advance our collective understanding of game engine architecture.

### We Welcome

- Bug fixes and stability improvements
- New features and systems
- Documentation and tutorials
- Editor improvements and tooling
- Performance optimizations
- Architecture suggestions
- Questions and discussions

**Any and all pull requests that benefit the engine will be seriously considered and merged.** We're here to learn together and build something amazing.




---

## Getting Started

### 1. Join the Community

Before contributing, we recommend joining our [Discord server](https://discord.gg/xQSB7CRzQE) to:
- Discuss your ideas with the team
- Get help with setup and development
- Coordinate on larger features
- Meet other contributors

### 2. Set Up Your Environment

Follow the [Quick Start guide](README.md#-quick-start) in the main README to get Lumina building on your machine.

### 3. Find Something to Work On

- Check the [Issues](https://github.com/mrdrelliot/lumina/issues) page for open tasks
- Look for issues labeled `good first issue` or `help wanted`
- Propose new features by opening an issue first
- Fix bugs you encounter while using the engine

### 4. Fork & Branch

```bash
# Fork the repository on GitHub, then:
git clone https://github.com/YOUR_USERNAME/luminaengine.git
cd lumina
git remote add upstream https://github.com/mrdrelliot/luminaengine.git

# Create a feature branch
git checkout -b feature/my-awesome-feature
```

---

## Coding Standards

Lumina enforces strict coding standards to maintain consistency and readability across the codebase.

### Naming Conventions

#### Prefixes

All types must use appropriate prefixes based on their purpose:

| Prefix | Usage | Example |
|--------|-------|---------|
| `F` | Internal engine types (non-reflected) | `FRenderer`, `FTexture`, `FRenderGraph` |
| `C` | Reflected classes (game-facing) | `CTransform`, `CMeshRenderer`, `CLight` |
| `S` | Reflected structs (data types) | `SVertex`, `SMaterial`, `SRenderSettings` |
| `E` | Enumerations | `ETextureFormat`, `ERenderPass` |
| `I` | Interfaces (abstract classes) | `ISystem`, `IRenderable` |
| `T` | Template parameters | `TObjectHandle<T>`, `TArray<T>` |

#### Case Conventions

```cpp
// CORRECT: PascalCase for everything
class FRenderer { };
void UpdateTransform();
int32 EntityCount;
glm::vec3 Position;
ETextureFormat Format;

// INCORRECT: No snake_case or camelCase
void update_transform();    // Wrong
int entity_count;          // Wrong
glm::vec3 m_position;        // Wrong (no Hungarian notation)
```

**All identifiers use PascalCase:**
- Classes, structs, enums
- Functions and methods
- Variables (local, member, global)
- Constants
- Namespaces

### Code Style

#### Indentation & Formatting

```cpp
// CORRECT: Tabs for indentation, braces on new lines
void ProcessEntities()
{
	for (const auto& Entity : Entities)
	{
		if (Entity.IsValid())
		{
			Entity.Update(DeltaTime);
		}
	}
}

// INCORRECT: Spaces and wrong brace placement
void ProcessEntities() {
    for (const auto& Entity : Entities) {
        if (Entity.IsValid()) {
            Entity.Update(DeltaTime);
        }
    }
}
```

**Rules:**
- Use **tabs** for indentation (not spaces)
- Opening braces on **new lines**
- One statement per line
- Space after control flow keywords: `if (`, `for (`, `while (`

#### Function Design

Functions should be **focused, minimal, and well-encapsulated**:

```cpp
// CORRECT: Single responsibility, minimal parameters
void UpdateEntityTransform(TObjectHandle<CTransform> Transform, const glm::vec3& NewPosition)
{
	if (!Transform.IsValid())
	{
		return;
	}
	
	Transform->SetPosition(NewPosition);
	Transform->MarkDirty();
}

// INCORRECT: Too much responsibility, unclear purpose
void DoStuff(CEntity* Entity, float X, float Y, float Z, bool Flag1, bool Flag2, int Mode)
{
	Entity->Transform->Position.X = X;
	Entity->Transform->Position.Y = Y;
	Entity->Transform->Position.Z = Z;
	
	if (Flag1)
	{
		Entity->DoSomething();
	}
	
	if (Flag2)
	{
		Entity->DoSomethingElse();
	}
	
	// ... 50 more lines
}
```

**Principles:**
- **Single Responsibility** - One function, one purpose
- **Minimal Parameters** - Prefer 3 or fewer parameters
- **Small Functions** - Keep functions under 50 lines when possible
- **Early Returns** - Validate inputs first, return early on failure
- **No Side Effects** - Functions should do what their name implies, nothing more

### Memory Management

#### Handle System (Required)

**Raw pointers to `CObject`-derived types are PROHIBITED.** Always use `TObjectHandle<T>`:

```cpp
// CORRECT: Use TObjectHandle for safe weak references
class CMyComponent
{
public:
	void SetTarget(TObjectHandle<CEntity> InTarget)
	{
		Target = InTarget;
	}
	
	void Update()
	{
		if (Target.IsValid())
		{
			// Safe to use, handle checks validity and generation
			Target->DoSomething();
		}
	}
	
private:
	TObjectHandle<CEntity> Target;
};

// INCORRECT: Raw pointers are unsafe
class CMyComponent
{
public:
	void SetTarget(CEntity* InTarget)  // PROHIBITED!
	{
		Target = InTarget;  // Dangling pointer risk!
	}
	
private:
	CEntity* Target;  // NEVER DO THIS!
};
```

**Why TObjectHandle?**
- Generational safety prevents use-after-free
- Automatic invalidation on object destruction
- Weak reference semantics
- Serialization-friendly
- Thread-safe validation

**Exceptions:**
Raw pointers are allowed for:
- Non-`CObject` types (e.g., `FRenderer*`, `FTexture*`)
- Local function parameters (lifetime guaranteed by caller)
- Platform/external APIs

### Data-Oriented Design

**Prefer data-oriented programming over heavy OOP inheritance.**

```cpp
// CORRECT: Data-oriented, cache-friendly
struct STransformData
{
	glm::vec3 Position;
	glm::quat Rotation;
	glm::vec3 Scale;
};

// Process in batches
void UpdateTransforms(TArray<STransformData>& Transforms, float DeltaTime)
{
	for (auto& Transform : Transforms)
	{
		// Efficient iteration, good cache locality
		Transform.Position += Transform.Velocity * DeltaTime;
	}
}

// AVOID: Heavy inheritance, poor cache performance
class CBaseTransform { virtual void Update() = 0; };
class CDerivedTransform1 : public CBaseTransform { /* ... */ };
class CDerivedTransform2 : public CBaseTransform { /* ... */ };
// Virtual calls, scattered memory, cache misses
```

**Principles:**
- Separate **data from behavior**
- Use **plain structs** for data storage
- Process data in **batches** when possible
- Minimize **virtual function calls** in hot paths
- Favor **composition over inheritance**

### Metaprogramming

**Template metaprogramming is allowed but MUST be constrained with C++20 concepts.**

```cpp
// CORRECT: Concepts enforce constraints at compile-time
template<typename T>
concept Transformable = requires(T Object)
{
	{ Object.GetPosition() } -> std::convertible_to<glm::vec3>;
	{ Object.SetPosition(glm::vec3{}) } -> std::same_as<void>;
};

template<Transformable T>
void ApplyTranslation(T& Object, const glm::vec3& Translation)
{
	Object.SetPosition(Object.GetPosition() + Translation);
}

// INCORRECT: Unconstrained template
template<typename T>
void ApplyTranslation(T& Object, const glm::vec3& Translation)
{
	// What if T doesn't have GetPosition/SetPosition?
	Object.SetPosition(Object.GetPosition() + Translation);
}
```

**Requirements:**
- **All templates** must use concepts or SFINAE
- **Clear error messages** through concept constraints
- **Document requirements** in comments
- **Avoid** template metaprogramming for simple cases

### Documentation

**All code must be well-documented with clear, concise comments.**

```cpp
/**
 * Renders a mesh with the specified material and transform.
 * 
 * @param Mesh - The mesh to render (must be loaded and valid)
 * @param Material - Material to apply during rendering
 * @param Transform - World-space transformation matrix
 * 
 * @return True if rendering was successful, false if validation failed
 * 
 * @note This function is not thread-safe and must be called from the render thread
 */
bool RenderMesh(const FMesh& Mesh, const FMaterial& Material, const FMatrix& Transform);

// Comment complex logic
void CalculateClusterBounds()
{
	// Use conservative depth bounds to minimize false positives
	// Based on "Practical Clustered Shading" (Olsson et al.)
	for (uint32 Z = 0; Z < ClusterDepth; ++Z)
	{
		// Exponential depth slicing for better near-plane resolution
		float NearPlane = CalculateDepthSlice(Z);
		// ...
	}
}

// Explain why, not what
// Flush commands here to avoid synchronization issues with async compute
CommandBuffer.Submit();

// Useless comment
// Increment i by 1
++i;
```

**Documentation Guidelines:**
- **Doxygen-style comments** for all public APIs
- Explain **why** decisions were made, not just what the code does
- Reference **papers/articles** for non-obvious algorithms
- Document **thread-safety** and **performance characteristics**
- Include **usage examples** for complex systems

---

## Testing & Quality

### Code Quality Checklist

Before submitting a PR, ensure:

- [ ] Code compiles without warnings on MSVC
- [ ] Follows all naming and style conventions
- [ ] No raw `CObject*` pointers (uses `TObjectHandle`)
- [ ] Functions are small and focused
- [ ] All templates use concepts
- [ ] Code is well-documented
- [ ] No memory leaks or crashes
- [ ] Performance impact considered


**Test Requirements:**
- Write tests for new features
- Ensure existing tests pass
- Test edge cases and error conditions
- Verify no performance regressions

---

## Pull Request Process

### 1. Before Submitting

- Discuss major changes on Discord or in an issue first
- Ensure code follows all standards above
- Update documentation as needed
- Test thoroughly
- Rebase on latest `main` branch

### 2. Commit Messages

Write clear, descriptive commit messages:

```bash
# GOOD. Clear, specific, imperative mood
Add clustered lighting support to deferred renderer
Fix crash when destroying entities during iteration
Optimize mesh batching for static geometry

# BAD: Vague, unclear, or too casual
Fixed stuff
WIP
Updated things
asdfasdf
```

**Format:**
```
[Category] Brief description (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Explain the problem this commit solves and why you chose
this particular solution.

Fixes #123
```

**Categories:** `Feature`, `Fix`, `Refactor`, `Docs`, `Test`, `Perf`

### 3. Pull Request Template

When opening a PR, include:

```markdown
## Description
Brief overview of what this PR does.

## Motivation
Why is this change needed? What problem does it solve?

## Changes Made
- Added X system
- Refactored Y for better performance
- Fixed crash in Z

## Testing
- [ ] Tested in Editor
- [ ] Tested in Sandbox
- [ ] No performance regression
- [ ] All existing tests pass

## Screenshots (if applicable)
[Add screenshots for visual changes]

## Related Issues
Fixes #123
Related to #456
```

### 4. Review Process

- A maintainer will review your PR within a few days
- Address feedback and requested changes
- Once approved, your PR will be merged!
- You'll be credited in the release notes

---

## 🎯 Priority Areas

Areas where we especially welcome contributions:

### High Priority
- **Performance optimizations** - Profiling and bottleneck fixes
- **Platform support** - Linux/macOS ports
- **Documentation** - API docs, tutorials, examples
- **Bug fixes** - Stability improvements

### Medium Priority
- **Editor features** - Workflow improvements
- **Rendering features** - New techniques and effects
- **Asset pipeline** - Import/export improvements
- **Scripting** - Gameplay scripting support

### Nice to Have
- **Example projects** - Demos and samples
- **Tools** - External editor plugins
- **CI/CD** - Automated testing and builds

---

## Resources

### Learning Materials
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Game Engine Architecture](https://www.gameenginebook.com/)
- [Real-Time Rendering](https://www.realtimerendering.com/)
- [GPU Gems](https://developer.nvidia.com/gpugems/gpugems/contributors)

### Lumina Architecture
- [Engine Overview](docs/EngineOverview.md) *(Coming Soon)*
- [Rendering Architecture](docs/RenderingArchitecture.md) *(Coming Soon)*
- [ECS Guide](docs/ECSGuide.md) *(Coming Soon)*
- [Reflection System](docs/ReflectionSystem.md) *(Coming Soon)*

---

## Questions?

**Don't hesitate to ask!** We're here to help:

- [Discord Server](https://discord.gg/xQSB7CRzQE) - Real-time chat
- [GitHub Issues](https://github.com/mrdrelliot/lumina/issues) - Bug reports and features
- Contact maintainers directly for sensitive matters

---

## Contributors

Thank you to all our contributors! Every contribution, big or small, helps make Lumina better.

<!-- ALL-CONTRIBUTORS-LIST:START -->
<!-- This section will be automatically updated -->
<!-- ALL-CONTRIBUTORS-LIST:END -->

---

## Code of Conduct

### Our Pledge

We are committed to providing a welcoming and inclusive environment for everyone, regardless of:
- Experience level
- Gender identity and expression
- Sexual orientation
- Disability
- Personal appearance
- Body size
- Race or ethnicity
- Age
- Religion
- Nationality

### Our Standards

**Positive behavior includes:**
- Using welcoming and inclusive language
- Being respectful of differing viewpoints
- Gracefully accepting constructive criticism
- Focusing on what's best for the community
- Showing empathy towards others

**Unacceptable behavior includes:**
- Harassment, trolling, or insulting comments
- Personal or political attacks
- Publishing others' private information
- Any conduct inappropriate in a professional setting

### Enforcement

Violations may result in temporary or permanent bans from the project. Report issues to project maintainers via Discord or email.

---

<div align="center">

## Thank You!

**Your contributions make Lumina possible.**

Every bug fix, feature, and documentation improvement helps the entire community learn and grow.

**Let's build something amazing together!**

[Start Contributing →](https://github.com/mrdrelliot/lumina/issues)

</div>
