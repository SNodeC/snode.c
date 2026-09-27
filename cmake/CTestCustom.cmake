# Use the executing user's default directories, not inherited XDG overrides.
unset(ENV{XDG_CONFIG_HOME})
unset(ENV{XDG_STATE_HOME})
unset(ENV{XDG_RUNTIME_DIR})
unset(ENV{XDG_DATA_HOME})
unset(ENV{XDG_CACHE_HOME})
unset(ENV{XDG_CONFIG_DIRS})
unset(ENV{XDG_DATA_DIRS})
