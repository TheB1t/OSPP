#!/bin/bash

IMAGE_FILE=disk
IMAGE_SIZE_MB=32
LOOP_DEV=loop100
MOUNT_DIR=image
PXE_DIR=/var/lib/tftpboot
OUTPUT_DIR=output
KERNEL_NAME=kernel

KERNEL_PATH=${OUTPUT_DIR}/${KERNEL_NAME}
DEBUG_KERNEL_PATH=${OUTPUT_DIR}/${KERNEL_NAME}.debug

function get_color() {
    case $1 in
        black)      echo "\033[30m" ;;
        red)        echo "\033[31m" ;;
        green)      echo "\033[32m" ;;
        yellow)     echo "\033[33m" ;;
        blue)       echo "\033[34m" ;;
        magenta)    echo "\033[35m" ;;
        cyan)       echo "\033[36m" ;;
        white)      echo "\033[37m" ;;
    esac
}

function reset_color() {
    echo -ne "\033[0m"
}

function _log() {
    local type=$3
    local msg=$4

    local color_code0=$(get_color $1)
    local color_code1=$(get_color $2)

    echo -e "${color_code0}[${type}]${color_code1} ${msg}"
    reset_color
}

function log() {
    _log green yellow "${@}"
}

function log_err() {
    _log red yellow ERR "${@}"
}

function run() {
    log RUN "${*}"
    "${@}"
}

function run_sudo() {
    log RUN_S "${*}"
    sudo sh -c "${*}"
}

safe_rm() {
    local file="$1"
    local abs_path=$(readlink -f -- "$file")

    if [[ -z "$abs_path" || "$abs_path" == "/" || "$abs_path" == "/home" || "$abs_path" == "$HOME" ]]; then
        log_err "Refusing to delete critical directory: $abs_path"
        return 1
    fi

    run_sudo rm -rf -- "$abs_path"
}

ask_sudo() {
    if sudo -n true 2>/dev/null; then
        return 0
    fi

    log INFO "Sudo privileges required. Please enter your password:"
    if sudo -v; then
        return 0
    else
        log_err "Failed to obtain sudo privileges."
        return 1
    fi
}

#  Remove old image
function remove_image() {
    safe_rm ${IMAGE_FILE}.img
}

#  Create file
function create_image() {
    run dd if=/dev/zero of=${IMAGE_FILE}.img bs=1M count=${IMAGE_SIZE_MB}
}

#  Create loop device
function create_loop() {
    run_sudo losetup -P /dev/${LOOP_DEV} ${IMAGE_FILE}.img
}

#  Create ext2 filesystem
function make_filesystem() {
    run_sudo parted -s /dev/${LOOP_DEV} mklabel msdos
    run_sudo parted -s /dev/${LOOP_DEV} mkpart primary ext2 1MiB 100%

    run_sudo mkfs.ext2 /dev/${LOOP_DEV}p1
}

#  Mount loop device
function mount_loop() {
    run mkdir ${MOUNT_DIR}

    run_sudo mount /dev/${LOOP_DEV}p1 $MOUNT_DIR
}

#  Install grub
function install_grub() {
    run_sudo grub-install --target=i386-pc --root-directory=${MOUNT_DIR} --boot-directory=${MOUNT_DIR}/boot --no-floppy --modules="normal part_msdos ext2 multiboot" /dev/${LOOP_DEV}
}

#  Umount loop device
function umount_loop() {
    run_sudo umount /dev/${LOOP_DEV}p1

    safe_rm ${MOUNT_DIR}
}

#  Remove loop device
function remove_loop() {
    run_sudo losetup -D /dev/${LOOP_DEV}
}

function build() {
    local type=$1

    if [ -z "${type}" ]; then
        type=Debug
    fi

    safe_rm ${OUTPUT_DIR}/*

    log BUILD "Type: ${type}"
    run cmake -B build . -DCMAKE_BUILD_TYPE=${type}
    run make -C build -j$(nproc)

    run mv ${KERNEL_PATH} ${KERNEL_PATH}.orig
    run objcopy --only-keep-debug ${KERNEL_PATH}.orig ${DEBUG_KERNEL_PATH}
    run objcopy --strip-debug  ${KERNEL_PATH}.orig ${KERNEL_PATH}
    run strip --strip-all ${KERNEL_PATH}
}

function pack() {
    local dst=$1

    safe_rm ${dst}/*

    run_sudo mkdir -p ${dst}/boot/grub
    sudo tee "${dst}/boot/grub/grub.cfg" > /dev/null << EOF
timeout=1
default=0

menuentry 'OS++' {
    multiboot /kernel
    module /debug.ko
    boot
}
EOF

    cat ${dst}/boot/grub/grub.cfg
    run_sudo cp ${KERNEL_PATH} ${dst}/
    run_sudo python3 scripts/make_module.py debug_symbols ${DEBUG_KERNEL_PATH} debug --output-path ${dst}/
}

#  Run qemu
function run_qemu() {
    run qemu-system-i386 -m 1024M -hda ${IMAGE_FILE}.img -no-reboot -no-shutdown ${@}
}

function main() {
    local action=$1; shift

	case "$action" in
    "init")
        ask_sudo || return 1

        remove_image
        create_image
        create_loop
        make_filesystem
        mount_loop
        install_grub

        umount_loop
        remove_loop
        ;;
	"pack")
        ask_sudo || return 1

        create_loop
        mount_loop

        build $@
        pack ${MOUNT_DIR}

        umount_loop
        remove_loop
	    ;;
    "pack_pxe")
        ask_sudo || return 1

        build $@
        pack ${PXE_DIR}
        run_sudo grub-mknetdir --net-directory ${PXE_DIR}
        ;;
    "runb")
        safe_rm serial.txt
        safe_rm ${IMAGE_FILE}.img.lock
        run bochs
        ;;
    "run")
        run_qemu -serial stdio -machine q35 --cpu max -smp 4
        ;;
    "runk")
        run_qemu -serial stdio -machine q35 --enable-kvm --cpu host -smp 4
        ;;
    "rund")
        run_qemu -serial file:serial.log -s -S -daemonize
        ;;
	"mount")
        ask_sudo || return 1
        create_loop
        mount_loop
	    ;;
    "umount")
        ask_sudo || return 1
        umount_loop
        remove_loop
        ;;
    "a2l")
        local addr=$1

        if [ -z "${addr}" ]; then
            log_err "Address not passed"
            exit 1
        fi

        if [ ! -f ${DEBUG_KERNEL_PATH} ]; then
            log_err "Kernel not found (build it first)"
            exit 1
        fi

        run addr2line -e ${DEBUG_KERNEL_PATH} ${addr}
        ;;
    "burn")
        ask_sudo || return 1
        local drive=$1

        if [ -z "${drive}" ]; then
            log_err "Drive not passed"
            exit 1
        fi

        if [ ! -f ${IMAGE_FILE}.img ]; then
            log_err "Image not found (pack it first)"
            exit 1
        fi

        run_sudo dd if=${IMAGE_FILE}.img of=${drive} status=progress
        ;;
	*)
	    [[ -n "$action" ]] && log_err "Not implemented action $1" || log_err "Action not passed"
	    echo "Avaliable actions:"
        echo "	run                     : run qemu"
        echo "	runk                    : run qemu with kvm"
        echo "	rund                    : run qemu with debug (gdb server)"
        echo "	runb                    : run bochs"
        echo "	burn <drive>            : burn image to drive"
        echo "	init                    : init image"
        echo "	mount                   : mount image"
        echo "	umount                  : umount image"
        echo "	a2l <addr>              : addr2line"
        echo "	burn <drive>            : burn image to drive"
        echo "	pack <build_type>       : packing kernel"
        echo "	pack_pxe <build_type>   : packing kernel for pxe"
        ;;
	esac

}

main $@