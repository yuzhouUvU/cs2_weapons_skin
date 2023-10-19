#pragma once

template <typename T, int index, typename ...Args>
constexpr T CallVFunc(void* pThis, Args... args) noexcept
{
    return reinterpret_cast<T(*)(void*, Args...)> (reinterpret_cast<void***>(pThis)[0][index])(pThis, args...);
}

#ifdef _WIN32
#define WIN_LINUX(win, linux) win
#else
#define WIN_LINUX(win, linux) linux
#endif

