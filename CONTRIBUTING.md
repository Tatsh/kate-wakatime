# How to contribute to kate-wakatime

Thank you for your interest in contributing to kate-wakatime! Please follow these guidelines to help
maintain code quality and consistency.

## General Guidelines

- Follow the coding standards and rules described below for each file type.
- Ensure all code passes linting and tests before submitting a pull request.
- Write clear commit messages and document your changes in the changelog if relevant.
- Contributors are listed in `package.json` and `pyproject.toml`.
- Update relevant fields in `.wiswa.jsonnet` such as authors, dependencies, etc.
- All contributed code must have a license compatible with the project's license (MIT).
- Add missing words to `.vscode/dictionary.txt` as necessary (sorted and lower-cased).

## Development Environment

- Use [vcpkg](https://vcpkg.io/) to manage C/C++ dependencies:
  - Install dependencies: `vcpkg install`
  - Add a dependency: `vcpkg install <package>`
- Use [Yarn](https://yarnpkg.com/) to install Node.js based dependencies:
  - Install Node.js dependencies: `yarn`
- Install [pre-commit](https://pre-commit.com/) and make sure it is enabled by running
  `pre-commit install` in the repository checkout.

## Quality Assurance & Scripts

The following scripts are available via `yarn` (see `package.json`):

- `yarn qa`: Run all QA checks (type checking, linting, spelling, formatting).
- `yarn check-formatting`: Check code formatting.
- `yarn format`: Auto-format code.
- `yarn check-spelling`: Run spell checker.

The above all need to pass for any code changes to be accepted.

## C++ Code Guidelines

- Follow [KDE C++ coding style](https://community.kde.org/Policies/Frameworks_Coding_Style), with
  the changes following this point.
- Use `#pragma once` in all header files instead of include guards.
- Use Google-style naming for members: `memberVariable_`.
- Use `nullptr` instead of `NULL`.
- Feel free to use `auto`. Add qualifiers like `const`, `*`, and `&` as appropriate to improve
  clarity.
- Use `QString`, `QList`, etc., instead of STL types for Qt/KDE APIs unless interoperability is
  required.
- All UI strings must be wrapped in the appropriate translation macro (e.g., `i18n()`, `tr`).
- Do not use C-style casts; use C++ casts (`static_cast`, `dynamic_cast`, etc.).
- All new classes must have a corresponding header and source file, and must be added to the
  relevant `CMakeLists.txt`.
- Prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers unless required by
  Qt/KDE APIs.
- Document all public classes and methods using Doxygen-style comments.
- Private methods and members should have brief comments if their purpose is not obvious.
- All new logic must be covered by unit tests in `autotests`.
- See [C++ instructions] for more details.

## Markdown Guidelines

- `<kbd>` tags are allowed.
- Headers do not have to be unique if in different sections.
- Line length rules do not apply to code blocks.
- See [Markdown instructions] for more.

## JSON, YAML, TOML, INI Guidelines

- JSON and YAML files should generally be recursively sorted by key.
- In TOML/INI, `=` must be surrounded by a single space on both sides.
- See [JSON/YAML guidelines] and [TOML/INI guidelines] for more details.

## Submitting Changes

Do not submit PRs solely for dependency bumps. Dependency bumps are either handled by running Wiswa
locally or allowing Dependabot to do them.

1. Fork the repository and create your branch from `master`.
2. Ensure your code follows the above guidelines.
3. Run all tests (`yarn test`) and QA scripts (`yarn qa`). Be certain pre-commit runs on your
   commits.
4. Submit a pull request with a clear description of your changes.

[C++ instructions]: .github/instructions/cpp.instructions.md
[Markdown instructions]: .github/instructions/markdown.instructions.md
[JSON/YAML guidelines]: .github/instructions/json-yaml.instructions.md
[TOML/INI guidelines]: .github/instructions/toml-ini.instructions.md
