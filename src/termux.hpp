#ifndef TERMUX_HPP
#define TERMUX_HPP

#define TERMUX_FS "/data/data/com.termux/files"
#define TERMUX_PREFIX TERMUX_FS "/usr"
#define TERMUX_PATH TERMUX_PREFIX "/bin:" TERMUX_PREFIX "/bin/applets"
#define ROOT_HOME TERMUX_FS "/home/psh"
#define ANDROID_SYSPATHS "/system/bin:/system/xbin"
#define EXTRA_SYSPATHS "/system/bin:/system/xbin:/data/data/com.termux/files/usr/bin"

#endif // TERMUX_HPP