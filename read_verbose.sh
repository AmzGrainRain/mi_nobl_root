#!/system/bin/sh
# 读取最新verbose日志的前200行和关键行
LOGDIR="/data/adb/lspd/log"
LATEST=$(ls -t $LOGDIR/verbose_*.log 2>/dev/null | head -1)
LATEST2=$(ls -t $LOGDIR/verbose_*.log 2>/dev/null | head -2 | tail -1)

echo "=== 最新verbose: $LATEST ==="
echo "大小: $(wc -c < "$LATEST" 2>/dev/null) bytes"
echo "总行数: $(wc -l < "$LATEST" 2>/dev/null)"
echo ""
echo "--- 前200行 ---"
head -200 "$LATEST" 2>/dev/null
echo ""
echo "--- 关键行(bridge/error/manager/module/version) ---"
grep -nE 'binder received|sent service|no response|Main start|starting server|LSPosed version|Loaded module|Loaded manager|Parasitic|HandleSystemServer|startBootstrap|error.*Xposed|APP_INIT|skipped system' "$LATEST" 2>/dev/null | tail -30

echo ""
echo "=== 第二个verbose(旧): $LATEST2 ==="
if [ "$LATEST2" != "$LATEST" ] && [ -n "$LATEST2" ]; then
    echo "大小: $(wc -c < "$LATEST2" 2>/dev/null) bytes"
    echo "--- 关键行 ---"
    grep -nE 'binder received|sent service|no response|Main start|starting server|LSPosed version|Loaded module|Loaded manager|HandleSystemServer|startBootstrap|error.*Xposed|skipped system' "$LATEST2" 2>/dev/null | tail -20
fi
echo "END"
