# 免解锁 Bootloader Root 方案

KernelSU 目前已经可以在 SELinux 宽容模式下直获取 Root 权限。

您可直接安装 KernelSU Manager，然后在其中一键获取 Root 权限（亦支持每次开机自动获取）。

关于实现细节可以查看 [KernelSU Pull Request #3268](https://github.com/tiann/KernelSU/pull/3268)

由于本项目利用的是 MIUI 的 `miui.mqsas.IMQSNative` 服务漏洞，本身仅适用于小米手机。而且在可预见的未来某一天，官方必定会修复此漏洞。所以不再推荐使用此项目（此项目目前仍然可用，具体请参考下面的使用方法）。

## 如何使用新版的 KernelSU

在 Fastboot 模式下，你可以通过以下命令将 SELinux 设置为宽容模式：

```sh
fastboot oem set-gpu-preemption 0 androidboot.selinux=permissive
fastboot reboot
```

开机后，安装最新版 KernelSU Manager App，应用首页会出现一个越狱按钮，点击即可获取 Root 权限。

## 关于 MIUI 的提权漏洞

处于 SELinux 宽容模式下，MIUI 的 `miui.mqsas.IMQSNative` 服务可以以 root 权限执行任意程序。

漏洞利用方法如下：

```shell
adb shell service call miui.mqsas.IMQSNative 21 i32 1 s16 "可执行程序" i32 1 s16 "命令行参数" s16 "日志输出路径" i32 60
```

## 关于一键部署 KernelSU 工具的使用方法

- 双击运行 `run.exe`，根据提示操作即可。
