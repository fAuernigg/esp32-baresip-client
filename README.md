This is an esp32 SIP phone project using
- esp-idf
- esp-baresip submodule including baresip, libre, librem
- arduino-esp32 as an esp-idf component
- pubsubclient for call control and firmware updates


Build howto:
First checkout all required git submodules
- git submodule update --init --recursive
Apply patches on baresip and libre
- esp32phone/components/esp-baresip/components/apply_patches.sh


Create a docker for building
- dockerbuild/createDocker.sh

Run the docker in interactive mode
- dockerbuild/startContainerBash.sh
- cd esp32-baresip-client/esp32phone
- make flash monitor




