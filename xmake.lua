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
    add_includedirs("include")
    add_links("asound")
    -- Exclude NAPI binding from binary
    remove_files("src/napi_binding.c")

    -- copy lib files
    after_build(function (target)
        local libdir = path.join(os.projectdir(), "lib")
        local targetdir = target:targetdir()
        for _, file in ipairs(os.files(path.join(libdir, "*.so"))) do
            os.cp(file, targetdir)
        end
    end)

-- Node.js N-API target
target("midi_player_napi")
    set_kind("shared")
    set_filename("midi_player.node")  -- Node.js addon naming convention
    add_files("src/*.c")
    remove_files("src/main.c")  -- Remove the main.c file from this target
    add_includedirs("include")
    add_links("asound")
    
    -- Add Node.js include paths for different platforms
    if is_plat("linux") then
        add_includedirs("/usr/include/node")
    elseif is_plat("macosx") then
        add_includedirs("/usr/local/include/node")
        -- Try to find node headers in brew location too
        add_includedirs("/opt/homebrew/include/node")
    elseif is_plat("windows") then
        -- Try common locations on Windows
        add_includedirs("$(env NODE_HOME)/include/node")
        add_includedirs("C:/Program Files/nodejs/include/node")
    end
    
    -- Try to find node headers using node-addon-api method
    on_load(function(target)
        import("core.project.config")
        import("lib.detect.find_path")
        
        local node_include = nil
        -- Try to detect Node.js include path using 'node -e' command
        os.exec("node -e \"console.log(require('node-addon-api').include)\"", {stdout = function(stdout)
            node_include = stdout:trim()
        end})
        
        if node_include and os.isdir(node_include) then
            target:add("includedirs", node_include)
        end
    end)
    
    -- Set proper build flags for Node.js addon
    add_cxflags("-fPIC")
    add_ldflags("-fPIC")
    
    -- copy lib files
    after_build(function (target)
        local libdir = path.join(os.projectdir(), "lib")
        local targetdir = target:targetdir()
        for _, file in ipairs(os.files(path.join(libdir, "*.so"))) do
            os.cp(file, targetdir)
        end
    end)