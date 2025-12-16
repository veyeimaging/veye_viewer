#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_JSON="$SCRIPT_DIR/auto_camera_index.json"

# 检查 gawk 是否安装
if ! command -v gawk >/dev/null 2>&1; then
    echo "Error: 'gawk' is not installed."
    echo "Install it with: sudo apt install gawk"
    exit 1
fi

camera_entries=()

for media in /dev/media*; do
    media_output=$(media-ctl -d "$media" -p)

    # 使用 gawk 同时提取 sensor 和 video 节点，按顺序配对
    entries=$(echo "$media_output" | gawk '
        BEGIN {
            RS = "";
            sensor_count = 0;
            video_count = 0;
        }

        # 提取 sensor 信息
        /subtype Sensor/ {
            if (match($0, /entity [0-9]+: ([^(\n]+) \(/, m)) {
                sensor = m[1];
                if (match(sensor, /([0-9]+)-[0-9a-fA-F]+/, i2c)) {
                    i2c_bus = i2c[1];
                } else {
                    i2c_bus = "unknown";
                }
                sensors[sensor_count++] = i2c_bus;
            }
        }

        # 提取 video 节点
        /subtype V4L/ {
            if (match($0, /device node name (\/dev\/video[0-9]+)/, v)) {
                videos[video_count++] = v[1];
            }
        }

        END {
            count = (sensor_count < video_count) ? sensor_count : video_count;
            for (i = 0; i < count; i++) {
                printf("{\"i2c_bus\": \"%s\", \"video_node\": \"%s\"}\n", sensors[i], videos[i]);
            }
        }
    ')

    if [[ -n "$entries" ]]; then
        camera_entries+=("$entries")
    fi
done

# 输出 JSON 格式
{
  echo "["
  printf "%s\n" "${camera_entries[@]}" | sed '$!s/$/,/'
  echo "]"
} > "$OUTPUT_JSON"

echo "Done --> $OUTPUT_JSON"
