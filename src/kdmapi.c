#include <stdio.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
typedef FARPROC ProcAddr;
#else
#include <dlfcn.h>
typedef void* ProcAddr;
#endif

typedef bool (*IsKDMAPIAvailableFunc)();
typedef bool (*InitializeKDMAPIStreamFunc)();
typedef bool (*SendDirectDataFunc)(unsigned int);

void* initialize_midi(SendDirectDataFunc* SendDirectData) {
    void* midi_lib = NULL;

#ifdef _WIN32
    HMODULE lib = LoadLibraryA("OmniMIDI.dll");
    if (!lib) {
        fprintf(stderr, "Failed to load OmniMIDI.dll\n");
        return NULL;
    }
    midi_lib = (void*)lib;

    #define LOAD_SYM(lib, name) (ProcAddr)GetProcAddress((HMODULE)(lib), name)

#else
    void* lib = dlopen("./libOmniMIDI.so", RTLD_LAZY);
    if (!lib) {
        fprintf(stderr, "Failed to load libOmniMIDI.so: %s\n", dlerror());
        return NULL;
    }
    midi_lib = lib;

    // Clear any existing error
    dlerror();

    #define LOAD_SYM(lib, name) dlsym(lib, name)
#endif

    IsKDMAPIAvailableFunc IsKDMAPIAvailable = (IsKDMAPIAvailableFunc)LOAD_SYM(lib, "IsKDMAPIAvailable");
    InitializeKDMAPIStreamFunc InitializeKDMAPIStream = (InitializeKDMAPIStreamFunc)LOAD_SYM(lib, "InitializeKDMAPIStream");

    if (!IsKDMAPIAvailable || !InitializeKDMAPIStream || 
        !IsKDMAPIAvailable() || !InitializeKDMAPIStream()) {
        fprintf(stderr, "MIDI initialization failed\n");
#ifdef _WIN32
        FreeLibrary((HMODULE)lib);
#else
        dlclose(lib);
#endif
        return NULL;
    }

    *SendDirectData = (SendDirectDataFunc)LOAD_SYM(lib, "SendDirectData");
    if (!*SendDirectData) {
        fprintf(stderr, "Cannot load SendDirectData\n");
#ifdef _WIN32
        FreeLibrary((HMODULE)lib);
#else
        dlclose(lib);
#endif
        return NULL;
    }

    return midi_lib;
}

void unload_midi(void* midi_lib) {
    if (!midi_lib) return;
#ifdef _WIN32
    FreeLibrary((HMODULE)midi_lib);
#else
    dlclose(midi_lib);
#endif
}
