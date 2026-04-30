# MarkEd

A lightweight Markdown editor built with Qt, featuring a live preview panel and a clean dark interface.

**Try it online: [MaekEdWeb](https://markedweb.netlify.app)**

## Versions

| | Desktop | Web |
|---|---|---|
| Language | C++ / Qt | HTML, CSS, JavaScript |
| Platform | Linux, Windows, macOS | Any browser |
| Install required | Yes | No |

## Features

- **Live preview** - rendered HTML updates as you type
- **Syntax highlighting** - Markdown syntax colored in the editor
- **File tree** - browse and open files from a folder
- **Scroll synchronization** - editor and preview scroll together
- **Auto-save** - automatically saves open files every 30 seconds
- **PDF export** - export your document as a PDF file
- **Zoom controls** - zoom the preview in/out/reset

### Insert shortcuts

| Action | Shortcut |
|---|---|
| Bold | `Ctrl+B` |
| Italic | `Ctrl+I` |
| Strikethrough | `Ctrl+Shift+X` |
| Link | `Ctrl+K` |
| Code block | `Ctrl+Shift+C` |

### View shortcuts

| Action | Shortcut |
|---|---|
| Toggle file tree | `Ctrl+\` |
| Toggle preview | `Ctrl+Shift+P` |
| Zoom in | `Ctrl++` |
| Zoom out | `Ctrl+-` |
| Reset zoom | `Ctrl+0` |

### File shortcuts

| Action | Shortcut |
|---|---|
| New | `Ctrl+N` |
| Save | `Ctrl+S` |
| Save As | `Ctrl+Shift+S` |
| Export as PDF | `Ctrl+E` |

## Requirements

- Qt 6 (with WebEngineWidgets)
- C++17 or later

## Build

```bash
qmake MarkEd.pro
make
```

Or open `MarkEd.pro` in Qt Creator and build from there.
