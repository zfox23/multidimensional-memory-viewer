# Multidimensional Memory Viewer

A native macOS (and future Android) application for viewing **Multidimensional Memories (MDMs)** — a personal media format that pairs stereoscopic VR180 still images with First-Order Ambisonic spatial audio. The viewer projects the image onto a virtual hemisphere, plays the looping audio, and rotates the ambisonic sound field in real time as you pan around the scene with your mouse.

---

## Features

- Equirectangular hemisphere projection via a Metal-accelerated GLSL shader
- Real-time First-Order Ambisonic (FOA) B-format head tracking — audio sources stay anchored to their real-world positions as you pan
- Seamlessly looping 4-channel spatial audio decoded to stereo headphone output
- Asynchronous loading of 8192×4096 images — the UI stays responsive while large files decode
- Thumbnail strip showing all MDMs in the selected folder; click any thumbnail to jump to it
- **M** key toggles mute
- Smooth fade-to-black at the 180° hemisphere boundary (no hard edge)
- Folder structure prepared for Android and VR platform support

---

## MDM File Format

### Still Image

MDM images are captured with a **Canon EOS R5** fitted with the **Canon RF 5.2mm F2.8L Dual Fisheye Lens**. This lens captures a stereoscopic VR180 scene — both fisheye elements together cover a 180° forward hemisphere.

The capture-to-viewer pipeline:

1. Raw capture on the R5
2. Developed and colour-graded in **Adobe Lightroom**, exported as JPG
3. Re-processed in **Canon EOS VR Utility**, which rectifies the dual-fisheye geometry and outputs a **Side-By-Side (SBS) equirectangular** JPG

The final image is **8192 × 4096 pixels**:

```
┌─────────────────────────┬─────────────────────────┐
│                         │                         │
│       Left eye          │       Right eye         │
│    4096 × 4096 px       │    4096 × 4096 px       │
│  equirectangular 1:1    │  equirectangular 1:1    │
│                         │                         │
└─────────────────────────┴─────────────────────────┘
```

Each half is a **VR180 equirectangular projection** covering:
- **Horizontal**: −90° to +90° azimuth (the forward hemisphere)
- **Vertical**: −90° to +90° elevation (full vertical range)

The viewer uses only the **left-eye half** for monoscopic desktop display. The right-eye half is reserved for future stereoscopic VR rendering.

### Spatial Audio

Audio is captured with a **Zoom H2n** portable recorder in its native **4-channel B-format Ambisonic** mode, then processed in **Audacity** to:

- Normalise signal amplitude
- Trim to approximately 15 seconds
- Create a seamless loop (fade-to-loop at the edit point)

The finished audio is exported as a **4-channel Opus file** (`.ambisonic.opus`) encoded at 48 kHz. The four channels carry **First-Order Ambisonics (FOA) B-format** signals in **FuMa channel order**:

| Channel | Signal | Description |
|---------|--------|-------------|
| 0 | W | Omnidirectional (pressure), normalised to 1/√2 |
| 1 | X | Front–back figure-of-eight |
| 2 | Y | Left–right figure-of-eight |
| 3 | Z | Up–down figure-of-eight |

At playback, the viewer applies a **yaw/pitch rotation matrix** to the B-format channels based on the current camera orientation, then decodes to stereo using a **virtual speaker pair at ±30° azimuth** (ITU-R BS.775 stereo). This means that as you pan left or right, sound sources rotate to match their apparent on-screen positions.

### Filename Convention

```
2026-04-25 13-48-43 ZFP_3447.JPG
└──────────────────┘ └─────────┘
  Timestamp prefix    Camera identifier

2026-04-25 13-48-43 ZFP_3447.JPG SPTL004.ambisonic.opus
                    └──────────┘ └──────────────────────┘
                     Image token   Audio file suffix
```

A single audio file may be associated with **multiple images** by listing all image tokens in its filename:

```
2026-04-25 13-48-43 ZFP_3447.JPG ZFP_3448.JPG SPTL004.ambisonic.opus
```

The viewer matches audio to images by extracting every `ZFP_XXXX.JPG` token from each audio filename, then finding the image file whose name ends with that token. Images with no matching audio file are shown in the viewer without sound.

---

## Building

### Prerequisites

- macOS 13 or later (Apple Silicon or Intel)
- Xcode Command Line Tools: `xcode-select --install`
- [Homebrew](https://brew.sh)

Install build dependencies:

```bash
brew install qt opusfile cmake
```

### Build

```bash
git clone <repo-url>
cd multidimensional-memory-viewer

cmake -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The application bundle is produced at `build/mmv.app`.

### Run

```bash
open build/mmv.app
```

Or from the command line (useful for seeing log output):

```bash
./build/mmv.app/Contents/MacOS/mmv
```

### Run Tests

```bash
cd build && ctest --output-on-failure
```

---

## Usage

1. Launch the app
2. Click **Open Folder** and select a folder containing MDM files
3. The first MDM loads automatically — the image is projected onto the hemisphere and the audio begins playing
4. **Pan**: click and drag anywhere on the scene to look around; audio sources track your view direction
5. **Mute**: click the **Mute** button in the toolbar or press **M**
6. **Switch MDMs**: click any thumbnail in the strip at the bottom; the view resets to forward and the new audio starts

---

## Project Structure

```
multidimensional-memory-viewer/
├── CMakeLists.txt              Root build file
├── src/
│   ├── core/                   Platform-agnostic logic
│   │   ├── mdm/                MDM data model and folder scanner
│   │   ├── audio/              Opus decoder, ambisonic decoder, audio engine
│   │   └── rendering/          Async texture/thumbnail loader
│   ├── ui/                     Qt Quick bridge (AppController, models, image provider)
│   ├── platforms/
│   │   ├── macos/              macOS-specific code
│   │   └── android/            Android stub (future)
│   └── main.cpp
├── qml/                        QML UI components
├── shaders/                    GLSL hemisphere projection shaders
└── tests/                      Unit tests (MdmScanner filename matching)
```

---

## Technology Stack

| Component | Technology |
|-----------|-----------|
| UI framework | Qt 6 / Qt Quick (QML) |
| Rendering | QML `ShaderEffect` with SPIR-V shaders (`qt_add_shaders`) |
| Rendering backend | Metal (macOS), Vulkan/OpenGL ES (Android, future) |
| Audio output | `QAudioSink` with custom `QIODevice` pull model |
| Opus decoding | `opusfile` (libopus/Xiph.Org) |
| Image loading | Qt Quick `Image` (async, GPU upload handled by scene graph) |
| Build system | CMake 3.25+ |
