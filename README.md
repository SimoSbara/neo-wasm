# ☄️ NASA NEO Wasm Viewer

This project was born from a coding challenge: to build an interactive **Near Earth Object (NEO)** viewer using data provided by the **official NASA APIs**.

## 🛠️ Tech Stack

The application was developed using a "vanilla" and low-level approach, focusing on performance and completely avoiding modern frontend frameworks:

*   **Core Logic:** Written in **C** and compiled to **WebAssembly (WASM)** for high-performance processing of spatial data directly in the browser.
*   **Frontend:** Pure **HTML, CSS, and JavaScript**.
*   **UI / Design:** Strictly retro graphical interface, based on the Windows 3.1 theme using the [classic-stylesheets](https://nielssp.github.io/classic-stylesheets/?theme=win3x&skin=3.1) framework.

## 💡 Development & AI Usage

To maintain the spirit of the challenge and full control over the application's core:
*   **Backend / Wasm:** The entire C codebase was designed and written manually, **without the assistance of any Artificial Intelligence tools**.
*   **Frontend:** AI was used *exclusively* as a coding assistant to speed up the development of the graphical interface (HTML structure, CSS layout, and DOM event handling in JavaScript).

---

### 🚀 How to Run the Project
You do not need to download or install any external tools or compilers to simply run the viewer.<br> 
Just start a basic local web server in the project folder (e.g., using Python, VS Code Live Server, or Node.js) and open `index.html` in your browser.

### 🛠️ How to Build from Source
If you want to modify the C core and recompile the WebAssembly module, you will need to:
1. Download the **WASI SDK** (WebAssembly System Interface).
2. Properly configure your **VS Code** environment (tasks and include paths) to compile the C source into `.wasm`.
