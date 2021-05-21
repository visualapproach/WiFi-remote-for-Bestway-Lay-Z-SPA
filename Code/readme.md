Keeping track of versions has been increasingly difficult over the time.
Arduino is really nice and I will still use it, but it has its limits.
To mitigate this problem I decided to move to another platform:
Visual Studio Code (free) with PlatformIO extension (free)
This way I can sync my local files with github, and hopefully use version numbering more correctly.
I'll keep present Arduino files as a separate branch "Backup" until further.
My plan is that future updates (if any) will be made to "development" branch, and pulled to "master" when I think it's time.

After downloading and unpacking the files, you can open VSCode, click on PlatformIO icon and open a project folder. Either "4-wire-version" or "6-wire-version". Edit the file "platformio.ini" to your liking. Upload program then upload filesystem.

If you still want to use Arduino, just rename "main.cpp" to "src.ino" (or "src-for-mqtt.ino" etc. It must have the same name as the parent directory) then move "data" folder to the same folder:
--my source folder
  |-my source folder.ino
  |--data
    |-index.html
    |-...
Install libraries listed in "platformio.ini"