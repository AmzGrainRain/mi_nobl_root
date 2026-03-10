#!/system/bin/sh
# Step 1: 拉取 kallsyms
echo "=== 拉取 kallsyms ==="
echo 0 > /proc/sys/kernel/kptr_restrict
cat /proc/kallsyms > /data/local/tmp/kallsyms.txt
chmod 644 /data/local/tmp/kallsyms.txt
COUNT=$(wc -l < /data/local/tmp/kallsyms.txt)
echo "符号数: $COUNT"

# 检查模块是否已加载
if grep -q "kernelsu" /proc/modules 2>/dev/null; then
    echo "注意: 内核模块已加载,无需重新操作"
    grep "kernelsu" /proc/modules
    echo "ALREADY_LOADED"
fi

echo "STEP1_DONE"
