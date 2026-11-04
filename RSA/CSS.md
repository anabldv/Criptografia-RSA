# CSS  
:root {  
  --bg-main: #050806;  
  --bg-panel: #0a110c;  
  --bg-input: #040705;  
  --border: #16261a;  
  --border-focus: #00ff66;  
  --primary: #00ff66;  
  --primary-glow: rgba(0, 255, 102, 0.2);  
  --primary-hover: #00cc52;  
  --text-main: #e6f7ec;  
  --text-muted: #5f8067;  
  --font-mono: 'JetBrains Mono', monospace;  
}  
  
* {  
  box-sizing: border-box;  
  margin: 0;  
  padding: 0;  
}  
  
body {  
  background-color: var(--bg-main);  
  color: var(--text-main);  
  font-family: var(--font-mono);  
  min-height: 100vh;  
  display: flex;  
  justify-content: center;  
  align-items: center;  
  padding: 24px;  
}  
  
.container {  
  width: 100%;  
  max-width: 680px;  
}  
  
.header {  
  text-align: center;  
  margin-bottom: 24px;  
}  
  
.terminal-badge {  
  display: inline-block;  
  font-size: 11px;  
  color: var(--primary);  
  border: 1px solid var(--border);  
  padding: 4px 10px;  
  border-radius: 4px;  
  margin-bottom: 12px;  
  letter-spacing: 1px;  
}  
  
.header h1 {  
  font-size: 1.6rem;  
  color: var(--text-main);  
  font-weight: 700;  
  margin-bottom: 6px;  
}  
  
.header p {  
  font-size: 0.85rem;  
  color: var(--text-muted);  
}  
  
.panel {  
  background: var(--bg-panel);  
  border: 1px solid var(--border);  
  border-radius: 8px;  
  padding: 24px;  
  box-shadow: 0 10px 30px rgba(0, 0, 0, 0.7);  
}  
  
.box-group {  
  margin-bottom: 16px;  
}  
  
.box-label {  
  display: flex;  
  justify-content: space-between;  
  font-size: 0.75rem;  
  font-weight: 600;  
  letter-spacing: 0.5px;  
  color: var(--primary);  
  margin-bottom: 8px;  
}  
  
.counter {  
  color: var(--text-muted);  
  font-weight: 400;  
}  
  
textarea {  
  width: 100%;  
  height: 120px;  
  background: var(--bg-input);  
  border: 1px solid var(--border);  
  border-radius: 6px;  
  color: var(--text-main);  
  font-family: var(--font-mono);  
  font-size: 0.9rem;  
  padding: 12px;  
  resize: vertical;  
  outline: none;  
  transition: border-color 0.2s, box-shadow 0.2s;  
}  
  
textarea:focus {  
  border-color: var(--border-focus);  
  box-shadow: 0 0 10px var(--primary-glow);  
}  
  
.controls {  
  display: flex;  
  flex-wrap: wrap;  
  gap: 12px;  
  align-items: center;  
  justify-content: space-between;  
  margin: 16px 0 20px 0;  
  padding-bottom: 16px;  
  border-bottom: 1px dashed var(--border);  
}  
  
.method-selector, .cesar-config {  
  display: flex;  
  align-items: center;  
  gap: 8px;  
  font-size: 0.85rem;  
  color: var(--text-muted);  
}  
  
select, input[type="number"] {  
  background: var(--bg-input);  
  border: 1px solid var(--border);  
  color: var(--primary);  
  font-family: var(--font-mono);  
  font-size: 0.85rem;  
  padding: 6px 10px;  
  border-radius: 4px;  
  outline: none;  
}  
  
input[type="number"] {  
  width: 60px;  
}  
  
.hidden {  
  display: none !important;  
}  
  
.action-buttons {  
  display: flex;  
  gap: 8px;  
  margin-left: auto;  
}  
  
.btn {  
  font-family: var(--font-mono);  
  font-size: 0.85rem;  
  font-weight: 600;  
  padding: 8px 14px;  
  border-radius: 4px;  
  cursor: pointer;  
  transition: all 0.2s;  
}  
  
.btn.primary {  
  background: var(--primary);  
  color: #000;  
  border: none;  
}  
  
.btn.primary:hover {  
  background: var(--primary-hover);  
  box-shadow: 0 0 8px var(--primary-glow);  
}  
  
.btn.secondary {  
  background: transparent;  
  color: var(--primary);  
  border: 1px solid var(--primary);  
}  
  
.btn.secondary:hover {  
  background: rgba(0, 255, 102, 0.1);  
}  
  
.btn.ghost {  
  background: transparent;  
  color: var(--text-muted);  
  border: 1px solid var(--border);  
}  
  
.btn.ghost:hover {  
  color: #ff5555;  
  border-color: #ff5555;  
}  
  
.output-wrapper {  
  position: relative;  
}  
  
.output-wrapper textarea {  
  color: var(--primary);  
  padding-right: 90px;  
}  
  
.btn-copy {  
  position: absolute;  
  top: 10px;  
  right: 10px;  
  background: rgba(10, 17, 12, 0.85);  
  border: 1px solid var(--border);  
  color: var(--text-muted);  
  font-family: var(--font-mono);  
  font-size: 0.75rem;  
  padding: 5px 8px;  
  border-radius: 4px;  
  display: flex;  
  align-items: center;  
  gap: 6px;  
  cursor: pointer;  
  backdrop-filter: blur(4px);  
  transition: all 0.2s;  
}  
  
.btn-copy:hover {  
  border-color: var(--primary);  
  color: var(--primary);  
}  
  
@media (max-width: 600px) {  
  .controls {  
    flex-direction: column;  
    align-items: stretch;  
  }  
  .action-buttons {  
    margin-left: 0;  
    justify-content: stretch;  
  }  
  .action-buttons .btn {  
    flex: 1;  
  }  
}  
