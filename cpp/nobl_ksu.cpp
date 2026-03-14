#include <filesystem>
#include <iostream>
#include <print>
#include <format>
#include <tuple>
#include <chrono>
#include <thread>
#include <cstdio>
#include <numeric>
#include <cstdint>

#define NOMINMAX
#include <Windows.h>


using namespace std::chrono_literals;
namespace fs = std::filesystem;

const auto cwd = fs::current_path();
auto adb_bin = cwd / "adb" / "adb.exe";
auto fastboot_bin = cwd / "adb" / "fastboot.exe";
auto ksum = cwd / "ksu.apk";

uint16_t skip_flag = 0;

[[maybe_unused]]
static auto Exec(const std::string& bin, const std::string& args, const bool print_error = true) -> std::tuple<int, std::string>
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
	if (exit_code != 0 && print_error)
	{
		std::println("❌ 命令执行失败！错误信息：{}", output);
	}

	return { exit_code, output };
}

static auto Step1() -> bool
{
	std::cout <<
		"========================================\n"
		"   第一步：准备进入 Fastboot 模式\n"
		"========================================\n";

	if (skip_flag & 0b0001)
	{
		std::println("✅ 跳过此步骤...");
		return true;
	}

	std::cout <<
		"📱 请完成以下操作：\n"
		"  1.打开开发者模式（设置 - 关于手机 - 连续点击版本号）\n"
		"  2.解锁手机并进入系统桌面\n"
		"  3.确保手机已连接电脑并允许 USB 调试\n"
		"✅ 确认后按回车键继续...";

	std::string tmp;
	std::getline(std::cin, tmp); (void)tmp;

	std::println("🔍 等待设备连接...");
	{
		const auto [code, res] = Exec(adb_bin.string(), "devices");
		if (code != 0)
		{
			std::println("❌ 命令执行失败！错误信息：{}", res);
			return false;
		}
	}


	std::println("🔄 正在重启到 Fastboot 模式...");
	{
		const auto [code, res] = Exec(adb_bin.string(), "reboot bootloader");
		if (code != 0)
		{
			std::println("❌ 命令执行失败！错误信息：{}", res);
			return false;
		}
	}

	return true;
}

static auto Step2() -> bool
{
	std::cout <<
		"\n========================================\n"
		"   第二步：宽容 SELinux\n"
		"========================================\n";

	if (skip_flag & 0b0010)
	{
		std::println("✅ 跳过此步骤...");
		return true;
	}

	std::println("🚀 将 SELinux 设置为宽容模式...");
	{
		const auto [code, res] = Exec(fastboot_bin.string(), "oem set-gpu-preemption 0 androidboot.selinux=permissive");
		if (code != 0) return false;
	}

	std::println("🚀 正在引导设备进入系统...");
	{
		const auto [code, res] = Exec(fastboot_bin.string(), "continue");
		if (code != 0) return false;
	}

	return true;
}

static auto Step3() -> bool
{
	std::cout <<
		"\n========================================\n"
		"   第三步：检查 SELinux 状态\n"
		"========================================\n";

	if (skip_flag & 0b0100)
	{
		std::println("✅ 跳过此步骤...");
		return true;
	}

	std::println("🔍 等待设备连接...");
	Exec(adb_bin.string(), "wait-for-device");

	std::println("🔍 检查 SELinux 状态...");
	{
		const auto [code, res] = Exec(adb_bin.string(), "shell getenforce");
		if (code != 0) return false;
		if (res.find("Permissive") == std::string::npos)
		{
			std::println("❌ SELinux 仍处于 Enforcing 模式，可能是漏洞利用失败了！");
			return false;
		}
	}

	std::println("✅ SELinux 已成功设置为 Permissive 模式！");
	return true;
}

static auto Step4() -> bool
{
	std::cout <<
		"\n========================================\n"
		"   第四步：安装 KernelSU\n"
		"========================================\n";

	if (skip_flag & 0b1000)
	{
		std::println("✅ 跳过此步骤...");
		return true;
	}

	std::cout <<
		"📱 请打开锁屏并进入桌面\n"
		"✅ 确认后按回车键继续...";
	std::string tmp;
	std::getline(std::cin, tmp); (void)tmp;

	std::println("🔍 等待设备连接...");
	Exec(adb_bin.string(), "wait-for-device");

	std::println("🔍 检查 KernelSU Manager 的安装情况...");
	{
		const auto [code, _] = Exec(adb_bin.string(), "shell pm path me.weishu.kernelsu", false);
		if (code == 0) // KernelSU Manager 已安装会返回 0 并且输出路径，否则会返回 1
		{
			std::println("\n\n✅ 检测到您已安装 KernelSU Manager，是否需要重新安装？");
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
			const auto [code, res] = Exec(adb_bin.string(), "shell pm uninstall me.weishu.kernelsu");
			if (code != 0) return false;
			std::println("✅ 已成功卸载 KernelSU Manager！");

		}
	}

	std::println("📦 推送 KernelSU Manager 到 /data/local/tmp/ksu.apk");
	{
		const auto [code, res] = Exec(adb_bin.string(), std::format("push {} /data/local/tmp/ksu.apk", ksum.string()));
		if (code != 0) return false;
	}


	std::cout <<
		"🛠️ 接下来请留意手机上的安装提示\n"
		"✅ 确认后按回车键继续...";
	std::getline(std::cin, tmp); (void)tmp;

	std::println("🚀 安装 KernelSU Manager...");
	{
		const auto [code, res] = Exec(adb_bin.string(), "shell pm install -r /data/local/tmp/ksu.apk");
		if (code != 0) return false;
	}

	return true;
}

auto main() -> int
{
	try {
		SetConsoleOutputCP(CP_UTF8);

		std::println("===================================================================\n\n");
		std::println("   ✨ 项目地址 https://github.com/AmzGrainRain/nobl_ksu\n");
		std::println("   📝 工作目录：{}\n\n", cwd.string());
		std::println("===================================================================\n\n");

		if (!fs::exists(adb_bin))
		{
			throw std::runtime_error("当前目录下没有 adb/adb.exe");
		}
		adb_bin = adb_bin.lexically_relative(cwd);

		if (!fs::exists(fastboot_bin))
		{
			throw std::runtime_error("当前目录下没有 adb/fastboot.exe");
		}
		fastboot_bin = fastboot_bin.lexically_relative(cwd);

		if (!fs::exists(ksum))
		{
			throw std::runtime_error("当前目录下没有 ksu.apk");
		}
		ksum = ksum.lexically_relative(cwd);

		std::println("🚀 一些准备工作...");
		Exec(adb_bin.string(), "kill-server", false);

		std::println("🔍 检查设备是否处于 Fastboot 模式...");
		{
			const auto [code, res] = Exec(fastboot_bin.string(), "devices");
			if (code == 0 && !res.empty())
			{
				skip_flag |= 0b0001; // 跳过步骤 1
			}
		}

		std::println("🔍 检查设备 SELinux 状态...");
		{
			const auto [code, res] = Exec(adb_bin.string(), "shell getenforce");
			if (code == 0 && res.find("Permissive") != std::string::npos)
			{
				skip_flag |= 0b0111; // 跳过步骤 1 2 3
			}
		}

		if (!Step1()) throw std::runtime_error("step 1 error");
		if (!Step2()) throw std::runtime_error("step 2 error");
		if (!Step3()) throw std::runtime_error("step 3 error");
		if (!Step4()) throw std::runtime_error("step 4 error");

		std::println("🚀 一些清理工作...");
		Exec(adb_bin.string(), "shell rm /data/local/tmp/ksu.apk", false);
		Exec(adb_bin.string(), "kill-server", false);

		std::cout <<
			"\n========================================\n"
			"   🎉 所有步骤均已执行完成！\n"
			"========================================\n";
		std::println("接下来打开 KernelSU Manager，点击越狱即可。");
	}
	catch (const std::exception& ex)
	{
		std::println("程序发生异常：{}", ex.what());
		std::this_thread::sleep_for(5s);
		system("pause");
		return 1;
	}

	std::this_thread::sleep_for(5s);
	system("pause");
	return 0;
}
