#!/system/bin/sh
echo "=== /data/adb/lspd/log/ ==="
ls -la /data/adb/lspd/log/ 2>/dev/null
echo ""
echo "=== 最新verbose日志 (最后100行) ==="
LATEST=$(ls -t /data/adb/lspd/log/verbose_*.log 2>/dev/null | head -1)
if [ -n "$LATEST" ]; then
    echo "文件: $LATEST"
    echo "大小: $(wc -c < "$LATEST") bytes"
    echo "---"
    tail -100 "$LATEST"
else
    echo "(无verbose日志)"
fi
echo ""
echo "=== 最新modules日志 ==="
MLOG=$(ls -t /data/adb/lspd/log/modules_*.log 2>/dev/null | head -1)
if [ -n "$MLOG" ]; then
    echo "文件: $MLOG"
    tail -50 "$MLOG"
else
    echo "(无modules日志)"
fi
echo ""
echo "=== log.old/ ==="
ls -la /data/adb/lspd/log.old/ 2>/dev/null || echo "(无log.old)"
echo "END"
