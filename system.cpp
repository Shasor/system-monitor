#include "header.h"

// get cpu id and information, you can use `proc/cpuinfo`
string CPUinfo()
{
    char CPUBrandString[0x40];
    unsigned int CPUInfo[4] = {0, 0, 0, 0};

    // unix system
    // for windoes maybe we must add the following
    // __cpuid(regs, 0);
    // regs is the array of 4 positions
    __cpuid(0x80000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nExIds = CPUInfo[0];

    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    for (unsigned int i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(i, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);

        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    string str(CPUBrandString);
    return str;
}

// getOsName, this will get the OS of the current computer
const char *getOsName()
{
#ifdef _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

char *getHostname()
{
    static char hostname[1024]; // Static = mémoire persiste après la fin de la fonction
    gethostname(hostname, 1024);
    return hostname;
}

char *getTotalProcesses()
{
    static char result[32]; // Buffer statique pour stocker le nombre de processus
    std::ifstream file("/proc/stat");
    std::string line;

    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            if (line.rfind("processes", 0) == 0) // Vérifie si la ligne commence par "processes"
            {
                std::sscanf(line.c_str(), "processes %s", result);
                return result;
            }
        }
        file.close();
    }
    // Si échec, retourne une valeur par défaut
    std::snprintf(result, sizeof(result), "N/A");
    return result;
}

char *getCPUName()
{
    static char result[512]; // Buffer statique pour stocker le nom du CPU
    std::ifstream file("/proc/cpuinfo");
    std::string line;

    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            if (line.rfind("model name", 0) == 0) // Vérifie si la ligne commence par "model name"
            {
                std::size_t pos = line.find(": "); // Trouve ": " pour extraire le nom
                if (pos != std::string::npos)
                {
                    const char *name = line.c_str() + pos + 2; // Pointeur vers le début du nom du CPU
                    std::snprintf(result, sizeof(result), "%s", name);
                    return result;
                }
            }
        }
        file.close();
    }
    // Si échec, retourne une valeur par défaut
    std::snprintf(result, sizeof(result), "Unknown CPU");
    return result;
}