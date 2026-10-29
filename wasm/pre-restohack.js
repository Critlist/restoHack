/* wasm/pre-restohack.js - restoHack-specific pre-js additions.
 *
 * Linked after the unix-tui-to-wasm pre.js (both are concatenated into the
 * module by --pre-js, in link order; see CMakeLists.txt). The browser has
 * no login identity (Emscripten defaults USER/LOGNAME to "web_user"):
 * unset both so the game's name lookup comes up empty and it asks the
 * player for a name instead of saving games as "web_user".
 *
 * NOTE: --pre-js content shares the module scope, so `Module` and `ENV`
 * are already in scope here. Do not redeclare them.
 */
Module["preRun"] = Module["preRun"] || [];
Module["preRun"].push(function () {
  if (typeof ENV !== "undefined") {
    ENV.USER = undefined;
    ENV.LOGNAME = undefined;
  }
});
