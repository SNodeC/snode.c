add_custom_target(
    uninstall
    COMMAND sudo xargs rm -v < install_manifest.txt
    COMMAND sudo xargs -L1 dirname < install_manifest.txt | sort | uniq | sudo
            xargs rmdir -v --ignore-fail-on-non-empty -p
    COMMENT "Uninstall project"
)

# xargs rm < install_manifest.txt 140
