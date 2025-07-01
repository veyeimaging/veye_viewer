#!/bin/bash

OUTPUT_JSON="./config/auto_camera_index.json"
> "$OUTPUT_JSON"

if ! command -v gawk >/dev/null 2>&1; then
    echo "Error: 'gawk' is not installed."
    echo "Install it with: sudo apt install gawk"
    exit 1
fi

camera_entries=()

for media in /dev/media*; do
    media_output=$(media-ctl -d "$media" -p)
    media_node="$media"

    entry=$(echo "$media_output" | gawk -v media_node="$media_node" '
        BEGIN {
            RS = "";
            OFS = "";
            sensor_name = "";
            i2c_bus = "";
            subdev_node = "";
            csi_entity = "";
            chn0_video_node = "";
        }

        # 提取 sensor 节点的信息
        /subtype Sensor/ {
            if (match($0, /entity [0-9]+: ([^(\n]+) \(/, m1)) {
                sensor_name = m1[1];
                if (match(sensor_name, /([0-9]+)-[0-9a-fA-F]+/, i2c)) {
                    i2c_bus = i2c[1];
                }
                if (match($0, /device node name (\/dev\/v4l-subdev[0-9]+)/, m2)) {
                    subdev_node = m2[1];
                }
                if (match($0, /-> "([^"]+)":/, m3)) {
                    csi_entity = m3[1];
                }
            }
        }

        # 提取 ch0 视频节点（如 /dev/video0）
        /rp1-cfe-csi2_ch0/ {
            if (match($0, /device node name (\/dev\/video[0-9]+)/, m)) {
                chn0_video_node = m[1];
            }
        }

        END {
            if (sensor_name != "" && i2c_bus != "" && subdev_node != "" && chn0_video_node != "") {
                printf("{\"media_node\": \"%s\", \"video_node\": \"%s\", \"video_subnode\": \"%s\", \"media_entity_name\": \"%s\", \"i2c_bus\": \"%s\"}\n",
                    media_node, chn0_video_node, subdev_node, sensor_name, i2c_bus);
            }
        }
    ')

    if [[ -n "$entry" ]]; then
        camera_entries+=("$entry")
    fi
done

# 输出为 JSON 数组
{
  echo "["
  printf "%s\n" "${camera_entries[@]}" | sed '$!s/$/,/'
  echo "]"
} > "$OUTPUT_JSON"

echo "Done --> $OUTPUT_JSON"
