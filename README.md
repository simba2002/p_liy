## Quick Start Guide

### 1. Initial Setup
1. Launch `p_liy.exe`. The application starts silently in the background with the console hidden.
2. Press **`Insert`** to open the **Settings** menu.
3. Paste your **Google Gemini API keys** into the keys area (one key per line).
4. Choose your preferred AI model from the dropdown or type a custom model name manually (default: `gemini-3.5-flash-lite`).
5. Select your interface language (**English**, **Українська**, or **Русский**).
6. Click **Save**. Your configuration and encrypted keys will be stored locally in `config.ini`.

---

### 2. How to Use
* **Analyze Screen (`F8`):** Instantly captures your current screen in memory, queries Gemini, and receives the solution.
* **Toggle Answer (`F9`):** Shows or hides the floating response window with the AI's answer.
* **Toggle Console (`Home`):** Shows or hides the background debugging console for status logs.
* **Change Settings (`Insert`):** Opens the configuration window to update keys, switch models, adjust language, or rebind hotkeys.
* **Exit Program (`F4`):** Terminates the application and cleans up all background processes.

---

### 3. File Structure
* `p_liy.exe` — Standalone executable.
* `config.ini` — Generated automatically next to the executable upon first save; contains encrypted API keys and your hotkey configuration.
