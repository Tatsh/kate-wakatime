local utils = import 'utils.libjsonnet';

{
  local top = self,
  github_username: 'Tatsh',
  security_policy_supported_versions: { '1.5.x': ':white_check_mark:' },
  project_name: 'kate-wakatime',
  version: '1.5.3',
  description: 'Kate plugin to interface with WakaTime.',
  keywords: ['kate', 'kde', 'plasma', 'wakatime'],
  want_main: false,
  copilot+: {
    intro: 'kate-wakatime is a Kate plugin that interfaces with WakaTime, a service that provides metrics and insights about your coding activity.',
  },
  // C++ only
  cmake+: {
    uses_qt: true,
  },
  project_type: 'c++',
  vcpkg+: {
    dependencies: [{
      name: 'ecm',
      'version>=': '6.7.0',
    }],
  },
  github+: {
    pages_using_jekyll: false,
  },
  vscode+: {
    c_cpp+: {
      configurations: [
        {
          cStandard: 'gnu23',
          compilerPath: '/usr/bin/gcc',
          cppStandard: 'gnu++23',
          defines: [
            'QT_NO_CAST_FROM_ASCII',
            'QT_NO_CAST_FROM_BYTEARRAY',
            'QT_NO_CAST_TO_ASCII',
            'QT_NO_SIGNALS_SLOTS_KEYWORDS',
            'QT_NO_URL_CAST_FROM_STRING',
            'QT_STRICT_ITERATORS',
            'QT_USE_FAST_OPERATOR_PLUS',
            'QT_USE_QSTRINGBUILDER',
            'VERSION="unknown"',
          ],
          includePath: [
            '${workspaceFolder}/build/src',
            '${workspaceFolder}/build/src/autotests/test_wakatime_autogen/include',
            '${workspaceFolder}/build/src/ktexteditor_wakatime_autogen/include',
            '${workspaceFolder}/src/**',
            '/usr/include/KF6/KCompletion',
            '/usr/include/KF6/KConfig',
            '/usr/include/KF6/KConfigGui',
            '/usr/include/KF6/KConfigWidgets',
            '/usr/include/KF6/KCoreAddons',
            '/usr/include/KF6/KI18n',
            '/usr/include/KF6/KParts',
            '/usr/include/KF6/KSyntaxHighlighting',
            '/usr/include/KF6/KTextEditor',
            '/usr/include/KF6/KWidgetsAddons',
            '/usr/include/KF6/KXmlGui',
            '/usr/include/qt6',
            '/usr/include/qt6/QtCore',
            '/usr/include/qt6/QtGui',
            '/usr/include/qt6/QtWidgets',
          ],
          name: 'Linux',
        },
      ],
    },
  },
  cz+: {
    commitizen+: {
      version_files+: [
        'src/ktexteditor_wakatime.json',
      ],
    },
  },
}
