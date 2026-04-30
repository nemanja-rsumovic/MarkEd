let fileHandle = null;
let unsavedChanges = false;
let previewZoom = 1.0;
let previewVisible = true;

const preview = document.getElementById("preview");
const statusMsg = document.getElementById("status-msg");
const wordCountEl = document.getElementById("word-count");
const cursorPosEl = document.getElementById("cursor-pos");

const cm = CodeMirror.fromTextArea(document.getElementById("editor"), {
    mode: "markdown",
    lineNumbers: true,
    lineWrapping: true,
    theme: "default",
    indentWithTabs: false,
    tabSize: 4,
    autofocus: true,
});

/* Preview update */

let previewTimer = null;

cm.on("change", () => {
    markUnsaved();
    clearTimeout(previewTimer);
    previewTimer = setTimeout(updatePreview, 300);
    updateWordCount();
});

cm.on("cursorActivity", () => {
    const c = cm.getCursor();
    cursorPosEl.textContent = `Ln ${c.line + 1}, Col ${c.ch + 1}`;
});

function updatePreview() {
    preview.innerHTML = marked.parse(cm.getValue());
}

function updateWordCount() {
    const text = cm.getValue().trim();
    const words = text ? text.split(/\s+/).length : 0;
    wordCountEl.textContent = `${words} word${words !== 1 ? "s" : ""}`;
}

function markUnsaved() {
    unsavedChanges = true;
    document.title = "MarkEd *";
}

function markSaved() {
    unsavedChanges = false;
    document.title = fileHandle ? `MarkEd — ${fileHandle.name}` : "MarkEd";
}

function showStatus(msg, duration = 2000) {
    statusMsg.textContent = msg;
    setTimeout(() => { statusMsg.textContent = ""; }, duration);
}

/* Auto-save */

setInterval(() => {
    if (unsavedChanges) saveFile();
}, 30000);

/* File operations */

let currentFileName = "document.md";

function newFile() {
    if (unsavedChanges && !confirm("Discard unsaved changes?")) return;
    cm.setValue("");
    fileHandle = null;
    currentFileName = "document.md";
    markSaved();
    updatePreview();
}

function openFile() {
    if (window.showOpenFilePicker) {
        openWithFilesystemAPI();
    } else {
        openWithInput();
    }
}

async function openWithFilesystemAPI() {
    try {
        [fileHandle] = await window.showOpenFilePicker({
            types: [{ description: "Markdown", accept: { "text/markdown": [".md", ".markdown"] } }]
        });
        const file = await fileHandle.getFile();
        currentFileName = file.name;
        cm.setValue(await file.text());
        markSaved();
        updatePreview();
        updateWordCount();
    } catch {}
}

function openWithInput() {
    const input = document.createElement("input");
    input.type = "file";
    input.accept = ".md,.markdown";
    input.onchange = async () => {
        const file = input.files[0];
        if (!file) return;
        currentFileName = file.name;
        cm.setValue(await file.text());
        fileHandle = null;
        markSaved();
        updatePreview();
        updateWordCount();
    };
    input.click();
}

async function saveFile() {
    if (fileHandle) {
        try {
            const writable = await fileHandle.createWritable();
            await writable.write(cm.getValue());
            await writable.close();
            markSaved();
            showStatus("Saved");
            return;
        } catch {}
    }
    downloadFile(currentFileName);
}

function saveFileAs() {
    if (window.showSaveFilePicker) {
        saveWithFilesystemAPI();
    } else {
        const name = prompt("File name:", currentFileName);
        if (name) { currentFileName = name; downloadFile(name); }
    }
}

async function saveWithFilesystemAPI() {
    try {
        fileHandle = await window.showSaveFilePicker({
            suggestedName: currentFileName,
            types: [{ description: "Markdown", accept: { "text/markdown": [".md"] } }]
        });
        const writable = await fileHandle.createWritable();
        await writable.write(cm.getValue());
        await writable.close();
        currentFileName = fileHandle.name;
        markSaved();
        showStatus("Saved");
    } catch {}
}

function downloadFile(filename) {
    const blob = new Blob([cm.getValue()], { type: "text/markdown" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = filename || "document.md";
    a.click();
    URL.revokeObjectURL(url);
    markSaved();
    showStatus("Downloaded");
}

function exportPdf() {
    window.print();
}

/* Insert helpers */

function wrapSelection(before, after) {
    const sel = cm.getSelection();
    if (sel) {
        cm.replaceSelection(before + sel + after);
    } else {
        const cur = cm.getCursor();
        cm.replaceRange(before + after, cur);
        cm.setCursor({ line: cur.line, ch: cur.ch + before.length });
    }
    cm.focus();
}

function prependToLine(prefix) {
    const cur = cm.getCursor();
    const line = cm.getLine(cur.line);
    cm.replaceRange(prefix + line, { line: cur.line, ch: 0 }, { line: cur.line, ch: line.length });
    cm.focus();
}

function insertBold()          { wrapSelection("**", "**"); }
function insertItalic()        { wrapSelection("*", "*"); }
function insertStrikethrough() { wrapSelection("~~", "~~"); }
function insertH1()            { prependToLine("# "); }
function insertH2()            { prependToLine("## "); }
function insertH3()            { prependToLine("### "); }
function insertHR()            { cm.replaceRange("\n---\n", cm.getCursor()); cm.focus(); }

function insertLink() {
    const sel = cm.getSelection();
    const text = sel || "link text";
    cm.replaceSelection(`[${text}](url)`);
    cm.focus();
}

function insertCodeBlock() {
    const cur = cm.getCursor();
    cm.replaceRange("```\n\n```", cur);
    cm.setCursor({ line: cur.line + 1, ch: 0 });
    cm.focus();
}

/* View */

function togglePreview() {
    previewVisible = !previewVisible;
    document.getElementById("preview-panel").style.display = previewVisible ? "" : "none";
    document.getElementById("divider").style.display = previewVisible ? "" : "none";
}

function zoomIn() {
    if (previewZoom < 3.0) {
        previewZoom = Math.min(3.0, previewZoom + 0.1);
        applyZoom();
    }
}

function zoomOut() {
    if (previewZoom > 0.3) {
        previewZoom = Math.max(0.3, previewZoom - 0.1);
        applyZoom();
    }
}

function zoomReset() {
    previewZoom = 1.0;
    applyZoom();
}

function applyZoom() {
    preview.style.fontSize = (previewZoom * 15) + "px";
}

/* Scroll sync */

const previewPanel = document.getElementById("preview-panel");

cm.on("scroll", () => {
    const info = cm.getScrollInfo();
    const maxScroll = info.height - info.clientHeight;
    if (maxScroll <= 0) return;
    const pct = info.top / maxScroll;
    previewPanel.scrollTop = pct * (previewPanel.scrollHeight - previewPanel.clientHeight);
});

/* Keyboard shortcuts */

document.addEventListener("keydown", (e) => {
    if (e.ctrlKey && !e.shiftKey && e.key === "n") { e.preventDefault(); newFile(); }
    if (e.ctrlKey && !e.shiftKey && e.key === "o") { e.preventDefault(); openFile(); }
    if (e.ctrlKey && !e.shiftKey && e.key === "s") { e.preventDefault(); saveFile(); }
    if (e.ctrlKey &&  e.shiftKey && e.key === "S") { e.preventDefault(); saveFileAs(); }
    if (e.ctrlKey &&  e.shiftKey && e.key === "P") { e.preventDefault(); togglePreview(); }
    if (e.ctrlKey && (e.key === "=" || e.key === "+")) { e.preventDefault(); zoomIn(); }
    if (e.ctrlKey && e.key === "-")                    { e.preventDefault(); zoomOut(); }
    if (e.ctrlKey && e.key === "0")                    { e.preventDefault(); zoomReset(); }
    if (e.ctrlKey &&  e.shiftKey && e.key === "X") { e.preventDefault(); insertStrikethrough(); }
    if (e.ctrlKey && e.key === "k")                 { e.preventDefault(); insertLink(); }
    if (e.ctrlKey &&  e.shiftKey && e.key === "C") { e.preventDefault(); insertCodeBlock(); }
});

/* Prevent browser zoom with Ctrl+scroll */
document.addEventListener("wheel", (e) => {
    if (e.ctrlKey) e.preventDefault();
}, { passive: false });

/* Print style for PDF export */
const printStyle = document.createElement("style");
printStyle.textContent = `
    @media print {
        #toolbar, #editor-panel, #divider { display: none !important; }
        #preview-panel { all: unset; display: block; }
        #preview { max-width: 100%; color: black; font-family: serif; }
        #preview h1, #preview h2, #preview h3 { color: black; border-color: #ccc; }
        #preview code, #preview pre { background: #f5f5f5; color: black; }
        #preview blockquote { border-color: #999; color: #555; }
    }
`;
document.head.appendChild(printStyle);

/* Init */
updatePreview();
updateWordCount();
