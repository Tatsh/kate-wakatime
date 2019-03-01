# Note

This is for Kate 5 (KTextEditor from KF 5). If you need a version for Kate 4, use the [v0.4 release](https://github.com/Tatsh/kate-wakatime/releases).

# How to use

1. Get an account at [WakaTime](https://wakatime.com).
2. Get your [API key](https://wakatime.com/settings).
3. Clone this project and compile:
   ```bash
   git clone git@github.com:Tatsh/kate-wakatime.git
   cd kate-wakatime
   mkdir build
   cd build
   ```

   Linux:

   ```bash
   cmake .. -DCMAKE_INSTALL_PREFIX=/usr
   ```

   MacPorts:

   ```bash
   cmake .. -DCMAKE_INSTALL_PREFIX=/opt/local/
   ```

   Finish the task:

   ```bash
   make
   sudo make install
   ```

4. Once this plugin is installed, open Kate and go to *Settings*, *Configure Kate...*, then in the dialog choose *Plugins*.
5. Use the checkbox to enable *WakaTime* and click *OK*:

   ![screenshot](https://user-images.githubusercontent.com/724848/53671349-f6a91280-3c4b-11e9-88b9-01f2cdc3cf67.png)

6. Restart Kate to be sure the plugin initialises properly.
7. Go to *Settings*, *Configure WakaTime...*. In the dialog, fill in your API key. Click *OK* to save.

To be certain this will work, check the file at `~/.wakatime.cfg`.

# Is this a keylogger?

Short answer is no.

See the [`WakaTimeView::sendAction()`](https://github.com/Tatsh/kate-wakatime/blob/master/wakatimeplugin.cpp#L198) method if you want to be certain.
