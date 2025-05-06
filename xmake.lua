add_rules("mode.debug", "mode.release")

set_toolchains("gcc")

-- Set C language standard
set_languages("c++17")

-- Add release mode flags
if is_mode("release") then
    add_cxflags("-O3", "-march=native", "-funroll-loops", "-fomit-frame-pointer")
end

target("midi_player")
    set_kind("binary")
    add_files("src/*.c")
    add_includedirs("src")