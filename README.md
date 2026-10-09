# Particles

A simple bouncing ball simulation written in C using Raylib.

## Controls

Spawn particles with left click, create boxes by dragging right click and switch their types with left & right arrow keys.
Clear all particles: C

## How to run it

You can view [the web demo](https://papertarsier692.github.io/particles/build/particles.html) or compile it yourself.

### Linux / MacOS

To compile it run in the root directory:

`cmake .`
`make`
`./particles`

You may have to make the compiled file executable:
`chmod +x particles`

### Web Version

To compile the web version yourself ensure you have the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) installed.

Then run:
`emcmake cmake -S . -B build -DPLATFORM=Web`
`cmake --build build`

To access the web build you need to run a local HTTP server:
`python3 -m http.server <PORT>`
