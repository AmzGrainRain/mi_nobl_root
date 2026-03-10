#!/system/bin/sh
# Step 2: insmod + ksud + trigger Manager
KSU_DIR="/data/adb/ksu"

echo "========================================"
echo " KernelSU 加载 Step 2"
echo "========================================"

# === insmod ===
echo ""
echo "=== 加载内核模块 ==="
if grep -q "kernelsu" /proc/modules 2>/dev/null; then
    echo "已加载，跳过"
else
    chmod 644 /data/local/tmp/kernelsu_patched.ko 2>/dev/null
    insmod /data/local/tmp/kernelsu_patched.ko
    RET=$?
    echo "insmod 返回码: $RET"
    if [ $RET -ne 0 ]; then
        echo "LOAD_FAILED"
        dmesg | grep -i "kernelsu\|Unknown symbol" | tail -10
        exit 1
    fi
fi
grep "kernelsu" /proc/modules
echo ""

# === ksud ===
echo "=== 部署 ksud ==="
mkdir -p "$KSU_DIR/bin" "$KSU_DIR/log" "$KSU_DIR/modules"

if [ -f /data/local/tmp/ksud-aarch64 ]; then
    cp /data/local/tmp/ksud-aarch64 "$KSU_DIR/bin/ksud"
    chmod 755 "$KSU_DIR/bin/ksud"
fi
chown -R 0:1000 "$KSU_DIR" 2>/dev/null
echo "ksud 就绪"
echo ""

# === ksud 启动阶段 ===
echo "=== 执行启动阶段 ==="
"$KSU_DIR/bin/ksud" post-fs-data 2>&1

# 修复: 删除 KSU 自己创建的 magisk 兼容符号链接
# 否则 Manager 的 hasMagisk() 会通过 root shell 的 which magisk 找到它
# 导致误报 "因与magisk有冲突 所有模块不可用"
if [ -L "$KSU_DIR/bin/magisk" ]; then
    rm -f "$KSU_DIR/bin/magisk"
    echo "已移除 magisk 兼容链接 (防止误检测)"
fi

"$KSU_DIR/bin/ksud" services 2>&1
"$KSU_DIR/bin/ksud" boot-completed 2>&1
echo "启动阶段完成"
echo ""

# === 触发 Manager 识别 ===
echo "=== 触发 Manager 识别 ==="
APK_PATH=$(pm path me.weishu.kernelsu 2>/dev/null | head -1 | cut -d: -f2)
if [ -z "$APK_PATH" ]; then
    echo "Manager 未安装！跳过"
else
    echo "APK: $APK_PATH"
    cp "$APK_PATH" /data/local/tmp/_mgr_tmp.apk
    pm install -r /data/local/tmp/_mgr_tmp.apk 2>&1
    rm -f /data/local/tmp/_mgr_tmp.apk
    sleep 2
    dmesg | grep "install fd for manager" | tail -1
fi
echo ""

# === 状态 ===
echo "=== 最终状态 ==="
if grep -q "kernelsu" /proc/modules 2>/dev/null; then
    echo "[OK] 内核模块已加载"
else
    echo "[NO] 内核模块未加载"
fi

"$KSU_DIR/bin/ksud" -V 2>&1

echo ""
echo "=== 最近 KernelSU 日志 ==="
dmesg | grep -i "KernelSU" | tail -10
echo ""

echo "ALL_DONE"
