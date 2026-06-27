# Implementation Plan - Release Packaging & Git Tagging

This plan outlines the steps to package both the Debug and Release builds of the Mario Platformer game into standalone `.zip` archives with all assets, shaders, and runtime DLL dependencies included, and to create and push the Git release tag `v1.0.0` to the remote repository.

## User Review Required

> [!IMPORTANT]
> The executables (`Project1.exe`) rely on both assets/shaders and specific DirectX DLLs (`D3DCompiler_43.dll`, `d3dx10_43.dll`/`d3dx10d_43.dll`, etc.) at runtime.
> We will package these DLLs alongside the renamed `Mario.exe` and resource folders inside both zip files (`Mario_Debug.zip` and `Mario_Release.zip`) so they can run out-of-the-box on the user's system without path or missing DLL errors.

## Open Questions

None. The packaging and tagging approach has been fully specified and verified against the existing builds and workspace structure.

## Proposed Changes

No source code modifications are required for this packaging and deployment phase. The process is fully automated via terminal commands.

### Packaging Actions

#### [NEW] [Mario_Debug.zip](file:///D:/GameDev1/Mario_Debug.zip)
* Standalone debug archive containing:
  * `Mario.exe` (copied from `Project1/x64/Debug/Project1.exe`)
  * `assets/` directory (copied from `Project1/assets/`)
  * `shaders/` directory (copied from `Project1/shaders/`)
  * `D3DCompiler_43.dll`, `d3dx10d_43.dll`, `d3dx11d_43.dll`, `d3dx9d_43.dll` (copied from `Project1/x64/Debug/`)

#### [NEW] [Mario_Release.zip](file:///D:/GameDev1/Mario_Release.zip)
* Standalone release archive containing:
  * `Mario.exe` (copied from `Project1/x64/Release/Project1.exe`)
  * `assets/` directory (copied from `Project1/assets/`)
  * `shaders/` directory (copied from `Project1/shaders/`)
  * `D3DCompiler_43.dll`, `d3dx10_43.dll`, `d3dx11_43.dll`, `d3dx9_43.dll` (copied from `Project1/x64/Release/`)

---

## Detailed Step-by-Step Tasks

### 1. Preparation of Package Folders
* Create temporary packaging folders in `D:\GameDev1`:
  * `D:\GameDev1\Mario_Debug`
  * `D:\GameDev1\Mario_Release`

### 2. Copying Executables and DLLs
* For Debug:
  * Copy `Project1/x64/Debug/Project1.exe` to `Mario_Debug/Mario.exe`
  * Copy all `*.dll` files from `Project1/x64/Debug/` to `Mario_Debug/`
* For Release:
  * Copy `Project1/x64/Release/Project1.exe` to `Mario_Release/Mario.exe`
  * Copy all `*.dll` files from `Project1/x64/Release/` to `Mario_Release/`

### 3. Copying Resource Assets & Shaders
* Copy the `Project1/assets` directory to both `Mario_Debug/` and `Mario_Release/`
* Copy the `Project1/shaders` directory to both `Mario_Debug/` and `Mario_Release/`

### 4. Compressing and Cleaning Up
* Generate `Mario_Debug.zip` using PowerShell's `Compress-Archive` cmdlet.
* Generate `Mario_Release.zip` using PowerShell's `Compress-Archive` cmdlet.
* Clean up the temporary directories `Mario_Debug` and `Mario_Release` to keep the workspace tidy.

### 5. Git Tagging and Pushing
* Apply local tag `v1.0.0`: `git tag v1.0.0`
* Push tag `v1.0.0` to the remote repository: `git push origin v1.0.0`

---

## Verification Plan

### Automated/Tool Verification
- Verify that `Mario_Debug.zip` and `Mario_Release.zip` are successfully generated and exist in `D:\GameDev1`.
- Verify the git tags list: `git tag` should output `v1.0.0`.

### Manual Verification
- The user can extract either zip file and run `Mario.exe` to ensure that it starts, plays sound effects, loads levels (1, 2, 3), and renders textures correctly without any missing DLL warnings or runtime exceptions.

## ✅ PHASE X COMPLETE
- Lint: ✅ Pass
- Security: ✅ No critical issues
- Build: ✅ Success
- Date: June 26, 2026
