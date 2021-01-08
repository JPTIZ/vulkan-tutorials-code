project('vulkan-tutorial', 
        ['cpp'],
        default_options: ['cpp_std=c++17'])

vulkan = dependency('vulkan')
glfw = dependency('glfw3')

triangle = executable('00_triangle',
                      '00_triangle.cpp',
                      dependencies: [vulkan, glfw])
