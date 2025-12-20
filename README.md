# kate-wakatime

[![C++](https://img.shields.io/badge/C++-00599C?logo=c%2B%2B)](https://isocpp.org)
[![GitHub tag (with filter)](https://img.shields.io/github/v/tag/Tatsh/kate-wakatime)](https://github.com/Tatsh/kate-wakatime/tags)
[![License](https://img.shields.io/github/license/Tatsh/kate-wakatime)](https://github.com/Tatsh/kate-wakatime/blob/master/LICENSE.txt)
[![GitHub commits since latest release (by SemVer including pre-releases)](https://img.shields.io/github/commits-since/Tatsh/kate-wakatime/v1.5.3/master)](https://github.com/Tatsh/kate-wakatime/compare/v1.5.3...master)
[![CodeQL](https://github.com/Tatsh/kate-wakatime/actions/workflows/codeql.yml/badge.svg)](https://github.com/Tatsh/kate-wakatime/actions/workflows/codeql.yml)
[![QA](https://github.com/Tatsh/kate-wakatime/actions/workflows/qa.yml/badge.svg)](https://github.com/Tatsh/kate-wakatime/actions/workflows/qa.yml)
[![Tests](https://github.com/Tatsh/kate-wakatime/actions/workflows/tests.yml/badge.svg)](https://github.com/Tatsh/kate-wakatime/actions/workflows/tests.yml)
[![Coverage Status](https://coveralls.io/repos/github/Tatsh/kate-wakatime/badge.svg?branch=master)](https://coveralls.io/github/Tatsh/kate-wakatime?branch=master)
[![Dependabot](https://img.shields.io/badge/Dependabot-enabled-blue?logo=dependabot)](https://github.com/dependabot)
[![GitHub Pages](https://github.com/Tatsh/kate-wakatime/actions/workflows/pages.yml/badge.svg)](https://tatsh.github.io/kate-wakatime/)
[![Stargazers](https://img.shields.io/github/stars/Tatsh/kate-wakatime?logo=github&style=flat)](https://github.com/Tatsh/kate-wakatime/stargazers)
[![CMake](https://img.shields.io/badge/CMake-6E6E6E?logo=cmake)](https://cmake.org/)
[![KDE Plasma](https://img.shields.io/badge/KDE%20Plasma-6E6E6E?logo=kdeplasma)](https://kde.org/)
[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://github.com/pre-commit/pre-commit)
[![Prettier](https://img.shields.io/badge/Prettier-enabled-black?logo=prettier)](https://prettier.io/)

[![@Tatsh](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fpublic.api.bsky.app%2Fxrpc%2Fapp.bsky.actor.getProfile%2F%3Factor=did%3Aplc%3Auq42idtvuccnmtl57nsucz72&query=%24.followersCount&style=social&logo=bluesky&label=Follow+%40Tatsh)](https://bsky.app/profile/Tatsh.bsky.social)
[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-Tatsh-black?logo=buymeacoffee)](https://buymeacoffee.com/Tatsh)
[![Libera.Chat](https://img.shields.io/badge/Libera.Chat-Tatsh-black?logo=liberadotchat)](irc://irc.libera.chat/Tatsh)
[![Mastodon Follow](https://img.shields.io/mastodon/follow/109370961877277568?domain=hostux.social&style=social)](https://hostux.social/@Tatsh)
[![Patreon](https://img.shields.io/badge/Patreon-Tatsh2-F96854?logo=patreon)](https://www.patreon.com/Tatsh2)

Kate plugin to interface with WakaTime.

[![Packaging status](https://repology.org/badge/vertical-allrepos/kate-wakatime.svg)](https://repology.org/project/kate-wakatime/versions)

## Note

This is for Kate 6 (KTextEditor from KF 6).

If you need a version for Kate 5, use the [v1.3.10 release](https://github.com/Tatsh/kate-wakatime/releases).

If you need a version for Kate 4, use the [v0.6 release](https://github.com/Tatsh/kate-wakatime/releases).

Older versions are not supported.

## Dependencies

- [CMake](https://cmake.org/)
- [Extra CMake Modules](https://invent.kde.org/frameworks/extra-cmake-modules)
- [KF6:TextEditor](https://develop.kde.org/products/frameworks/)
- [wakatime-cli](https://github.com/wakatime/wakatime-cli)

How to install these on your distro is beyond the scope of this document. Generally, install the
Kate text editor, CMake, KDE framework development packages, and Qt development packages.

## How to use

`wakatime-cli` or `wakatime` must be in `PATH` or located in `~/.wakatime`. Any Linux/macOS/Windows
binary [listed in the releases](https://github.com/wakatime/wakatime-cli/releases) can be placed as
is in `~/.wakatime`.

1. Get an account at [WakaTime](https://wakatime.com).
1. Get your [API key](https://wakatime.com/settings).
1. Install with your package manager or clone this project and compile:

   ```bash
   git clone git@github.com:Tatsh/kate-wakatime.git
   cd kate-wakatime
   mkdir build
   cd build
   cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release
   cmake --install . --config Release
   ```

   If your package manager does not have this package, please ask the maintainers to add it.
   Installing to a production system with `sudo make install` or similar is not recommended.

1. Once this plugin is installed, open Kate and go to _Settings_, _Configure Kate..._, then in the
   dialog choose _Plugins_.
1. Use the checkbox to enable _WakaTime_ and click _OK_:

   ![screenshot](https://user-images.githubusercontent.com/724848/53671349-f6a91280-3c4b-11e9-88b9-01f2cdc3cf67.png)

1. Restart Kate to be sure the plugin initialises properly.
1. Go to _Settings_, _Configure WakaTime..._. In the dialog, fill in your API key. Click _OK_ to
   save.
