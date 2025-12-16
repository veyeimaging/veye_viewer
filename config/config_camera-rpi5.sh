#!/bin/bash

# Default resolution and format
WIDTH_YUV=1920
HEIGHT_YUV=1080
MEDIA_FMT_YUV=UYVY8_1X16
PIXEL_FMT_YUV=UYVY

WIDTH_MV=1280
HEIGHT_MV=1024
MEDIA_FMT_MV=Y8_1X8
PIXEL_FMT_MV=GREY

# Global variables
g_camera_type=null
g_camera_name=null
g_unpacked_supported=0

g_roi_x=0
g_roi_y=0
g_width=0
g_height=0
g_media_fmt=null
g_pixel_fmt=null

g_i2c_bus=-1
g_media_device=null
g_video_device=null
g_video_subdevice=null

print_usage() {
    echo "Usage:"
    echo "$0 <camera_name> -fmt <format> -x <roi_x> -y <roi_y> -w <width> -h <height> \\"
    echo "   -bus <i2c_bus> -media <media_dev> -subdev <subdev_dev> -video <video_dev>"
    echo ""
    echo "Supported camera_name: veyecam2m | csimx307 | cssc132 | mvcam | gxcam"
    echo "Supported format: UYVY | RAW8 | RAW10 | RAW12"
}

check_camera_name() {
    case $1 in
        veyecam2m|csimx307|cssc132|gxcam)
            g_camera_name=$1
            g_camera_type="YUV_type"
            ;;
        mvcam)
            g_camera_name=mvcam
            g_camera_type="MV_type"
            ;;
        *)
            echo "Invalid camera name: $1"
            exit 1
            ;;
    esac

    if [ "$g_camera_type" = "YUV_type" ]; then
        g_width=$WIDTH_YUV
        g_height=$HEIGHT_YUV
        g_media_fmt=$MEDIA_FMT_YUV
        g_pixel_fmt=$PIXEL_FMT_YUV
    else
        g_width=$WIDTH_MV
        g_height=$HEIGHT_MV
        g_media_fmt=$MEDIA_FMT_MV
        g_pixel_fmt=$PIXEL_FMT_MV
    fi
}

parse_fmt() {
    case $1 in
        UYVY)
            g_media_fmt=UYVY8_1X16
            g_pixel_fmt=UYVY
            ;;
        RAW8)
            g_media_fmt=Y8_1X8
            g_pixel_fmt=GREY
            ;;
        RAW10)
            g_media_fmt=Y10_1X10
            g_pixel_fmt=$([ $g_unpacked_supported -eq 1 ] && echo "Y10 " || echo "Y10P")
            ;;
        RAW12)
            g_media_fmt=Y12_1X12
            g_pixel_fmt=$([ $g_unpacked_supported -eq 1 ] && echo "Y12 " || echo "Y12P")
            ;;
        *)
            echo "Unsupported format: $1"
            exit 1
            ;;
    esac
}

parse_arguments() {
    while [ "$#" -gt 0 ]; do
        case "$1" in
            veyecam2m|csimx307|cssc132|mvcam|gxcam)
                check_camera_name "$1"
                ;;
            -fmt) shift; parse_fmt "$1" ;;
            -x) shift; g_roi_x="$1" ;;
            -y) shift; g_roi_y="$1" ;;
            -w) shift; g_width="$1" ;;
            -h) shift; g_height="$1" ;;
            -bus) shift; g_i2c_bus="$1" ;;
            -media) shift; g_media_device="$1" ;;
            -subdev) shift; g_video_subdevice="$1" ;;
            -video) shift; g_video_device="$1" ;;
            *)
                echo "Unknown option: $1"
                print_usage
                exit 1
                ;;
        esac
        shift
    done
}

check_kernel_version() {
    kernel_version=$(uname -r | awk -F '+' '{print $1}')
    ref_version="6.6.31"
    IFS='.' read -r k_major k_minor k_patch <<<"$kernel_version"
    IFS='.' read -r r_major r_minor r_patch <<<"$ref_version"

    if ((k_major > r_major)) || \
       ((k_major == r_major && k_minor > r_minor)) || \
       ((k_major == r_major && k_minor == r_minor && k_patch >= r_patch)); then
        g_unpacked_supported=0
    else
        g_unpacked_supported=1
    fi
}

validate_required_parameters() {
    if [[ "$g_camera_name" == "null" || "$g_i2c_bus" -lt 0 || "$g_media_device" == "null" || "$g_video_device" == "null" || "$g_video_subdevice" == "null" ]]; then
        echo "Error: Missing required parameters."
        print_usage
        exit 1
    fi
}

set_camera_entity() {
    media-ctl -d "$g_media_device" -r
    media-ctl -d "$g_media_device" -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0 [1]"
    if [ "$g_camera_name" = "mvcam" ]; then
        v4l2-ctl --set-ctrl roi_x=$g_roi_x -d $g_video_subdevice
        v4l2-ctl --set-ctrl roi_y=$g_roi_y -d $g_video_subdevice
    fi
    media-ctl -d "$g_media_device" --set-v4l2 "'$g_camera_name $g_i2c_bus-003b':0[fmt:${g_media_fmt}/${g_width}x${g_height} field:none]"
    media-ctl -d "$g_media_device" -V "'csi2':0 [fmt:${g_media_fmt}/${g_width}x${g_height} field:none]"
    media-ctl -d "$g_media_device" -V "'csi2':4 [fmt:${g_media_fmt}/${g_width}x${g_height} field:none]"
    v4l2-ctl -d "$g_video_device" --set-fmt-video=width=$g_width,height=$g_height,pixelformat=$g_pixel_fmt,colorspace=rec709,ycbcr=rec709,xfer=rec709,quantization=lim-range
}

### Main

check_kernel_version

if [ "$#" -lt 1 ]; then
    print_usage
    exit 1
fi

parse_arguments "$@"
validate_required_parameters
set_camera_entity

#echo "Camera setup complete. Get frame from $g_video_device and use $g_video_subdevice for settings."
exit 0
