#include "header.h"

std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();
float cpuUsage = 0.0f;
std::vector<float> cpuUsageHistory;
float fps = 30.0f;
float scale = 100.0f;
bool animate = true;
CPUStats prevStats = getCPUStats();
const char *focus = "CPU";

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
    static char totalProcesses[16];
    DIR *dir = opendir("/proc");
    if (!dir)
    {
        std::snprintf(totalProcesses, sizeof(totalProcesses), "Error");
        return totalProcesses;
    }
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0]))
        {
            std::string pidPath = "/proc/" + std::string(entry->d_name) + "/stat";
            std::ifstream pidFile(pidPath);
            if (pidFile.is_open())
            {
                count++;
                pidFile.close();
            }
        }
    }
    closedir(dir);
    std::snprintf(totalProcesses, sizeof(totalProcesses), "%d", count);
    return totalProcesses;
}

// Fonction générique pour extraire un long à partir d'une chaîne de caractères
long extractValue(const std::string &line, size_t &pos)
{
    size_t start = line.find_first_not_of(' ', pos);   // ignorer les espaces
    size_t end = line.find(' ', start);                // trouver l'espace suivant
    pos = end;                                         // mettre à jour la position pour la prochaine extraction
    return std::stol(line.substr(start, end - start)); // convertir la sous-chaîne en long
}

CPUStats getCPUStats()
{
    std::ifstream file("/proc/stat");
    std::string line;

    // Lire la première ligne contenant les informations du CPU
    std::getline(file, line);

    // Trouver le début des informations après "cpu"
    size_t pos = line.find("cpu") + 3;

    // Créer une structure de données pour stocker les statistiques
    CPUStats stats;
    stats.user = extractValue(line, pos);
    stats.nice = extractValue(line, pos);
    stats.system = extractValue(line, pos);
    stats.idle = extractValue(line, pos);
    stats.iowait = extractValue(line, pos);
    stats.irq = extractValue(line, pos);
    stats.softirq = extractValue(line, pos);
    stats.steal = extractValue(line, pos);
    stats.guest = extractValue(line, pos);
    stats.guestNice = extractValue(line, pos);

    return stats;
}

float calculateCPUUsage(const CPUStats &prev, const CPUStats &curr)
{
    long prevIdle = prev.idle + prev.iowait;
    long currIdle = curr.idle + curr.iowait;
    long prevNonIdle = prev.user + prev.nice + prev.system + prev.irq + prev.softirq + prev.steal;
    long currNonIdle = curr.user + curr.nice + curr.system + curr.irq + curr.softirq + curr.steal;
    long prevTotal = prevIdle + prevNonIdle;
    long currTotal = currIdle + currNonIdle;

    long totalDiff = currTotal - prevTotal;
    long idleDiff = currIdle - prevIdle;

    return 100.0 * (totalDiff - idleDiff) / totalDiff;
}

// Fonction pour afficher le graphique CPU
void renderCPUUsageGraph(float currentUsage)
{
    // Ajoute l'usage actuel du CPU à l'historique
    cpuUsageHistory.push_back(currentUsage);
    if (cpuUsageHistory.size() > 100)
    {
        cpuUsageHistory.erase(cpuUsageHistory.begin());
    }

    // Crée un label pour afficher le pourcentage d'usage du CPU
    char overlayText[32];
    snprintf(overlayText, sizeof(overlayText), "percentage: %.2f %%", cpuUsage);

    // Affiche le graphique avec l'overlay
    ImGui::PlotLines("CPU", cpuUsageHistory.data(), cpuUsageHistory.size(), 0, overlayText, 0.0f, scale, ImVec2(ImGui::GetContentRegionAvail().x * 0.75, ImGui::GetContentRegionAvail().y));
}

void renderCPU(std::chrono::duration<float> deltaTime)
{
    auto frameDuration = std::chrono::duration<float>(1.0f / fps);
    if (deltaTime >= frameDuration && animate)
    {
        lastTime = std::chrono::high_resolution_clock::now();

        CPUStats currStats = getCPUStats();
        cpuUsage = calculateCPUUsage(prevStats, currStats);
        prevStats = currStats;

        std::this_thread::sleep_for(frameDuration - deltaTime);
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ImGui::BeginChild("", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y));
    {
        ImGui::Checkbox("Animate", &animate);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.75);
        ImGui::SliderFloat("FPS", &fps, 1.0f, 60.0f, "%.0f");
        ImGui::SliderFloat("Max Scale", &scale, 20.0f, 100.0f, "%.3f");
        ImGui::PopItemWidth();
        ImGui::Separator();
        // Afficher le graphique avec ImGui
        renderCPUUsageGraph(cpuUsage);
    }
    ImGui::EndChild();
}

void renderSystem(std::chrono::duration<float> deltaTime)
{
    std::vector<const char *> sections = {"CPU", "Fan", "Thermal"};
    ImGui::BeginChild("System", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y));
    {
        ImGui::BeginChild("Base", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 5));
        {
            ImGui::Text("Operating System Used: %s", getOsName());
            ImGui::Text("Computer Name: %s", getHostname());
            ImGui::Text("User logged: %s", getenv("USER"));
            ImGui::Text("Number of working processes: %s", getTotalProcesses());
            ImGui::Text("CPU: %s", CPUinfo().c_str());
        }
        ImGui::EndChild();

        ImGui::Separator();

        ImGui::BeginChild("Menu", ImVec2(0, ImGui::GetFrameHeightWithSpacing() + 1));
        {
            // Affichage dynamique des boutons
            for (size_t i = 0; i < sections.size(); i++)
            {
                bool isActive = (focus == sections[i]); // Vérifie si le bouton est actif
                if (isActive)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(100, 100, 255, 255)); // Couleur active
                }
                if (ImGui::Button(sections[i]))
                {
                    focus = sections[i]; // Change l'état actif
                }
                if (isActive)
                {
                    ImGui::PopStyleColor(); // Toujours pop si on a push avant !
                }
                if (i < sections.size() - 1)
                    ImGui::SameLine();
            }
            ImGui::PushStyleColor(ImGuiCol_Separator, IM_COL32(100, 100, 255, 255));
            ImGui::Separator();
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();

        ImGui::BeginChild("Sections");
        {
            for (size_t i = 0; i < sections.size(); i++)
            {
                bool isActive = (focus == sections[i]);
                if (isActive)
                {
                    ImGui::BeginChild(sections[i], ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y));
                    {
                        if (strcmp(sections[i], "CPU") == 0)
                            renderCPU(deltaTime);
                    }
                    ImGui::EndChild();
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
}