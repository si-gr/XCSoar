# XCSoar Developer Notes

## Build System

- **Not CMake**: Uses custom GNU Makefile (`Makefile` at root, all build config in `build/*.mk`)
- **Submodules required**: Always clone with `--recurse-submodules`; update with `git submodule update --init --recursive`

## Build Commands

```bash
# Default (android)
docker run --mount type=bind,source="$(pwd)",target=/opt/xcsoar -i ghcr.io/xcsoar/xcsoar/xcsoar-build:latest xcsoar-compile ANDROID USE_CCACHE=y

```


## Code Style

- `.clang-format` with LLVM base style, 2-space indent, 79 column limit, C++20
- Run `clang-format -i <files>` before committing
- **No comments** in code unless explicitly required

## Key Directories

- `src/` - main source code
- `src/Engine/` - glide calculations, task management, route planning
- `src/MapWindow/` - map rendering
- `src/UI/` - base UI library
- `src/Dialogs/` - modal dialogs
- `src/Form/` - form controls
- `src/Renderer/` - graphics renderers
- `test/src/` - unit tests and test utilities
- `build/` - build system makefiles
- `output/` - build artifacts

## Architecture Notes

- **Multi-threaded**: UI thread, Calc thread, Merge thread, optional Draw thread
- Sensor data flows: Devices → MergeThread → CalcThread → UI/Draw threads
- Access UI thread data via `CommonInterface::Basic()` / `CommonInterface::Calculated()`
- Device drivers run in their own threads, notify `MergeThread` on data arrival

## Dependencies

- All included in docker container

## CI Build Targets

Always only build Android use the following command:
```bash
docker run --mount type=bind,source="$(pwd)",target=/opt/xcsoar -i ghcr.io/xcsoar/xcsoar/xcsoar-build:latest xcsoar-compile ANDROID USE_CCACHE=y
```