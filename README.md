# kate-wakatime

[![GitHub tag (with filter)](https://img.shields.io/github/v/tag/Tatsh/kate-wakatime)](https://github.com/Tatsh/kate-wakatime/tags)
[![License](https://img.shields.io/github/license/Tatsh/kate-wakatime)](https://github.com/Tatsh/kate-wakatime/blob/master/LICENSE.txt)
[![GitHub commits since latest release (by SemVer including pre-releases)](https://img.shields.io/github/commits-since/Tatsh/kate-wakatime/v1.5.1/master)](https://github.com/Tatsh/kate-wakatime/compare/v1.5.1...master)
[![QA](https://github.com/Tatsh/kate-wakatime/actions/workflows/qa.yml/badge.svg)](https://github.com/Tatsh/kate-wakatime/actions/workflows/qa.yml)
[![Tests](https://github.com/Tatsh/kate-wakatime/actions/workflows/tests.yml/badge.svg)](https://github.com/Tatsh/kate-wakatime/actions/workflows/tests.yml)
[![Coverage Status](https://coveralls.io/repos/github/Tatsh/kate-wakatime/badge.svg?branch=master)](https://coveralls.io/github/Tatsh/kate-wakatime?branch=master)
[![GitHub Pages](https://github.com/Tatsh/kate-wakatime/actions/workflows/pages.yml/badge.svg)](https://tatsh.github.io/kate-wakatime/)
[![Downloads](https://static.pepy.tech/badge/kate-wakatime/month)](https://pepy.tech/project/kate-wakatime)
[![Stargazers](https://img.shields.io/github/stars/Tatsh/kate-wakatime?logo=github&style=flat)](https://github.com/Tatsh/kate-wakatime/stargazers)

[![@Tatsh](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fpublic.api.bsky.app%2Fxrpc%2Fapp.bsky.actor.getProfile%2F%3Factor%3Ddid%3Aplc%3Auq42idtvuccnmtl57nsucz72%26query%3D%24.followersCount%26style%3Dsocial%26logo%3Dbluesky%26label%3DFollow%2520%40Tatsh&query=%24.followersCount&style=social&logo=bluesky&label=Follow%20%40Tatsh)](https://bsky.app/profile/Tatsh.bsky.social)
[![Mastodon Follow](https://img.shields.io/mastodon/follow/109370961877277568?domain=hostux.social&style=social)](https://hostux.social/@Tatsh)

Kate plugin to interface with WakaTime.

## Note

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
