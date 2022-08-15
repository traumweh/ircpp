# SPDX-FileCopyrightText: 2022 traumweh
#
# SPDX-License-Identifier: Apache-2.0

project('ircpp',
        'cpp',
        version: '0.1.0',
        license: 'Apache-2.0',
        default_options: [
          'warning_level=3',
          'cpp_std=c++17'
        ])

cmake = import('cmake')
ixws_opt = cmake.subproject_options()
ixws_opt.add_cmake_defines({'USE_TLS': true})
ixws_sub = cmake.subproject('IXWebSocket', options: ixws_opt)
ixws_dep = ixws_sub.dependency('ixwebsocket')

incl_ircpp = include_directories('include')

libircpp = library('ircpp',
                   sources: ['src/irc.cpp'],
                   include_directories: incl_ircpp,
                   dependencies: [ixws_dep],
                   version: '0.1.0')

ircpp_dep = declare_dependency(link_with: libircpp, include_directories: incl_ircpp)
meson.override_dependency('libircpp', ircpp_dep)

import('pkgconfig').generate(
  libircpp,
  name: 'ircpp',
  description: 'A simple C++ IRC parser and client library',
  url: 'https://github.com/traumweh/ircpp',
  version: '0.1.0',
  filebase: 'libircpp')
