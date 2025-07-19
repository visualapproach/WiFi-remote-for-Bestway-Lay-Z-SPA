# WiFi remote for Bestway Lay-Z-SPA (fork – i18n + UI update)
=================================

Fork based on the great original project by [visualapproach](https://github.com/visualapproach).  
This version includes:

- 🌐 Internationalization (i18n) The UI is now available in six languages:

    🇬🇧 English
    🇫🇷 Français
    🇪🇸 Español
    🇮🇹 Italiano
    🇩🇪 Deutsch
    🇵🇹 Português

- 🎨 Updated CSS styles (modern, mobile-first layout)
- 🖼️ Refreshed UI with a pseudo-LCD display and icon tweaks
- 📁 All web files are stored under `/data/` and easily editable (HTML, CSS, JS, TXT)

> No changes were made to the ESP8266 core logic – only the web interface.

---

## Preview

Here are a few screenshots of the updated UI with multilingual support and enhanced styling:

![screenshot](https://raw.githubusercontent.com/dodemodexter/WiFi-remote-for-Bestway-Lay-Z-SPA/master/Code/Screenshots/05.png)

![screenshot](https://raw.githubusercontent.com/dodemodexter/WiFi-remote-for-Bestway-Lay-Z-SPA/master/Code/Screenshots/01.png)

![screenshot](https://raw.githubusercontent.com/dodemodexter/WiFi-remote-for-Bestway-Lay-Z-SPA/master/Code/Screenshots/02.png)

![screenshot](https://raw.githubusercontent.com/dodemodexter/WiFi-remote-for-Bestway-Lay-Z-SPA/master/Code/Screenshots/03.png)

![screenshot](https://raw.githubusercontent.com/dodemodexter/WiFi-remote-for-Bestway-Lay-Z-SPA/master/Code/Screenshots/04.png)

![screenshot](https://raw.githubusercontent.com/dodemodexter/WiFi-remote-for-Bestway-Lay-Z-SPA/master/Code/Screenshots/06.png)

---

## How to customize

Each language is defined in a .txt file (e.g., Langue_fr-FR.txt) located in the /data directory. These files are fully editable and allow easy customization or translation updates.

➕ Add a new language

To add support for another language:

1. Create a new `.txt` file named `Langue_xx-XX.txt` in the `/data` folder, using the same key/value structure.
2. Edit the file `webconfig.html` and:

   - Add a new `<option>` entry inside the hidden `<select>` used by the script:

     ```html
     <option value="xx-XX">YourLanguage</option>
     ```

   - Add a new flag inside the `#lang-flags` block:

     ```html
     <span class="flag" data-lang="xx-XX" title="YourLanguage">🌐</span>
     ```

That’s all — the system will automatically pick up and apply the new language file via `i18n.js`.

---

## Credit

This fork exists thanks to the amazing original work of [visualapproach](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA).

All license terms from the original repo still apply.
