# Note

This is for Kate 5 (KTextEditor from KF 5). If you need a version for Kate 4, use the [v0.4 release](https://github.com/Tatsh/kate-wakatime/releases).

# How to use

1. Get an account at [WakaTime](https://wakatime.com).
2. Get your [API key](https://wakatime.com/settings).
3. Create a file `~/.wakatime.cfg` and make sure it looks similar to this:

   ```ini
   [settings]
   api_key = myapikey-0000-0000-0000-000000000000
   ```

4. Clone this project and compile:

   ```bash
   git clone git@github.com:Tatsh/kate-wakatime.git
   cd kate-wakatime
   mkdir build
   cd build
   ```

   If on 64-bit:

   ```bash
   cmake .. -DCMAKE_INSTALL_PREFIX=/usr
   ```

   If on 32-bit:

   ```bash
   cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DLIB_SUFFIX=""
   ```

   MacPorts (tested on Mid-2012 MacBook Pro running Yosemite):

   ```bash
   cmake .. -DCMAKE_INSTALL_PREFIX=/opt/local/ -DLIB_SUFFIX=""
   ```

   Finish the task:

   ```bash
   make
   ```

6. You can choose to install with `sudo make install` or you can set the `KDEDIRS` environment variable. You may need to run `kbuildsycoca5` as yourself once before launching Kate again (this is true on MacPorts).
7. Once this plugin is installed, open Kate (or any other katepart editor (KWrite, etc)) and go to *Settings*, *Configure <name>...*, then in the dialog choose *Extensions*.
8. Use the checkbox to enable *WakaTime* and click *OK*:

   ![screenie](https://cloud.githubusercontent.com/assets/724848/5060831/c4f5d0ca-6d24-11e4-8257-568e697a197b.png)

9. Restart Kate to be sure the plugin initialises properly.

Make sure you absolutely have your `~/.wakatime.cfg` file properly set.

# Is this a keylogger?

Short answer is no.

See the [`WakaTimeView::sendAction()`](https://github.com/Tatsh/kate-wakatime/blob/master/wakatimeplugin.cpp#L113) method if you want to be certain.
