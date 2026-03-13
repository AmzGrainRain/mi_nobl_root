#include <filesystem>
#include <iostream>
#include <print>
#include <format>
#include <tuple>
#include <chrono>
#include <thread>
#include <cstdio>
#include <numeric>

#define NOMINMAX
#include <Windows.h>


using namespace std::chrono_literals;
namespace fs = std::filesystem;

const auto cwd = fs::current_path();
auto adb_bin = cwd / "adb" / "adb.exe";
auto fastboot_bin = cwd / "adb" / "fastboot.exe";
auto ksum = cwd / "ksu.apk";
auto ksud = cwd / "ksud";

[[maybe_unused]]
static auto Exec(const std::string& bin, const std::string& args) -> std::tuple<int, std::string>
{
	std::string cmd = std::format("{} {} 2>&1", bin, args);
	FILE* pipe = _popen(cmd.c_str(), "r");

	if (!pipe)
	{
		int error_code = errno;
		return { error_code, strerror(error_code) };
	}

	std::string output;
	char buffer[1024];
	while (true)
	{
		if (fgets(buffer, sizeof(buffer), pipe) == nullptr)
		{
			if (ferror(pipe))
			{
				std::println("读取命令输出失败：{}", ferror(pipe));
			}
			break;
		}
		output += buffer;
	}

	int exit_code = _pclose(pipe);
	return { exit_code, output };
}

static auto Step1() -> bool
{
	std::cout <<
		"========================================\n"
		"   第一步：准备进入 Fastboot 模式\n"
		"========================================\n"
		"📱 请完成以下操作：\n"
		"  1.打开开发者模式（设置 - 关于手机 - 连续点击版本号）\n"
		"  2.解锁手机并进入系统桌面\n"
		"  3.确保手机已连接电脑并允许 USB 调试\n"
		"✅ 确认后按回车键继续...";

	std::string tmp;
	std::getline(std::cin, tmp); (void)tmp;

	std::println("🔍 等待设备连接...");
	const auto r1 = Exec(adb_bin.string(), "devices");
	if (std::get<0>(r1) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r1));
		return false;
	}

	std::println("🔄 正在重启设备进入 Fastboot 模式...");
	const auto r2 = Exec(adb_bin.string(), "reboot bootloader");
	if (std::get<0>(r2) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r2));
		return false;
	}

	return true;
}

static auto Step2() -> bool
{
	std::cout <<
		"\n========================================\n"
		"   第二步：宽容 SELinux\n"
		"========================================\n"
		"📱 请确认设备已进入 Fastboot 模式（屏幕显示 Fastboot 字样）\n"
		"✅ 确认后按回车键继续...";

	std::string tmp;
	std::getline(std::cin, tmp); (void)tmp;

	std::println("🚀 正在通过漏洞设置 SELinux 为 Permissive...");
	const auto r1 = Exec(fastboot_bin.string(), "oem set-gpu-preemption 0 androidboot.selinux=permissive");
	if (std::get<0>(r1) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r1));
		return false;
	}

	std::println("🚀 正在引导设备进入系统...");
	const auto r2 = Exec(fastboot_bin.string(), "continue");
	if (std::get<0>(r2) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r2));
		return false;
	}

	return true;
}

static auto Step3() -> bool
{
	std::cout <<
		"\n========================================\n"
		"   第三步：检查 SELinux 状态\n"
		"========================================\n"
		"📱 请确认设备已开机\n"
		"✅ 确认后按回车键继续...";

	std::string tmp;
	std::getline(std::cin, tmp); (void)tmp;

	std::println("🔍 等待设备连接...");
	Exec(adb_bin.string(), "wait-for-device");

	std::println("🔍 检查 SELinux 状态...");
	const auto r1 = Exec(adb_bin.string(), "shell getenforce");
	if (std::get<0>(r1) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r1));
		return false;
	}

	if (std::get<1>(r1).find("Permissive") == std::string::npos)
	{
		std::println("❌ SELinux 仍处于 Enforcing 模式，可能是漏洞利用失败了！");
		return false;
	}

	std::println("✅ SELinux 已成功设置为 Permissive 模式！");
	return true;
}

static auto Step4() -> bool
{
	std::cout <<
		"\n========================================\n"
		"   第四步：部署 KernelSU\n"
		"========================================\n"
		"📱 请打开锁屏并进入桌面\n"
		"✅ 确认后按回车键继续...";

	std::string tmp;
	std::getline(std::cin, tmp); (void)tmp;

	std::println("🔍 等待设备连接...");
	Exec(adb_bin.string(), "wait-for-device");

	std::println("📦 推送 KernelSU Daemon 到 /data/local/tmp/ksud");
	const auto r1 = Exec(adb_bin.string(), std::format("push {} /data/local/tmp/ksud", ksud.string()));
	if (std::get<0>(r1) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r1));
		return false;
	}

	std::println("🚀 赋予 KernelSU Daemon 可执行权限...");
	const auto r2 = Exec(adb_bin.string(), "shell chmod 755 /data/local/tmp/ksud");
	if (std::get<0>(r2) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r2));
		return false;
	}

	std::println("🚀 执行 ksud late-load...");
	const auto r3 = Exec(adb_bin.string(), "shell service call miui.mqsas.IMQSNative 21 i32 1 s16 '/data/local/tmp/ksud' i32 1 s16 'late-load' s16 '/data/local/tmp/ksud-log.txt' i32 60");
	if (std::get<0>(r3) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r3));
		return false;
	}

	std::println("🔍 等待 KernelSU Daemon 启动...");
	std::this_thread::sleep_for(3s);

	std::println("🔍 获取内核模块载入情况...");
	const auto r4 = Exec(adb_bin.string(), "shell grep 'kernelsu' /proc/modules");
	if (std::get<0>(r4) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r4));
		return false;
	}

	std::println("🔍 检查 KernelSU Daemon 是否成功启动...");
	if (std::get<1>(r4).find("kernelsu") == std::string::npos)
	{
		std::println("❌ KernelSU Daemon 启动失败！");
		return false;
	}

	std::println("🔍 检查 KernelSU Manager 的安装情况...");
	const auto r5 = Exec(adb_bin.string(), "shell pm path me.weishu.kernelsu");
	if (std::get<0>(r5) == 0) // KernelSU Manager 已安装会返回 0 并且输出路径，否则会返回 1
	{
		std::println("✅ 已安装 KernelSU Manager，路径：{}", std::get<1>(r5));
		std::println("🛠️ 是否需要重新安装 KernelSU Manager？");
		std::println("📝 输入 Y 或 y 后按回车即代表重新安装");
		std::println("📝 输入其他任意内容或直接按回车即代表跳过");
		std::print("您的选择: ");
		std::string choice;
		std::getline(std::cin, choice); (void)choice;
		if (choice.empty() || (choice[0] != 'y' && choice[0] != 'Y'))
		{
			std::println("✅ 跳过安装...");
			return true;
		}

		std::println("🚀 正在卸载 KernelSU Manager...");
		const auto r6 = Exec(adb_bin.string(), "shell pm uninstall me.weishu.kernelsu");
		if (std::get<0>(r6) != 0)
		{
			std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r6));
			return false;
		}
		std::println("✅ 已成功卸载 KernelSU Manager！");
	}

	std::println("📦 推送 KernelSU Manager 到 /data/local/tmp/ksu.apk");
	const auto r7 = Exec(adb_bin.string(), std::format("push {} /data/local/tmp/ksu.apk", ksum.string()));
	if (std::get<0>(r7) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r7));
		return false;
	}

	std::println("🛠️ 接下来请留意手机上的安装提示");
	std::println("✅ 确认后按回车键继续...");
	std::getline(std::cin, tmp); (void)tmp;

	std::println("🚀 安装 KernelSU Manager...");
	const auto r8 = Exec(adb_bin.string(), "shell pm install -r /data/local/tmp/ksu.apk");
	if (std::get<0>(r8) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r8));
		return false;
	}

	std::println("🚀 一些清理工作...");
	const auto r9 = Exec(adb_bin.string(), "shell rm /data/local/tmp/ksu.apk");
	if (std::get<0>(r9) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r9));
		return false;
	}

	return true;
}

static auto Step5() -> bool
{
	std::cout <<
		"\n========================================\n"
		"   第五步：强制 SELinux\n"
		"========================================\n"
		"📱 请打开 KernelSU Manager 进入超级用户页面搜索 Shell 并授予 root 权限\n"
		"✅ 授权后按任意键继续...";

	std::string tmp;
	std::getline(std::cin, tmp); (void)tmp;

	std::println("🔍 等待设备连接...");
	Exec(adb_bin.string(), "wait-for-device");

	while (true)
	{
		std::println("🔍 正在验证 shell 是否已获得 root 权限...");
		const auto r1 = Exec(adb_bin.string(), "shell su -c 'id -u'");
		if (std::get<1>(r1).find_first_of('0') == std::string::npos)
		{
			std::println("❌ Shell 仍未获得 root 权限，请确认已正确授权！");
			std::print("✅ 请授权后按任意键继续...");
			std::getline(std::cin, tmp); (void)tmp;
			continue;
		}

		std::println("✅ Shell 已成功获得 root 权限！");
		break;
	}

	std::println("🚀 将 SELinux 状态设置为 Enforcing...");
	const auto r1 = Exec(adb_bin.string(), "shell su -c 'setenforce 1'");
	if (std::get<0>(r1) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r1));
		return false;
	}

	std::println("🔍 检查 SELinux 状态...");
	auto r2 = Exec(adb_bin.string(), "shell getenforce");
	if (std::get<0>(r2) != 0)
	{
		std::println("❌ 命令执行失败！错误信息：{}", std::get<1>(r2));
		return false;
	}

	if (std::get<1>(r2).find("Enforcing") == std::string::npos)
	{
		std::println("❌ SELinux 仍处于 Permissive 模式！");
		return false;
	}
	std::println("✅ SELinux 已成功设置为 Enforcing 模式！");
	return true;
}

auto main() -> int
{
	try {
		SetConsoleOutputCP(CP_UTF8);

		std::println("===================================================================\n\n");
		std::println("   ✨ 项目地址 https://github.com/AmzGrainRain/mi_nobl_ksu\n");
		std::println("   📝 工作目录：{}\n\n", cwd.string());
		std::println("===================================================================\n\n");

		if (!fs::exists(adb_bin))
		{
			std::println("当前目录下没有 adb/adb.exe");
			return 1;
		}
		adb_bin = adb_bin.lexically_relative(cwd);

		if (!fs::exists(fastboot_bin))
		{
			std::println("当前目录下没有 adb/fastboot.exe");
			return 1;
		}
		fastboot_bin = fastboot_bin.lexically_relative(cwd);

		if (!fs::exists(ksud))
		{
			std::println("当前目录下没有 ksud");
			return 1;
		}
		ksud = ksud.lexically_relative(cwd);

		if (!fs::exists(ksum))
		{
			std::println("当前目录下没有 ksu.apk");
			return 1;
		}
		ksum = ksum.lexically_relative(cwd);

		if (!Step1()) throw std::runtime_error("step 1 error");
		if (!Step2()) throw std::runtime_error("step 2 error");
		if (!Step3()) throw std::runtime_error("step 3 error");
		if (!Step4()) throw std::runtime_error("step 4 error");
		if (!Step5()) throw std::runtime_error("step 5 error");

		std::cout <<
			"\n========================================\n"
			"   🎉 所有步骤均已执行完成！\n"
			"========================================\n";
	}
	catch (const std::exception& ex)
	{
		std::println("程序发生异常：{}", ex.what());
		return 1;
	}

	std::this_thread::sleep_for(5s);

	system("pause");
	return 0;
}
