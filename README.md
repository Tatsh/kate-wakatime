# Note

This is for Kate 6 (KTextEditor from KF 6).

If you need a version for Kate 5, use the [v1.3.10 release](https://github.com/Tatsh/kate-wakatime/releases).

If you need a version for Kate 4, use the [v0.6 release](https://github.com/Tatsh/kate-wakatime/releases).

## Dependencies

- [CMake](https://cmake.org/)
- [Extra CMake Modules](https://invent.kde.org/frameworks/extra-cmake-modules)
- [KF5:I18n](https://develop.kde.org/products/frameworks/)
- [KF5:TextEditor](https://develop.kde.org/products/frameworks/)
- [wakatime-cli](https://github.com/wakatime/wakatime-cli)

How to install these on your distro is beyond the scope of this document. Generally, install the
Kate text editor, CMake, KDE framework development packages, and Qt development packages.

## How to use

`wakatime-cli` or `wakatime` must be in `PATH` or located in `~/.wakatime`.

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

4. Once this plugin is installed, open Kate and go to _Settings_, _Configure Kate..._, then in the dialog choose _Plugins_.
5. Use the checkbox to enable _WakaTime_ and click _OK_:

   ![screenshot](https://user-images.githubusercontent.com/724848/53671349-f6a91280-3c4b-11e9-88b9-01f2cdc3cf67.png)

6. Restart Kate to be sure the plugin initialises properly.
7. Go to _Settings_, _Configure WakaTime..._. In the dialog, fill in your API key. Click _OK_ to save.

To be certain this will work, check the file at `~/.wakatime.cfg`.
