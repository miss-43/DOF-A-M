#!/bin/bash
set -e  # 出错立即退出（对应 PowerShell 的 $ErrorActionPreference = 'Stop'）

# ===== 配置参数（与原脚本一致，可按需修改）=====
DEVICE="/dev/video0"          # Linux 摄像头设备节点（默认 video0，可通过 v4l2-ctl --list-devices 查看）
VIDEO_SIZE="640x480"          # 分辨率
FRAMERATE=30                  # 帧率
PIX_IN="yuyv422"              # 输入像素格式（需与摄像头支持格式一致）
PIX_OUT="uyvy422"             # 输出像素格式（与原脚本保持一致）
PORT=5004                     # 推流端口
PAYLOAD=96                    # RTP 负载类型
PKT_SIZE=1200                 # 数据包大小
SDP_FILE="/tmp/yuyv.sdp"      # SDP 文件路径（与 C++ 代码读取路径一致）
WSL_IP=$(hostname -I | awk '{print $1}')  # 自动获取 Linux/WSL 的 IP（取第一个有效 IP）

# 校验 IP 是否获取成功
if [ -z "$WSL_IP" ]; then
    echo "ERROR: 无法获取本机 IP，请手动替换 \$WSL_IP 为实际 IP（如 172.23.117.167）"
    exit 1
fi

# ===== FFmpeg 推流命令（核心逻辑与原脚本完全对齐）=====
ffmpeg -f v4l2 \
       -video_size $VIDEO_SIZE \
       -framerate $FRAMERATE \
       -pixel_format $PIX_IN \
       -rtbufsize 4M \
       -thread_queue_size 4 \
       -i $DEVICE \
       -fps_mode passthrough \
       -vsync passthrough \
       -fflags nobuffer \
       -flags low_delay \
       -max_delay 0 \
       -muxpreload 0 \
       -muxdelay 0 \
       -c:v rawvideo \
       -pix_fmt $PIX_OUT \
       -f rtp \
       -payload_type $PAYLOAD \
       -sdp_file $SDP_FILE \
       "rtp://$WSL_IP:$PORT?pkt_size=$PKT_SIZE"