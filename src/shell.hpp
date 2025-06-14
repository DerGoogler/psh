#ifndef SHELL_HPP
#define SHELL_HPP

struct SuBinary {
    const char* path;
    /* Defines if a binary could be from Magisk or is from Magisk */
    bool is_magisk;
};

#endif  // SHELL_HPP