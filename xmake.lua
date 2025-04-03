-- switch build type:
-- xmake f -m debug
set_project("SimpleRayTracer")

set_config("buildir", "build")

add_rules("mode.debug", "mode.release")
add_requires("glfw", "glm")
add_requires("stb")
add_requires("gtest")

-- Main App
target("SimpleRayTracer")
    set_kind("binary")
    set_languages("c++17")
    add_files(os.files("src/**.cpp"), "src/glad.c")
    add_includedirs("include")

    add_packages("stb", "glfw", "glm")

    if is_plat("windows") then
        add_syslinks("opengl32", "gdi32", "user32", "kernel32")
    elseif is_plat("linux") then
        add_syslinks("GL", "X11", "pthread", "dl")
    end

    if is_mode("debug") then
        add_cxxflags("-Og", "-g", "-ggdb",  "-Wall", {force = true})
    elseif is_mode("release") then
        add_cxxflags("-O3")
    end

    after_build(function (target)
        if is_mode("release") then
            local output_dir = path.directory(target:targetfile())
            local packaged_folders = {"objects", "shaders"}

            for _, folder in ipairs(packaged_folders) do
                local src = path.join(os.projectdir(), folder)
                local dest = path.join(output_dir, folder)
                
                if os.isdir(src) then
                    os.cp(src, dest)
                end
            end
        end
    end)

    on_run(function (target)
        if is_mode("release") then
            os.execv(target:targetfile(), {})
        else
            local bin_path = path.join(path.directory(target:targetfile()), target:name())
            os.execv(bin_path, {})
        end
    end)

-- TESTS
-- target("IntersectionUtilsTests")
--     set_kind("binary")
--     set_languages("c++17")

--     add_files("include/intersection_utils/tests/*.cpp")
--     add_includedirs("include")

--     add_packages("gtest", "gtest_main")

--     if is_plat("linux") then
--       add_syslinks("pthread")
--     end

--     if is_mode("debug") then
--         add_cxxflags("-Og", "-g", "-ggdb",  "-Wall", "-Wextra", {force = true})
--     elseif is_mode("release") then
--         add_cxxflags("-O3")
--     end

-- target("ComputeTests")
--     set_kind("binary")
--     set_languages("c++17")

--     add_files("include/compute/tests/*.cpp", "src/glad.c", "src/asset_utils/*.cpp")
--     add_includedirs("include")

--     add_packages("stb", "glfw", "glm", "gtest", "gtest_main")

--     if is_plat("linux") then
--       add_syslinks("pthread")
--     end

--     if is_mode("debug") then
--         add_cxxflags("-Og", "-g", "-ggdb",  "-Wall", "-Wextra", {force = true})
--     elseif is_mode("release") then
--         add_cxxflags("-O3")
--     end

--     on_run(function (target)
--         local bin_path = path.join(path.directory(target:targetfile()), target:name())
--         os.execv(bin_path, {})
--     end)