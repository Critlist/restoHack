# restoHack in the browser (WebAssembly port)

Play Hack 1.0.3 (1985) in a web page. Emscripten compiles the C sources to
`hack.js`/`hack.wasm`; an [xterm.js] terminal talks to the game through the
[xterm-pty] bridge.

The generic browser infrastructure comes from the [forked unix-tui-to-wasm]
template, included as the git submodule `wasm/utw`: termcap shim, xterm-pty
link setup, IDBFS persistence and bundle staging. Only the restoHack
specific pieces live next to this file:

| File | Purpose |
| ---- | ------- |
| `wasm/index.html.in` | page (title, save list) generated into the bundle |
| `wasm/pre-restohack.js` | unsets USER/LOGNAME so the game asks for a name |
| `wasm/build-wasm.sh` | wrapper around the template's build script |

## Tools you need installed

| Tool | Why | Get it |
| ---- | --- | ------ |
| Emscripten SDK (`emcc`, `emcmake`) | the wasm compiler | pinned version, see below |
| CMake (≥3.16), a host C compiler (`cc`) | configure + host-built `makedefs` | distro package (`cmake`, `build-essential`/`base-devel`) |
| Node.js | ships with emsdk tooling | distro package or nodejs.org |
| Python 3 (`http.server`) | serve the build locally (any static server works) | distro package |
| A modern browser | run it | Chrome/Firefox/Safari, current |

The build needs network access: it fetches the pinned browser assets
(xterm.js and xterm-pty, with their Emscripten glue) from unpkg and verifies
their SHA-256s. The built bundle references only local files, so the page
plays fully offline.

## Emscripten version

The template pins Emscripten **3.1.47** (`wasm/utw/wasm/emsdk-version`), the
version xterm-pty 0.12.0 is verified against:

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install 3.1.47
./emsdk activate 3.1.47
source ./emsdk_env.sh
```

`build-wasm.sh` refuses to build with another active `emcc`. To try a newer
SDK, set `ALLOW_UNTESTED_EMSDK=1`; those builds are separate from the pinned
configuration.

## Build and run

```bash
git submodule update --init wasm/utw   # first checkout only
./wasm/build-wasm.sh
cd build/wasm && python3 -m http.server 8000
# open http://localhost:8000/
```

The wrapper points the template's build script at this tree: bundle in
`build/wasm/`, CMake build in `build/wasm-build/`. The script fetches the
pinned assets (`xterm.js`/`xterm.css`/`xterm-pty.js` into the bundle dir,
`emscripten-pty.js` for the link step - see `wasm/.gitignore`), runs CMake,
and the `hack_web` target stages `index.html`, `storage.js` plus
`hack.js`/`hack.wasm`/`hack.data` - a self-contained directory holding only
the files needed to run. Serve it (or copy it anywhere) and play offline.
No special headers (COOP/COEP) are needed: the build uses Asyncify on the
main thread, not pthreads.

To update the infrastructure, bump the submodule pointer (e.g.
`git submodule update --remote wasm/utw`) and rebuild.

Enjoy!

---

[xterm.js]: https://github.com/xtermjs/xterm.js
[xterm-pty]: https://github.com/mame/xterm-pty
[unix-tui-to-wasm]: https://github.com/Critlist/unix-tui-to-wasm
