
<div align="center">

# 3D Graphics Engine

**Development of a 3D graphics engine from scratch for educational purposes.**

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)](#)
[![Version](https://img.shields.io/badge/version-0.1.0-orange.svg)](#)

[Demo](#demo) · [Getting Started](#getting-started)

</div>

---

## Table of Contents

- [About](#about)
- [Features](#features)
- [Demo](#demo)
- [Tech Stack](#tech-stack)
- [Getting Started](#getting-started)
- [Usage](#usage)
- [Project Structure](#project-structure)
- [License](#license)
- [Contact](#contact)
- [Acknowledgements](#acknowledgements)

## About

While reading 3D Math Primer for Graphics and Game Development (2nd ed.) by Dunn and Parberry (2011), I wanted to put the theory into practice by implementing some of the mathematical concepts and formulas directly in code and observing their results. I believe that connecting theoretical concepts with practical implementation is essential to truly understanding how 3D graphics work.

Inspired by the YouTube series "Code-It-Yourself! 3D Graphics Engine" by javidx9, I decided to build a simple 3D graphics engine from scratch, without relying on a GPU or graphics APIs.

This project is not intended to provide anything beyond what is already covered in the original YouTube series. Instead, it offers two small adaptations:

1. **macOS compatibility:** I implemented and adapted the code to run on a MacBook Pro with an M4 Max chip.
2. **Educational documentation:** I added extensive comments throughout the code to explain what each part does, making it easier to understand the underlying concepts and follow the implementation step by step.

If you're using a Mac and want to experiment with a 3D graphics engine, or simply want to explore how the code works and understand the theory behind it, this project might be a good starting point!

## Features

- Triangles and Projections
- Normals
- Culling
- Lighting and Object Files
- Cameras and Clipping
- Texturing and Depth Buffers

## Demo

A short video showcasing the 3D Graphics Engine using a 3D object file and a texture generated with Claude.
![Screenshot](docs/screenshot.png)

## Tech Stack

olcPixelGameEngine: free, open-source C++ framework for 2D graphics, games, and prototypes, created by javidx9 (David Barr) of OneLoneCoder (OLC-3 License).

## Getting Started

### Prerequisites

- A Mac with an Apple Silicon (M-series) processor.
- C++ installed, including a working C++ compiler.

### Installation

1. Clone the repository
   ```bash
   git clone https://github.com/lmsbuildsworlds/3d_graphics_engine.git
   cd [repo]
   ```
2. Compile and run the project: 
   ```bash
   clang++ -std=c++17 -g -Wall main.cpp -o main
   ```
**Build note (macOS):** If you hit build errors like `'png.h' file not found`, `library not found for -lpng`, or undefined OpenGL/GLUT symbols, use the command below. It compiles `main.cpp` and runs it, and it fixes two common issues:

- **Linking Apple's frameworks:** `-framework OpenGL -framework GLUT -framework Carbon` links the graphics and windowing libraries macOS provides.
- **Finding libpng:** `-I` and `-L` use `brew --prefix libpng` to point the compiler to the Homebrew install, which works on both Intel and Apple Silicon Macs. (Requires `brew install libpng`).

   ```bash
   clang++ -std=c++17 -g -Wall main.cpp -o main \
   -framework OpenGL -framework GLUT -framework Carbon \
   -lpng \
   -I"$(brew --prefix libpng)/include" \
   -L"$(brew --prefix libpng)/lib" \
   && ./main; echo "exit code: $?"
   ```

> **Tip:** Apple has deprecated OpenGL and GLUT, so you may see deprecation warnings. They're harmless, and you can silence them by adding `-Wno-deprecated-declarations`.

## Usage

You can use the `.obj` and texture files in the `assets` folder. If you want to use your own, be careful: the loader only supports a simplified subset of the OBJ format.

**Supported**
- **Vertices:** lines starting with `v`, followed by three floats (x, y, z).
- **Texture coordinates:** lines starting with `vt`, followed by two floats (u, v).
- **Faces:** lines starting with `f`, using either plain vertex indices (`f 1 2 3`) or the `v/vt` format (`f 1/1 2/2 3/3`).

**Requirements**
- **Triangles only:** every face must be a triangle. If your model contains quads or larger polygons, triangulate it first.

**Ignored**
- Normals (`vn`), materials (`.mtl`), groups, and comments. Normals aren't needed because the engine calculates its own per triangle using the cross product.

**Not supported**
- The `v/vt/vn` and `v//vn` face formats, and negative (relative) indices.

## Project Structure

```
.
├── assets/                 # 3D models (.obj) and textures
├── docs/                   # Documentation and screenshots
├── main.cpp                # Engine implementation and entry point
├── olcPixelGameEngine.h    # olcPixelGameEngine (single-header library)
└── README.md
```

## License

Distributed under the [MIT License](LICENSE). See `LICENSE` for more information.

## Contact

Linkedin: [@loicmartins](https://linkedin.com/loicmartins)

Project link: [https://github.com/lmsbuildsworlds/3d_graphics_engine](lmsbuildsworlds/3d_graphics_engine)

## Acknowledgements

- Pixel Game Engine by javidx9 (OneLoneCoder), OLC-3 License: https://github.com/OneLoneCoder/olcPixelGameEngine.
- Dunn, F., & Parberry, I. (2011). 3D math primer for graphics and game development (2nd ed.).
