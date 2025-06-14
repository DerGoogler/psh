#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "psh.hpp"
#include "shell.hpp"
#include "termux.hpp"

// declare environ
extern char **environ;

bool PSH_DEBUG = false;
std::string LOG_FILE;

std::string executeCommand(const char *cmd) {
    std::array<char, 256> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        if (PSH_DEBUG)
            std::cerr << "popen failed for: " << cmd << std::endl;
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    if (!result.empty() && result.back() == '\n')
        result.pop_back();
    return result;
}

void gather_debug_info() {
    std::ofstream logStream(LOG_FILE, std::ios_base::app);
    if (!logStream)
        return;

    logStream << "Environment:" << std::endl;
    for (char **env = environ; *env != nullptr; ++env)
        logStream << *env << std::endl;

    logStream << "============================" << std::endl;
    logStream << "Architecture: " << executeCommand("dpkg --print-architecture") << std::endl;
    logStream << "Android version: " << executeCommand("getprop ro.build.version.release") << std::endl;
    logStream << "Device: " << executeCommand("getprop ro.product.manufacturer") << " " << executeCommand("getprop ro.product.model") << std::endl;
    logStream << "Kernel: " << executeCommand("uname -a") << std::endl;
}

bool is_executable(const std::filesystem::path &p) {
    struct stat sb;
    if (stat(p.c_str(), &sb) != 0)
        return false;
    return (sb.st_mode & S_IFREG) && (access(p.c_str(), X_OK) == 0);
}

std::string readlink_f(const std::filesystem::path &p) {
    try {
        return std::filesystem::canonical(p).string();
    } catch (...) {
        return "";
    }
}

std::string quote_argument(const std::string &arg) {
    std::string result = "'";
    for (char c : arg) {
        if (c == '\'')
            result += "'\"'\"'";
        else
            result += c;
    }
    result += "'";
    return result;
}

void show_usage() {
    std::cout << R"EOF(
psh usage:
  psh [options] [user]
    -p, --syspre       Prepend system paths to PATH
    -a, --sysadd       Append system paths to PATH
    -s, --shell        Specify shell to use
    -c command         Execute command
    --version          Show version
    -h, --help         Show help
)EOF" << std::endl;
}

void show_usage_sudo() {
    std::cout << R"EOF(
sudo usage:
  sudo command
  sudo [-E] [-u USER] command
)EOF" << std::endl;
}

std::string get_shell_basename(const std::string &shell_path) {
    auto pos = shell_path.find_last_of('/');
    if (pos != std::string::npos)
        return shell_path.substr(pos + 1);
    return shell_path;
}

int main(int argc, char *argv[]) {
    std::vector<std::string> args(argv, argv + argc);
    bool psh_as_sudo = false;
    std::string program_name = std::filesystem::path(args[0]).filename().string();
    if (program_name == "sudo")
        psh_as_sudo = true;

    if (argc > 1 && args[1] == "--dbg") {
        PSH_DEBUG = true;
        char buf[20];
        std::time_t t = std::time(nullptr);
        std::strftime(buf, sizeof(buf), "%Y%m%d", std::localtime(&t));
        LOG_FILE = "./psh_debug_" + std::string(buf);
        gather_debug_info();
        args.erase(args.begin() + 1);
        argc--;
    }

    std::string switch_user;
    bool environment_preserve = false;
    bool prepend_system_path = false;
    bool append_system_path = false;
    std::string alt_shell;
    std::string command_string;
    std::vector<std::string> command_args;

    int arg_idx = 1;

    if (psh_as_sudo) {
        if (argc > 1 && args[1] == "su") {
            psh_as_sudo = false;
            arg_idx++;
        } else {
            while (arg_idx < argc) {
                const std::string &arg = args[arg_idx];
                if (arg.rfind('-', 0) != 0)
                    break;
                if (arg == "-u" || arg == "--user") {
                    if (arg_idx + 1 < argc) {
                        switch_user = args[arg_idx + 1];
                        arg_idx += 2;
                    } else {
                        std::cerr << "sudo: option '-u' requires argument" << std::endl;
                        return 1;
                    }
                } else if (arg == "-E" || arg == "--preserve-environment") {
                    environment_preserve = true;
                    arg_idx++;
                } else
                    break;
            }
        }
    }

    if (!psh_as_sudo) {
        while (arg_idx < argc) {
            const std::string &arg = args[arg_idx];
            if (arg.rfind('-', 0) != 0) {
                command_args.push_back(arg);
                arg_idx++;
                continue;
            }
            if (arg == "-p" || arg == "--syspre")
                prepend_system_path = true;
            else if (arg == "-a" || arg == "--sysadd")
                append_system_path = true;
            else if (arg == "-s" || arg == "--shell") {
                if (arg_idx + 1 < argc) {
                    alt_shell = args[arg_idx + 1];
                    arg_idx++;
                } else {
                    std::cerr << "psh: option '-s' requires argument" << std::endl;
                    return 1;
                }
            } else if (arg == "-c") {
                if (arg_idx + 1 < argc) {
                    command_string = args[arg_idx + 1];
                    arg_idx++;
                } else {
                    std::cerr << "psh: option '-c' requires argument" << std::endl;
                    return 1;
                }
            } else if (arg == "--version") {
                std::cout << "psh - " << PSH_VERSION << std::endl;
                return 0;
            } else if (arg == "-h" || arg == "--help") {
                show_usage();
                return 0;
            } else
                command_args.push_back(arg);
            arg_idx++;
        }
        if (!command_args.empty()) {
            switch_user = command_args.front();
            command_args.erase(command_args.begin());
        }
    }

    if (psh_as_sudo) {
        while (arg_idx < argc)
            command_args.push_back(args[arg_idx++]);
        if (command_args.empty()) {
            show_usage_sudo();
            return 1;
        }
    }

    std::map<std::string, std::string> exp_env;
    std::string root_shell;

    if (!environment_preserve) {
        std::string new_home, new_path;
        if (switch_user.empty()) {
            try {
                std::filesystem::create_directories(ROOT_HOME);
            } catch (...) {
                std::cerr << "Failed to create " << ROOT_HOME << std::endl;
                return 1;
            }
            new_home = ROOT_HOME;
            exp_env["PREFIX"] = TERMUX_PREFIX;
            exp_env["TMPDIR"] = ROOT_HOME "/.tmp";
            if (getenv("LD_PRELOAD"))
                exp_env["LD_PRELOAD"] = getenv("LD_PRELOAD");

            if (psh_as_sudo) {
                new_path = getenv("PATH") ? getenv("PATH") : "";
                exp_env["SUDO_GID"] = std::to_string(getgid());
                exp_env["SUDO_UID"] = std::to_string(getuid());
                passwd *pw = getpwuid(getuid());
                if (pw)
                    exp_env["SUDO_USER"] = pw->pw_name;
            } else
                new_path = TERMUX_PATH;

            if (getenv("LD_LIBRARY_PATH"))
                exp_env["LD_LIBRARY_PATH"] = std::string(getenv("LD_LIBRARY_PATH")) + ":/system/lib64";
            else {
                std::string asp = ANDROID_SYSPATHS ":" EXTRA_SYSPATHS;
                new_path = prepend_system_path ? asp + ":" + new_path : new_path + ":" + asp;
            }
        } else {
            new_home = "/";
            new_path = ANDROID_SYSPATHS;
        }

        exp_env["PATH"] = new_path;
        exp_env["HOME"] = new_home;
        exp_env["TERM"] = "xterm-256color";
        if (getenv("ANDROID_ROOT"))
            exp_env["ANDROID_ROOT"] = getenv("ANDROID_ROOT");
        if (getenv("ANDROID_DATA"))
            exp_env["ANDROID_DATA"] = getenv("ANDROID_DATA");
    }

    std::string startup_script;
    if (psh_as_sudo) {
        for (const auto &arg : command_args)
            startup_script += quote_argument(arg) + " ";
    } else {
        std::vector<std::string> shells_to_try = {
            TERMUX_BASH,
            SYSTEM_BASH,
            TERMUX_SH,
            SYSTEM_SH,
        };

        if (!alt_shell.empty())
            root_shell = alt_shell;
        else {
            for (const auto &candidate : shells_to_try)
                if (is_executable(candidate)) {
                    root_shell = candidate;
                    break;
                }
        }
        startup_script = root_shell;
        if (!command_string.empty()) {
            startup_script += " -c ";
            startup_script += quote_argument(command_string);
        } else if (!command_args.empty()) {
            startup_script += " -c ";
            for (const auto &arg : command_args)
                startup_script += quote_argument(arg) + " ";
        }
    }

    if (!environment_preserve) {
        unsetenv("LD_LIBRARY_PATH");
        unsetenv("LD_PRELOAD");

        std::string shell_name = get_shell_basename(root_shell);
        std::string pshrc_path = ROOT_HOME "/.pshrc";

        if (std::filesystem::exists(pshrc_path)) {
            if (shell_name == "bash") {
                exp_env["BASH_ENV"] = pshrc_path;
            } else if (shell_name == "mksh" || shell_name == "sh" || shell_name == "dash" || shell_name == "ash") {
                exp_env["ENV"] = pshrc_path;
            }
        }
    }

    std::string env_built;
    for (const auto &[key, value] : exp_env)
        env_built += key + "=" + value + " ";

    std::vector<SuBinary> su_to_try = {
        // Possible Magisk su binaries
        {"/sbin/su", true},
        {"/debug_ramdisk/su", true},
        // Possible non-Magisk su binaries
        {"/system/xbin/su", false},
        {"/system/bin/su", false},
        {"/system/bin/su", false},
        {"/data/local/bin/su", false},
        {"/data/local/xbin/su", false},
        {"/data/local/su", false},
    };

    std::string su_binary_path;
    bool magisk_mode = false;

    for (const auto &su : su_to_try) {
        if (is_executable(su.path)) {
            su_binary_path = su.path;
            magisk_mode = su.is_magisk;
            break;
        }
    }

    if (su_binary_path.empty()) {
        std::cerr << "No su binary found" << std::endl;
        return 1;
    }

    std::vector<std::string> exec_args;
    exec_args.push_back(su_binary_path);
    if (!switch_user.empty())
        exec_args.push_back(switch_user);
    exec_args.push_back("-c");
    exec_args.push_back(magisk_mode ? ("PATH=" TERMUX_PREFIX "/bin env -i " + env_built + startup_script)
                                    : (env_built + startup_script));

    std::vector<char *> c_args;
    for (auto &s : exec_args)
        c_args.push_back(&s[0]);
    c_args.push_back(nullptr);

    if (PSH_DEBUG) {
        std::ofstream logStream(LOG_FILE, std::ios_base::app);
        logStream << "Exec: ";
        for (const auto &s : exec_args)
            logStream << s << " ";
        logStream << std::endl;
    }

    execv(su_binary_path.c_str(), c_args.data());
    std::cerr << "execv failed: " << strerror(errno) << std::endl;
    return 1;
}
