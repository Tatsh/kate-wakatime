local utils = import 'utils.libjsonnet';

(import 'defaults.libjsonnet') + {
  local top = self,
  // General settings

  // Shared
  github_username: 'Tatsh',
  security_policy_supported_versions: { '1.5.x': ':white_check_mark:' },
  authors: [
    {
      'family-names': 'Udvare',
      'given-names': 'Andrew',
      email: 'audvare@gmail.com',
      name: '%s %s' % [self['given-names'], self['family-names']],
    },
  ],
  project_name: 'kate-wakatime',
  version: '1.5.1',
  description: 'Kate plugin to interface with WakaTime.',
  keywords: ['kate', 'kde', 'plasma', 'wakatime'],
  want_main: false,
  copilot: {
    intro: 'kate-wakatime is a Kate plugin that interfaces with WakaTime, a service that provides metrics and insights about your coding activity.',
  },
  social+: {
    mastodon+: { id: '109370961877277568' },
  },

  // GitHub
  github+: {
    funding+: {
      ko_fi: 'tatsh2',
      liberapay: 'tatsh2',
      patreon: 'tatsh2',
    },
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
}
