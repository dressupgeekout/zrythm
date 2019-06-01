gen_gtk_gresources = find_program (
  join_paths (
    resources_dir, 'gen-gtk-gresources-xml.py'))
zrythm_gresources_xml = configure_file (
  output: 'zrythm.gresources.xml',
  command: [
    gen_gtk_gresources,
    resources_dir,
    '@OUTPUT@'
    ])
zrythm_resources = gnome.compile_resources (
  'zrythm-resources',
  zrythm_gresources_xml,
  source_dir: resources_dir,
  c_name: 'zrythm',
)

zrythm_main = files ('main.c')
zrythm_srcs = files ([
  'project.c',
  'zrythm.c',
  ])

if os_darwin
  zrythm_srcs += 'Wrapper.m'
endif

subdir('actions')
subdir('audio')
subdir('gui')
subdir('plugins')
subdir('settings')
subdir('utils')

zrythm_args = []

executable (
  'zrythm', [
    zrythm_main,
    zrythm_srcs,
    ext_srcs,
    zrythm_resources
    ],
  c_args: zrythm_args,
  dependencies : zrythm_deps,
  include_directories : all_inc,
  link_with: cyaml,
  install : true,
  )